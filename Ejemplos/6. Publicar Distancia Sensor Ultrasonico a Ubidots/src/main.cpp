// NOTE Explicación Ejemplo
/*
  * Ejemplo de visualización de distancia de un Sensor Ultrasónico:
  ? Display distance of an Ultrasonic Sensor:
  En este ejemplo, a través de un sensor ultrasónico HC-SR04, se medirá y se visualizará en
  Ubidots la distancia que mide este.
  
  Importante: el sensor se alimenta con 5.0[V], por lo tanto se necesita un divisor resistivo para
  el Pin ECHO:

  ? -> English:
  In this example, the distance measured by the HC-SR04 ultrasonic sensor will be measured and 
  displayed on Ubidots.
  
  Important: the sensor is powered by 5.0 [V], therefore a resistive divider is needed
  for the ECHO Pin:

    Pin ECHO
    |
    [] 10k
    |
    ---- Pin 23 ESP32
    |
    [] 20k
    |
    ___ GND

  * Publish:
  Para visualizar la distancia en centimetros [cm] que mide el sensor ultrasónico, se utilizará un
  Widget Métrica.

  ? -> English:
  To view the distance in centimeters [cm] measured by the ultrasonic sensor, a Metric Widget will
  be used.

  ! Requisitos:
  ! Requirements:
  * - Conectar Sensor Ultrasónico HC-SR04:
    -> Pin VCC:     5.0[V] (Pin Vin del ESP32)
    -> Pin TRIGGER: Pin 22 del ESP32
    -> Pin ECHO:    A arreglo de resistencias 10k a 20k, y al Pin 23 del ESP32
    -> Pin GND:     GND
  ? * - Connect HC-SR04 Ultrasonic Sensor:
    -> Pin VCC:     5.0[V] (Pin Vin ESP32)
    -> Pin TRIGGER: Pin 22 ESP32
    -> Pin ECHO:    To resistor divider (10k to 20k), and to Pin 23 ESP32
    -> Pin GND:     GND

  * - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.
  ? - Configure the WiFi Credentials, and the Ubidots TOKEN.

  * - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
  *   que en Ubidots se cree el dispositivo automáticamente.
  ? - Turn on the device, and check on the Serial Monitor that it sends at least 1 data, so that 
  ?   the device is created automatically in Ubidots.

  * - En el Tablero, crear 1 Widget:
    -> Widget Métrica: asociarlo a la variable "distancia"
  ? - On the Dashboard, create 1 Widget:
    -> Metric Widget: -> Metric Widget: associate it with the variable "distancia"
*/

#include <Arduino.h>          // Incluirla siempre al utilizar PlatformIO
                              // Always include it when using PlatformIO
#include "delayMillis.h"      // Para utilizar Delays no-bloqueantes
                              // Non-blocking Delays
#include "UbidotsESP32MQTT.h" // Libreria de Ubidots MQTT
                              // Ubidots MQTT Library
#include "Ultrasonic.h"       // Libreria para medir distancia con sensor ultrasónico
                              // Ultrasonic sensor Library


// NOTE Credenciales WiFi
/* -------------------------------------------------------------------------- */
#define SSID  ""  // WiFi SSID
#define PASS  ""  // WiFi PASS
#define TOKEN ""  // Ubidots TOKEN
/* -------------------------------------------------------------------------- */

// NOTE Variables Ubidots
/* -------------------------------------------------------------------------- */
#define DISPOSITIVO ""  // Nombre dispositivo *device name*

// Nombres de variables en Ubidots *variable names on Ubidots*
#define VAR_DISTANCIA "distancia"
/* -------------------------------------------------------------------------- */

// NOTE Pines ESP32
/* -------------------------------------------------------------------------- */
// LED para indicar estado de conexión WiFi.
// Por defecto, se utiliza el LED incluido en la placa. Si la placa no lo tiene, reemplazarlo por
// otro pin digital del ESP32.
/**
 * LED to indicate WiFi connection status.
 * By default, the LED included on the board is used. If the board doesn't have it, replace it with
 * another ESP32 digital pin.
 */
#define LED_WIFI LED_BUILTIN

#define TRIG 22 // Pin TRIGGER del sensor
#define ECHO 23 // Pin ECHO del sensor
/* -------------------------------------------------------------------------- */

/* ---------------------------- Objetos (Objects) --------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
/* Main Constructor, pass Ubidots Token as a parameter */
Ubidots ubidots(TOKEN);

// Delays para Envío de Paquetes, reconexión WiFi y lectura sensor
/* Delays for Sending Packages, WiFi reconnection and reading sensor */
DelayMillis t_envio, t_reconexion, t_lectura;

// Constructor sensor ultrasónico, pasan como parámetros los pines
/* Ultrasonic sensor constructor, pass the pins as parameters */
Ultrasonic ultrasonic(TRIG, ECHO);
/* -------------------------------------------------------------------------- */

/* ------------------ Variables Globales (Global Variables) ----------------- */
bool wifi_on = false; // Indica si hay conexión activa a WiFi
                      /* Indicates WiFi connection */
/* -------------------------------------------------------------------------- */

/* -------------- Declaracion Funciones (Function Declarations) ------------- */
void callback(char* topic, uint8_t* payload, unsigned int length);  // Callback de Ubidots
                                                                    /* Ubidots Callback */
void callbackWifiConectado(WiFiEvent_t event);    // Se ejecuta cuando WiFi asoció dirección IP
                                                  /* WiFi connected Callback */
void callbackWifiDesconectado(WiFiEvent_t event); // Se ejecuta cuando WiFi se desconecta
                                                  /* WiFi disconnected Callback */
/* -------------------------------------------------------------------------- */

// ANCHOR setup()
// * ---------------------------------------------------------------------------
void setup() {
  // Configurar pines
  /* Configure Pins */
  pinMode(LED_WIFI, OUTPUT);

  // Configurar estado inicial Led WiFi
  /* Configure Initial State of WiFi Led */
  digitalWrite(LED_WIFI, LOW);

  // Iniciar Puerto Serial
  /* Start Serial Port */
  Serial.begin(115200);

  // Esperar 2 segundos para abrir terminal y visualizar todo el texto
  /* Wait 2 seconds to open terminal */
  delay(2000);

  // Iniciar delays
  /* Start delays */
  t_envio.empezar(10000);     // Enviar cada 10s a Ubidots
                              /* Send every 10s to Ubidots */
  t_reconexion.empezar(3000); // Intentar reconectar cada 3s en caso de que no haya conexión
                              /* Try to reconnect every 3s if there are no connection */
  t_lectura.empezar(500);     // Leer cada 500ms el sensor ultrasónico
                              /* Read every 500ms the ultrasonic sensor */

  // Asociar Callbacks del WiFi
  /* Associate WiFi Callbacks */
  WiFi.onEvent(callbackWifiConectado, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(callbackWifiDesconectado, SYSTEM_EVENT_STA_DISCONNECTED);

  Serial.printf("\n[WIFI] Conectando a %s ...\n", SSID);

  // Configurar cliente de Ubidots
  /* Configure Ubidots client */
  ubidots.setDebug(true);   // Setear a TRUE para visualizar información útil
                            /* Set to TRUE to display useful info */
  ubidots.begin(callback);  // Iniciar Servidor y asociar Callback para recibir paquetes
                            /* Start Server and associate Callback to receive packages */
  ubidots.wifiConnection(SSID, PASS); // Iniciar conexión WiFi
                                      /* Start WiFi connection */
}
// * ---------------------------------------------------------------------------

// ANCHOR loop()
// * ---------------------------------------------------------------------------
void loop() {
  // Revisar si el cliente MQTT y el WiFi se encuentran conectados
  /* Check if the MQTT client and WiFi are connected */
  if (!ubidots.connected() && wifi_on == true) {
    
    // Intentar reconectar cliente MQTT cada cierto tiempo de reintento (t_reconexion)
    /* Try to reconnect MQTT client every t_reconexion time */
    if (t_reconexion.finalizado()) {
      ubidots.reconnect();
      t_reconexion.repetir(); // Reiniciar tiempo *Restart time*
    }
  }

  // Si el cliente se encuentra conectado, y el tiempo de envío ha finalizado, publicar variable/s a
  // Ubidots y reiniciar tiempo
  /* If the client is connected, and the send time has finished, publish variables to Ubidots */
  if (ubidots.connected() && t_envio.finalizado()) {    
    uint16_t distancia = ultrasonic.read();
    
    ubidots.add(VAR_DISTANCIA, distancia);  // Añadir variable al buffer
                                            /* Add variable to buffer */

    Serial.println("[INFO] Enviando datos...");
    ubidots.ubidotsPublish(DISPOSITIVO);    // Publicar variable al dispositivo en Ubidots
                                            /* Publish variable to device on Ubidots */
    Serial.println("[INFO] Datos enviados\n");

    t_envio.repetir(); // Reiniciar tiempo *Restart time*
  }

  // Mostrar distancia por el monitor serial
  /* Display distance on the serial monitor */
  if (t_lectura.finalizado()) {
    Serial.print("Distancia: ");
    Serial.println(ultrasonic.read());

    t_lectura.repetir();
  }

  // loop() debe ser llamado constantemente para verificar conexión al servidor y revisar mensajes
  // entrantes (mensajes de un Subscribe)
  /* loop() must be constantly called to verify server connection and check incoming messages */
  ubidots.loop(); 
}
// * ---------------------------------------------------------------------------

// ANCHOR Callback Ubidots
// * ---------------------------------------------------------------------------
void callback(char* topic, uint8_t* payload, unsigned int length) {
  // Función vacia. No hay variables a las que suscribirse
  /* Empty function. There is no variables to subscribe */
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