#include "DHT.h"
#include <Arduino.h>
#include <SensirionI2cScd30.h>
#include <Wire.h>

//Specify available sensors in compile time using a bitflag
//DHT is often the most common termometer, so adding in as hygrometer as well, support for other kinds of hardware may be added later
//Thermometer and hygrometer 16
//SoilHygrometer 8
//Luminometer 4
//Co2 2
//PH 
#ifndef SENSORS
  #define SENSORS 0
#endif

//Same thing with actuators
//Irrigator 16
//Heater 8
//Lighting 4
//UV light 2
//Shading 1
#ifndef ACTUATORS
  #define ACTUATORS 0
#endif

//Inverted sensors flags
#ifndef INVERT
  #define INVERT 0
#endif

// macro definitions
// make sure that we use the proper definition of NO_ERROR
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

//Might later add support for pin selection
DHT dht(2, DHT11);
SensirionI2cScd30 sensor;

//Runtime read variables
float temperature = 0;
float humidity = 0;
float soil_hum = 0;
float light = 0;
float co2 = 0;
float ph = 0;

//Recieve commands on serial with bytes
char byte_stream[2];
int byte_read;

//Error messages for SCD30
static char errorMessage[64];
static int16_t error;

bool has_bit(int bitflag, int position) {
  int bit_val = ceil(pow(2, position));
  return (bitflag % (bit_val * 2)) >= bit_val;
}

//Decode sensor bitflag currently 5 sensors supported
bool sensor_active(int id) {
  return has_bit(SENSORS, id);
}

//Decode sensor bitflag currently 5 sensors supported
bool actuator_active(int id) {
  return has_bit(ACTUATORS, id);
}

//Helper function for displaying sensor data
String def_or_empty(float value, bool defined) {
  if (defined) {
    return String(value) + ",";
  }
  else {
    return "";
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  if (SENSORS > 31 || SENSORS < 0) {
    //Arduino doesn't handle exceptions very well, trap into loop instead
    while (true) {
      Serial.println("SENSORS bitflag unsupported, provide an integer between 0 and 31");
      delay(5000);
    }
  }
  else if (ACTUATORS > 31 || ACTUATORS < 0) {
    while (true) {
      Serial.println("ACTUATOR bitflag unsupported, provide an integer between 0 and 31");
      delay(5000);
    }
  }
  else if (ACTUATORS == 0 && SENSORS == 0) {
    while (true) {
      Serial.println("No sensors nor actuators bitflags provided, make sure to define them at compile time");
      delay(5000);
    }
  }

  //Start actuator pins as outputs
  for (int i = 8; i <= 12; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  //Activate DHT11
  if (sensor_active(4)){
    dht.begin();
  }
  //Sensiron scd30 configuration
  if (sensor_active(1)){
    Wire.begin();
    sensor.begin(Wire, SCD30_I2C_ADDR_61);

    sensor.stopPeriodicMeasurement();
    sensor.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    error = sensor.readFirmwareVersion(major, minor);
    if (error != NO_ERROR) {
        Serial.print("Error trying to execute readFirmwareVersion(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    error = sensor.startPeriodicMeasurement(0);
    if (error != NO_ERROR) {
      Serial.print("Error trying to execute startPeriodicMeasurement(): ");
      errorToString(error, errorMessage, sizeof errorMessage);
      Serial.println(errorMessage);
    }
  }
}

void loop() {
  //await instruction
  if (Serial.available() > 0){
    // read the incoming byte:
    byte_read = Serial.readBytes(byte_stream, 1);

    char input = byte_stream[0];
    int i_value;

    //Encode input into a bitflag from 0 to 35 more characters will be added if necessary
    if (isdigit(input)) {
      i_value = input - 48;

      if (i_value == 0) {
        if (SENSORS <= 0) {
          Serial.println("No data");
          return;
        }

        if (sensor_active(4)) {
          humidity = dht.readHumidity();
          temperature = dht.readTemperature();
        }

        if (sensor_active(3)) {
          soil_hum = (1023 - analogRead(A0)) / 10.23;
        }

        if (sensor_active(2)) {
          light = analogRead(A1) / 10.23;
        }

        if (sensor_active(1)) {
          //Prefer retrieving temperature and humidity from SCD30 sensor rather than DHT11
          sensor.blockingReadMeasurementData(co2, temperature, humidity);
        }

        Serial.println(
            def_or_empty(temperature, sensor_active(4)) +
            def_or_empty(humidity, sensor_active(4)) +
            def_or_empty(soil_hum, sensor_active(3)) +
            def_or_empty(light, sensor_active(2)) +
            def_or_empty(co2, sensor_active(1)) +
            def_or_empty(ph, sensor_active(0))
          );
        return;
      }
    }
    else if (isupper(input)){
      i_value = input - 55;
    }
    else {
      return;
    }

    //Limit bitflag to 5 bits + 1
    if (i_value > 32) { return; } 
    //Activate actuators using the provided bitflag
    for (int i = 8; i <= 12; i++) {
      digitalWrite(i, ((actuator_active(i - 8) && has_bit(i_value - 1, i - 8)) != has_bit(INVERT, i - 8)? HIGH : LOW));
    }

    Serial.print(input);
  }
}
