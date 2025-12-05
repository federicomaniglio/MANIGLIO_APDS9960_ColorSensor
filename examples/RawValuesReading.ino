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
    // Create a structure to hold the raw color values
    ADPS9960_ColorSensor::RawColor raw_color{};

    // Read raw color data from the sensor
    if (sensor.readRawData(raw_color)) {
        // Print all raw color values to Serial Monitor
        Serial.println("=== Color Sensor Reading ===");
        Serial.print("Ambient: ");
        Serial.println(raw_color.ambient);
        Serial.print("Red: ");
        Serial.println(raw_color.red);
        Serial.print("Green: ");
        Serial.println(raw_color.green);
        Serial.print("Blue: ");
        Serial.println(raw_color.blue);
        Serial.println("========================");
    }
    delay(1000); // Wait 1 second between readings
}
