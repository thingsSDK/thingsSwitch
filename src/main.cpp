#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"

//Required for saving configuration to the device
#include "FS.h"
#include <ArduinoJson.h>

//Required for WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define SERIAL_BAUDRATE 115200

fauxmoESP fauxmo;

char device_name[40] = "indicator";

char pin[2] = "2";

int pinInt = pin[0] - '0';

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback()
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

// -----------------------------------------------------------------------------
// File System
// -----------------------------------------------------------------------------
void setupFS()
{

    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin())
    {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json"))
        {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile)
            {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);
                configFile.readBytes(buf.get(), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject &json = jsonBuffer.parseObject(buf.get());
                json.printTo(Serial);
                if (json.success())
                {
                    Serial.println("\nparsed json");
                    strcpy(device_name, json["device_name"]);
                    strcpy(pin, json["pin"]);
                }
                else
                {
                    Serial.println("failed to load json config");
                }
            }
        }
    }
    else
    {
        Serial.println("failed to mount FS");
    }
}

// -----------------------------------------------------------------------------
// Wifi
// -----------------------------------------------------------------------------

void wifiSetup()
{

    WiFiManagerParameter custom_device_name("device_name", "Device Name", device_name, 40);
    WiFiManagerParameter custom_pin("pin", "Pin", pin, 2);
    WiFiManager wifiManager;
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    wifiManager.addParameter(&custom_device_name);
    wifiManager.addParameter(&custom_pin);

    wifiManager.autoConnect("thingsSwitch");

    strcpy(device_name, custom_device_name.getValue());
    strcpy(pin, custom_pin.getValue());

    if (shouldSaveConfig)
    {
        Serial.println("saving config");
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.createObject();
        json["device_name"] = device_name;
        json["pin"] = pin;
        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile)
        {
            Serial.println("failed to open config file for writing");
        }

        json.printTo(Serial);
        json.printTo(configFile);
        configFile.close();
        //end save
    }

    if (pin)
    {
        pinInt = pin[0] - '0';
        pinMode(pinInt, OUTPUT);
        digitalWrite(pinInt, LOW);
    }
    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void callback(const char *device_name, bool state)
{

    Serial.print("Device ");
    Serial.print(device_name);
    Serial.print(" state: ");
    if (state)
    {
        digitalWrite(pinInt, HIGH);
        Serial.println("ON");
    }
    else
    {
        digitalWrite(pinInt, LOW);
        Serial.println("OFF");
    }
}

void setup()
{
    // Init serial port and clean garbage
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println("thingsSwitch");
    Serial.println("After connection, ask Alexa to 'turn <device_name> on' or 'turn <device_name> off'");

    //FS
    setupFS();

    // Wifi
    wifiSetup();

    // Fauxmo
    if (device_name)
    {
        fauxmo.addDevice(device_name);
        fauxmo.onMessage(callback);
    }
}

void loop()
{
}
