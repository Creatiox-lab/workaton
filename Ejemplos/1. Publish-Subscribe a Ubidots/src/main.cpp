// NOTE Explicación Ejemplo
/*
  * Ejemplo de conexión básica a Ubidots con ESP32:
  Este ejemplo demuestra el funcionamiento básico del protocolo MQTT, para realizar "Publish" y
  "Subscribe" (Publicar y Suscribir) a la plataforma Ubidots. Para visualizar lo que ocurre en el
  programa, se utilizará el Monitor Serial a 115200 Baudios.

  * Publish:
  Para realizar un Publish a Ubidots, se utilizará un Potenciómetro para leer un número mediante un
  Pin Análogo del ESP32, en el rango de 0 a 4095 (ADC con resolución 12 bits). Este dato se
  utilizará para enviarlo a la plataforma, y se visualizará en un Widget de Tanque.

  * Subscribe:
  Para recibir un dato desde la plataforma (Subscribe) se utilizará un Widget de Interruptor, para
  enviar un dato binario (0 o 1) al ESP32. Este dato encenderá un Led conectado a un Pin Digital del
  microcontrolador cuando el Widget Interruptor se encuentre encendido, y viceversa.

  ? Requisitos:
  - Conectar Potenciómetro:
    -> Pin 1: 3.3[V]
    -> Pin 2: Pin 34 ESP32
    -> Pin 3: Tierra

  - Conectar Led y Resistencia 1[kOhm]:
    -> Pin 1 Resistencia: Pin 23 ESP32
    -> Pin 2 Resistencia: Ánodo Led
    -> Cátodo Led: Tierra

  - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.

  - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
    que en Ubidots se cree el dispositivo automáticamente.

  - Crear 1 variable llamada "boton".

  - En el Tablero, crear 2 Widgets:
    -> Widget Tanque: asociarlo a la variable "pot"
    -> Widget Interruptor: asociarlo a la variable "boton"
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

#define VAR_BOTON   "boton"
#define VAR_POT     "pot"
/* -------------------------------------------------------------------------- */

// NOTE Pines ESP32
/* -------------------------------------------------------------------------- */
// LED para indicar estado de conexión WiFi.
// Por defecto, se utiliza el LED incluido en la placa. Si la placa no lo tiene, reemplazarlo por
// otro pin digital del ESP32.
#define LED_WIFI LED_BUILTIN

#define LED 23 // Pin Digital para LED
#define POT 34 // Pin Análogo para Potenciómetro
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
  pinMode(LED, OUTPUT);
  pinMode(POT, ANALOG);

  // Configurar estado inicial de los pines
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED, LOW);

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
  
    // Suscribirse, si el cliente se conectó
    if (ubidots.connected()) {
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_BOTON);
    }
  }

  // Si el cliente se encuentra conectado, y el tiempo de envío ha finalizado, publicar variable/s a
  // Ubidots y reiniciar tiempo
  if (ubidots.connected() && t_envio.finalizado()) {
    ubidots.add(VAR_POT, analogRead(POT));  // Añadir variable al buffer

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
  // NOTA: Recordar que Ubidots envía los datos en forma de texto, por lo tanto, para comparar
  // números por ejemplo, se deben comparar CARACTERES, no números.
  
  Serial.println("[UDOTS] Mensaje recibido!");
  
  // Sentencia simple para revisar si el mensaje que llegó, corresponde a cierto tópico
  if (strstr(topic, VAR_BOTON) != NULL)
  {
    // Revisar si el valor corresponde a un '0' o a un '1'
    if (payload[0] == '0') {
      Serial.println("[INFO] Apagando LED\n");
      digitalWrite(LED, LOW);

    } else if (payload[0] == '1') {
      Serial.println("[INFO] Encendiendo LED\n");
      digitalWrite(LED, HIGH);

    }
  }
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