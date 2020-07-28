// NOTE Explicación Ejemplo
/*
  * Ejemplo de publicación de localización GPS a Ubidots:
  Este ejemplo demuestra cómo enviar la localización del dispositivo mediante los datos que
  entrega un GPS, es decir, Latitud y Longitud. Para efectos prácticos, en este caso, se
  utilizarán las coordenadas fijas de Creatiox, sacadas desde Google Maps. Para visualizar lo que
  ocurre en el programa, se utulizará el Monitor Serial a 115200 Baudios.

  * Publish:
  Ubidots automáticamente toma la ubicación del dispositivo, de alguna variable llamada "gps". Por
  lo tanto, se enviará una variable llamada "gps", con el siguiente contexto:

  Contexto -> "lat": {latitud}, "lng": {longitud}

  ? Requisitos:
  - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.

  - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
    que en Ubidots se cree el dispositivo automáticamente.

  - En Ubidots/Dispositivos/{nombre_dispositivo}, visualizar que se haya detectado correctamente la
    ubicación de Creatiox

  - En el Tablero, crear 1 Widget:
    -> Widget Mapa: asociarlo al dispositivo {nombre_dispositivo}
*/

#include <Arduino.h>          // Incluirla siempre al utilizar PlatformIO
#include "delayMillis.h"      // Para utilizar Delays no-bloqueantes
#include "UbidotsESP32MQTT.h" // Libreria de Ubidots MQTT

// NOTE Credenciales WiFi
/* -------------------------------------------------------------------------- */
#define SSID  ""
#define PASS  ""
#define TOKEN ""
/* -------------------------------------------------------------------------- */

// NOTE Variables Ubidots
/* -------------------------------------------------------------------------- */
#define DISPOSITIVO ""

#define VAR_GPS "gps"

// Ubicación real de Creatiox.
// El '\' se antepone al '"' para indicar que lea las comillas como un caracter
char ubicacion_creatiox[] = "\"lat\": -33.023034, \"lng\": -71.546400";
/* -------------------------------------------------------------------------- */

// NOTE Pines ESP32
/* -------------------------------------------------------------------------- */
// LED para indicar estado de conexión WiFi.
// Por defecto, se utiliza el LED incluido en la placa. Si la placa no lo tiene, reemplazarlo por
// otro pin digital del ESP32.
#define LED_WIFI LED_BUILTIN
/* -------------------------------------------------------------------------- */

/* --------------------------------- Objetos -------------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
Ubidots ubidots(TOKEN);

// Delays para Envío de Paquetes, y reconexión WiFi
DelayMillis t_envio, t_reconexion;
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

  // Configurar estado inicial de los pines
  digitalWrite(LED_WIFI, LOW);

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

    // Añadir variable "gps" al buffer, y pasarle como tercer parámetro el contexto, en este caso,
    // la localización GPS del dispositivo
    ubidots.add(VAR_GPS, 0, ubicacion_creatiox);

    Serial.println("[INFO] Enviando datos...");
    ubidots.ubidotsPublish(DISPOSITIVO);    // Publicar variable al dispositivo en Ubidots
    Serial.println("[INFO] Datos enviados\n");
    
    t_envio.repetir(); // Reiniciar tiempo
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