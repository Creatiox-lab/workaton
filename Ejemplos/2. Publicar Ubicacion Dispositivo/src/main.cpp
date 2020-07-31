// NOTE Explicación Ejemplo
/*
  * Ejemplo de publicación de localización GPS a Ubidots:
  ? Publish GPS location to Ubiddots:
  Este ejemplo demuestra cómo enviar la localización del dispositivo mediante los datos que
  entrega un GPS, es decir, Latitud y Longitud. Para efectos prácticos, en este caso, se
  utilizarán las coordenadas fijas de Creatiox, sacadas desde Google Maps. Para visualizar lo que
  ocurre en el programa, se utulizará el Monitor Serial a 115200 Baudios.

  ? -> English:
  This example demonstrates how to send the location of the device using the data provided by a GPS,
  that is, Latitude and Longitude. For practical purposes, in this case, Creatiox's fixed
  coordinates, taken from Google Maps, will be used. To visualize what happens in the program, the
  Serial Monitor at 115200 Baud will be used.

  * Publish:
  Ubidots automáticamente toma la ubicación del dispositivo, de alguna variable llamada "gps". Por
  lo tanto, se enviará una variable llamada "gps", con el siguiente contexto:

  Contexto -> "lat": {latitud}, "lng": {longitud}

  ? -> English:
  Ubidots automatically takes the location of the device, from some variable called "gps".
  Therefore, a variable called "gps" will be sent, with the following context:
  
  Context -> "lat": {latitude}, "lng": {longitude}

  ! Requisitos:
  ! Requirements:
  * - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.
  ? - Configure the WiFi Credentials, and the Ubidots TOKEN.

  * - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
  *   que en Ubidots se cree el dispositivo automáticamente.
  ? - Turn on the device, and check on the Serial Monitor that it sends at least 1 data, so that 
  ?   the device is created automatically in Ubidots.

  * - En Ubidots/Dispositivos/{nombre_dispositivo}, visualizar que se haya detectado correctamente la
  *   ubicación de Creatiox
  ? - In Ubidots/Devices/{device_name}, check that the Creatiox location has been detected correctly

  * - En el Tablero, crear 1 Widget:
    -> Widget Mapa: asociarlo al dispositivo {nombre_dispositivo}
  ? - On the Dashboard, create 1 Widget:
    -> Map Widget: associate to device {device_name}
*/

#include <Arduino.h>          // Incluirla siempre al utilizar PlatformIO
                              // Always include it when using PlatformIO
#include "delayMillis.h"      // Para utilizar Delays no-bloqueantes
                              // Non-blocking Delays
#include "UbidotsESP32MQTT.h" // Libreria de Ubidots MQTT
                              // Ubidots MQTT Library

// NOTE Credenciales WiFi
/* -------------------------------------------------------------------------- */
#define SSID  ""  // WiFi SSID
#define PASS  ""  // WiFi PASS
#define TOKEN ""  // Ubidots TOKEN
/* -------------------------------------------------------------------------- */

// NOTE Variables Ubidots
/* -------------------------------------------------------------------------- */
#define DISPOSITIVO ""  // Nombre dispositivo *device name*

#define VAR_GPS "gps" // Nombres de variables en Ubidots *variable names on Ubidots*

// Ubicación real de Creatiox.
// El '\' se antepone al '"' para indicar que lea las comillas como un caracter
/* Creatiox real Location */
char ubicacion_creatiox[] = "\"lat\": -33.023034, \"lng\": -71.546400";
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
/* -------------------------------------------------------------------------- */

/* ---------------------------- Objetos (Objects) --------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
/* Main Constructor, pass Ubidots Token as a parameter */
Ubidots ubidots(TOKEN);

// Delays para Envío de Paquetes, y reconexión WiFi
/* Delays for Sending Packages, and WiFi reconnection */
DelayMillis t_envio, t_reconexion;
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

  // Configurar estado inicial de los pines
  /* Configure Initial State of the Pins */
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

    // Añadir variable "gps" al buffer, y pasarle como tercer parámetro el contexto, en este caso,
    // la localización GPS del dispositivo
    /** Add variable "gps" to the buffer, and pass it as third parameter the context, in this case,
     * the GPS location of the device */
    ubidots.add(VAR_GPS, 0, ubicacion_creatiox);

    Serial.println("[INFO] Enviando datos...");
    ubidots.ubidotsPublish(DISPOSITIVO);    // Publicar variable al dispositivo en Ubidots
                                            /* Publish variable to device on Ubidots */
    Serial.println("[INFO] Datos enviados\n");
    
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