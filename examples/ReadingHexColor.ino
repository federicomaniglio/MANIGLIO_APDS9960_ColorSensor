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
    // Read and print the color in hexadecimal format (#RRGGBB)
    // Example output: "#FF0000" for red, "#00FF00" for green, etc.
    Serial.println(sensor.getColorHexString());
    delay(1000); // Wait 1 second between readings
}
