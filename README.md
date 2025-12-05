# APDS9960 Color Sensor Library

A high-level Arduino library for the APDS9960 RGB color sensor, featuring automatic calibration, multiple output formats, and robust error handling.

## Features

- 🎨 **Multiple Output Formats**: Raw 16-bit values, normalized RGB (0-255), and hexadecimal color codes
- 🔧 **Automatic Calibration**: Self-calibrating system with fallback defaults
- 🛡️ **Robust Error Handling**: Comprehensive validation and fail-safe mechanisms
- 📊 **Calibration Quality Tracking**: Monitor calibration status and quality
- 🚀 **Easy to Use**: Simple API with sensible defaults

## Hardware Requirements

### Sensor
- **APDS9960** RGB and Gesture Sensor module

### Microcontroller
- ESP32 (or any Arduino-compatible board with I2C support)

### Connections (I2C Wiring)

| APDS9960 Pin | ESP32 Pin | Description |
|--------------|-----------|-------------|
| VCC          | 3.3V      | Power supply (3.3V) |
| GND          | GND       | Ground |
| SDA          | GPIO 21   | I2C Data line |
| SCL          | GPIO 22   | I2C Clock line |

> **Note**: The APDS9960 operates at **3.3V logic level**. Do not connect to 5V!

#### Default ESP32 I2C Pins
- **SDA**: GPIO 21
- **SCL**: GPIO 22

You can use different pins by calling `Wire.begin(SDA_PIN, SCL_PIN)` before `sensor.begin()`.

## Dependencies

This library requires the following dependency:

- **SparkFun APDS9960 RGB and Gesture Sensor** (v1.4.3 or higher)
  - Repository: [SparkFun_APDS9960_RGB_and_Gesture_Sensor](https://github.com/sparkfun/SparkFun_APDS-9960_RGB_and_Gesture_Sensor_Arduino_Library)

### PlatformIO Installation

The dependency is automatically installed when using PlatformIO. The `platformio.ini` file already includes:
```ini
lib_deps =
    sparkfun/SparkFun APDS9960 RGB and Gesture Sensor@^1.4.3
```
### Arduino IDE Installation

1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for "SparkFun APDS9960"
4. Install **SparkFun APDS9960 RGB and Gesture Sensor** library

## Installation

### PlatformIO

1. Clone or download this library into your project's `lib` folder:
   ```bash
   cd your_project/lib
   git clone https://github.com/federicomaniglio/MANIGLIO_APDS9960_ColorSensor.git "Easy APDS9960_ColorSensor"
   ```

2. Or add it as a dependency in `platformio.ini`:
   ```ini
   lib_deps =
       sparkfun/SparkFun APDS9960 RGB and Gesture Sensor@^1.4.3
       fmaniglio/Easy APDS9960_ColorSensor@^1.0.0
   ```

### Arduino IDE

1. Download this library as a ZIP file
2. Open Arduino IDE
3. Go to **Sketch → Include Library → Add .ZIP Library**
4. Select the downloaded ZIP file
5. Ensure SparkFun APDS9960 library is also installed (see Dependencies section)

## Usage Examples

### Basic Color Reading (Hex Format)
```cpp
#include <APDS9960_ColorSensor.h>

ADPS9960_ColorSensor sensor;

void setup() {
    Serial.begin(115200);
    
    // Initialize sensor
    if (!sensor.begin()) {
        Serial.println("Sensor initialization failed!");
        while(1);
    }
    
    // Calibrate (point at white surface)
    Serial.println("Calibrating... point at white surface");
    if (!sensor.calibrate()) {
        Serial.println("Calibration failed! Using defaults.");
    }
    
    Serial.println("Ready!");
}

void loop() {
    // Read color as hex string
    String color = sensor.getColorHexString();
    Serial.println(color);  // Example output: "#FF0000"
    
    delay(1000);
}
```
### Reading Raw Values
```cpp
#include <APDS9960_ColorSensor.h>

ADPS9960_ColorSensor sensor;

void setup() {
    Serial.begin(115200);
    sensor.begin();
    sensor.calibrate();
}

void loop() {
    ADPS9960_ColorSensor::RawColor raw;
    
    if (sensor.readRawData(raw)) {
        Serial.print("Ambient: "); Serial.println(raw.ambient);
        Serial.print("Red: ");     Serial.println(raw.red);
        Serial.print("Green: ");   Serial.println(raw.green);
        Serial.print("Blue: ");    Serial.println(raw.blue);
    }
    
    delay(1000);
}
```
### Reading Normalized RGB Values
```cpp
#include <APDS9960_ColorSensor.h>

ADPS9960_ColorSensor sensor;

void setup() {
    Serial.begin(115200);
    sensor.begin();
    sensor.calibrate();
}

void loop() {
    uint8_t r, g, b;
    
    if (sensor.readRGB(r, g, b)) {
        Serial.print("RGB: ");
        Serial.print(r); Serial.print(", ");
        Serial.print(g); Serial.print(", ");
        Serial.println(b);
    }
    
    delay(1000);
}
```
## API Reference

### Initialization

#### `bool begin()`
Initialize the APDS9960 sensor hardware. Must be called before any other operations.

**Returns**: `true` if successful, `false` otherwise

---

### Calibration

#### `bool calibrate(int samplingTimeSeconds = 5, bool useDefaultsOnFail = true)`
Calibrate the sensor by sampling colors over time.

**Parameters**:
- `samplingTimeSeconds`: Duration of calibration (1-10s, default: 5s)
- `useDefaultsOnFail`: If true, uses default values on failure

**Returns**: `true` if calibration successful, `false` otherwise

**Note**: Point sensor at a white surface during calibration for best results.

#### `bool isCalibrated()`
Check if sensor has been calibrated.

**Returns**: `true` if calibrated (any method), `false` otherwise

#### `CalibrationStatus getCalibrationStatus()`
Get current calibration status.

**Returns**: Enum value (`NOT_CALIBRATED`, `CALIBRATED_OK`, or `CALIBRATED_WITH_DEFAULTS`)

#### `const char* getCalibrationStatusName()`
Get human-readable calibration status name.

---

### Reading Colors

#### `bool readRawData(RawColor &raw)`
Read raw 16-bit color data from sensor.

**Returns**: `true` if read successful, `false` otherwise

#### `bool readRGB(uint8_t &r, uint8_t &g, uint8_t &b)`
Read normalized RGB values (0-255 range).

**Returns**: `true` if read successful, `false` otherwise

#### `uint32_t readColorHex()`
Read color as 24-bit hexadecimal value.

**Returns**: Color in 0xRRGGBB format, 0x000000 on error

#### `String getColorHexString()`
Get color as formatted hex string.

**Returns**: String in "#RRGGBB" format (e.g., "#FF0000" for red)

---

## Calibration Process

The calibration process samples the sensor over a specified time period (default 5 seconds) and records maximum values for each color channel. These values are used to normalize future readings.

### Best Practices

1. **Use a white surface**: Point the sensor at a white or light-colored surface
2. **Stable lighting**: Ensure consistent lighting conditions during calibration
3. **Check status**: Verify calibration status after calibration
4. **Avoid saturation**: Don't calibrate under extremely bright light

### Calibration Validation Criteria

- Minimum sample count (5 samples/second)
- Values above threshold (avoids dark conditions)
- No saturation (prevents overexposure)
- Non-zero readings (detects hardware issues)

## Troubleshooting

### Sensor Not Responding

- Check wiring connections
- Verify 3.3V power supply (not 5V!)
- Ensure I2C pins are correct
- Check if I2C address conflicts exist

### Poor Calibration Results

- Increase calibration time: `sensor.calibrate(10)`
- Use a brighter white surface
- Ensure stable lighting conditions
- Check sensor isn't covered or obstructed

### Incorrect Color Readings

- Recalibrate the sensor
- Check lighting conditions
- Verify sensor is clean and unobstructed
- Ensure proper distance from target (optimal: 5-10mm)

## Author

Federico Maniglio - 2025

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## Acknowledgments

Built on top of SparkFun's APDS9960 library.


