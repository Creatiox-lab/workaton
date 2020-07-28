// NOTE Explicación Ejemplo
/*
  * Ejemplo de control Led RGB mediante Ubidots:
    Este sencillo ejemplo muestra cómo utilizar el periférico Led Control (LEDC) del ESP32, para
    controlar el color de un Led RGB por medio de la plataforma. Para este caso, se utilizará un Led
    RGB de cátodo común, con resistencias de 330[ohm] para cada Led individual.

  * Subscribe:
    Para recibir los datos del nivel de intensidad de cada color, se utilizará un Widget Deslizador,
    con 3 variables (1 variable para cada color).

  ? Requisitos:
  - Conectar Led RGB:
    -> Pin R (rojo):    Pin 1 Resistencia. Pin 2 Resistencia a Pin 22 ESP32
    -> Pin G (verde):   Pin 1 Resistencia. Pin 2 Resistencia a Pin 21 ESP32
    -> Pin B (azul):    Pin 1 Resistencia. Pin 2 Resistencia a Pin 23 ESP32
    -> Pin - (cátodo):  Tierra

  - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.

  - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
    que en Ubidots se cree el dispositivo automáticamente.

  - Crear 3 variables en el dispositivo:
    -> Variable 1: "led_r"
    -> Variable 2: "led_g"
    -> Variable 3: "led_b"

  - En el Tablero, crear 1 Widget:
    -> Widget Deslizador: asociarlo a la variables creadas en el paso anterior
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

#define VAR_LED_R   "led_r"
#define VAR_LED_G   "led_g"
#define VAR_LED_B   "led_b"
/* -------------------------------------------------------------------------- */

// NOTE Pines ESP32
/* -------------------------------------------------------------------------- */
// LED para indicar estado de conexión WiFi.
// Por defecto, se utiliza el LED incluido en la placa. Si la placa no lo tiene, reemplazarlo por
// otro pin digital del ESP32.
#define LED_WIFI LED_BUILTIN

#define LED_R 22 // Pin Digital para LED RED
#define LED_G 21 // Pin Digital para LED GREEN
#define LED_B 23 // Pin Digital para LED BLUE

#define LED_R_CHANNEL 0 // Canales respectivos para cada LED
#define LED_G_CHANNEL 1 // ESP32 tiene 16 canales PWM
#define LED_B_CHANNEL 2 // con resolución máxima de 16 bits
/* -------------------------------------------------------------------------- */

/* --------------------------------- Objetos -------------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
Ubidots ubidots(TOKEN);

// Delays para reconexión WiFi
DelayMillis t_reconexion;
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

  // Configurar frecuencia y resolucion de los canales de control del Led RGB
  ledcSetup(LED_R_CHANNEL, 5000, 8);
  ledcSetup(LED_G_CHANNEL, 5000, 8);
  ledcSetup(LED_B_CHANNEL, 5000, 8);

  // Añadir canal a su respectivo Pin Digital a ser controlado
  ledcAttachPin(LED_R, LED_R_CHANNEL);
  ledcAttachPin(LED_G, LED_G_CHANNEL);
  ledcAttachPin(LED_B, LED_B_CHANNEL);

  // Configurar estado inicial Led RGB (duty 0% para cada uno)
  ledcWrite(LED_R_CHANNEL, 0);
  ledcWrite(LED_G_CHANNEL, 0);
  ledcWrite(LED_B_CHANNEL, 0);

  // Iniciar Puerto Serial
  Serial.begin(115200);

  // Esperar 2 segundos para abrir terminal y visualizar todo el texto
  delay(2000);

  // Iniciar delays
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
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_LED_R);
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_LED_G);
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_LED_B);
    }
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
  
  // Buffer que almacena mensaje de Ubidots. 
  // Longitud 4, dada por el mayor número (255 = 3 caracteres) más el caracter nulo '\0'
  char buffer[4]; 
  uint8_t duty; // Variable de tipo entero 8 bits, que almacenará el número correspondiente del mensaje

  Serial.println("[UDOTS] Mensaje recibido!");

  // Copiar el mensaje entrante al buffer, para posteriormente convertirlo a número
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = (char)payload[i];
  }
  buffer[length] = '\0';

  duty = atoi(buffer); // Convertir mensaje de tipo texto, a número decimal
  
  // Sentencia simple para revisar si el mensaje que llegó, corresponde a cierto tópico
  if (strstr(topic, VAR_LED_R) != NULL)
  {    
    Serial.printf("[INFO] Led Rojo = %u\n\n", duty);
    ledcWrite(LED_R_CHANNEL, duty); // Cambiar ciclo de trabajo del canal LED R
  }

  if (strstr(topic, VAR_LED_G) != NULL)
  {
    Serial.printf("[INFO] Led Verde = %u\n\n", duty);
    ledcWrite(LED_G_CHANNEL, duty); // Cambiar ciclo de trabajo del canal LED G
  }

  if (strstr(topic, VAR_LED_B) != NULL)
  {
    Serial.printf("[INFO] Led Azul = %u\n\n", duty);
    ledcWrite(LED_B_CHANNEL, duty); // Cambiar ciclo de trabajo del canal LED B
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