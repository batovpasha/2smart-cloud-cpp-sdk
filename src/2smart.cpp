#include "2smart.h"

#include "file_system/src/file_system.h"
#include "web_server_base/src/web_server_base.h"
#include "wifi_ap/src/wifi_ap.h"
#include "wifi_client/src/wifi_client.h"

MqttClient *mqtt_client = new MqttClient();
Homie homie(mqtt_client);
Notifier notifier(mqtt_client);
Device device(&homie);
WebServerBase web_server(&device);
NtpTimeClient *time_client = new NtpTimeClient();

WifiClient wifi_client;

// ----------------------------------------------------------HTTP-----------
String ssid_name = "Wifi_Name";  // WiFi name
String ssid_password = "";       // WiFi password
String person_mail = "";
String person_id = "";
String token = "";
String host = "cloud.2smart.com";
String broker_port = "1883";
String web_auth_password = "";
const char *http_username = "admin";
// -------------------------------------------------------Production settings
String device_id = "";  // DeviceID/ MAC:adress
// -------------------------------------------------------MQTT variables

const char *firmware_name = product_id.c_str();

Cloud2Smart::Cloud2Smart() {}

void Cloud2Smart::setup() {
    Serial.begin(115200);
    if (!InitFiles() || !LoadConfig()) {
        delay(5000);
        ESP.restart();
    }

    String mac = WiFi.macAddress().c_str();
    if (device_id.length() <= 1) {
        String bufferMacAddr = WiFi.macAddress();
        bufferMacAddr.toLowerCase();
        bufferMacAddr.replace(":", "-");
        device_id = bufferMacAddr;
    }
    String ip_addr = WiFi.localIP().toString();

    // ---------------------------------------------- Homie convention init
    AutoUpdateFw *firmware = new AutoUpdateFw("Firmware", "firmware", &device);                   // (name, id,device)
    Notifications *notifications = new Notifications("Notifications", "notifications", &device);  // (name,id, device)

    Property *update_status = new Property("update status", "updatestate", firmware, SENSOR, false, false, "string");
    Property *update_button = new Property("update button", "update", firmware, SENSOR, true, false, "boolean");
    Property *update_time = new Property("update time", "updatetime", firmware, SENSOR, true, true, "string");
    Property *auto_update = new Property("autoUpdate", "autoupdate", firmware, SENSOR, true, true, "boolean");
    Property *fw_version = new Property("version", "version", firmware, SENSOR, false, true, "integer");
    // ------------- notification`s properties
    Property *system_notification =
        new Property("System Notifications", "system", notifications, SENSOR, true, true, "boolean");
    Property *update_notification =
        new Property("Update Notifications", "update", notifications, SENSOR, true, true, "boolean");

    DeviceData device_data{device_name, device_version, product_id.c_str(), ip_addr.c_str(), "esp32",
                           mac.c_str(), "ready",        device_id.c_str()};
    notifier.SetUserHash(person_id);

    device.SetCredentials(device_data);
    device.SetNotifier(&notifier);

    Property *dev_ip = new Property("ipw", "ipw", &device, TELEMETRY, false, true, "string");

    firmware->SetTimeClient(time_client);

    homie.SetDevice(&device);

    WifiAp wifiAP;
    if (ssid_name == "Wifi_Name" || ssid_name == "") {
        wifiAP.Start(device_name);
        web_server.Init();
    }
    while (ssid_name == "Wifi_Name" || ssid_name == "") {
        // Handling buttons and offline logic
        device.HandleCurrentState();
        wifiAP.Blink();
    }
    wifi_client.SetCredentials(ssid_name, ssid_password);
    while (!wifi_client.Connect()) {
        // Handling buttons and offline logic
        device.HandleCurrentState();
        if (erase_flag) {
            EraseFlash();
        }
    }
    time_client->Init();
    web_server.Init();

    ip_addr = WiFi.localIP().toString();
    Serial.print("IP: ");
    Serial.println(ip_addr);

    while (!homie.Init(person_id, host, broker_port, token, HandleMessage)) {
        device.HandleCurrentState();
    }
    dev_ip->SetValue(ip_addr);

    // ---------------------------------------------- Homie convention end
}

void Cloud2Smart::loop() {
    wifi_client.Connect();

    homie.HandleCurrentState();  // mqttLoop();

    if (erase_flag) {
        EraseFlash();
    }
}

Device *Cloud2Smart::GetDevice() { return &device; }

void HandleMessage(char *topic, byte *payload, unsigned int length) {
    homie.HandleMessage(String(topic), payload, length);
}
