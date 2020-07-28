// NOTE Explicación Ejemplo
/*
  * Ejemplo de visualización de distancia de un Sensor Ultrasónico:
    En este ejemplo, a través de un sensor ultrasónico HC-SR04, se medirá y se visualizará en
    Ubidots la distancia que mide este.

    Importante: el sensor se alimenta con 5.0[V], por lo tanto se necesita un divisor resistivo para
    el Pin ECHO:

    Pin ECHO
    |
    [] 10k
    |
    ---- Pin 23 ESP32
    |
    [] 20k
    |
    ___ Tierra

  * Publish:
    Para visualizar la distancia en centimetros [cm] que mide el sensor ultrasónico, se utilizará un
    Widget Métrica.

  ? Requisitos:
  - Conectar Sensor Ultrasónico HC-SR04:
    -> Pin VCC:     5.0[V] (Pin Vin del ESP32)
    -> Pin TRIGGER: Pin 22 del ESP32
    -> Pin ECHO:    A arreglo de resistencias 10k a 20k, y al Pin 23 del ESP32
    -> Pin GND:     Tierra

  - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.

  - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
    que en Ubidots se cree el dispositivo automáticamente.

  - En el Tablero, crear 1 Widget:
    -> Widget Métrica: asociarlo a la variable "distancia"
*/

#include <Arduino.h>           // Incluirla siempre al utilizar PlatformIO
#include "delayMillis.h"       // Para utilizar Delays no-bloqueantes
#include "UbidotsESP32MQTT.h"  // Libreria de Ubidots MQTT
#include "Ultrasonic.h"        // Libreria para medir distancia con sensor ultrasónico

// NOTE Credenciales WiFi
/* -------------------------------------------------------------------------- */
#define SSID  ""
#define PASS  ""
#define TOKEN ""
/* -------------------------------------------------------------------------- */

// NOTE Variables Ubidots
/* -------------------------------------------------------------------------- */
#define DISPOSITIVO ""

#define VAR_DISTANCIA "distancia"
/* -------------------------------------------------------------------------- */

// NOTE Pines ESP32
/* -------------------------------------------------------------------------- */
// LED para indicar estado de conexión WiFi.
// Por defecto, se utiliza el LED incluido en la placa. Si la placa no lo tiene, reemplazarlo por
// otro pin digital del ESP32.
#define LED_WIFI LED_BUILTIN

#define TRIG 22 // Pin TRIGGER del sensor
#define ECHO 23 // Pin ECHO del sensor
/* -------------------------------------------------------------------------- */

/* --------------------------------- Objetos -------------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
Ubidots ubidots(TOKEN);

// Delays para reconexión WiFi
DelayMillis t_envio, t_reconexion, t_lectura;

// Constructor sensor ultrasónico, pasan como parámetros los pines
Ultrasonic ultrasonic(TRIG, ECHO);
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

  // Iniciar Puerto Serial
  Serial.begin(115200);

  // Esperar 2 segundos para abrir terminal y visualizar todo el texto
  delay(2000);

  // Iniciar delays
  t_envio.empezar(5000); // Enviar cada 5s a Ubidots
  t_reconexion.empezar(3000); // Intentar reconectar cada 3s en caso de que no haya conexión
  t_lectura.empezar(500);

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
    uint16_t distancia = ultrasonic.read();
    
    ubidots.add(VAR_DISTANCIA, distancia); // Añadir variable al buffer

    Serial.println("[INFO] Enviando datos...");
    ubidots.ubidotsPublish(DISPOSITIVO);    // Publicar variable al dispositivo en Ubidots
    Serial.println("[INFO] Datos enviados\n");

    t_envio.repetir();
  }

  if (t_lectura.finalizado()) {
    Serial.print("Distancia: ");
    Serial.println(ultrasonic.read());

    t_lectura.repetir();
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