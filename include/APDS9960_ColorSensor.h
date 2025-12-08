/**
 * @file APDS9960_ColorSensor.h
 * @author Federico Maniglio
 * @date 2025-12-05
 * @brief High-level color sensing library for APDS9960 sensor
 * 
 * This library provides an easy-to-use interface for the APDS9960 RGB sensor,
 * featuring automatic calibration, multiple output formats (raw, RGB, hex),
 * and robust error handling. Built on top of SparkFun's APDS9960 library.
 */

#ifndef MANIGLIO_APDS_LIBRARY_APDS9960_COLORSENSOR_H
#define MANIGLIO_APDS_LIBRARY_APDS9960_COLORSENSOR_H

#include "SparkFun_APDS9960.h"

/**
 * @enum StandardColor
 * @brief Predefined standard colors for easy color detection
 * 
 * This enum defines common colors that can be detected by the color sensor.
 * Each color corresponds to specific HSV (Hue, Saturation, Value) ranges.
 * 
 * Color ranges (semi-open intervals [min, max)):
 * - RED:     H=[340-360)∪[0-20), S≥0.5, V≥0.3
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
 * @note Use StandardColor::RED, StandardColor::GREEN, etc.
 * @note UNKNOWN is returned when no color matches or on sensor read error
 */
enum class StandardColor : uint8_t {
    UNKNOWN = 0,  ///< No color detected or read error
    RED,          ///< Red color (wraps around 0°)
    ORANGE,       ///< Orange color
    YELLOW,       ///< Yellow color
    GREEN,        ///< Green color
    CYAN,         ///< Cyan color (includes light blue)
    BLUE,         ///< Blue color
    PURPLE,      ///< Purple color
    MAGENTA,      ///< Magenta/Pink color
    WHITE,        ///< White (low saturation, high value)
    BLACK         ///< Black (very low value)
};

/**
 * @brief Get human-readable name of a standard color
 * @param color StandardColor enum value
 * @return Constant string with color name in uppercase
 * @note This is a free function, not a class method
 */
const char* getStandardColorName(StandardColor color);

/**
 * @class ADPS9960_ColorSensor
 * @brief Wrapper class for APDS9960 color sensing with calibration support
 * 
 * This class simplifies color sensing by handling calibration, normalization,
 * and providing multiple output formats. It automatically manages sensor
 * initialization and provides fallback mechanisms for robust operation.
 */
class ADPS9960_ColorSensor {
public:
    /**
     * @enum CalibrationStatus
     * @brief Enumeration for calibration state tracking
     */
    enum CalibrationStatus {
        NOT_CALIBRATED,           ///< Sensor has never been calibrated
        CALIBRATED_OK,            ///< Successfully calibrated with real data
        CALIBRATED_WITH_DEFAULTS  ///< Using default calibration values (fallback)
    };

    // Calibration constants - can be made configurable if needed
    static const int DEFAULT_SAMPLING_TIME = 5;        ///< Default calibration duration in seconds
    static const int MAX_SAMPLING_TIME = 10;           ///< Maximum allowed calibration time
    static const int MIN_SAMPLES_PER_SECOND = 5;       ///< Minimum sampling rate for valid calibration
    static const uint16_t MIN_THRESHOLD = 10;          ///< Minimum sensor reading to avoid dark conditions
    static const uint16_t SATURATION_THRESHOLD = 65000;///< Maximum value before sensor saturation
    static const uint16_t DEFAULT_MAX_VALUE = 1000;    ///< Default maximum value for normalization

    /**
     * @brief Constructor - initializes sensor object with uncalibrated state
     */
    ADPS9960_ColorSensor();

    /**
     * @brief Initialize the APDS9960 sensor hardware
     * @return true if initialization successful, false otherwise
     * @note Must be called before any other operations
     */
    bool begin();

    /**
     * @brief Calibrate the sensor by sampling colors over time
     * @param samplingTimeSeconds Duration of calibration (1-10s, default: 5s)
     * @param useDefaultsOnFail If true, uses default values on calibration failure
     * @return true if calibration successful, false otherwise
     * @note Point sensor at white surface during calibration for best results
     */
    bool calibrate(int samplingTimeSeconds = DEFAULT_SAMPLING_TIME,
                   bool useDefaultsOnFail = true);

    /**
     * @brief Get current calibration status
     * @return CalibrationStatus enum value
     */
    CalibrationStatus getCalibrationStatus() const;

    /**
     * @brief Check if sensor has been calibrated
     * @return true if calibrated (any method), false if not calibrated
     */
    bool isCalibrated() const;

    /**
     * @brief Get human-readable name of calibration status
     * @return String representation of current status
     */
    const char* getCalibrationStatusName() const;

    /**
     * @brief Force sensor to use default calibration values
     * @note Useful for testing or when proper calibration isn't possible
     */
    void setDefaultCalibration();

    /**
     * @struct RawColor
     * @brief Structure for raw 16-bit color sensor data
     */
    struct RawColor {
        uint16_t ambient;  ///< Ambient light level (0-65535)
        uint16_t red;      ///< Red channel raw value (0-65535)
        uint16_t green;    ///< Green channel raw value (0-65535)
        uint16_t blue;     ///< Blue channel raw value (0-65535)
    };

    /**
     * @struct RGB
     * @brief Structure for normalized 8-bit RGB values
     */
    struct RGB {
        uint8_t r;  ///< Red channel (0-255)
        uint8_t g;  ///< Green channel (0-255)
        uint8_t b;  ///< Blue channel (0-255)
    };

    /**
     * @struct HSV
     * @brief Represents a color in the HSV (Hue, Saturation, Value) color space
     */
    struct HSV {
        float h;  ///< Hue (0-360 degrees)
        float s;  ///< Saturation (0.0-1.0)
        float v;  ///< Value/Brightness (0.0-1.0)
    };

    /**
     * @brief Read raw 16-bit color data from sensor
     * @param raw Reference to RawColor struct to fill
     * @return true if read successful, false otherwise
     */
    bool readRawData(RawColor &raw);

    /**
     * @brief Read normalized RGB values (0-255 range)
     * @param r Reference to store red value
     * @param g Reference to store green value
     * @param b Reference to store blue value
     * @return true if read successful, false otherwise
     * @note Automatically calibrates with defaults if not yet calibrated
     */
    bool readRGB(uint8_t &r, uint8_t &g, uint8_t &b);

    /**
     * @brief Read color as 24-bit hexadecimal value
     * @return Color in 0xRRGGBB format, 0x000000 on error
     */
    uint32_t readColorHex();

    /**
     * @brief Read normalized RGB values into struct
     * @param rgb Reference to RGB struct to fill
     * @return true if read successful, false otherwise
     */
    bool readRGB(RGB &rgb);

    /**
     * @brief Read color as hexadecimal with error checking
     * @param hexColor Reference to store hex color value
     * @return true if read successful, false otherwise
     */
    bool readColorHex(uint32_t &hexColor);

    /**
     * @brief Get color as formatted hex string
     * @return String in "#RRGGBB" format (e.g., "#FF0000" for red)
     */
    String getColorHexString();

    /**
     * @brief Reads the current color as HSV from the sensor
     * @param hsvColor Reference to an HSV structure where the converted color
     *                 values (hue, saturation, value) will be stored
     * @return True if the color was successfully read and converted; false if
     *         the RGB reading failed or if the sensor encountered an error
     */
    bool readColorHSV(HSV &hsvColor);

    /**
     * @brief Check if current color matches custom HSV ranges
     * @param hMin Minimum hue value (0-360)
     * @param hMax Maximum hue value (0-360)
     * @param sMin Minimum saturation (0.0-1.0)
     * @param sMax Maximum saturation (0.0-1.0)
     * @param vMin Minimum value/brightness (0.0-1.0)
     * @param vMax Maximum value/brightness (0.0-1.0)
     * @return true if color is within specified ranges, false otherwise
     */
    bool isColorInRange(float hMin, float hMax, 
                       float sMin, float sMax, 
                       float vMin, float vMax);

    /**
     * @brief Check if current color matches a standard predefined color
     * @param color StandardColor enum value to check against
     * @param tolerance Optional tolerance factor (0.0-1.0, default: 0.15)
     * @return true if color matches the standard, false otherwise
     */
    bool isStandardColor(StandardColor color, float tolerance = 0.15f);

    /**
     * @brief Detect and return the closest matching standard color
     * @param tolerance Optional tolerance factor (0.0-1.0, default: 0.15)
     * @return StandardColor enum of detected color, UNKNOWN if no match or error
     * @note Checks colors in priority order: BLACK, WHITE, then chromatic colors
     */
    StandardColor detectColor(float tolerance = 0.15f);

private:
    CalibrationStatus calibrationStatus;  ///< Current calibration state
    uint16_t max_ambient;                 ///< Maximum ambient light during calibration
    uint16_t max_red;                     ///< Maximum red value during calibration
    uint16_t max_green;                   ///< Maximum green value during calibration
    uint16_t max_blue;                    ///< Maximum blue value during calibration

    SparkFun_APDS9960 sensor;  ///< Underlying sensor object from SparkFun library

    /**
     * @brief Internal calibration routine
     * @param samplingTimeSeconds Duration of calibration
     * @return true if calibration data is valid, false otherwise
     */
    bool performCalibration(int samplingTimeSeconds);

    /**
     * @brief Validate collected calibration data
     * @param samples Number of samples collected
     * @param minSamples Minimum required samples
     * @return true if data meets quality criteria, false otherwise
     */
    bool validateCalibrationData(int samples, int minSamples) const;
};

/**
 * @brief Normalize a 16-bit raw value to 8-bit RGB range
 * @param rawValue Raw sensor reading (0-65535)
 * @param maxValue Maximum value from calibration
 * @return Normalized value (0-255)
 * @note Handles overflow protection and division by zero
 */
uint8_t normalizeToRGB(uint16_t rawValue, uint16_t maxValue);

#endif //MANIGLIO_APDS_LIBRARY_APDS9960_COLORSENSOR_H