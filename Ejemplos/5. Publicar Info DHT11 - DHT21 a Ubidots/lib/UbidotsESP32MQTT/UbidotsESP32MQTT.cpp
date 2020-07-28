/**
 * @file UbidotsESP32MQTT.cpp
 * @mainpage Librería Ubidots MQTT para ESP32
 * 
 * @section sec_intro Introducción
 * Esta es la documentación para la librería Ubidots MQTT para ESP32 en español para la plataforma
 * Arduino, modificada desde su <a href="https://github.com/ubidots/ubidots-mqtt-esp">repositorio
 * original</a>, con la finalidad de crear compatibilidad con las placas basadas en ESP32. También
 * se ha optimizado para que el sistema no sea bloqueante (en el código original hay bucles
 * infinitos si no hay conexión a WiFi).
 * 
 * @section sec_dependencias Dependencias
 * Esta libreria depende de <a href="https://github.com/knolleary/pubsubclient">PubSubClient</a>,
 * por favor asegúrese de instalarla, o descargarla y añadirla a su proyecto.
 * 
 * @section sec_ejemplos Ejemplos
 * Para revisar ejemplos de esta librería, dirigirse a la carpeta "ejemplos" dentro del mismo
 * directorio. Estos han sido creados utilizando Visual Studio Code, con la extensión PlatformIO IDE.
 * 
 * @author Max (maximiliano.ramirez@creatiox.com) para <a href="https://www.creatiox.com">Creatiox</a>
 * @copyright Copyright (c) 2020 Creatiox
 * 
 */

#include "UbidotsESP32MQTT.h"

Ubidots::Ubidots(const char* token){
  initialize(token, NULL);
}

Ubidots::Ubidots(const char* token, char* clientName) {
  initialize(token, clientName);
}

void Ubidots::begin(void (*callback)(char*,uint8_t*,unsigned int)) {
  this->callback = callback;
  _client.setServer(_server, MQTT_PORT);
  _client.setCallback(callback);
}

void Ubidots::wifiConnection(const char* ssid, const char* pass) {
  WiFi.begin(ssid, pass);
  
  if(_clientName == NULL){
    _clientName = getMac();
  }
}

bool Ubidots::connected(){
  return _client.connected();
}

void Ubidots::reconnect() {
  if (_debug) {
    Serial.print("[UDOTS] Intentando conexion MQTT... ");
  }        

  if (_client.connect(_clientName, _token, NULL)) {
    if (_debug) {
      Serial.println("conectado!");
    }
        
  } else {
    if (_debug) {
      Serial.print("error, rc=");
      Serial.println(_client.state());      
    }      
  }
}

bool Ubidots::loop() {    
  return _client.loop();
}

void Ubidots::setDebug(bool debug){
    _debug = debug;
}

void Ubidots::add(const char* variableLabel, float value) {
  return add(variableLabel, value, NULL, 0);
}

void Ubidots::add(const char* variableLabel, float value, char *context) {
  return add(variableLabel, value, context, 0);
}

void Ubidots::add(const char* variableLabel, float value, char *context, uint32_t timestamp) {
  (val+currentValue)->_variableLabel = variableLabel;
  (val+currentValue)->_value = value;
  (val+currentValue)->_context = context;
  (val+currentValue)->_timestamp = timestamp;
  currentValue++;
  
  if (currentValue > MAX_VALUES) {
    // Serial.println("You are sending more than the maximum of consecutive variables");
    Serial.println("[UDOTS] Estas enviando mas variables consecutivas del maximo permitido!");
    currentValue = MAX_VALUES;
  }
}

bool Ubidots::ubidotsSubscribe(const char* deviceLabel, const char* variableLabel) {
    char topic[150];
    sprintf(topic, "%s%s/%s/lv", FIRST_PART_TOPIC, deviceLabel, variableLabel);
    
    if (!_client.connected()) {
        reconnect();
    }

    if (_debug){
      Serial.print("[UDOTS] Suscribiendose a: ");
      Serial.println(topic);
    }

    return _client.subscribe(topic);
}

bool Ubidots::ubidotsPublish(const char *deviceLabel) {
  char topic[150];
  char payload[500];
  sprintf(topic, "%s%s", FIRST_PART_TOPIC, deviceLabel);
  sprintf(payload, "{");
  
  for (int i = 0; i <= currentValue; ) {
    sprintf(payload, "%s\"%s\": [{\"value\": %.2f", payload, (val+i)->_variableLabel, (val+i)->_value);
    
    if ((val+i)->_timestamp != 0) {
      sprintf(payload, "%s, \"timestamp\": %u000", payload, (val+i)->_timestamp);
    }
    
    if ((val+i)->_context != NULL) {
      sprintf(payload, "%s, \"context\": {%s}", payload, (val+i)->_context);
    }

    i++;

    if (i >= currentValue) {
      sprintf(payload, "%s}]}", payload);
      break;

    } else {
      sprintf(payload, "%s}], ", payload);
    }
  }
  
  if (_debug){
    Serial.print("[UDOTS] Publicando al TOPIC: ");
    Serial.println(topic);
    Serial.print("[UDOTS] JSON dict: ");
    Serial.println(payload);
  }

  currentValue = 0;
  return _client.publish(topic, payload, 512);
}

void Ubidots::initialize(const char* token, char* clientName){
  _server = SERVER;
  _token = token;
  currentValue = 0;
  val = (Value *)malloc(MAX_VALUES*sizeof(Value));
  
  if(clientName != NULL){
    _clientName = clientName;
  }
}

char* Ubidots::getMac(){
  uint8_t mac[6];
  WiFi.macAddress(mac);  
  sprintf(_macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  if (_debug){
    Serial.print("[UDOTS] MAC: ");
    Serial.println(_macAddr);
  }
  
  return _macAddr;
}


