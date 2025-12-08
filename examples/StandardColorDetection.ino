#include <APDS9960_ColorSensor.h>
// Create an instance of the color sensor
ADPS9960_ColorSensor sensor;

void setup() {
    // Initialize serial communication at 115200 baud rate
    Serial.begin(115200);

    // Initialize the APDS9960 sensor
    sensor.begin();
    Serial.println("APDS9960 ready!");
    delay(1000);

    // Perform sensor calibration
    // Point sensor at a white surface during calibration for best results
    if (!sensor.calibrate())
        Serial.println("Error during calibration!");
    Serial.println("Calibration completed!");
    delay(1000);

    // Check and display the calibration status
    if (sensor.isCalibrated())
        Serial.println(sensor.getCalibrationStatusName());
}

void loop() {
    Serial.println("=== Color Detection Demo ===");

    // Method 1: detectColor() - Automatically identify the color
    StandardColor color = sensor.detectColor();

    // Method 2: getStandardColorName() - Convert enum to readable name
    // used above to display the color name
    Serial.print("Detected color: ");
    Serial.println(getStandardColorName(color));

    // Method 3: isStandardColor() - Check for specific colors
    if (sensor.isStandardColor(StandardColor::RED)) {
        Serial.println("-> This is RED!");
    }
    if (sensor.isStandardColor(StandardColor::GREEN, 0.20f)) {
        Serial.println("-> This is GREEN (with 20% tolerance)!");
    }

    // Method 4: isColorInRange() - Check custom HSV ranges
    // Example: Check if it's a warm color (red, orange, yellow)
    if (sensor.isColorInRange(0, 80, 0.4, 1.0, 0.3, 1.0)) {
        Serial.println("-> This is a WARM color!");
    }

    Serial.println("============================");
    delay(2000); // Wait 2 seconds between readings
}