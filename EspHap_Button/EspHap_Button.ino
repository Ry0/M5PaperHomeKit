#include <M5EPD.h>
#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>

const char *ssid = "ssid";
const char *password = "password";

extern "C"
{
#include "homeintegration.h"
}

typedef enum
{
    button_event_single_press,
    button_event_double_press,
    button_event_long_press,
} button_event_t;

homekit_service_t *hapservice = {0};
homekit_service_t *hapservice2 = {0};
String pair_file_name = "/pair.dat";

void Format()
{
    // Next lines have to be done ONLY ONCE!!!!!When SPIFFS is formatted ONCE you can comment these lines out!!
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
}

void init_hap_storage()
{

    Serial.print("init_hap_storage");
    if (!SPIFFS.begin(true))
    {
        Serial.print("SPIFFS Mount failed");
    }

    //HomeKitアクセサリーが認識しない場合は、SPIFFSをFormatしてください。
    //Format();

    File fsDAT = SPIFFS.open(pair_file_name, "r");
    if (!fsDAT)
    {
        Serial.println("Failed to read pair.dat");
        return;
    }
    int size = hap_get_storage_size_ex();
    char *buf = new char[size];
    memset(buf, 0xff, size);
    int readed = fsDAT.readBytes(buf, size);
    Serial.print("Readed bytes ->");
    Serial.println(readed);
    hap_init_storage_ex(buf, size);
    fsDAT.close();
    delete[] buf;
}

void storage_changed(char *szstorage, int size)
{
    SPIFFS.remove(pair_file_name);
    File fsDAT = SPIFFS.open(pair_file_name, "w+");
    if (!fsDAT)
    {
        Serial.println("Failed to open pair.dat");
        return;
    }
    fsDAT.write((uint8_t *)szstorage, size);

    fsDAT.close();
}

void buttonhandler_callback(homekit_service_t *service, button_event_t event)
{

    Serial.print("buttonhandler_callback");
    switch (event)
    {
    case button_event_single_press:
        notifyHap(service, 0);
        break;
    case button_event_double_press:
        notifyHap(service, 1);
        break;
    case button_event_long_press:
        notifyHap(service, 2);
        break;
    }
}

M5EPD_Canvas canvas(&M5.EPD);
void setup()
{
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

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    init_hap_storage();
    set_callback_storage_change(storage_changed);

    /// We will use for this example only one accessory (possible to use a several on the same esp)
    //Our accessory type is light bulb , apple interface will proper show that
    hap_setbase_accessorytype(homekit_accessory_category_programmable_switch);
    /// init base properties
    hap_initbase_accessory_service("M5Paper Button", "Ry0_Ka", "0", "EspHapButton", "1.0");

    //we will add only one light bulb service and keep pointer for nest using
    hapservice = hap_add_button_service("Button");  //,button_callback,(void*)&button_gpio);
    hapservice2 = hap_add_button_service("Button"); //,button_callback,(void*)&button_gpio);

    //and finally init HAP
    hap_init_homekit_server();
}

void ButtonTest(homekit_service_t *service, char *str)
{
    button_event_t event_type = button_event_single_press;
    buttonhandler_callback(service, event_type);
    canvas.fillCanvas(0);
    canvas.drawString(str, 100, 0);
    canvas.pushCanvas(0, 300, UPDATE_MODE_DU4);
    delay(500);
}

void loop()
{
    if (M5.BtnL.wasPressed())
    {
        ButtonTest(hapservice, "Btn L Pressed");
    }
    if (M5.BtnP.wasPressed())
    {
        ButtonTest(hapservice, "Btn P Pressed");
    }
    if (M5.BtnR.wasPressed())
    {
        ButtonTest(hapservice2, "Btn R Pressed");
    }
    M5.update();
    delay(100);
}

void notifyHap(homekit_service_t *service, uint8_t val)
{
    if (service)
    {
        Serial.println("notify hap");
        //getting PROGRAMMABLE_SWITCH_EVENT characteristic
        homekit_characteristic_t *ch = homekit_service_characteristic_by_type(service, HOMEKIT_CHARACTERISTIC_PROGRAMMABLE_SWITCH_EVENT);
        if (ch)
        {
            ch->value.int_value = val;
            homekit_characteristic_notify(ch, ch->value);
            //      if (ch->value.int_value != val)
            //      { //wil notify only if different
            //        Serial.println("wil notify only if different");
            //        ch->value.int_value = val;
            //        homekit_characteristic_notify(ch, ch->value);
            //      }
        }
    }
}
