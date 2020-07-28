// NOTE Explicación Ejemplo
/*
  * Ejemplo de visualización de variables de Ubidots en una LCD 16x2:
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

  * Subscribe:
    Para recibir los datos de entrada manual, se utilizarán dos Widgets de Entrada Manual, con rango
    0-100 en pasos de 1 (como viene por defecto al crear el Widget).

  ? Requisitos:
  - Conectar LCD 16x2 con adaptador I2C:
    -> Pin VCC: 3.3[V] --- LEER ENUNCIADO ANTERIOR
    -> Pin SDA: Pin 21 del ESP32   
    -> Pin SCL: Pin 22 del ESP32
    -> Pin GND: Tierra

  - Configurar las Credenciales WiFi, y el TOKEN de Ubidots.

  - Encender el dispositivo, y visualizar por el Monitor Serial de que envíe al menos 1 dato, para
    que en Ubidots se cree el dispositivo automáticamente.

  - Crear 2 variables en el dispositivo:
    -> Variable 1: "input_1"
    -> Variable 2: "input_2"

  - En el Tablero, crear 2 Widgets:
    -> Widget Deslizador 1: asociarlo a la variable "input_1"
    -> Widget Deslizador 2: asociarlo a la variable "input_2"
*/

#include <Arduino.h>           // Incluirla siempre al utilizar PlatformIO
#include "delayMillis.h"       // Para utilizar Delays no-bloqueantes
#include "UbidotsESP32MQTT.h"  // Libreria de Ubidots MQTT
#include "LiquidCrystal_I2C.h" // Libreria para manipular LCD con adaptador I2C

// NOTE Credenciales WiFi
/* -------------------------------------------------------------------------- */
#define SSID  ""
#define PASS  ""
#define TOKEN ""
/* -------------------------------------------------------------------------- */

// NOTE Variables Ubidots
/* -------------------------------------------------------------------------- */
#define DISPOSITIVO ""

#define VAR_INPUT_1 "input_1"
#define VAR_INPUT_2 "input_2"
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

// Delays para reconexión WiFi
DelayMillis t_reconexion;

// NOTE Config. LCD
// Constructor para el lcd, pasa como parámetro la dirección I2C, filas y columnas
// La dirección suele ser 0x27 o 0x3F, ya que se utiliza el mismo chip para la mayoría de pantallas
LiquidCrystal_I2C lcd(0x27, 16, 2);
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

  // Iniciar LCD y encender Backlight
  lcd.init();
  lcd.backlight();

  // Imprimir texto fijo, las variables se escriben en el Callback Ubidots
  lcd.print("Input 1:");
  lcd.setCursor(0, 1);
  lcd.print("Input 2:");

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
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_INPUT_1);
      ubidots.ubidotsSubscribe(DISPOSITIVO, VAR_INPUT_2);
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
  uint8_t input; // Variable de tipo entero 8 bits, que almacenará el número correspondiente del mensaje

  Serial.println("[UDOTS] Mensaje recibido!");

  // Copiar el mensaje entrante al buffer, para posteriormente convertirlo a número
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = (char)payload[i];
  }
  buffer[length] = '\0';

  input = atoi(buffer); // Convertir mensaje de tipo texto, a número decimal
  
  // Sentencia simple para revisar si el mensaje que llegó, corresponde a cierto tópico
  if (strstr(topic, VAR_INPUT_1) != NULL)
  {    
    Serial.printf("[INFO] Input 1 = %u\n\n", input);

    /**
     * Setear cursor en columna 9 - fila 0, ya que la variable se escribirá después del texto fijo,
     * con formato de número de 3 espacios
     * 
     * Columna    = 0 1 2 3 4 5 6 7 8 9
     * Texto LCD  = I n p u t   1 :   1 0 0
     */
    lcd.setCursor(9, 0);
    lcd.printf("%3u", input); // Imprimir con formato de 3 espacios fijos, sin relleno de ceros
  }

  if (strstr(topic, VAR_INPUT_2) != NULL)
  {
    Serial.printf("[INFO] Input 2 = %u\n\n", input);
    
    /**
     * Setear cursor en columna 9 - fila 1, ya que la variable se escribirá después del texto fijo,
     * con formato de número de 3 espacios
     * 
     * Columna    = 0 1 2 3 4 5 6 7 8 9
     * Texto LCD  = I n p u t   2 :   1 0 0
     */
    lcd.setCursor(9, 1);
    lcd.printf("%3u", input); // Imprimir con formato de 3 espacios fijos, sin relleno de ceros
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