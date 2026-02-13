#include <Arduino_MKRIoTCarrier.h>
#include "Images.h"

MKRIoTCarrier carrier;

char byte_stream[2];
int byte_read;

int col_r;
int col_g;
int col_b; 

float temperature = 0;
float humidity = 0;
int light = 0;
float moisture = 0;

//Pins
int moisture_p = A6;

//runtime
int count = 0;
int curr_display = -1;
const int rate = 100;

void set_display_mode(int mode){
  if (mode == curr_display){
    return;
  }

  switch (mode){
    case 0:
      carrier.display.setCursor(45, 160);
      carrier.display.setTextColor(ST77XX_WHITE);
      carrier.display.fillScreen(ST77XX_RED);
      carrier.display.drawBitmap(85, 50, thermometer, 62, 100, ST77XX_WHITE);
      carrier.display.print(String(temperature) + " C");
      break;

    case 1:
      carrier.display.setCursor(55, 160);
      carrier.display.setTextColor(ST77XX_BLACK);
      carrier.display.fillScreen(ST77XX_CYAN);
      carrier.display.drawBitmap(70, 50, cloud, 96, 99, ST77XX_BLACK);
      carrier.display.print(String(humidity) + "%");
      break;
  
    case 2:
      carrier.display.setCursor(65, 160);
      carrier.display.setTextColor(ST77XX_WHITE);
      carrier.display.fillScreen(ST77XX_BLUE);
      carrier.display.drawBitmap(80, 50, irrigation, 80, 99, ST77XX_WHITE);
      carrier.display.print(String(moisture) + "%");
      break;
    case 3:
      carrier.display.setCursor(30, 160);
      carrier.display.setTextColor(ST77XX_BLACK);
      carrier.display.fillScreen(ST77XX_YELLOW);
      carrier.display.drawBitmap(70, 50, sun, 102, 102, ST77XX_BLACK);
      carrier.display.print(String(light) + "W/M2");
      break;
  }

  curr_display = mode;
}

void setup() {
  carrier.withCase();
  carrier.begin();

  carrier.display.setTextSize(3);
  carrier.display.fillScreen(ST77XX_GREEN);
  carrier.display.setCursor(20, 140);
  carrier.display.drawBitmap(72, 50, leaf, 100, 100, 0xFFFF);
  carrier.display.print("Bienvenidos");
  delay(2000);

  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextSize(4);

  Serial.begin(9600);
  while (!Serial);
}

void loop() {
  // read all the sensor values
  while (!carrier.Light.colorAvailable()){
    delay(5);
  }
  carrier.Light.readColor(col_r, col_g, col_b);
  
  moisture = map(analogRead(moisture_p), 0, 1023, 100, 0);
  temperature = carrier.Env.readTemperature();
  humidity = carrier.Env.readHumidity();
  light = (col_r + col_g + col_b) / 3;

  //await instruction
  if (Serial.available() > 0){
    // read the incoming byte:
    byte_read = Serial.readBytes(byte_stream, 1);

    //
    switch (byte_stream[0]){
      case '0':
        // print each of the sensor values
        Serial.println(String(temperature) + "," + String(humidity) + "," + String(moisture) + ","  + String(light));
        break;

      //Open or close relays
      case '1':
        carrier.Relay1.close();
        carrier.Relay2.close();
        Serial.println("00");
        break;
      case '2':
        carrier.Relay1.open();
        carrier.Relay2.close();
        Serial.println("10");
        break;
      case '3':
        carrier.Relay1.close();
        carrier.Relay2.open();
        Serial.println("01");
        break;
      case '4':
        carrier.Relay1.open();
        carrier.Relay2.open();
        Serial.println("11");
        break;
    }
  }

  //Avoid freezing by counting loops
  set_display_mode(floor(count / 100));
  if (count >= rate * 4){
    count = 0;
  }
  count +=1;
}
