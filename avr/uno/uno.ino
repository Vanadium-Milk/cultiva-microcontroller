//Specify available sensors in compile time using a bitflag
//DHT is often the most common termometer, so adding in as hygrometer as well, support for other kinds of hardware may be added later
#ifndef SENSORS
  #define SENSORS 16
#endif

//Same thing with actuators
#ifndef ACTUATORS
  #define ACTUATORS 0
#endif

//Might later add support for pin selection
#include "DHT.h"
DHT dht(2, DHT11);

//Runtime read variables
float temperature = 0;
float humidity = 0;
float soil_hum = 0;
int light = 0;
float co2 = 0;
float ph = 0;

//Recieve commands on serial with bytes
char byte_stream[2];
int byte_read;

//Decode sensor bitflag currently 6 sensors supported
//Thermometer and hygrometer 16
//SoilHygrometer 8
//Luminometer 4
//Co2 2
//PH 
bool is_s_active(int id) {
  int bit_val = ceil(pow(2, id));
  return (SENSORS % (bit_val * 2)) >= bit_val;
}

String def_or_empty(int value, bool defined) {
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

  if (is_s_active(4)){
    dht.begin();
  }
}

void loop() {
  //await instruction
  if (Serial.available() > 0){
    // read the incoming byte:
    byte_read = Serial.readBytes(byte_stream, 1);

    //Using another bitflag for actuator relays
    switch (byte_stream[0]){
      case '0':
        // read all the sensor values, skip if no sensors are available
        if (SENSORS <= 0) {
          Serial.println("No data");
          return;
        }

        if (is_s_active(4)) {
          humidity = dht.readHumidity();
          temperature = dht.readTemperature();
        }

        Serial.println(
            def_or_empty(temperature, is_s_active(4)) +
            def_or_empty(humidity, is_s_active(4)) +
            def_or_empty(soil_hum, is_s_active(3)) +
            def_or_empty(light, is_s_active(2)) +
            def_or_empty(co2, is_s_active(1)) +
            def_or_empty(ph, is_s_active(0))
          );
        break;

      //Open or close relays
      case '1':
        Serial.println("00");
        break;
      case '2':
        Serial.println("10");
        break;
      case '3':
        Serial.println("01");
        break;
      case '4':
        Serial.println("11");
        break;
    }
  }
}
