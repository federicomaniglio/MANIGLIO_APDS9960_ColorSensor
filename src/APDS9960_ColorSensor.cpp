/**
 * @file APDS9960_ColorSensor.cpp
 * @author Federico Maniglio
 * @date 2025-12-05
 * @brief Implementation of APDS9960_ColorSensor class
 */

#include "APDS9960_ColorSensor.h"
#include <SparkFun_APDS9960.h>

/**
 * @brief Get human-readable name of a standard color
 * 
 * Converts a StandardColor enum value to its string representation.
 * This is a free function that can be used independently of the sensor class.
 * 
 * @param color StandardColor enum value
 * @return Constant string with color name in uppercase
 * 
 * @note Returns "UNKNOWN" for invalid enum values
 * @note All color names are in English and uppercase
 */
const char* getStandardColorName(StandardColor color) {
    switch (color) {
        case StandardColor::UNKNOWN: return "UNKNOWN";
        case StandardColor::RED:     return "RED";
        case StandardColor::ORANGE:  return "ORANGE";
        case StandardColor::YELLOW:  return "YELLOW";
        case StandardColor::GREEN:   return "GREEN";
        case StandardColor::CYAN:    return "CYAN";
        case StandardColor::BLUE:    return "BLUE";
        case StandardColor::PURPLE:  return "PURPLE";
        case StandardColor::MAGENTA: return "MAGENTA";
        case StandardColor::WHITE:   return "WHITE";
        case StandardColor::BLACK:   return "BLACK";
        default:                     return "UNKNOWN";
    }
}

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
    if (!sensor.init())
        return false;
    delay(100); // Allow sensor to stabilize
    return sensor.enableLightSensor(false);
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
    delay(500); // Allow sensor to stabilize

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

        delay(100); // Prevent sensor saturation and allow processing time
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
const char *ADPS9960_ColorSensor::getCalibrationStatusName() const {
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

    char buffer[8]; // "#RRGGBB\0" = 8 characters with null terminator
    sprintf(buffer, "#%06lX", hex); // Format with 6 hex digits, uppercase

    return {buffer};
}

/**
 * @brief Check if current color matches custom HSV ranges
 * 
 * Reads the current HSV color and checks if all components fall within
 * the specified ranges. Useful for detecting specific color ranges.
 * 
 * @param hMin Minimum hue value (0-360 degrees)
 * @param hMax Maximum hue value (0-360 degrees)
 * @param sMin Minimum saturation (0.0-1.0)
 * @param sMax Maximum saturation (0.0-1.0)
 * @param vMin Minimum value/brightness (0.0-1.0)
 * @param vMax Maximum value/brightness (0.0-1.0)
 * @return true if color is within all ranges, false otherwise
 * 
 * @note Returns false if sensor read fails
 * @note Handles hue wrap-around (e.g., red crossing 0/360)
 */
bool ADPS9960_ColorSensor::isColorInRange(float hMin, float hMax, 
                                         float sMin, float sMax, 
                                         float vMin, float vMax) {
    HSV hsv{};
    if (!readColorHSV(hsv)) {
        return false;
    }

    // Handle hue wrap-around (e.g., red: 350-10 degrees)
    bool hueInRange;
    if (hMin <= hMax) {
        hueInRange = (hsv.h >= hMin && hsv.h <= hMax);
    } else {
        // Wrap-around case
        hueInRange = (hsv.h >= hMin || hsv.h <= hMax);
    }

    return hueInRange && 
           (hsv.s >= sMin && hsv.s <= sMax) &&
           (hsv.v >= vMin && hsv.v <= vMax);
}

/**
 * @brief Check if current color matches a standard predefined color
 * 
 * Compares the current sensor reading against predefined HSV ranges
 * for common colors. Uses a tolerance factor to allow for variations
 * in lighting and sensor readings.
 * 
 * Standard color definitions (with NO overlap):
 * - RED:     H=[340-360) ∪ [0-20), S≥0.5, V≥0.3
 * - ORANGE:  H=[20-50), S≥0.5, V≥0.4
 * - YELLOW:  H=[50-80), S≥0.5, V≥0.5
 * - GREEN:   H=[80-165), S≥0.4, V≥0.3
 * - CYAN:    H=[165-210), S≥0.4, V≥0.4
 * - BLUE:    H=[210-265), S≥0.4, V≥0.3
 * - PURPLE:  H=[265-295), S≥0.4, V≥0.3
 * - MAGENTA: H=[295-340), S≥0.5, V≥0.4
 * - WHITE:   S<0.2, V≥0.7
 * - BLACK:   V<0.2
 * 
 * @param color StandardColor enum value to check
 * @param tolerance Tolerance factor (0.0-1.0, default 0.15 = 15%)
 * @return true if color matches the standard, false otherwise
 */
bool ADPS9960_ColorSensor::isStandardColor(StandardColor color, float tolerance) {
    HSV hsv{};
    if (!readColorHSV(hsv)) {
        return false;
    }

    // Clamp tolerance to valid range
    if (tolerance < 0.0f) tolerance = 0.0f;
    if (tolerance > 1.0f) tolerance = 1.0f;

    switch (color) {
        case StandardColor::UNKNOWN:
            return false;

        case StandardColor::RED:
            // Red wraps around 0/360: [340-360) and [0-20)
            return (hsv.h < 20.0f || hsv.h >= 340.0f) && 
                   hsv.s >= (0.5f - tolerance) && 
                   hsv.v >= (0.3f - tolerance);

        case StandardColor::ORANGE:
            // [20-50)
            return (hsv.h >= 20.0f && hsv.h < 50.0f) &&
                   hsv.s >= (0.5f - tolerance) &&
                   hsv.v >= (0.4f - tolerance);

        case StandardColor::YELLOW:
            // [50-80)
            return (hsv.h >= 50.0f && hsv.h < 80.0f) &&
                   hsv.s >= (0.5f - tolerance) &&
                   hsv.v >= (0.5f - tolerance);

        case StandardColor::GREEN:
            // [80-165)
            return (hsv.h >= 80.0f && hsv.h < 165.0f) &&
                   hsv.s >= (0.4f - tolerance) &&
                   hsv.v >= (0.3f - tolerance);

        case StandardColor::CYAN:
            // [165-210)
            return (hsv.h >= 165.0f && hsv.h < 210.0f) &&
                   hsv.s >= (0.4f - tolerance) &&
                   hsv.v >= (0.4f - tolerance);

        case StandardColor::BLUE:
            // [210-265)
            return (hsv.h >= 210.0f && hsv.h < 265.0f) &&
                   hsv.s >= (0.4f - tolerance) &&
                   hsv.v >= (0.3f - tolerance);

        case StandardColor::PURPLE:
            // [265-295)
            return (hsv.h >= 265.0f && hsv.h < 295.0f) &&
                   hsv.s >= (0.4f - tolerance) &&
                   hsv.v >= (0.3f - tolerance);

        case StandardColor::MAGENTA:
            // [295-340)
            return (hsv.h >= 295.0f && hsv.h < 340.0f) &&
                   hsv.s >= (0.5f - tolerance) &&
                   hsv.v >= (0.4f - tolerance);

        case StandardColor::WHITE:
            return hsv.s <= (0.2f + tolerance) &&
                   hsv.v >= (0.7f - tolerance);

        case StandardColor::BLACK:
            return hsv.v <= (0.2f + tolerance);

        default:
            return false;
    }
}

/**
 * @brief Detect and return the closest matching standard color
 *
 * Reads the current color from the sensor and determines which standard
 * color it most closely matches. Uses a priority system:
 * 1. First checks for BLACK (very low brightness)
 * 2. Then checks for WHITE (low saturation, high brightness)
 * 3. Finally checks chromatic colors in hue order
 *
 * This ordering prevents false positives (e.g., dark colors being
 * misidentified as chromatic colors with low brightness).
 *
 * Color ranges (NO overlap, using < for upper bounds):
 * - RED:     [340-360) ∪ [0-20)
 * - ORANGE:  [20-50)
 * - YELLOW:  [50-80)
 * - GREEN:   [80-165)
 * - CYAN:    [165-210)  ← Your H=199° falls here!
 * - BLUE:    [210-265)
 * - PURPLE:  [265-295)
 * - MAGENTA: [295-340)
 *
 * @param tolerance Tolerance factor (0.0-1.0, default 0.15 = 15%)
 * @return StandardColor enum of detected color, UNKNOWN if no match or read error
 *
 * @note Returns UNKNOWN if sensor read fails
 * @note Returns UNKNOWN if no standard color matches within tolerance
 */
StandardColor ADPS9960_ColorSensor::detectColor(float tolerance) {
    HSV hsv{};
    if (!readColorHSV(hsv)) {
        return StandardColor::UNKNOWN;
    }

    // Clamp tolerance to valid range
    if (tolerance < 0.0f) tolerance = 0.0f;
    if (tolerance > 1.0f) tolerance = 1.0f;
    
    // Priority 1: Check for BLACK (very low brightness)
    if (hsv.v <= (0.2f + tolerance)) {
        return StandardColor::BLACK;
    }

    if (hsv.s <= (0.2f + tolerance) && hsv.v >= (0.7f - tolerance)) {
        return StandardColor::WHITE;
    }

    // Priority 3: Check chromatic colors by hue ranges
    // Require minimum saturation and value for chromatic colors
    const float minChromatic_s = 0.3f - tolerance;
    const float minChromatic_v = 0.25f - tolerance;

    if (hsv.s < minChromatic_s || hsv.v < minChromatic_v) {
        return StandardColor::UNKNOWN; // Too desaturated or dark for chromatic colors
    }

    // RED (wraps around 0/360): [340-360) ∪ [0-20)
    if (hsv.h < 20.0f || hsv.h >= 340.0f) {
        if (hsv.s >= (0.5f - tolerance) && hsv.v >= (0.3f - tolerance)) {
            return StandardColor::RED;
        }
    }

    // ORANGE: [20-50)
    if (hsv.h >= 20.0f && hsv.h < 50.0f) {
        if (hsv.s >= (0.5f - tolerance) && hsv.v >= (0.4f - tolerance)) {
            return StandardColor::ORANGE;
        }
    }

    // YELLOW: [50-80)
    if (hsv.h >= 50.0f && hsv.h < 80.0f) {
        if (hsv.s >= (0.5f - tolerance) && hsv.v >= (0.5f - tolerance)) {
            return StandardColor::YELLOW;
        }
    }

    // GREEN: [80-165)
    if (hsv.h >= 80.0f && hsv.h < 165.0f) {
        if (hsv.s >= (0.4f - tolerance) && hsv.v >= (0.3f - tolerance)) {
            return StandardColor::GREEN;
        }
    }

    // CYAN: [165-210) ← Your H=199° will match here!
    if (hsv.h >= 165.0f && hsv.h < 210.0f) {
        if (hsv.s >= (0.4f - tolerance) && hsv.v >= (0.4f - tolerance)) {
            return StandardColor::CYAN;
        }
    }

    // BLUE: [210-265)
    if (hsv.h >= 210.0f && hsv.h < 265.0f) {
        if (hsv.s >= (0.4f - tolerance) && hsv.v >= (0.3f - tolerance)) {
            return StandardColor::BLUE;
        }
    }

    // PURPLE: [265-295)
    if (hsv.h >= 265.0f && hsv.h < 295.0f) {
        if (hsv.s >= (0.4f - tolerance) && hsv.v >= (0.3f - tolerance)) {
            return StandardColor::PURPLE;
        }
    }

    // MAGENTA: [295-340)
    if (hsv.h >= 295.0f && hsv.h < 340.0f) {
        if (hsv.s >= (0.5f - tolerance) && hsv.v >= (0.4f - tolerance)) {
            return StandardColor::MAGENTA;
        }
    }

    // No match found
    return StandardColor::UNKNOWN;
}



/**
 * @brief Converts RGB sensor data into HSV color model representation.
 *
 * Reads RGB values from the sensor and calculates the corresponding
 * HSV (Hue, Saturation, Value) representation. The conversion uses
 * normalized RGB values and computes the maximum, minimum, and delta
 * values to derive hue, saturation, and value components.
 *
 * @param hsvColor An HSV structure to store the converted color data.
 *
 * @return True if the color data is successfully read and converted,
 *         otherwise false if the RGB data cannot be read.
 */
bool ADPS9960_ColorSensor::readColorHSV(HSV &hsvColor) {
    RGB rgbColor{};
    if (!readRGB(rgbColor)) {
        return false;
    }
    const float rf = static_cast<float>(rgbColor.r) / 255.0f;
    const float gf = static_cast<float>(rgbColor.g) / 255.0f;
    const float bf = static_cast<float>(rgbColor.b) / 255.0f;

    float maxc = rf;
    if (gf > maxc) maxc = gf;
    if (bf > maxc) maxc = bf;

    float minc = rf;
    if (gf < minc) minc = gf;
    if (bf < minc) minc = bf;

    const float delta = maxc - minc;

    // V (value) = max
    hsvColor.v = maxc;

    // gray/white/black
    if (delta < 0.00001f) {
        hsvColor.h = 0;
        hsvColor.s = 0;
        return true;
    }

    // S (saturation)
    hsvColor.s = delta / maxc;

    // H (hue)
    float h;

    if (maxc == rf) {
        h = (gf - bf) / delta;
        if (h < 0.0f)
            h += 6.0f;
    }
    else if (maxc == gf) {
        h = ((bf - rf) / delta) + 2.0f;
    }
    else { // maxc == bf
        h = ((rf - gf) / delta) + 4.0f;
    }

    h *= 60.0f; // in degrees 0-360
    hsvColor.h = h;

    return true;
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
