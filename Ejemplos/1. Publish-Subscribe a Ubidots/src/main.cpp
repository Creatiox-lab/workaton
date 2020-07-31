// NOTE Explicación Ejemplo
/*
  * Ejemplo de conexión básica a Ubidots con ESP32:
  ? Basic connection to Ubidots with ESP32:
  Este ejemplo demuestra el funcionamiento básico del protocolo MQTT, para realizar "Publish" y
  "Subscribe" (Publicar y Suscribir) a la plataforma Ubidots. Para visualizar lo que ocurre en el
  programa, se utilizará el Monitor Serial a 115200 Baudios.

  ? -> English:
  This example demonstrates the basic operation of the MQTT protocol, to Publish and Subscribe to
  the Ubidots platform. To visualize what happens in runtime, the Serial Monitor at 115200 Baud Rate
  will be used.

  * Publish:
  Para realizar un Publish a Ubidots, se utilizará un Potenciómetro para leer un número mediante un
  Pin Análogo del ESP32, en el rango de 0 a 4095 (ADC con resolución 12 bits). Este dato se
  utilizará para enviarlo a la plataforma, y se visualizará en un Widget de Tanque.

  ? -> English:
  To make a Publish, a potentiometer will be used to read a number using a ESP32 Analog Pin, in the
  range of 0 to 4095 (ADC with 12-bit resolution). The data will be used to send it to the platform,
  and will be displayed in a Tank Widget.

  * Subscribe:
  Para recibir un dato desde la plataforma (Subscribe) se utilizará un Widget de Interruptor, para
  enviar un dato binario (0 o 1) al ESP32. Este dato encenderá un Led conectado a un Pin Digital del
  microcontrolador cuando el Widget Interruptor se encuentre encendido, y viceversa.

  ? -> English:
  To receive the data from the platform (Subscribe) a Switch Widget will be used, to send a binary
  data (0 or 1) to ESP32. This data will turn on a Led connected to a Digital Pin of the
  microcontroller when the Switch Widget is on, and vice versa.

  ! Requisitos:
  ! Requirements:
  * - Conectar Potenciómetro:  
  ? - Connect Potentiometer:
    -> Pin 1: 3.3[V]
    -> Pin 2: Pin 34 ESP32
    -> Pin 3: GND

  * - Conectar Led y Resistencia 1[kOhm]:  
    -> Pin 1 Resistencia: Pin 23 ESP32
    -> Pin 2 Resistencia: Ánodo Led
    -> Cátodo Led       : GND
  ? - Connect Led and Resistor 1[kOhm]:
    -> Pin 1 Resistor: Pin 23 ESP32
    -> Pin 2 Resistor: Anode Led
    -> Cathode Led   : GND

  * - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.
  ? - Configure WiFi Credentials, and Ubidots TOKEN.

  * - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
  *   que en Ubidots se cree el dispositivo automáticamente.
  ? - Turn on the device, and visualize at the Serial Monitor that it sends at least 1 data, so that
  ?   the device is created automatically in Ubidots.

  * - Crear 1 variable llamada "boton".
  ? - Create 1 variable called "boton".

  * - En el Tablero, crear 2 Widgets:
    -> Widget Tanque     : asociarlo a la variable "pot"
    -> Widget Interruptor: asociarlo a la variable "boton"
  ? - On the Dashboard, create 2 Widgets:
    -> Tank Widget  : associate it with the variable "pot"
    -> Switch Widget: associate it with the variable "boton"
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
#define VAR_BOTON   "boton"
#define VAR_POT     "pot"
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

#define LED 23  // Pin Digital para LED 
                /* Digital Pin for LED */
#define POT 34  // Pin Análogo para Potenciómetro
                /* Analog Pin for Potentiometer */
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
  pinMode(LED, OUTPUT);
  pinMode(POT, ANALOG);

  // Configurar estado inicial de los pines 
  /* Configure Initial State of the Pins */
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED, LOW);

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
  
    // Suscribirse, si el cliente se conectó
    /* Subscribe, if the client is connected */
    if (ubidots.connected()) {
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_BOTON);
    }
  }

  // Si el cliente se encuentra conectado, y el tiempo de envío ha finalizado, publicar variable/s a
  // Ubidots y reiniciar tiempo
  /* If the client is connected, and the send time has finished, publish variables to Ubidots */
  if (ubidots.connected() && t_envio.finalizado()) {
    ubidots.add(VAR_POT, analogRead(POT));  // Añadir variable al buffer
                                            /* Add variable to buffer */

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
  // NOTA: Recordar que Ubidots envía los datos en forma de texto, por lo tanto, para comparar
  // números por ejemplo, se deben comparar CARACTERES, no números.
  /** Remember: Ubidots sends data in text type, therefore, to compare numbers, CHARACTERS must be
   * compared, not numbers. */

  Serial.println("[UDOTS] Mensaje recibido!");
  
  // Sentencia simple para revisar si el mensaje que llegó, corresponde a cierto tópico
  /* Simple way to check if the received message, corresponds to a topic */
  if (strstr(topic, VAR_BOTON) != NULL)
  {
    // Revisar si el valor corresponde a un '0' o a un '1'
    /* Check if the value corresponds to a '0' or '1' */
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