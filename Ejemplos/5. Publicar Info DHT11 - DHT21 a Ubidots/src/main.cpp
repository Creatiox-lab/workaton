// NOTE Explicación Ejemplo
/*
  * Ejemplo de visualización de variables de Ubidots en una LCD 16x2:
    En este ejemplo, se utilizará un sensor DHT11, comúnmente utilizado en proyectos de Arduino, y
    se publicarán las lecturas de Temperatura y Humedad Relativa a Ubidots.

  * Publish:
    Para visualizar los datos de Temperatura y Humedad del sensor, se utilizarán un Widget
    Termómetro y un Widget Métrica respectivamente.

  ? Requisitos:
  - Conectar Sensor DHT11:
    -> Pin VCC:   3.3[V]
    -> Pin DATA:  Pin 23 del ESP32   
    -> Pin GND:   Tierra

  - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.

  - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
    que en Ubidots se cree el dispositivo automáticamente.

  - En el Tablero, crear 2 Widgets:
    -> Widget Termómetro: asociarlo a la variable "temperatura"
    -> Widget Métrica: asociarlo a la variable "humedad"
*/

#include <Arduino.h>           // Incluirla siempre al utilizar PlatformIO
#include "delayMillis.h"       // Para utilizar Delays no-bloqueantes
#include "UbidotsESP32MQTT.h"  // Libreria de Ubidots MQTT
#include "DHT.h"               // Libreria para comunicarse con sensores DHT

// NOTE Credenciales WiFi
/* -------------------------------------------------------------------------- */
#define SSID  ""
#define PASS  ""
#define TOKEN ""
/* -------------------------------------------------------------------------- */

// NOTE Variables Ubidots
/* -------------------------------------------------------------------------- */
#define DISPOSITIVO ""

#define VAR_TEMPERATURA "temperatura"
#define VAR_HUMEDAD     "humedad"
/* -------------------------------------------------------------------------- */

// NOTE Pines ESP32
/* -------------------------------------------------------------------------- */
// LED para indicar estado de conexión WiFi.
// Por defecto, se utiliza el LED incluido en la placa. Si la placa no lo tiene, reemplazarlo por
// otro pin digital del ESP32.
#define LED_WIFI LED_BUILTIN

#define DHT_DATA 23
/* -------------------------------------------------------------------------- */

/* --------------------------------- Objetos -------------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
Ubidots ubidots(TOKEN);

// Delays para reconexión WiFi
DelayMillis t_envio, t_reconexion;

// NOTE Config. DHT
// Constructor DHT, pasa como parámetro el Pin de Datos, y el tipo de sensor DHT
// La librería tiene compatibilidad para el DHT11, DHT12, DHT21 y DHT22
DHT dht(DHT_DATA, DHT11);
/* -------------------------------------------------------------------------- */

/* --------------------------- Variables Globales --------------------------- */
bool wifi_on = false; // Indica si hay conexión activa a WiFi
/* -------------------------------------------------------------------------- */

/* -------------------------- Declaracion Funciones ------------------------- */
void callback(char* topic, uint8_t* payload, unsigned int length); // Callback de Ubidots
void callbackWifiConectado(WiFiEvent_t event);  // Se ejecuta cuando WiFi asoció dirección IP
void callbackWifiDesconectado(WiFiEvent_t event); // Se ejecuta cuando WiFi se desconecta
/* -------------------------------------------------------------------------- */

// ANCHOR setup()
// * ---------------------------------------------------------------------------
void setup() {
  // Configurar pines
  pinMode(LED_WIFI, OUTPUT);

  // Configurar estado inicial Led WiFi
  digitalWrite(LED_WIFI, LOW);

  // Iniciar sensor DHT
  dht.begin();

  // Iniciar Puerto Serial
  Serial.begin(115200);

  // Esperar 2 segundos para abrir terminal y visualizar todo el texto
  delay(2000);

  // Iniciar delays
  t_envio.empezar(10000); // Enviar cada 10s a Ubidots
  t_reconexion.empezar(3000); // Intentar reconectar cada 3s en caso de que no haya conexión

  // Asociar Callbacks del WiFi
  WiFi.onEvent(callbackWifiConectado, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(callbackWifiDesconectado, SYSTEM_EVENT_STA_DISCONNECTED);

  Serial.printf("\n[WIFI] Conectando a %s ...\n", SSID);

  // Configurar cliente de Ubidots
  ubidots.setDebug(true); // Setear a TRUE para visualizar información útil
  ubidots.begin(callback); // Iniciar Servidor y asociar Callback para recibir paquetes
  ubidots.wifiConnection(SSID, PASS); // Iniciar conexión WiFi
}
// * ---------------------------------------------------------------------------

// ANCHOR loop()
// * ---------------------------------------------------------------------------
void loop() {
  // Revisar si el cliente MQTT y el WiFi se encuentran conectados
  if (!ubidots.connected() && wifi_on == true) {
    
    // Intentar reconectar cliente MQTT cada cierto tiempo de reintento (t_reconexion)
    if (t_reconexion.finalizado()) {
      ubidots.reconnect();
      t_reconexion.repetir(); // Reiniciar tiempo
    }
  }

  // Si el cliente se encuentra conectado, y el tiempo de envío ha finalizado, publicar variable/s a
  // Ubidots y reiniciar tiempo
  if (ubidots.connected() && t_envio.finalizado()) {
    float temp = dht.readTemperature(); // Leer temperatura sensor
    float hum  = dht.readHumidity();    // Leer humedad sensor

    // Revisar si alguna de las variables no es un número (isnan), y arrojar error...
    if (isnan(temp) || isnan(hum)) {
      Serial.println("[ERROR] No se pudo leer el sensor DHT!");      
    }

    ///...de lo contrario, enviar variables a Ubidots
    else {
      ubidots.add(VAR_TEMPERATURA, temp); // Añadir variable al buffer
      ubidots.add(VAR_HUMEDAD, hum);      // Añadir variable al buffer

      Serial.println("[INFO] Enviando datos...");
      ubidots.ubidotsPublish(DISPOSITIVO);    // Publicar variable al dispositivo en Ubidots
      Serial.println("[INFO] Datos enviados\n");
    }

    t_envio.repetir();
  }

  // loop() debe ser llamado constantemente para verificar conexión al servidor y revisar mensajes
  // entrantes (mensajes de un Subscribe)
  ubidots.loop(); 
}
// * ---------------------------------------------------------------------------

// ANCHOR Callback Ubidots
// * ---------------------------------------------------------------------------
void callback(char* topic, uint8_t* payload, unsigned int length) {
  // Función vacia. No hay variables a las que suscribirse
}
// * ---------------------------------------------------------------------------

// ANCHOR Callbacks WiFi
// * ---------------------------------------------------------------------------
void callbackWifiConectado(WiFiEvent_t event) {
  wifi_on = true;
  digitalWrite(LED_WIFI, HIGH);
  Serial.print("[WIFI] WiFi Conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void callbackWifiDesconectado(WiFiEvent_t event) {
  wifi_on = false;
  digitalWrite(LED_WIFI, LOW);
  Serial.print("[WIFI] WiFi Desconectado!");
}
// * ---------------------------------------------------------------------------