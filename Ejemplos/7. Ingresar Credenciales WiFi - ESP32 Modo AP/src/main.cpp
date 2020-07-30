// NOTE Explicación Ejemplo
/*
  * Ejemplo de conexión a WiFi por medio de un Smartphone:
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

  ? Información:
  - En este ejemplo, se configura un Timeout de 1 minuto, para intentar conectarse a una red. Si
    pasa más de 1 minuto, el programa dentro del loop() comienza a ejecutarse. El portal no se
    cerrará y se puede seguir conectándose a la red "esp32ap".

    Si no se configura esto, la opción por defecto es que el programa dentro del loop() nunca se
    ejecute hasta que tenga una conexión activa.

  - No se ha hecho el ejemplo con conexión a Ubidots, para que sea simple de entender.

  - Esta librería ocupa gran cantidad de memoria, ya que almacena todas las páginas del Portal, para
    hacerlo más atractivo al usuario. Esta es una de las maneras que existen para guardar
    credenciales en el ESP32, hay muchas otras, pero esta es una de las maneras más completas, ya
    que tiene características como: auto conexión, múltiples redes, configuración de nuevas páginas,
    etc.
*/

#include <Arduino.h>           // Incluirla siempre al utilizar PlatformIO
#include "delayMillis.h"       // Para utilizar Delays no-bloqueantes
#include "AutoConnect.h"       // Libreria para interfaz Modo AP

// NOTE Pines ESP32
/* -------------------------------------------------------------------------- */
// LED para indicar estado de conexión WiFi.
// Por defecto, se utiliza el LED incluido en la placa. Si la placa no lo tiene, reemplazarlo por
// otro pin digital del ESP32.
#define LED_WIFI LED_BUILTIN
/* -------------------------------------------------------------------------- */

/* --------------------------------- Objetos -------------------------------- */
// Constructores básicos para inicializar un Portal de Conexión
WebServer server;
AutoConnect portal(server);
AutoConnectConfig config;

// Delays
DelayMillis t_parpadeo, t_cuenta;
/* -------------------------------------------------------------------------- */

/* -------------------------- Declaracion Funciones ------------------------- */
void callbackWifiConectado(WiFiEvent_t event);  // Se ejecuta cuando WiFi asoció dirección IP
void callbackWifiDesconectado(WiFiEvent_t event); // Se ejecuta cuando WiFi se desconecta
/* -------------------------------------------------------------------------- */

/* ------------ Función a ejecutar al ingresar a Página de Inicio ----------- */
void rootPage() {
  char content[] = "Hola Mundo";  // Mensaje de prueba

  // Al ingresar a la página de inicio, mandar de vuelta un OK (codigo 200),
  // y el texto "Hola Mundo" (definido por el tipo de contenido HTML 'text/plain')
  server.send(200, "text/plain", content);
}
/* -------------------------------------------------------------------------- */

/* --------------------------- Variables Globales --------------------------- */
bool wifi_on = false; // Indica si hay conexión activa a WiFi
uint8_t num = 0;  // Simple contador
/* -------------------------------------------------------------------------- */

// ANCHOR setup()
// * ---------------------------------------------------------------------------
void setup() {
  // Configurar pines
  pinMode(LED_WIFI, OUTPUT);

  // Configurar estado inicial Led WiFi
  digitalWrite(LED_WIFI, LOW);

  // Iniciar Puerto Serial
  Serial.begin(115200);
  Serial.println();

  // Esperar 2 segundos para abrir terminal y visualizar todo el texto
  delay(2000);

  // Iniciar delays
  t_parpadeo.empezar(500);
  t_cuenta.empezar(2000);

  // Llamar a rootPage() cuando se ingrese a la página principal
  server.on("/", rootPage);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Asociar Callbacks del WiFi
  WiFi.onEvent(callbackWifiConectado, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(callbackWifiDesconectado, SYSTEM_EVENT_STA_DISCONNECTED);

  Serial.println("[WIFI] Conectando...");
  
  // Configuración personalizada del Portal
  config.portalTimeout = 60000; // Timeout de 1 minuto
  config.retainPortal = true;   // Mantener portal abierto si no encuentra la red
  config.autoReconnect = true;  // Intentar reconectar a última red guardada
  portal.config(config);        // Aplicar configuración

  // Iniciar conexión a una red o abrir portal
  // Si no se pudo encontrar una red existente, arrojar mensaje
  if (!portal.begin()) {
    Serial.println("[WIFI] No se pudo conectar a la red WiFi!");
  }
}
// * ---------------------------------------------------------------------------

// ANCHOR loop()
// * ---------------------------------------------------------------------------
void loop() {
  // Si el WiFi (modo STATION) está activo, parpadear el Led cada "t_parpadeo" tiempo
  if (wifi_on == true && t_parpadeo.finalizado()) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    t_parpadeo.repetir();
  }

  // Contador simple para indicar que el loop() se encuentra ejecutándose
  if (t_cuenta.finalizado()) {
    Serial.printf("[INFO] Cuenta: %u\n", num++);
    t_cuenta.repetir();
  }

  // Esta sentencia debe ser llamada constantemente para encargarse de los mensajes entrantes
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