/**
 * @file UbidotsESP32MQTT.h
 */

#ifndef UbidotsESP32MQTT_H
#define UbidotsESP32MQTT_H

#include "PubSubClient.h"
#include <WiFi.h>

#define SERVER                "industrial.api.ubidots.com"  //!< Servidor de Ubidots
#define FIRST_PART_TOPIC      "/v1.6/devices/"              //!< Texto por defecto del tópico
#define MQTT_PORT             1883                          //!< Puerto de MQTT
#define MAX_VALUES            5                             //!< Máximas variables en un Publish. Máximo 5!
#define DEFAULT_DEVICE_LABEL  "esp32"                       //!< Nombre opcional por defecto del dispositivo

typedef struct Value {
  const char* _variableLabel;
  float _value;               
  char* _context;             
  uint32_t _timestamp;           
} Value;

/**
 * @brief Clase principal
 */
class Ubidots {
  public:
    /**
     * @brief Construir un nuevo objeto Ubidots.
     * 
     * Si se utiliza este constructor, la librería automáticamente creará el clientName de acuerdo a
     * la MAC del dispositivo (el clientName debe ser único).
     * 
     * @param token Default token de Ubidots
     */
    Ubidots(const char* token);

    /**
     * @brief Construir un nuevo objeto Ubidots.
     * 
     * @param token Default token de Ubidots
     * @param clientName Nombre del cliente (debe ser único)
     */
    Ubidots(const char* token, char* clientName);

    /**
     * @brief Establecer comunicación MQTT y asociar una función callback para recibir mensajes.
     * 
     * @param callback Función para mensajes entrantes (debe tener el mismo formato)
     */
    void begin(void (*callback)(char*,uint8_t*,unsigned int));

    /**
     * @brief Comenzar conexión WiFi.
     * 
     * @param ssid Nombre de la red WiFi
     * @param pass Contraseña de la red WiFi
     */
    void wifiConnection(const char* ssid, const char* pass);

    /**
     * @brief Comprobar si el cliente MQTT se encuentra conectado.
     * 
     * @return true Cliente conectado
     * @return false Cliente desconectado
     */
    bool connected();

    /**
     * @brief Reconectar cliente MQTT.
     */
    void reconnect();

    /**
     * @brief Esto debe ser llamado regularmente para permitir al cliente procesar mensajes
     * entrantes y mantener su conexión al servidor.
     * 
     * @return true Cliente aún se encuentra conectado
     * @return false Cliente ya no está conectado
     */
    bool loop();

    /**
     * @brief Establecer el parámetro Debug para ver información por Serial.
     * 
     * @param debug Activar Debug
     */
    void setDebug(bool debug);

    /**
     * @brief Agregar valor a una variable especificada.
     * 
     * @param variableLabel Nombre de la variable
     * @param value Valor numérico
     */
    void add(const char* variableLabel, float value);

    /**
     * @brief Agregar valor y contexto a una variable especificada.
     * 
     * @param variableLabel Nombre de la variable
     * @param value Valor numérico
     * @param context Contexto
     */
    void add(const char* variableLabel, float value, char* context);

    /**
     * @brief Agregar valor, contexto y timestamp a una variable especificada.
     * 
     * @param variableLabel Nombre de la variable
     * @param value Valor numérico
     * @param context Contexto
     * @param timestamp Valor Unix Timestamp en segundos
     */
    void add(const char* variableLabel, float value, char* context, uint32_t timestamp);

    /**
     * @brief Suscribirse a una variable de un dispositivo de Ubidots.
     * 
     * @param deviceLabel Nombre del dispositivo
     * @param variableLabel Nombre de la variable
     * @return true Suscripción tuvo éxito 
     * @return false Suscripción falló
     */
    bool ubidotsSubscribe(const char* deviceLabel, const char* variableLabel);

    /**
     * @brief Publicar variable/s a Ubidots. La cantidad máxima está definida por MAX_VALUES.
     * 
     * @param deviceLabel Nombre del dispositivo
     * @return true Publicación tuvo éxito
     * @return false Publicación falló
     */
    bool ubidotsPublish(const char *deviceLabel);
  
  private:
    void (*callback)(char*,uint8_t*,unsigned int);
    void initialize(const char* token, char* clientName);
    char* getMac();
    
    char _macAddr[18];    
    WiFiClient espClient;
    PubSubClient _client = PubSubClient(espClient);
    char* _clientName = NULL;
    bool _debug = false;
    uint8_t currentValue;    
    const char* _token;
    const char* _server;
    Value * val;
};

#endif
