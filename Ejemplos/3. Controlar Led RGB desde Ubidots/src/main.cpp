// NOTE Explicación Ejemplo
/*
  * Ejemplo de control Led RGB mediante Ubidots:
  ? RGB Led control through Ubidots:
  Este sencillo ejemplo muestra cómo utilizar el periférico Led Control (LEDC) del ESP32, para
  controlar el color de un Led RGB por medio de la plataforma. Para este caso, se utilizará un Led
  RGB de cátodo común, con resistencias de 330[ohm] para cada Led individual.

  ? -> English:
  This simple example shows how to use the ESP32 peripheral Led Control (LEDC) to control the color
  of an RGB Led through the platform. For this case, a common cathode RGB Led will be used, with 330
  [ohm] resistances for each individual Led.

  * Subscribe:
  Para recibir los datos del nivel de intensidad de cada color, se utilizará un Widget Deslizador,
  con 3 variables (1 variable para cada color).

  ? -> English:
  To receive the intensity level data for each color, a Slider Widget will be used, with 3 variables
  (1 variable for each color).

  ! Requisitos:
  ! Requirements:
  * - Conectar Led RGB:
    -> Pin R (rojo):    Pin 1 Resistencia. Pin 2 Resistencia a Pin 22 ESP32
    -> Pin G (verde):   Pin 1 Resistencia. Pin 2 Resistencia a Pin 21 ESP32
    -> Pin B (azul):    Pin 1 Resistencia. Pin 2 Resistencia a Pin 23 ESP32
    -> Pin - (cátodo):  GND
  ? - Connect RGB Led:
    -> Pin R (red):     Pin 1 Resistor. Pin 2 Resistor to Pin 22 ESP32
    -> Pin G (green):   Pin 1 Resistor. Pin 2 Resistor to Pin 21 ESP32
    -> Pin B (blue):    Pin 1 Resistor. Pin 2 Resistor to Pin 23 ESP32
    -> Pin - (cathode): GND

  * - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.
  ? - Configure the WiFi Credentials, and the Ubidots TOKEN.

  * - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
  *   que en Ubidots se cree el dispositivo automáticamente.
  ? - Turn on the device, and check on the Serial Monitor that it sends at least 1 data, so that 
  ?   the device is created automatically in Ubidots.

  * - Crear 3 variables en el dispositivo:
  ? - Create 3 variables on the device:
    -> Variable 1: "led_r"
    -> Variable 2: "led_g"
    -> Variable 3: "led_b"

  * - En el Tablero, crear 1 Widget:
    -> Widget Deslizador: asociarlo a la variables creadas en el paso anterior
  ? - On the Dashboard, create 1 Widget:
    -> Slider Widget; associate it with the variables created in the previous step
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

// Nombres de variables en Ubidots *variable names on Ubidots*
#define VAR_LED_R   "led_r"
#define VAR_LED_G   "led_g"
#define VAR_LED_B   "led_b"
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

#define LED_R 22 // Pin Digital para LED RED    *Digital Pin for RED LED*
#define LED_G 21 // Pin Digital para LED GREEN  *Digital Pin for GREEN LED*
#define LED_B 23 // Pin Digital para LED BLUE   *Digital Pin for BLUE LED*

// Canales respectivos para cada LED. ESP32 tiene 16 canales PWM con resolución máxima de 16 bits
/* Respective channels for each LED. ESP32 has 16 PWM channels with maximum resolution of 16 bits */
#define LED_R_CHANNEL 0
#define LED_G_CHANNEL 1
#define LED_B_CHANNEL 2
/* -------------------------------------------------------------------------- */

/* ---------------------------- Objetos (Objects) --------------------------- */
// Constructor principal, pasa como parámetro el Token de Ubidots
/* Main Constructor, pass Ubidots Token as a parameter */
Ubidots ubidots(TOKEN);

// Delay para reconexión WiFi
/* Delay for WiFi reconnection */
DelayMillis t_reconexion;
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

  // Configurar frecuencia y resolucion de los canales de control del Led RGB
  /* Configure Frequency and resolution of the RGB Led control channels */
  ledcSetup(LED_R_CHANNEL, 5000, 8);
  ledcSetup(LED_G_CHANNEL, 5000, 8);
  ledcSetup(LED_B_CHANNEL, 5000, 8);

  // Añadir canal a su respectivo Pin Digital a ser controlado
  /* Add channel to its respective Digital Pin to be controlled */
  ledcAttachPin(LED_R, LED_R_CHANNEL);
  ledcAttachPin(LED_G, LED_G_CHANNEL);
  ledcAttachPin(LED_B, LED_B_CHANNEL);

  // Configurar estado inicial Led RGB (duty 0% para cada uno)
  /* Set initial status RGB Led (duty 0% for each) */
  ledcWrite(LED_R_CHANNEL, 0);
  ledcWrite(LED_G_CHANNEL, 0);
  ledcWrite(LED_B_CHANNEL, 0);

  // Iniciar Puerto Serial
  /* Start Serial Port */
  Serial.begin(115200);

  // Esperar 2 segundos para abrir terminal y visualizar todo el texto
  /* Wait 2 seconds to open terminal */
  delay(2000);

  // Iniciar delays
  /* Start delays */
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
  
    // Suscribirse, si el cliente se conectó
    /* Subscribe, if the client is connected */
    if (ubidots.connected()) {
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_LED_R);
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_LED_G);
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_LED_B);
    }
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
  // NOTA: Recordar que Ubidots envía los datos en forma de texto, por lo tanto, para comparar
  // números por ejemplo, se deben comparar CARACTERES, no números.
  /** Remember: Ubidots sends data in text type, therefore, to compare numbers, CHARACTERS must be
   * compared, not numbers. */
  
  // Buffer que almacena mensaje de Ubidots. 
  // Longitud 4, dada por el mayor número (255 = 3 caracteres) más el caracter nulo '\0'
  /**
   * Buffer that stores message of Ubidots.
   * Length 4, given by the largest number (255 = 3 characters) plus the null character '\0'
   */
  char buffer[4]; 
  uint8_t duty; // Variable de tipo entero 8 bits, que almacenará el número correspondiente del mensaje
                /* 8 bits int variable, which will store the corresponding number of the message */  

  Serial.println("[UDOTS] Mensaje recibido!");

  // Copiar el mensaje entrante al buffer, para posteriormente convertirlo a número
  /* Copy the incoming message to the buffer, to later convert it to number */
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = (char)payload[i];
  }
  buffer[length] = '\0';

  duty = atoi(buffer);  // Convertir mensaje de tipo texto, a número decimal
                        /* Convert text message, to decimal number */
  
  // Sentencia simple para revisar si el mensaje que llegó, corresponde a cierto tópico
  /* Simple way to check if the received message, corresponds to a topic */
  if (strstr(topic, VAR_LED_R) != NULL)
  {    
    Serial.printf("[INFO] Led Rojo = %u\n\n", duty);
    ledcWrite(LED_R_CHANNEL, duty); // Cambiar ciclo de trabajo del canal LED R
                                    /* Change duty cycle of channel LED R */
  }

  if (strstr(topic, VAR_LED_G) != NULL)
  {
    Serial.printf("[INFO] Led Verde = %u\n\n", duty);
    ledcWrite(LED_G_CHANNEL, duty); // Cambiar ciclo de trabajo del canal LED G
                                    /* Change duty cycle of channel LED G */
  }

  if (strstr(topic, VAR_LED_B) != NULL)
  {
    Serial.printf("[INFO] Led Azul = %u\n\n", duty);
    ledcWrite(LED_B_CHANNEL, duty); // Cambiar ciclo de trabajo del canal LED B
                                    /* Change duty cycle of channel LED B */
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