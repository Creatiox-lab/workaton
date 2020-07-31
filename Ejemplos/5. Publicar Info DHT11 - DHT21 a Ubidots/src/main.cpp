// NOTE Explicación Ejemplo
/*
  * Ejemplo de visualización Sensor DHT11 en Ubidots:
  ? Displaying DHT11 Sensor Data on Ubidots:
  En este ejemplo, se utilizará un sensor DHT11, comúnmente utilizado en proyectos de Arduino, y
  se publicarán las lecturas de Temperatura y Humedad Relativa a Ubidots.

  ? -> English:
  In this example, a DHT11 sensor, commonly used in Arduino projects, will be used, and the Relative
  Humidity and Temperature readings will be published on Ubidots.

  * Publish:
  Para visualizar los datos de Temperatura y Humedad del sensor, se utilizarán un Widget
  Termómetro y un Widget Métrica respectivamente.

  ? -> English:
  To visualize the Temperature and Humidity data of the sensor, a Thermometer Widget and a Metric
  Widget will be used respectively.

  ! Requisitos:
  ! Requirements:
  * - Conectar Sensor DHT11:
  ? - Connect DHT11 Sensor:
    -> Pin VCC:   3.3[V]
    -> Pin DATA:  Pin 23 ESP32   
    -> Pin GND:   GND

  * - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.
  ? - Configure the WiFi Credentials, and the Ubidots TOKEN.

  * - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
  *   que en Ubidots se cree el dispositivo automáticamente.
  ? - Turn on the device, and check on the Serial Monitor that it sends at least 1 data, so that 
  ?   the device is created automatically in Ubidots.

  * - En el Tablero, crear 2 Widgets:
    -> Widget Termómetro: asociarlo a la variable "temperatura"
    -> Widget Métrica: asociarlo a la variable "humedad"
  ? On the Dashboard, create 2 Widgets:
    -> Thermometer Widget: associate it with the variable "temperatura"
    -> Metric Widget: associate it with the variable "humedad"
*/

#include <Arduino.h>          // Incluirla siempre al utilizar PlatformIO
                              // Always include it when using PlatformIO
#include "delayMillis.h"      // Para utilizar Delays no-bloqueantes
                              // Non-blocking Delays
#include "UbidotsESP32MQTT.h" // Libreria de Ubidots MQTT
                              // Ubidots MQTT Library
#include "DHT.h"              // Libreria para comunicarse con sensores DHT
                              // DHT sensors Library

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
#define VAR_TEMPERATURA "temperatura"
#define VAR_HUMEDAD     "humedad"
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

// Pin de comunicación sensor DHT
/* Data communication pin DHT sensor */
#define DHT_DATA 23
/* -------------------------------------------------------------------------- */

/* ---------------------------- Objetos (Objects) --------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
/* Main Constructor, pass Ubidots Token as a parameter */
Ubidots ubidots(TOKEN);

// Delays para Envío de Paquetes, y reconexión WiFi
/* Delays for Sending Packages, and WiFi reconnection */
DelayMillis t_envio, t_reconexion;

// NOTE Config. DHT
// Constructor DHT, pasa como parámetro el Pin de Datos, y el tipo de sensor DHT
// La librería tiene compatibilidad para el DHT11, DHT12, DHT21 y DHT22
/**
 * DHT constructor, passes Data Pin as parameter, and DHT sensor type
 * The library has compatibility for DHT11, DHT12, DHT21 and DHT22
 */
DHT dht(DHT_DATA, DHT11);
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
  /* Configure Initial State of the Pins */
  digitalWrite(LED_WIFI, LOW);

  // Iniciar sensor DHT
  /* Start DHT sensor */
  dht.begin();

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
    float temp = dht.readTemperature(); // Leer temperatura sensor
                                        /* Read sensor temperature */
    float hum  = dht.readHumidity();    // Leer humedad sensor
                                        /* Read sensor relative humidity */

    // Revisar si alguna de las variables no es un número (isnan), y arrojar error...
    /* Check if any of the variables is not a number (isnan), and throw error... */
    if (isnan(temp) || isnan(hum)) {
      Serial.println("[ERROR] No se pudo leer el sensor DHT!");      
    }

    // ...de lo contrario, enviar variables a Ubidots
    /* ...otherwise send variables to Ubidots */
    else {
      ubidots.add(VAR_TEMPERATURA, temp); // Añadir variable al buffer
                                          /* Add variable to buffer */
      ubidots.add(VAR_HUMEDAD, hum);      // Añadir variable al buffer
                                          /* Add variable to buffer */

      Serial.println("[INFO] Enviando datos...");
      ubidots.ubidotsPublish(DISPOSITIVO);  // Publicar variable al dispositivo en Ubidots
                                            /* Publish variable to device on Ubidots */
      Serial.println("[INFO] Datos enviados\n");
    }

    t_envio.repetir(); // Reiniciar tiempo *Restart time*
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