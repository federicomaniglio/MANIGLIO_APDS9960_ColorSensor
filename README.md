# APDS9960 Color Sensor Library

A high-level Arduino library for the APDS9960 RGB color sensor, featuring automatic calibration, multiple output
formats, and robust error handling.

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

| APDS9960 Pin | ESP32 Pin | Description         |
|--------------|-----------|---------------------|
| VCC          | 3.3V      | Power supply (3.3V) |
| GND          | GND       | Ground              |
| SDA          | GPIO 21   | I2C Data line       |
| SCL          | GPIO 22   | I2C Clock line      |

> **Note**: The APDS9960 operates at **3.3V logic level**. Do not connect to 5V!

#### Default ESP32 I2C Pins

- **SDA**: GPIO 21
- **SCL**: GPIO 22

You can use different pins by calling `Wire.begin(SDA_PIN, SCL_PIN)` before `sensor.begin()`.

## Dependencies

This library requires the following dependency:

- **SparkFun APDS9960 RGB and Gesture Sensor** (v1.4.3 or higher)
  -
  Repository: [SparkFun_APDS9960_RGB_and_Gesture_Sensor](https://registry.platformio.org/libraries/sparkfun/SparkFun%20APDS9960%20RGB%20and%20Gesture%20Sensor)

### PlatformIO Installation

The dependency is automatically installed when using PlatformIO.

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
       fmaniglio/Easy APDS9960_ColorSensor@^1.0.1
   ```

### Arduino IDE

1. Download this library as a ZIP file
2. Open Arduino IDE
3. Go to **Sketch → Include Library → Add .ZIP Library**
4. Select the downloaded ZIP file
5. Ensure SparkFun APDS9960 library is also installed (see Dependencies section)

## Usage Examples

### Basic Color Reading (Hex Format)

```c++
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

```c++
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

```c++
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

## Color Detection

The library provides advanced color detection capabilities with predefined standard colors and customizable HSV range
checking.

### Standard Colors Supported

The library can detect the following predefined colors:

- **RED**: Warm red tones (wraps around 0° in HSV)
- **ORANGE**: Orange and amber tones
- **YELLOW**: Yellow and golden tones
- **GREEN**: Green tones (wide range)
- **CYAN**: Light blue and turquoise
- **BLUE**: Blue tones
- **PURPLE**: Purple tones
- **MAGENTA**: Pink and magenta tones
- **WHITE**: Light colors with low saturation
- **BLACK**: Very dark colors

### Detection Methods

The library offers multiple approaches to color detection:

#### 1. Automatic Color Detection

Automatically identifies the closest matching standard color:

```c++
#include <APDS9960_ColorSensor.h>
ADPS9960_ColorSensor sensor;
void setup() {
    Serial.begin(115200);
    sensor.begin();
    sensor.calibrate();
}
void loop() {
    // Detect color automatically
    StandardColor color = sensor.detectColor();
    // Get human-readable name
    Serial.print("Detected color: ");
    Serial.println(getStandardColorName(color));
    delay(1000);
}
```

#### 2. Specific Color Checking

Check if the current color matches a specific standard color:

```c++
// Check for red color with default tolerance (15%)
if (sensor.isStandardColor(StandardColor::RED)) { 
    Serial.println("This is RED!"); 
}

// Check for green with custom tolerance (20%)
if (sensor.isStandardColor(StandardColor::GREEN, 0.20f)) {
    Serial.println("This is GREEN (with 20% tolerance)!");
}
``` 

#### 3. Custom HSV Range Detection

For advanced use cases, you can define custom color ranges in HSV space:

```c++
// Check if color is in warm range (red, orange, yellow) 
// Hue: 0-80°, Saturation: 40-100%, Value: 30-100% 
if (sensor.isColorInRange(0, 80, 0.4, 1.0, 0.3, 1.0)) {
    Serial.println("This is a WARM color!"); 
}

// Check for pastel colors (low saturation, medium-high value)
if (sensor.isColorInRange(0, 360, 0.1, 0.4, 0.5, 0.9)) {
    Serial.println("This is a PASTEL color!");
}
```

#### 4. Reading HSV Values Directly

Access HSV values directly for custom processing:

```c++ 
ADPS9960_ColorSensor::HSV hsv;
if (sensor.readColorHSV(hsv)) {
    Serial.print("Hue: "); Serial.print(hsv.h);
    Serial.print("° | Sat: "); Serial.print(hsv.s * 100);
    Serial.print("% | Val: "); Serial.print(hsv.v * 100);
    Serial.println("%");
}
```

### Complete Color Detection Example

To see a complete example of color detection in action, check out the [code in the example](examples/StandardColorDetection.ino)


### Tolerance Parameter

The `tolerance` parameter (0.0-1.0) allows fine-tuning of color matching:

- **0.0**: Exact match required (very strict)
- **0.15**: Default tolerance (recommended for most cases)
- **0.30**: Relaxed matching (accepts similar colors)
- **1.0**: Very loose matching


### HSV Color Space Reference

![](/examples/HSV_color_wheel.png "HSV Color Wheel")

Understanding HSV values helps with custom color detection:

**Hue (H)**: Color type (0-360°)

- 0° = Red
- 60° = Yellow
- 120° = Green
- 180° = Cyan
- 240° = Blue
- 300° = Magenta

**Saturation (S)**: Color intensity (0.0-1.0)

- 0.0 = Grayscale (no color)
- 0.5 = Muted colors
- 1.0 = Vivid, pure colors

**Value (V)**: Brightness (0.0-1.0)

- 0.0 = Black
- 0.5 = Medium brightness
- 1.0 = Maximum brightness

### Tips for Color Detection

1. **Calibrate properly**: Always calibrate with a white surface for accurate color detection
2. **Consistent lighting**: Use stable lighting conditions for reliable results
3. **Optimal distance**: Keep sensor 5-10mm from target surface
4. **Adjust tolerance**: Increase tolerance for similar color detection, decrease for precision
5. **Use HSV for custom ranges**: HSV is more intuitive than RGB for defining color ranges

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

The calibration process samples the sensor over a specified time period (default 5 seconds) and records maximum values
for each color channel. These values are used to normalize future readings.

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


