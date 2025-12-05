/**
 * @file APDS9960_ColorSensor.cpp
 * @author Federico Maniglio
 * @date 2025-12-05
 * @brief Implementation of APDS9960_ColorSensor class
 */

#include "APDS9960_ColorSensor.h"
#include <SparkFun_APDS9960.h>

/**
 * @brief Constructor - initializes all calibration values to zero
 * 
 * Sets the sensor to NOT_CALIBRATED state. All color channel maximums
 * are initialized to zero, requiring calibration before accurate readings.
 */
ADPS9960_ColorSensor::ADPS9960_ColorSensor()
    : calibrationStatus(NOT_CALIBRATED),
      max_ambient(0),
      max_red(0),
      max_green(0),
      max_blue(0) {
}

/**
 * @brief Initialize the APDS9960 sensor hardware
 * 
 * Calls the underlying SparkFun library to initialize I2C communication
 * and enables the light sensor without interrupts.
 * 
 * @return true if both init and light sensor enable succeed, false otherwise
 */
bool ADPS9960_ColorSensor::begin() {
    // Initialize sensor hardware and enable light sensing (no interrupts)
    return sensor.init() && sensor.enableLightSensor(false);
}

/**
 * @brief Calibrate the sensor by sampling maximum color values
 * 
 * This method performs calibration by continuously sampling the sensor
 * over a specified time period and recording the maximum values seen
 * for each color channel. These maximums are used to normalize future
 * readings to the 0-255 range.
 * 
 * Algorithm:
 * 1. Validate and normalize sampling time (1-10 seconds)
 * 2. Perform calibration sampling
 * 3. Validate collected data quality
 * 4. On success: Mark as CALIBRATED_OK
 * 5. On failure: Use defaults if requested, otherwise mark NOT_CALIBRATED
 * 
 * @param samplingTimeSeconds Duration of calibration (1-10s, auto-corrected if out of range)
 * @param useDefaultsOnFail If true, falls back to default values on failure
 * @return true if calibration successful, false if failed
 * 
 * @note For best results, point sensor at a white surface during calibration
 * @note Requires consistent lighting conditions during sampling period
 */
bool ADPS9960_ColorSensor::calibrate(int samplingTimeSeconds, bool useDefaultsOnFail) {
    // Validate and normalize sampling time to acceptable range
    if (samplingTimeSeconds < 1 || samplingTimeSeconds > MAX_SAMPLING_TIME) {
        samplingTimeSeconds = DEFAULT_SAMPLING_TIME;
    }

    // Perform the actual calibration routine
    bool success = performCalibration(samplingTimeSeconds);

    if (success) {
        calibrationStatus = CALIBRATED_OK;
        return true;
    }

    // Calibration failed - handle fallback strategy
    if (useDefaultsOnFail) {
        setDefaultCalibration();
        calibrationStatus = CALIBRATED_WITH_DEFAULTS;
    } else {
        calibrationStatus = NOT_CALIBRATED;
    }
    return false;
}

/**
 * @brief Internal routine that performs the calibration sampling
 * 
 * This method implements the core calibration algorithm:
 * 1. Waits 500ms for sensor stabilization
 * 2. Resets all maximum values to zero
 * 3. Continuously samples sensor for specified duration
 * 4. Tracks maximum value seen for each color channel
 * 5. Validates collected data meets quality criteria
 * 
 * The sampling rate is approximately 10 Hz (100ms delay between reads).
 * At least MIN_SAMPLES_PER_SECOND * samplingTimeSeconds samples must
 * be successfully collected for calibration to be valid.
 * 
 * @param samplingTimeSeconds Duration of sampling period
 * @return true if calibration data is valid, false otherwise
 */
bool ADPS9960_ColorSensor::performCalibration(int samplingTimeSeconds) {
    delay(500);  // Allow sensor to stabilize

    // Reset all maximum values to zero
    max_ambient = 0;
    max_red = 0;
    max_green = 0;
    max_blue = 0;

    const unsigned long startTime = millis();
    int samples = 0;
    const int minSamples = samplingTimeSeconds * MIN_SAMPLES_PER_SECOND;

    // Sampling phase - collect maximum values over time
    while (millis() - startTime < samplingTimeSeconds * 1000) {
        uint16_t temp_ambient, temp_red, temp_green, temp_blue;

        // Attempt to read all color channels from sensor
        if (sensor.readAmbientLight(temp_ambient) &&
            sensor.readRedLight(temp_red) &&
            sensor.readGreenLight(temp_green) &&
            sensor.readBlueLight(temp_blue)) {

            // Update maximum values if current reading is higher
            if (temp_ambient > max_ambient) max_ambient = temp_ambient;
            if (temp_red > max_red) max_red = temp_red;
            if (temp_green > max_green) max_green = temp_green;
            if (temp_blue > max_blue) max_blue = temp_blue;

            samples++;
        }

        delay(100);  // Prevent sensor saturation and allow processing time
    }

    // Validate that collected data meets quality requirements
    return validateCalibrationData(samples, minSamples);
}

/**
 * @brief Validate that calibration data meets quality criteria
 * 
 * Implements multiple validation checks to ensure calibration quality:
 * 
 * Criterion 1: Sample Count
 *   - Must have at least minSamples successful readings
 *   - Ensures statistical reliability
 * 
 * Criterion 2: Minimum Light Level
 *   - Ambient light must exceed MIN_THRESHOLD
 *   - At least one RGB channel must exceed MIN_THRESHOLD
 *   - Prevents calibration in dark/covered conditions
 * 
 * Criterion 3: Saturation Check
 *   - No channel should exceed SATURATION_THRESHOLD
 *   - Prevents calibration with overexposed sensor
 * 
 * Criterion 4: Non-Zero Values
 *   - Ensures sensor is actually reading data
 *   - Detects potential hardware failures
 * 
 * @param samples Number of successful samples collected
 * @param minSamples Minimum required samples for validity
 * @return true if all criteria pass, false otherwise
 */
bool ADPS9960_ColorSensor::validateCalibrationData(int samples, int minSamples) const {
    // Criterion 1: Insufficient sample count
    if (samples < minSamples) {
        return false;
    }

    // Criterion 2: Values too low (sensor covered or not functioning)
    if (max_ambient < MIN_THRESHOLD) {
        return false;
    }

    // At least one RGB channel must exceed minimum threshold
    if (max_red < MIN_THRESHOLD &&
        max_green < MIN_THRESHOLD &&
        max_blue < MIN_THRESHOLD) {
        return false;
    }

    // Criterion 3: Saturated values (too much light or sensor malfunction)
    if (max_ambient > SATURATION_THRESHOLD ||
        max_red > SATURATION_THRESHOLD ||
        max_green > SATURATION_THRESHOLD ||
        max_blue > SATURATION_THRESHOLD) {
        return false;
    }

    // Criterion 4: Verify values aren't all zero (sanity check)
    if (max_ambient == 0 || (max_red == 0 && max_green == 0 && max_blue == 0)) {
        return false;
    }

    return true;
}

/**
 * @brief Force sensor to use default calibration values
 * 
 * Sets all color channel maximums to DEFAULT_MAX_VALUE (1000).
 * This provides a reasonable baseline for color sensing when
 * proper calibration isn't possible or for testing purposes.
 * 
 * @note This does NOT change calibrationStatus - call from calibrate()
 */
void ADPS9960_ColorSensor::setDefaultCalibration() {
    max_ambient = DEFAULT_MAX_VALUE;
    max_red = DEFAULT_MAX_VALUE;
    max_green = DEFAULT_MAX_VALUE;
    max_blue = DEFAULT_MAX_VALUE;
}

/**
 * @brief Get the current calibration status enum value
 * @return CalibrationStatus enum (NOT_CALIBRATED, CALIBRATED_OK, or CALIBRATED_WITH_DEFAULTS)
 */
ADPS9960_ColorSensor::CalibrationStatus ADPS9960_ColorSensor::getCalibrationStatus() const {
    return calibrationStatus;
}

/**
 * @brief Quick check if sensor has been calibrated by any method
 * @return true if calibrated (OK or WITH_DEFAULTS), false if NOT_CALIBRATED
 */
bool ADPS9960_ColorSensor::isCalibrated() const {
    return calibrationStatus != NOT_CALIBRATED;
}

/**
 * @brief Get human-readable string representation of calibration status
 * 
 * Useful for debugging and user interface display.
 * 
 * @return String representation of current status:
 *         - "NOT_CALIBRATED" - Never calibrated
 *         - "CALIBRATED_OK" - Successfully calibrated with real data
 *         - "CALIBRATED_WITH_DEFAULTS" - Using fallback default values
 *         - "UNKNOWN" - Should never occur
 */
const char* ADPS9960_ColorSensor::getCalibrationStatusName() const {
    switch (calibrationStatus) {
        case NOT_CALIBRATED:
            return "NOT_CALIBRATED";
        case CALIBRATED_OK:
            return "CALIBRATED_OK";
        case CALIBRATED_WITH_DEFAULTS:
            return "CALIBRATED_WITH_DEFAULTS";
        default:
            return "UNKNOWN";
    }
}


/**
 * @brief Read raw 16-bit color data directly from sensor
 * 
 * Reads all four light channels (ambient, red, green, blue) from the
 * APDS9960 sensor. Raw values range from 0 to 65535 depending on
 * light intensity and sensor configuration.
 * 
 * @param raw Reference to RawColor struct to populate with sensor data
 * @return true if all four channels read successfully, false if any read fails
 * 
 * @note Does not require calibration
 * @note Values are not normalized
 */
bool ADPS9960_ColorSensor::readRawData(RawColor &raw) {
    // Read all raw values from sensor - fail fast on any error
    if (!sensor.readAmbientLight(raw.ambient)) return false;
    if (!sensor.readRedLight(raw.red)) return false;
    if (!sensor.readGreenLight(raw.green)) return false;
    if (!sensor.readBlueLight(raw.blue)) return false;

    return true;
}

/**
 * @brief Read normalized RGB color values (0-255 range)
 * 
 * Reads raw color data and normalizes it to standard 8-bit RGB values
 * using calibration maximums. If sensor hasn't been calibrated, it
 * automatically applies default calibration values.
 * 
 * Normalization algorithm:
 * RGB_value = (raw_value * 255) / max_value_from_calibration
 * 
 * @param r Reference to store normalized red value (0-255)
 * @param g Reference to store normalized green value (0-255)
 * @param b Reference to store normalized blue value (0-255)
 * @return true if read successful, false on sensor read error
 * 
 * @note Automatically calibrates with defaults if not yet calibrated
 * @note Safe to call even without explicit calibration
 */
bool ADPS9960_ColorSensor::readRGB(uint8_t &r, uint8_t &g, uint8_t &b) {
    // Auto-calibrate with defaults if necessary (fail-safe mechanism)
    if (calibrationStatus == NOT_CALIBRATED) {
        setDefaultCalibration();
        calibrationStatus = CALIBRATED_WITH_DEFAULTS;
    }

    // Read raw sensor data
    RawColor raw{};
    if (!readRawData(raw)) {
        return false;
    }

    // Normalize raw values to 0-255 range using calibration maximums
    r = normalizeToRGB(raw.red, max_red);
    g = normalizeToRGB(raw.green, max_green);
    b = normalizeToRGB(raw.blue, max_blue);

    return true;
}

/**
 * @brief Read normalized RGB values into a struct
 * 
 * Convenience method that wraps the reference parameter version.
 * Useful when you want to pass RGB as a single object.
 * 
 * @param rgb Reference to RGB struct to populate
 * @return true if read successful, false on error
 */
bool ADPS9960_ColorSensor::readRGB(RGB &rgb) {
    // Struct version - delegates to reference parameter version
    return readRGB(rgb.r, rgb.g, rgb.b);
}

/**
 * @brief Read color as 24-bit hexadecimal value (no error checking)
 * 
 * Reads RGB color and packs it into a single 32-bit value in the
 * format 0x00RRGGBB (compatible with web colors, graphics libraries).
 * 
 * @return Color in 0xRRGGBB format, returns 0x000000 (black) on error
 * 
 * @note Cannot distinguish between actual black and read errors
 * @note Use readColorHex(uint32_t&) if error checking is needed
 */
uint32_t ADPS9960_ColorSensor::readColorHex() {
    uint8_t r, g, b;

    // If read fails, return black (0x000000)
    if (!readRGB(r, g, b)) {
        return 0x000000;
    }

    // Combine R, G, B into 0xRRGGBB format
    // Bit shift: R << 16 | G << 8 | B
    return (static_cast<uint32_t>(r) << 16) | 
           (static_cast<uint32_t>(g) << 8) | 
           static_cast<uint32_t>(b);
}

/**
 * @brief Read color as hexadecimal with error reporting
 * 
 * Same as readColorHex() but provides explicit error checking through
 * return value. Useful when you need to distinguish between black
 * color and read failure.
 * 
 * @param hexColor Reference to store hex color value (0xRRGGBB format)
 * @return true if read successful, false on error (hexColor set to 0x000000)
 */
bool ADPS9960_ColorSensor::readColorHex(uint32_t &hexColor) {
    uint8_t r, g, b;

    if (!readRGB(r, g, b)) {
        hexColor = 0x000000;
        return false;
    }

    // Pack RGB into hexadecimal format
    hexColor = (static_cast<uint32_t>(r) << 16) | 
               (static_cast<uint32_t>(g) << 8) | 
               static_cast<uint32_t>(b);
    return true;
}

/**
 * @brief Get color as a formatted hexadecimal string
 * 
 * Reads color and formats it as a CSS/HTML-compatible hex string
 * with leading hash symbol (e.g., "#FF0000" for red).
 * 
 * @return String in "#RRGGBB" format (7 characters including '#')
 * 
 * @note Returns "#000000" on read error
 * @note Useful for web applications, displays, logging
 */
String ADPS9960_ColorSensor::getColorHexString() {
    const uint32_t hex = readColorHex();

    char buffer[8];  // "#RRGGBB\0" = 8 characters with null terminator
    sprintf(buffer, "#%06lX", hex);  // Format with 6 hex digits, uppercase

    return {buffer};
}

/**
 * @brief Normalize a 16-bit raw sensor value to 8-bit RGB range
 * 
 * Performs linear normalization of sensor readings to standard RGB range.
 * Uses integer arithmetic to avoid floating-point overhead.
 * 
 * Formula: result = (rawValue * 255) / maxValue
 * 
 * Safety features:
 * - Division by zero protection
 * - Overflow protection using uint32_t intermediate calculation
 * - Automatic clamping to 0-255 range
 * 
 * @param rawValue Raw sensor reading (0-65535)
 * @param maxValue Maximum value from calibration
 * @return Normalized value (0-255)
 * 
 * @note Returns 0 if maxValue is 0 (prevents division by zero)
 * @note Clamps result to 255 if normalization exceeds maximum
 */
uint8_t normalizeToRGB(const uint16_t rawValue, const uint16_t maxValue) {
    // Prevent division by zero
    if (maxValue == 0) {
        return 0;
    }

    // Normalize raw value to 0-255 range
    // Use uint32_t to prevent overflow during multiplication
    const uint32_t normalized = (static_cast<uint32_t>(rawValue) * 255UL) / maxValue;

    // Clamp to valid RGB range (0-255)
    if (normalized > 255) {
        return 255;
    }

    return static_cast<uint8_t>(normalized);
}
