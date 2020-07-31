// NOTE Explicación Ejemplo
/*
  * Ejemplo de conexión a WiFi por medio de un Smartphone:
  ? Connection to WiFi through a Smartphone:
  En este ejemplo, se utiliza la librería "AutoConnect", la cual crea un Portal de Conexión para
  ingresar las credenciales de una red WiFi por medio de un navegador en un Smartphone.

  El esquema del programa es el siguiente:
  1.  El equipo inicia, y busca una red WiFi existente.
      Red WiFi existente, se refiere a que si el ESP32 se ha conectado alguna vez a una red
      WiFi, las credenciales quedan guardadas, por ende, se intentará conectar a ella.

  2.  Si la red no existe y/o no se puede conectar, abre la conexión al Portal. Para ingresar a
      este, en el Smartphone se debe navegar hasta las redes wifi disponibles, y conectarse a la
      siguiente red (actualizar hasta verla disponible):
      - Nombre    : esp32ap
      - Contraseña: 12345678

  3.  En los Smartphones más recientes, al conectarse a la red, inmediatamente lo redirige al
      Portal en el navegador. Si no es así, ingresar la siguiente dirección en el navegador:
      - Dirección: 172.217.28.1/_ac

      Al entrar al portal, tocar en la barra triple de la esquina superior derecha, y luego tocar
      en "Configure new AP".

      Elegir una red WiFi disponible, e ingresar la contraseña en "Passphrase". Activar el ticket
      "Enable DHCP", y luego presionar en "Apply".

  4.  Si las credenciales son correctas, el ESP32 intentará conectarse a Internet. Además, si
      estas credenciales son correctas, en un próximo reinicio se utilizarán como red principal
      para conectarse a Internet.

  5.  Para reiniciar el ESP32 y cerrar el Portal, presionar sobre el icono de barra triple,
      luego en "Reset...", y por último, presionar sobre el botón "RESET" que aparecerá.

  ? -> English:
  In this example, the "AutoConnect" library is used, which creates a Connection Portal to enter the
  credentials of a WiFi network through a browser on a Smartphone.

  The scheme of the program is as follows:
  1.  The computer starts up and searches for an existing WiFi network. Existing WiFi network, means
      that if the ESP32 has ever connected to a WiFi network, the credentials are saved, therefore, an
      attempt will be made to connect to it.

  2.  If the network does not exist and / or cannot connect, it opens the connection to the Portal. To
      enter this, on the Smartphone you must navigate to the available Wi-Fi networks, and connect to
      the following network (update until you see it available):
      - Name: esp32ap
      - Password: 12345678

  3.  In the latest Smartphones, when connecting to the network, it immediately redirects you to the
      Portal in the browser. If not, enter the following address in the browser:
      - Address: 172.217.28.1/_ac

      When entering the portal, tap on the triple bar in the upper right corner, and then tap on "Configure new AP".
      Choose an available WiFi network, and enter the password in "Passphrase". Activate the "Enable
      DHCP" ticket, and then click "Apply".

  4.  If the credentials are correct, ESP32 will try to connect to Internet. Also, if these
      credentials are correct, they will be used as the main network to connect to the Internet in a
      subsequent reboot.

  5.  To restart ESP32 and close the Portal, press on the triple bar icon, then on "Reset...", and
      finally, press on the "RESET" button that will appear.  

  ! Información:
  ! Information:
  * - En este ejemplo, se configura un Timeout de 1 minuto, para intentar conectarse a una red. Si
  *   pasa más de 1 minuto, el programa dentro del loop() comienza a ejecutarse. El portal no se
  *   cerrará y se puede seguir conectándose a la red "esp32ap".
  *   Si no se configura esto, la opción por defecto es que el programa dentro del loop() nunca se
  *   ejecute hasta que tenga una conexión activa.
  ? - In this example, a 1 minute Timeout is configured to try to connect to a network. If more
  ?   than 1 minute passes, the program inside the loop () starts running. The portal will not close
  ?   and you can continue connecting to the "esp32ap" network.
  ?   If this is not configured, the default is for the program inside loop () to never run until
  ?   it has an active connection.
    
  * - No se ha hecho el ejemplo con conexión a Ubidots, para que sea simple de entender.
  ? - The example with connection to Ubidots has not been done, to make it simple to understand.

  * - Esta librería ocupa gran cantidad de memoria, ya que almacena todas las páginas del Portal, para
  *   hacerlo más atractivo al usuario. Esta es una de las maneras que existen para guardar
  *   credenciales en el ESP32, hay muchas otras, pero esta es una de las maneras más completas, ya
  *   que tiene características como: auto conexión, múltiples redes, configuración de nuevas páginas,
  *   etc.
  ? - This library occupies a large amount of memory, since it stores all the pages of the
  ?   Portal, to make it more attractive to the user. This is one of the ways that exist to save
  ?   credentials in the ESP32, there are many others, but this is one of the most complete ways,
  ?   since it has features such as: auto connection, multiple networks, configuration of new pages,
  ?   etc.
  
*/

#include <Arduino.h>          // Incluirla siempre al utilizar PlatformIO
                              // Always include it when using PlatformIO
#include "delayMillis.h"      // Para utilizar Delays no-bloqueantes
                              // Non-blocking Delays
#include "AutoConnect.h"      // AutoConnect Captive Portal Library

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
// Constructores básicos para inicializar un Portal de Conexión
/* Constructors needed to start a Captive Portal */
WebServer server;
AutoConnect portal(server);
AutoConnectConfig config;

// Delays
DelayMillis t_parpadeo, t_cuenta;
/* -------------------------------------------------------------------------- */

/* -------------- Declaracion Funciones (Function Declarations) ------------- */
void callbackWifiConectado(WiFiEvent_t event);    // Se ejecuta cuando WiFi asoció dirección IP
                                                  /* WiFi connected Callback */
void callbackWifiDesconectado(WiFiEvent_t event); // Se ejecuta cuando WiFi se desconecta
                                                  /* WiFi disconnected Callback */
/* -------------------------------------------------------------------------- */

/* ---------------------- Página de Inicio (Root Page) ---------------------- */
void rootPage() {
  char content[] = "Hola Mundo";  // Mensaje de prueba
                                  /* Test message */

  // Al ingresar a la página de inicio, mandar de vuelta un OK (codigo 200),
  // y el texto "Hola Mundo" (definido por el tipo de contenido HTML 'text/plain')
  /** 
  * When entering the home page, send back an OK (code 200), 
  * and the text "Hola Mundo" (defined by the HTML content type 'text/plain')
  */
  server.send(200, "text/plain", content);
}
/* -------------------------------------------------------------------------- */

/* ------------------ Variables Globales (Global Variables) ----------------- */
bool wifi_on = false; // Indica si hay conexión activa a WiFi
                      /* Indicates WiFi connection */
uint8_t num = 0;      // Simple contador
                      /* Simple counter */
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
  Serial.println();

  // Esperar 2 segundos para abrir terminal y visualizar todo el texto
  /* Wait 2 seconds to open terminal */
  delay(2000);

  // Iniciar delays
  /* Start delays */
  t_parpadeo.empezar(500);
  t_cuenta.empezar(2000);

  // Llamar a rootPage() cuando se ingrese a la página principal
  /* Call rootPage() when you enter main page */
  server.on("/", rootPage);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Asociar Callbacks del WiFi
  /* Associate WiFi Callbacks */
  WiFi.onEvent(callbackWifiConectado, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(callbackWifiDesconectado, SYSTEM_EVENT_STA_DISCONNECTED);

  Serial.println("[WIFI] Conectando...");
  
  // Configuración personalizada del Portal
  /* Custom configuration of the Portal */
  config.portalTimeout = 60000; // Timeout de 1 minuto
                                /* 1 minute Timeout */
  config.retainPortal = true;   // Mantener portal abierto si no encuentra la red
                                /* Keep Portal if it can't find the network */
  config.autoReconnect = true;  // Intentar reconectar a última red guardada
                                /* Try to reconnect to last saved network */
  portal.config(config);        // Aplicar configuración
                                /* Apply configuration */

  // Iniciar conexión a una red o abrir portal
  // Si no se pudo encontrar una red existente, arrojar mensaje
  /* Start network connection, or open Portal */
  /* If an existing network could not be found, display message */
  if (!portal.begin()) {
    Serial.println("[WIFI] No se pudo conectar a la red WiFi!");
  }
}
// * ---------------------------------------------------------------------------

// ANCHOR loop()
// * ---------------------------------------------------------------------------
void loop() {
  // Si el WiFi (modo STATION) está activo, parpadear el Led cada "t_parpadeo" tiempo
  /* If WiFi (STATION mode) is active, blink the Led every "t_parpadeo" time */
  if (wifi_on == true && t_parpadeo.finalizado()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    t_parpadeo.repetir();
  }

  // Contador simple para indicar que el loop() se encuentra ejecutándose
  /* Simple counter to indicate that loop() is running */
  if (t_cuenta.finalizado()) {
    Serial.printf("[INFO] Cuenta: %u\n", num++);
    t_cuenta.repetir();
  }

  // Esta sentencia debe ser llamada constantemente para encargarse de los mensajes entrantes
  /* This statement must be constantly called to deal with incoming messages */
  portal.handleClient();
}
// * ---------------------------------------------------------------------------

// ANCHOR Callbacks WiFi
// * ---------------------------------------------------------------------------
void callbackWifiConectado(WiFiEvent_t event) {
  wifi_on = true;
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.print("[WIFI] WiFi Conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void callbackWifiDesconectado(WiFiEvent_t event) {
  wifi_on = false;
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("[WIFI] WiFi Desconectado!");
}
// * ---------------------------------------------------------------------------