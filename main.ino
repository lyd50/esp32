#define MS_PER_HOUR    3.6e6

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "config.h"
#include "main.h"

WiFiClient espClient;
PubSubClient client(espClient);


unsigned long SEND_FREQUENCY = 10000;
unsigned long lastSend;
char msg[50]; //mqtt message buffer

Sensor power;
Sensor gas;
Sensor water;

void sensorSetup(){
    pinMode(13, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(13), powerPulse, RISING);
    power.id = "power";
    power.factor = 6000; //rotations per unit
    power.deBounce = 250; //ms
    power.reset = 60000; //ms reset to 0
    power.topicUsage = "utilities/power/wat";
    power.topicMeter = "utilities/power/kwh";
    power.topicCounter = "utilities/power/count";

    pinMode(4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(4), gasPulse, RISING);
    gas.id = "gas";
    gas.factor = 100; //rotations per unit
    gas.deBounce = 250; //ms
    gas.reset = 60000; //ms value timeout
    gas.topicUsage = "utilities/gas/liter";
    gas.topicMeter = "utilities/gas/m3";
    gas.topicCounter = "utilities/gas/count";

    pinMode(25, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(25), waterPulse, RISING);
    water.id = "water";
    water.factor = 1000; //rotations per unit
    water.deBounce = 250; //ms
    water.reset = 60000; //ms value timeout
    water.topicUsage = "utilities/water/liter";
    water.topicMeter = "utilities/water/m3";
    water.topicCounter = "utilities/water/count";
}

void sendSensors()
{
    sendSensor(&power);
    sendSensor(&gas);
    sendSensor(&water);
}

void powerPulse()
{
    Pulse(&power);
}
void gasPulse()
{
    Pulse(&gas);
}
void waterPulse()
{
    Pulse(&water);
}

void receiveCounters(char* topic, byte* payload, unsigned int length)
{
    receiveCount(&power, topic, payload, length);
    receiveCount(&gas, topic, payload, length);
    receiveCount(&water, topic, payload, length);
}

void receiveSubscribes()
{
    client.subscribe(power.topicCounter);
    client.subscribe(gas.topicCounter);
    client.subscribe(water.topicCounter);
}

/*
 * Start of core functions
 */

void setup() {
    sensorSetup();
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, SSID_PASSWORD); 
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback);
    ArduinoOTA.onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
            else // U_SPIFFS
            type = "filesystem";

            noInterrupts();
            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start updating " + type);
            });
    ArduinoOTA.onEnd([]() {
            Serial.println("\nEnd");
            });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            });
    ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
            });
    ArduinoOTA.begin();
    Serial.begin(115200);
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() {
    unsigned long now = millis();
    // Only send values at a the configured frequency or when woken up from sleep
    bool sendTime = now - lastSend > SEND_FREQUENCY;
    if (sendTime) {
        lastSend = now;
        sendSensors();
    }

    //MQTT
    if (!client.connected()) {
        mqttReconnect();
    }
    client.loop();
    ArduinoOTA.handle();
    yield();
}


void Pulse(Sensor *sensor) {
    unsigned long now = millis();
    unsigned long time = now - sensor->lastPulse; 

    if(time > sensor->deBounce)
    {
        sensor->count++;
        sensor->usage = 1000 * ((double) MS_PER_HOUR / time) / sensor->factor;
        sensor->lastPulse = now;
    }
}

void sendSensor(Sensor *sensor)
{
    unsigned long now = millis();
    unsigned long time = now - sensor->lastPulse; 
    if(time > sensor->reset && sensor->usage > 0)
    {
        sensor->usage = 0;
        snprintf (msg, 50, "%ld", sensor->usage);
        client.publish(sensor->topicUsage, msg, true);
        Serial.print(sensor->id);
        Serial.print("-U: ");
        Serial.println(msg);
    }

    if (sensor->count != sensor->oldCount) {
        sensor->oldCount = sensor->count;

        snprintf (msg, 50, "%ld", sensor->count);
        client.publish(sensor->topicCounter, msg, true);
        Serial.print(sensor->id);
        Serial.print("-C: ");
        Serial.println(msg);

        double kwh = ((double)sensor->count/((double)sensor->factor));
        snprintf (msg, 50, "%s", String(kwh,3).c_str());
        client.publish(sensor->topicMeter, msg, true);
        Serial.print(sensor->id);
        Serial.print("-M: ");
        Serial.println(msg);

        snprintf (msg, 50, "%ld", sensor->usage);
        client.publish(sensor->topicUsage, msg, true);
        Serial.print(sensor->id);
        Serial.print("-U: ");
        Serial.println(msg);
    }
}

void receiveCount(Sensor *sensor, char* topic, byte* payload, unsigned int length)
{
    if ( strcmp(topic,sensor->topicCounter)==0)
    {
        char data[length];
        for (int i=0;i<length;i++) {
            data[i] = (char)payload[i];
        }
        unsigned long receivedPulseCount = atoi(data);
        if (receivedPulseCount > sensor->count || sensor->count == 0)
        {
            sensor->count = receivedPulseCount;
            Serial.print("Updated counter: ");
            Serial.println(sensor->id);
        }
    }
}

/*
 * Routine to process incomming messages from mqtt 
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    receiveCounters(topic,payload,length);
}

/*
 * MQTT reconnect routine
 */
void mqttReconnect() {
    // reconnect code from PubSubClient example
    String clientId = "SensorNode-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
        receiveSubscribes();
    } else {
        delay(5000);
    }
}
