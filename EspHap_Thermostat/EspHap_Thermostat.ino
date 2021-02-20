#include <M5EPD.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

const char* ssid     = "ssid";
const char* password = "password";

extern "C" {
#include "homeintegration.h"
}
homekit_service_t* hapservice = {0};
String pair_file_name = "/pair.dat";

void Format() {
  // Next lines have to be done ONLY ONCE!!!!!When SPIFFS is formatted ONCE you can comment these lines out!!
  Serial.println("Please wait 30 secs for SPIFFS to be formatted");
  SPIFFS.format();
  Serial.println("Spiffs formatted");
}

void init_hap_storage() {

  Serial.print("init_hap_storage");
  if (!SPIFFS.begin(true)) {
    Serial.print("SPIFFS Mount failed");
  }

  //HomeKitアクセサリーが認識しない場合は、SPIFFSをFormatしてください。
  // Format();

  File fsDAT = SPIFFS.open(pair_file_name, "r");
  if (!fsDAT) {
    Serial.println("Failed to read pair.dat");
    return;
  }
  int size = hap_get_storage_size_ex();
  char* buf = new char[size];
  memset(buf, 0xff, size);
  int readed = fsDAT.readBytes(buf, size);
  Serial.print("Readed bytes ->");
  Serial.println(readed);
  hap_init_storage_ex(buf, size);
  fsDAT.close();
  delete []buf;
}

void storage_changed(char * szstorage, int size) {
  SPIFFS.remove(pair_file_name);
  File fsDAT = SPIFFS.open(pair_file_name, "w+");
  if (!fsDAT) {
    Serial.println("Failed to open pair.dat");
    return;
  }
  fsDAT.write((uint8_t*)szstorage, size);

  fsDAT.close();
}

M5EPD_Canvas canvas(&M5.EPD);
homekit_service_t* temperature;
homekit_service_t* humidity;
void setup() {

  M5.begin();
  M5.SHT30.Begin();
  M5.EPD.SetRotation(180);
  M5.EPD.Clear(true);
  canvas.createCanvas(950, 530);
  canvas.setTextSize(6);  

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  init_hap_storage();
  set_callback_storage_change(storage_changed);

  hap_setbase_accessorytype(homekit_accessory_category_thermostat);
  hap_initbase_accessory_service("M5Paper Sensor", "Ry0_Ka", "000000", "HAP-ENV", "1.0");

  temperature = hap_add_temperature_service("Temperature Sensor");
  humidity = hap_add_humidity_service("Humidity Sensor");

  //and finally init HAP
  hap_init_homekit_server();

}

char temStr[10];
char humStr[10];

float tem;
float hum;

void loop() {
  M5.SHT30.UpdateData();
  tem = M5.SHT30.GetTemperature();
  hum = M5.SHT30.GetRelHumidity();
  Serial.printf("Temperature: %2.2f*C  Humidity: %0.2f%%\r\n", tem, hum);
  dtostrf(tem, 2, 2 , temStr);
  dtostrf(hum, 2, 2 , humStr);
  canvas.drawString("Temperature:" + String(temStr) + "*C", 100, 0);
  canvas.drawString("Humidity:" + String(humStr) , 100, 100);
  canvas.pushCanvas(0,300,UPDATE_MODE_A2);
  
  homekit_characteristic_t * ch_temp = homekit_service_characteristic_by_type(temperature, HOMEKIT_CHARACTERISTIC_CURRENT_TEMPERATURE);
  ch_temp->value.float_value = tem;
  homekit_characteristic_notify(ch_temp, ch_temp->value);
  homekit_characteristic_t * ch_hum = homekit_service_characteristic_by_type(humidity, HOMEKIT_CHARACTERISTIC_CURRENT_RELATIVE_HUMIDITY);
  ch_hum->value.float_value = hum;
  homekit_characteristic_notify(ch_hum, ch_hum->value);

  delay(2500);
}
