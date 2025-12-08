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
     * @class HSV
     * @brief Represents a color in the HSV (Hue, Saturation, Value) color space
     */
    struct HSV {
        float h;
        float s;
        float v;
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
     * @brief Reads the current color as HSV from the sensor.     *
     * @param hsvColor Reference to an HSV structure where the converted color
     *                 values (hue, saturation, value) will be stored.
     * @return True if the color was successfully read and converted; false if
     *         the RGB reading failed or if the sensor encountered an error.
     */
    bool readColorHSV(HSV &hsvColor);

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