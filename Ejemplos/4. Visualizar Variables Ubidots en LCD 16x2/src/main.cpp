// NOTE Explicación Ejemplo
/*
  * Ejemplo de visualización de variables de Ubidots en una LCD 16x2:
  ? Displaying Ubidots variables on a 16x2 LCD:
  En este ejemplo, se leerán dos variables de Ubidots, y se visualizarán en una pantalla LCD 16x2
  con adaptador Paralelo a I2C, el cual se puede alimentar con 3.3[V] y 5[V], y tiene un ajuste de
  contraste con un potenciómetro SMD por la cara inferior.
  Se debe tener muy en claro ciertas consideraciones antes de utilizar esta Pantalla LCD con ESP32:
  1.  La pantalla nativamente funciona a 5[V], pero se puede alimentar también con menos
      voltaje, es decir, 3.3[V]. El inconveniente de esto, es que el contraste no será el mejor,
      ya que al estar alimentada con menos voltaje, las letras tendrán el contraste bastante más
      bajo.
  2.  Los GPIO del ESP32 no son tolerantes a 5[V], por ende, no debe haber un voltaje más alto que
      3.3[V] en cualquiera de los pines, ya que podría ser dañado permanentemente.
  3.  Para resolver el tema del contraste, es posible utilizar un Buffer Bidireccional, o conocido
      como "Level Shifter", el cual intercambia los niveles de voltaje en las líneas de
      comunicación I2C (SDA y SCL), con el fin de mantener al ESP32 con niveles lógicos de 3.3[V].
      Más información en el siguiente link: https://www.luisllamas.es/arduino-level-shifter/
  4.  Para este ejemplo, como modo de estudio, se puede alimentar con 3.3[V] y tener en cuenta el
      problema del contraste (que sabemos, se soluciona con un Buffer).

  ? -> English:
  In this example, two Ubidots variables will be read, and displayed on a 16x2 LCD screen with a
  Parallel to I2C adapter, which can be powered with 3.3 [V] and 5 [V], and has a contrast
  adjustment with a potentiometer SMD on the underside.
  You must be very clear about certain considerations before using this LCD Screen with ESP32:
  1.  The display natively operates at 5 [V], but can also be powered with less voltage, ie 3.3 [V].
      The drawback of this is that the contrast will not be the best, since being supplied with less
      voltage, the letters will have a much lower contrast.
  2.  The ESP32 GPIOs are not tolerant to 5 [V], therefore, there should not be a voltage higher than
      3.3 [V] on any of the pins, as it could be permanently damaged.
  3.  To resolve the contrast issue, it is possible to use a Bidirectional Buffer, or known as "Level
      Shifter", which exchanges the voltage levels in the I2C communication lines (SDA and SCL), in
      order to maintain the ESP32 with logical levels of 3.3 [V]. More information at the following
      link: https://www.luisllamas.es/arduino-level-shifter/
  4.  For this example, as a study mode, you can feed 3.3 [V] and take into account the contrast
      problem (which we know is solved with a Buffer).

  * Subscribe:
  Para recibir los datos de entrada manual, se utilizarán dos Widgets de Entrada Manual, con rango
  0-100 en pasos de 1 (como viene por defecto al crear el Widget).

  ? -> English:
  To receive the manual entry data, two Manual Input Widgets will be used, with range 0-100 in steps
  of 1 (as it comes by default when creating the Widget).

  ! Requisitos:
  ! Requirements:
  * - Conectar LCD 16x2 con adaptador I2C:
  ? - Connect LCD 16x2 with I2C adapter:
    -> Pin VCC: 3.3[V] --- LEER ENUNCIADO ANTERIOR (READ PREVIOUS STATEMENT)
    -> Pin SDA: Pin 21 ESP32   
    -> Pin SCL: Pin 22 ESP32
    -> Pin GND: GND

  * - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.
  ? - Configure the WiFi Credentials, and the Ubidots TOKEN.

  * - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
  *   que en Ubidots se cree el dispositivo automáticamente.
  ? - Turn on the device, and check on the Serial Monitor that it sends at least 1 data, so that 
  ?   the device is created automatically in Ubidots.

  * - Crear 2 variables en el dispositivo:
  ? - Create 2 variables on the device:
    -> Variable 1: "input_1"
    -> Variable 2: "input_2"

  * - En el Tablero, crear 2 Widgets:
    -> Widget Entrada Manual 1: asociarlo a la variable "input_1"
    -> Widget Entrada Manual 2: asociarlo a la variable "input_2"
  ? - On the Dashboard, create 2 Widgets:
    -> Manual Input Widget 1: associate it with the variable "input_1"
    -> Manual Input Widget 2: associate it with the variable "input_2"
*/

#include <Arduino.h>            // Incluirla siempre al utilizar PlatformIO
                                // Always include it when using PlatformIO
#include "delayMillis.h"        // Para utilizar Delays no-bloqueantes
                                // Non-blocking Delays
#include "UbidotsESP32MQTT.h"   // Libreria de Ubidots MQTT
                                // Ubidots MQTT Library
#include "LiquidCrystal_I2C.h"  // Libreria para manipular LCD con adaptador I2C
                                // I2C 16x2 LCD Library

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
#define VAR_INPUT_1 "input_1"
#define VAR_INPUT_2 "input_2"
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

// Delay para reconexión WiFi
/* Delay for WiFi reconnection */
DelayMillis t_reconexion;

// NOTE Config. LCD
// Constructor para el lcd, pasa como parámetro la dirección I2C, filas y columnas
// La dirección suele ser 0x27 o 0x3F, ya que se utiliza el mismo chip para la mayoría de pantallas
/**
 * LCD Constructor, pass the I2C address, rows and columns as a parameter
 * The address is usually 0x27 or 0x3F, since the same chip is used for most displays
 */
LiquidCrystal_I2C lcd(0x27, 16, 2);
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

  // Iniciar LCD y encender Backlight
  /* Start LCD and turn on Backlight */
  lcd.init();
  lcd.backlight();

  // Imprimir texto fijo. Las variables se escriben en el Callback Ubidots
  /* Print fixed text. The variables are written on the Callback function */
  lcd.print("Input 1:");
  lcd.setCursor(0, 1);
  lcd.print("Input 2:");

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
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_INPUT_1);
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_INPUT_2);
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
  uint8_t input;  // Variable de tipo entero 8 bits, que almacenará el número correspondiente del mensaje
                  /* 8 bits int variable, which will store the corresponding number of the message */  

  Serial.println("[UDOTS] Mensaje recibido!");

  // Copiar el mensaje entrante al buffer, para posteriormente convertirlo a número
  /* Copy the incoming message to the buffer, to later convert it to number */
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = (char)payload[i];
  }
  buffer[length] = '\0';

  input = atoi(buffer); // Convertir mensaje de tipo texto, a número decimal
                        /* Convert text message, to decimal number */
  
  // Sentencia simple para revisar si el mensaje que llegó, corresponde a cierto tópico
  /* Simple way to check if the received message, corresponds to a topic */
  if (strstr(topic, VAR_INPUT_1) != NULL)
  {    
    Serial.printf("[INFO] Input 1 = %u\n\n", input);

    // Setear cursor en columna 9 - fila 0, ya que la variable se escribirá después del texto fijo,
    // con formato de número de 3 espacios
    // 
    // Columna    = 0 1 2 3 4 5 6 7 8 9
    // Texto LCD  = I n p u t   1 :   1 0 0
    /**
     * Set cursor in column 9 - row 1, since the variable will be written after the fixed text,
     * formatted with a number of 3 spaces
     * 
     * Column     = 0 1 2 3 4 5 6 7 8 9
     * LCD Text   = I n p u t   1 :   1 0 0
     */
    lcd.setCursor(9, 0);
    lcd.printf("%3u", input); // Imprimir con formato de 3 espacios fijos, sin relleno de ceros
  }

  if (strstr(topic, VAR_INPUT_2) != NULL)
  {
    Serial.printf("[INFO] Input 2 = %u\n\n", input);
    
    // Setear cursor en columna 9 - fila 1, ya que la variable se escribirá después del texto fijo,
    // con formato de número de 3 espacios
    // 
    // Columna    = 0 1 2 3 4 5 6 7 8 9
    // Texto LCD  = I n p u t   2 :   1 0 0
    /**
     * Set cursor in column 9 - row 1, since the variable will be written after the fixed text,
     * formatted with a number of 3 spaces
     * 
     * Column     = 0 1 2 3 4 5 6 7 8 9
     * LCD Text   = I n p u t   2 :   1 0 0
     */
    lcd.setCursor(9, 1);
    lcd.printf("%3u", input); // Imprimir con formato de 3 espacios fijos, sin relleno de ceros
                              /* Print with 3 fixed space format, without zero padding */
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