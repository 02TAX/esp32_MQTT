#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <mqtt_client.h>
#include <cJSON.h>
#include <vector>

using namespace std;

class Equipment{
  private:
    String Name;
    int Pin;
  public:
    // Equipment(int val) : Pin(val) {}
    void Equipment_name(String name_set) {
      Name = name_set;
    }
    void Equipment_pin(int pin_set) {
      Pin = pin_set;
    }
    String getName(){
          return Name;
      }
    int getPin(){
          return Pin;
      }
};

//Pin_set
int led_pin = 12;

// WiFi
// const char *ssid = "DoubleQ"; // Enter your WiFi name
// const char *password = "Doubleq.com";  // Enter WiFi password
const char *ssid = "ziroom304"; // Enter your WiFi name
const char *password = "4001001111";  // Enter WiFi password

// MQTT Broker
// const char *mqtt_broker = "192.168.3.116";
const char *mqtt_broker = "192.168.0.120";
const char *mqtt_topic_send  = "esp32/send";
const char *mqtt_topic_recv ="esp32/recv";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
// const char mqtt_topic[];

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// Function Declarations
void connectToWiFi();
void connectToMQTT();
void mqttCallback(char *mqtt_topic, byte *payload, unsigned int length);
String Json_msg(char *msg);
int Json_rcv(char *payload,std::vector<String> &Equipment_Control);
void Boot_Equipment_list(std::vector<Equipment> &Equipment_list,String name,int pin);

//Equipment_list
std::vector<Equipment> Equipment_list;

//创建设备表(包含每一个的具体信息:设备名、引脚、引脚模式)
void Boot_Equipment_list(std::vector<Equipment> &Equipment_list,String name,int pin,uint8_t mode){
  Equipment equipment;

  equipment.Equipment_name(name);
  equipment.Equipment_pin(pin);
  pinMode(pin,mode);  

  Equipment_list.push_back(equipment);
}

void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi\n");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi\n");
}

 /*发送结构
  {
    "msg":msg,
      "test":{
      "msg":msg
      }
  }
  */
String Json_msg(char *msg){

  cJSON *pRoot = cJSON_CreateObject();   
  cJSON_AddStringToObject(pRoot,"msg",msg);
  cJSON *test = cJSON_CreateObject(); 
  cJSON_AddItemToObject(pRoot, "test",test);
  cJSON_AddStringToObject(test,"msg",msg);
 
  char *sendData = cJSON_Print(pRoot);
  String data(sendData);

  return data;
}

//未写完，还需要加入纠错反馈
int Json_rcv(char *payload,std::vector<String> &Equipment_Control){
  
  // std::vector<String>Equipment_Control(2);
  Equipment_Control.resize(2);
  cJSON *pJsonRoot = cJSON_Parse(payload);
  if (pJsonRoot != NULL)
  {
    cJSON *pEquipment = cJSON_GetObjectItem(pJsonRoot, "equipment");
    cJSON *pControl = cJSON_GetObjectItem(pJsonRoot, "control");

    if (pEquipment != NULL&&pControl != NULL){
      if (cJSON_IsString(pEquipment)&&cJSON_IsString(pControl))                          
      {          
        String Equipment,Control;
        Serial.printf("Rcv Message: %s,%s\n",pEquipment->valuestring,pControl->valuestring);
        Equipment_Control[0] = pEquipment->valuestring;
        Equipment_Control[1] = pControl->valuestring;

        return 0;
      }
    }
  }
  return -1;
}

void device(std::vector<String> &Equipment_Control,std::vector<Equipment> Equipment_list){

  int Device_number = 0;
  for (int i = 0; i < Equipment_list.size(); i++)
  {
    if(strcmp(Equipment_Control[0].c_str(),Equipment_list[i].getName().c_str()) == 0){
      Device_number = i;
      if (Equipment_Control[1] == "ON")
      {
        digitalWrite(Equipment_list[i].getPin(),HIGH);
      }
      // Serial.printf("Device_number: %d\n",Device_number);
      else if (Equipment_Control[1] == "OFF")
      {
        digitalWrite(Equipment_list[i].getPin(),LOW);
      }
    }
    else
    {
      Serial.printf("error!\n");
    }
  }
}

void connectToMQTT() {
    while (!mqtt_client.connected()) {
        String client_id = "esp32-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic_recv);
            String send_data = Json_msg("Hi EMQX I'm ESP32 ^^");
            mqtt_client.publish(mqtt_topic_send, send_data.c_str()); // Publish message upon successful connection
        } 
        else {
            Serial.print("Failed, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *mqtt_topic, byte *payload, unsigned int length) {

    vector<String> Equipment_Control;
    Serial.print("Message received on mqtt_topic: ");
    Serial.println(mqtt_topic);
    if (strcmp(mqtt_topic,mqtt_topic_recv)==0)
    {
      // digitalWrite(led_pin, Json_rcv((char *)payload));
      String Equipment,Control;
      int ret = Json_rcv((char *)payload,Equipment_Control);
      if(ret == 0){
        Equipment = Equipment_Control[0];
        Control = Equipment_Control[1];
      }
      else{
        Serial.printf("data error!!");
      }
      Serial.printf("Equipment: %s\n",Equipment);
      Serial.printf("Control: %s\n",Control);
      //设备驱动(获取MQTT发来的控制设备名和控制指令)
      device(Equipment_Control,Equipment_list);
    }
    // for (unsigned int i = 0; i < length; i++) {
    //     Serial.print((char) payload[i]);
    // }
    Serial.println("\n-----------------------");
}

void setup() {
  //驱动的setup以及创建设备的类（具体信息）
  // Boot_Equipment_list(Equipment_list,"motor",1);
  // Boot_Equipment_list(Equipment_list,"window",5);
  Boot_Equipment_list(Equipment_list,"LED",led_pin,OUTPUT);

  Serial.begin(9600);

  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setKeepAlive(60);
  mqtt_client.setCallback(mqttCallback); // Corrected callback function name
  connectToMQTT();
}

void loop() {
    if (!mqtt_client.connected()) {
        connectToMQTT();
    }
    mqtt_client.loop();
}

