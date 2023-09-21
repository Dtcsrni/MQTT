/*
-------------------------------------
-    Erick Renato Vega Cerón        -
- Maestría en Internet de las Cosas -
-            UAEH                   -
-    https://github.com/Dtcsrni     -
-------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------
-Código para controlar un nodo de sensado y actuación donde se midan temperatura, humedad (DHT11) y presencia viva (PIR)                           -
-Los datos se muestran en la pantalla física cuando hay alguien cerca para verlo, y al no detectarse presencia humana por un tiempo considerable   -
-se apaga la pantalla para ahorrar energía y mejorar la vida de la pantalla OLED.                                                                  -
-Se publica en un broker de HiveQ los datos sensados en formato JSON y se controla el funcionamiento del ventilador con una app (IoT MQTT Panel)   -
-Basado en proyecto Cahuitl                                                                                                                        -
----------------------------------------------------------------------------------------------------------------------------------------------------
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define BUILTIN_LED 13
#define LED_2 15
#define LED_3 12
#define RELE 33
#define PIR 14

#define SCREEN_WIDTH 128  // Ancho de la pantalla OLED, en píxeles
#define SCREEN_HEIGHT 64  // Alto de la pantalla OLED, en píxeles


// Declaración para una pantalla SSD1306 conectada a través de I2C (pines SDA, SCL)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


#define DHTPIN 2  // Pin digital conectado al sensor DHT
// Descomenta el tipo de sensor que estás utilizando:
#define DHTTYPE DHT11  // DHT 11 (AM2302)

//Datos Wifi
const char* ssid = "ArmsysTech";
const char* password = "sjmahpe122512";

//Datos MQTT
//Broker Público
const char* mqtt_servidor_publico = "mqtt-dashboard.com";
//Broker Local
const char* mqtt_servidor_local = "127.0.0.1";

const char* topic_Sensores = "iot/UAEH/ErickVega/Estudio/Sensores";
const char* topic_Actuadores = "iot/UAEH/ErickVega/Estudio/Actuadores";

WiFiClient espClient;
PubSubClient client(espClient);
long ultimoMsj = 0;
char mensaje[50];
int value = 0;
bool presenciaHumanaCercana = false;
float temperaturaActual = 0;
float temperaturaAnterior = 0;
float humedadActual = 0;
float humedadAnterior = 0;
int ciclos_Encendidos_Pantalla = 0;
long now = 0;
bool ventilador = false;
bool modoAutomatico = false;
StaticJsonDocument<48> datosSensores;
StaticJsonDocument<16> datosActuadores;

DHT dht(DHTPIN, DHTTYPE);


// 'icono_armsys', 50x50px
const unsigned char icono_armsys[350] PROGMEM = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xb1, 0x8c, 0x7f, 0xff, 0xc0, 0xff, 0xff,
  0x10, 0x00, 0x3f, 0xff, 0xc0, 0xff, 0xff, 0x50, 0x00, 0x3f, 0xff, 0xc0, 0xff, 0xff, 0x90, 0x00,
  0x3f, 0xff, 0xc0, 0xff, 0xff, 0x10, 0x84, 0x3f, 0xff, 0xc0, 0xff, 0xe0, 0x00, 0x00, 0x03, 0xff,
  0xc0, 0xff, 0xc0, 0x00, 0x00, 0x03, 0xff, 0xc0, 0xff, 0x80, 0x00, 0x00, 0x01, 0xff, 0xc0, 0xff,
  0x07, 0xff, 0xff, 0xf0, 0xff, 0xc0, 0xfe, 0x0f, 0xff, 0xff, 0xf8, 0x7f, 0xc0, 0xfc, 0x1f, 0xff,
  0xff, 0xfc, 0x7f, 0xc0, 0xfc, 0x3c, 0x60, 0x60, 0x02, 0x4f, 0xc0, 0xfc, 0x7c, 0x20, 0x00, 0x02,
  0x4f, 0xc0, 0xfc, 0x7c, 0x20, 0x01, 0x02, 0x4f, 0xc0, 0xfc, 0x7c, 0x60, 0x00, 0x02, 0x0f, 0xc0,
  0x80, 0x7c, 0x60, 0x00, 0x02, 0x00, 0x40, 0x80, 0x78, 0x00, 0x00, 0x02, 0x00, 0x40, 0x80, 0x70,
  0x00, 0x40, 0x02, 0x00, 0x40, 0xf8, 0x72, 0x07, 0x0c, 0xb2, 0x4f, 0xc0, 0xc0, 0x72, 0x09, 0x0f,
  0xf2, 0x00, 0xc0, 0x80, 0x70, 0x10, 0x00, 0x02, 0x00, 0x40, 0x80, 0x70, 0x00, 0x00, 0x02, 0x00,
  0x40, 0x80, 0x78, 0x20, 0x00, 0x02, 0x00, 0x40, 0x80, 0x7c, 0x00, 0x00, 0x02, 0x40, 0xc0, 0xc4,
  0x7c, 0x00, 0x18, 0x02, 0x09, 0xc0, 0x80, 0x7c, 0x00, 0x18, 0x02, 0x01, 0x40, 0x80, 0x78, 0x00,
  0x18, 0x02, 0x00, 0x40, 0x80, 0x70, 0x00, 0x18, 0x06, 0x00, 0x40, 0x80, 0x70, 0x00, 0x18, 0x06,
  0x40, 0xc0, 0xfc, 0x70, 0x10, 0x18, 0x0e, 0x0f, 0xc0, 0x80, 0x60, 0x00, 0x3c, 0x3e, 0x01, 0x40,
  0x80, 0x40, 0x08, 0x7e, 0x7e, 0x00, 0x40, 0x80, 0x40, 0x1c, 0xff, 0xfe, 0x00, 0x40, 0xfc, 0x40,
  0x1f, 0xff, 0xfe, 0x47, 0xc0, 0xfc, 0x40, 0x1f, 0xff, 0xfe, 0x4f, 0xc0, 0xfc, 0x00, 0x1f, 0xff,
  0xfe, 0x0f, 0xc0, 0xfc, 0x00, 0x1f, 0xff, 0xfc, 0x0f, 0xc0, 0xfc, 0x80, 0x7f, 0xff, 0xf8, 0x0f,
  0xc0, 0xfc, 0x41, 0xff, 0xff, 0xf0, 0x1f, 0xc0, 0xfe, 0x23, 0xdf, 0xff, 0x70, 0x3f, 0xc0, 0xff,
  0x90, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0xff, 0x80, 0x00, 0x00, 0x00, 0xff, 0xc0, 0xff, 0xc0, 0x00,
  0x00, 0x01, 0xff, 0xc0, 0xff, 0xf9, 0x8c, 0x63, 0x03, 0xff, 0xc0, 0xff, 0xff, 0x58, 0x81, 0x3f,
  0xff, 0xc0, 0xff, 0xff, 0x50, 0x00, 0x3f, 0xff, 0xc0, 0xff, 0xff, 0x18, 0x43, 0x3f, 0xff, 0xc0,
  0xff, 0xff, 0x10, 0x84, 0x3f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'icono_iot', 20x15px
const unsigned char icono_iot[45] PROGMEM = {
  0x1f, 0x8f, 0xc0, 0x1f, 0x77, 0xc0, 0x1f, 0x8f, 0xc0, 0x1f, 0xff, 0xc0, 0x1f, 0x07, 0xc0, 0x16,
  0x7b, 0x40, 0x02, 0x8a, 0x00, 0x0a, 0xaa, 0x80, 0x02, 0x8a, 0x00, 0x16, 0xfb, 0x40, 0x1f, 0x07,
  0xc0, 0x1f, 0xff, 0xc0, 0x1f, 0x8f, 0xc0, 0x1f, 0x57, 0xc0, 0x1f, 0x8f, 0xc0
};
// 'icono_uaeh', 50x50px
const unsigned char icono_uaeh[350] PROGMEM = {
  0x07, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xff, 0xfb, 0xff, 0xf8, 0x00, 0x07, 0xff,
  0xff, 0xf3, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xff, 0xe7, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xff, 0xcb,
  0xff, 0xf8, 0x00, 0x07, 0x81, 0x9f, 0x9d, 0xf2, 0xf8, 0x00, 0x06, 0x7f, 0x39, 0x3e, 0x1e, 0x78,
  0x00, 0x07, 0x2c, 0x78, 0x7a, 0x0d, 0x38, 0x00, 0x07, 0x00, 0x78, 0xfa, 0x29, 0x18, 0x00, 0x06,
  0x00, 0x37, 0xca, 0x5f, 0xf8, 0x00, 0x07, 0x20, 0x27, 0x48, 0x39, 0x18, 0x00, 0x04, 0xf0, 0x3c,
  0x09, 0xef, 0x98, 0x00, 0x04, 0xf0, 0x2c, 0x03, 0xee, 0x98, 0x00, 0x04, 0xf1, 0x0c, 0x07, 0xfe,
  0x98, 0x00, 0x07, 0xf1, 0x10, 0x60, 0x7f, 0x78, 0x00, 0x07, 0xb1, 0x30, 0x20, 0x7f, 0x78, 0x00,
  0x07, 0xf0, 0x7f, 0xe0, 0x78, 0x78, 0x00, 0x07, 0xa0, 0xf0, 0x70, 0xfc, 0xf8, 0x00, 0x07, 0x81,
  0xf0, 0x70, 0xfe, 0xf8, 0x00, 0x07, 0x92, 0x0f, 0x86, 0x3e, 0x98, 0x00, 0x07, 0xb2, 0x08, 0x86,
  0x7e, 0x98, 0x00, 0x06, 0x60, 0x0c, 0x80, 0x7f, 0xf8, 0x00, 0x06, 0x40, 0x0c, 0x80, 0xff, 0xe8,
  0x00, 0x06, 0x43, 0xfc, 0xe7, 0xff, 0xc8, 0x00, 0x06, 0x64, 0xfc, 0xff, 0xff, 0x88, 0x00, 0x07,
  0xff, 0xfc, 0xe7, 0xff, 0x18, 0x00, 0x06, 0x5f, 0xfc, 0xe7, 0xe9, 0x98, 0x00, 0x07, 0x00, 0x64,
  0xe7, 0xe1, 0xb8, 0x00, 0x07, 0xa0, 0x24, 0xe7, 0xe1, 0xf8, 0x00, 0x07, 0xa7, 0xec, 0xff, 0xff,
  0xf8, 0x00, 0x07, 0x8f, 0xf4, 0xff, 0xff, 0x78, 0x00, 0x07, 0x87, 0x87, 0xff, 0xc1, 0x78, 0x00,
  0x07, 0x87, 0x87, 0xfd, 0xc1, 0x78, 0x00, 0x07, 0x87, 0x9f, 0xfd, 0xff, 0x78, 0x00, 0x07, 0xe7,
  0x88, 0xff, 0xff, 0x78, 0x00, 0x07, 0xbf, 0xc3, 0xff, 0xff, 0x78, 0x00, 0x07, 0x9f, 0xff, 0xff,
  0xff, 0x98, 0x00, 0x07, 0xc3, 0xff, 0xff, 0xff, 0xb8, 0x00, 0x07, 0xc3, 0xff, 0xff, 0xff, 0xf8,
  0x00, 0x07, 0xe3, 0xff, 0xcb, 0xf9, 0x98, 0x00, 0x07, 0xa3, 0xff, 0xfb, 0xff, 0x98, 0x00, 0x07,
  0xe7, 0xff, 0xff, 0xf3, 0x98, 0x00, 0x07, 0xa3, 0xff, 0xff, 0xe7, 0x98, 0x00, 0x07, 0xb3, 0xff,
  0xff, 0xef, 0x98, 0x00, 0x07, 0xbd, 0xff, 0xff, 0xdf, 0x18, 0x00, 0x07, 0x9e, 0x09, 0xe3, 0xc3,
  0x38, 0x00, 0x07, 0x80, 0xff, 0x4f, 0xf1, 0xf8, 0x00, 0x07, 0xc1, 0xff, 0x1f, 0xf1, 0xf8, 0x00,
  0x07, 0xff, 0xff, 0xbf, 0xff, 0xf8, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x00
};
// 'icono_wifi', 43x15px
const unsigned char icono_wifi[90] PROGMEM = {
  0x07, 0x80, 0x00, 0x00, 0x78, 0x00, 0x06, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x01, 0xff,
  0xc0, 0x00, 0x04, 0x00, 0x03, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x03, 0x8d, 0xf0, 0x00, 0x00, 0x2d,
  0x03, 0x8f, 0xf0, 0x00, 0x00, 0x2d, 0x43, 0xbd, 0xf0, 0x00, 0x00, 0x24, 0x43, 0x8d, 0xf0, 0x00,
  0x00, 0x36, 0x43, 0xbd, 0xf0, 0x00, 0x00, 0x36, 0x43, 0xbd, 0xf0, 0x00, 0x00, 0x00, 0x07, 0xbd,
  0xf0, 0x00, 0x04, 0x00, 0x0f, 0xff, 0xe0, 0x00, 0x04, 0x00, 0x0f, 0xff, 0xc0, 0x00, 0x06, 0x00,
  0x00, 0x00, 0x08, 0x00, 0x07, 0x80, 0x00, 0x00, 0x38, 0x00
};
// 'icono_ventilador', 40x40px
const unsigned char icono_ventilador[30] PROGMEM = {
  // 'icono_ventilador, 15x15px
  0xfc, 0x3e, 0xe0, 0x1e, 0xc8, 0x16, 0xdc, 0x12, 0x86, 0x18, 0x82, 0x38, 0x00, 0x20, 0x00, 0x00,
  0x00, 0x80, 0x0c, 0x00, 0xb8, 0x42, 0x90, 0x7e, 0xc0, 0x32, 0xe0, 0x06, 0xf8, 0x1e
};
const unsigned char icono_automatico[30] PROGMEM = {
  // 'icono_automatico, 15x15px
  0xf9, 0xfe, 0xf1, 0xfe, 0xf1, 0xfe, 0xe3, 0xfe, 0xc3, 0xfe, 0xc0, 0x3e, 0x80, 0x3e, 0x00, 0x3e,
  0x00, 0x66, 0x00, 0xe6, 0xf0, 0xc2, 0xe1, 0xda, 0xe3, 0xda, 0xe3, 0x80, 0xe7, 0xbc
};
const unsigned char icono_wifi_2[30] PROGMEM = {
  // 'wifi, 16x15px
  0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x3f, 0xfc, 0x78, 0x1e, 0x60, 0x06, 0x01, 0x80, 0x0f, 0xf0,
  0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};
void generarMensajeConfort(int temperatura, int humedad) {
  // Establecer la posición del cursor en la pantalla para mostrar el mensaje de confort

  display.setCursor(2, 50);

  // Determinar el mensaje de confort según la temperatura
  if (temperatura <= 0) {
    display.print("Congelante (!!!)");
  } else if (temperatura > 0 && temperatura <= 5) {
    display.print("Muy Frio (!)");
  } else if (temperatura > 5 && temperatura <= 12) {
    display.print("Frio");
  } else if (temperatura > 12 && temperatura <= 20) {
    display.print("Comodo");
  } else if (temperatura > 20 && temperatura <= 25) {
    display.print("Calido");
  } else if (temperatura > 25 && temperatura <= 30) {
    display.print("Caluroso");
  } else if (temperatura > 30 && temperatura < 40) {
    display.print("Muy caluroso (!)");
  } else if (temperatura >= 40) {
    display.print("Extremo caluroso (!!!)");
  }
  // Establecer la posición del cursor en la pantalla para mostrar el mensaje de confort de humedad

  display.setCursor(2, 40);

  // Determinar el mensaje de confort según la humedad
  if (humedad <= 30) {
    display.print("Muy seco (!)");
  } else if (humedad > 30 && humedad <= 35) {
    display.print("Seco");
  } else if (humedad > 35 && humedad <= 40) {
    display.print("Ligeramente seco");
  } else if (humedad > 40 && humedad <= 50) {
    display.print("Comodo");
  } else if (humedad > 50 && humedad <= 60) {
    display.print("Ligeramente humedo");
  } else if (humedad > 60 && humedad <= 65) {
    display.print("Humedo");
  } else if (humedad > 65) {
    display.print("Muy humedo (!)");
  }
}
void conectar_Wifi() {
  // Iniciar conexion WiFi
  Serial.println();
  Serial.println();
  Serial.print("Conectando a red WIFI: ");
  display.setCursor(30, 10);  // Posición del texto en la pantalla (0, 0)
  display.print("Conectando a red WIFI:");
  display.setCursor(10, 40);  // Posición del texto en la pantalla (0, 0)
  display.display();
  Serial.println(ssid);

  display.print(ssid);

  WiFi.begin(ssid, password);  // Esta es la función que realiz la conexión a WiFi
  // Esparar hsata lograr conexion
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_2, HIGH);  // Se hace parpadear el led mientras se logra la conexión
    display.setCursor(5, 50);   // Posición del texto en la pantalla (0, 0)
    display.print("......");

    Serial.print(".");  // Se da retroalimentación al puerto serial mientras se logra la conexión
    digitalWrite(LED_2, LOW);
    delay(400);
    digitalWrite(LED_2, HIGH);
    display.print("   ");

    display.display();
  }  // fin de while (WiFi.status() != WL_CONNECTED)
  display.clearDisplay();
  Serial.println();
  Serial.println("WiFi conectado");  // Una vez lograda la conexión, se reporta al puerto serial
  Serial.println("Direccion IP ");
  Serial.println(WiFi.localIP());
  // Dibuja icono de wifi
  display.drawBitmap(20, 0, icono_wifi, 43, 15, WHITE);
  display.setCursor(10, 30);  // Posición del texto en la pantalla (0, 0)
  display.print("-WiFi Conectado");
  display.setCursor(5, 40);  // Posición del texto en la pantalla (0, 0)
  display.print("IP:");
  display.setCursor(10, 50);  // Posición del texto en la pantalla (0, 0)
  display.print(WiFi.localIP());
  display.display();
}

//----------Configuración del programa, esta parte se ejecuta sólo una vez al energizarse el sistema
void setup() {
  // Inicialización del programa
  // INicia comunicación serial
  Serial.begin(115200);
  dht.begin();
  while (!Serial) {};
  // Configuración de pines
  pinMode(BUILTIN_LED, OUTPUT);  // Se configuran los pines de los LEDs indicadores como salida
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(RELE, OUTPUT);  // Inicializa el rele como output con resistencia Pull- Up interna
  pinMode(PIR, INPUT);    // Inicializa el rele como input con resistencia Pull- Up interna
  // Condiciones Iniciales
  digitalWrite(BUILTIN_LED, LOW);  // Se inicia con los LEDs indicadores apagados
  digitalWrite(LED_2, LOW);
  digitalWrite(RELE, LOW);  // Se inicia con el relé apagado

  // Inicializar la pantalla SSD1306
  Serial.print("Inicializando pantalla ");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Fallo en la asignación de SSD1306"));
    for (;;) {
      // Se repite indefinidamente si falla la inicialización de la pantalla
    }
  }
  delay(300);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.clearDisplay();
  delay(1000);
  ///////////////////Intro
  // Dibuja logo de ArmsysTech/
  display.drawBitmap(0, 21, icono_armsys, 50, 50, WHITE);
  display.setCursor(10, 0);  // Posición del texto en la pantalla (0, 0)
  display.print("Erick       Renato ");
  display.setCursor(10, 10);  // Posición del texto en la pantalla (0, 0)
  display.print("Vega       Ceron");
  display.setCursor(53, 20);  // Posición del texto en la pantalla (0, 0)
  display.print("IoT");
  display.setCursor(53, 30);  // Posición del texto en la pantalla (0, 0)
  display.print("UAEH");
  display.drawBitmap(80, 21, icono_uaeh, 50, 50, WHITE);  //dibuja logo de la UAEH
  display.display();
  delay(5000);  // Espera 5 segundos
  display.clearDisplay();
  display.display();
  ///////////////////////Conectar WIFI
  conectar_Wifi();

  // Conexion al broker MQTT
  client.setServer(mqtt_servidor_publico, 1883);  // Se hace la conexión al servidor MQTT Público
  //client.setServer(mqtt_servidor_local, 1883);    // Se hace la conexión al servidor MQTT Local
  client.setCallback(callback);  //Se activa la función que permite recibir mensajes de respuesta
  display.display();
}  // Fin del void setup()

void refrescarDatosAmbientales() {
  // Leer temperatura y humedad
  temperaturaAnterior = temperaturaActual;
  humedadAnterior = humedadActual;
  temperaturaActual = dht.readTemperature();
  humedadActual = dht.readHumidity();

  if (presenciaHumanaCercana) {
    display.setCursor(2, 20);
    display.print("Temperatura: ");
    display.setCursor(80, 20);
    display.print(temperaturaActual);
    display.print(" ");
    display.cp437(true);
    display.write(167);
    display.print("C");
    // Mostrar humedad en la pantalla OLED
    display.setCursor(2, 30);
    display.print("Humedad: ");
    display.setCursor(80, 30);
    display.print(humedadActual);
    display.print(" %");

    // Generar el mensaje de confort y mostrarlo en la pantalla
    if (!isnan(humedadActual) && !isnan(temperaturaActual)) {
      generarMensajeConfort(round(temperaturaActual), round(humedadActual));

    } else {
      display.setCursor(2, 50);
      display.print("Sin datos");
    }
  }
}
void publicarDatosMQTT() {
  char payload[128];
  size_t n = 0;
  ultimoMsj = now;
  datosSensores["temperatura"] = temperaturaActual;
  datosSensores["humedad"] = humedadActual;
  datosSensores["presenciaHumana"] = presenciaHumanaCercana;
  datosSensores["ventilador"] = ventilador;

  serializeJson(datosSensores, payload);
  client.publish(topic_Sensores, payload);
  Serial.println(payload);
}
void ventilarArea(bool ventilar) {
  if (ventilar) {
    digitalWrite(LED_3, HIGH);
    digitalWrite(RELE, HIGH);
    ventilador = true;
    Serial.println("Ventilador Activado");
    delay(2500);
  } else {
    digitalWrite(LED_3, LOW);
    digitalWrite(RELE, LOW);
    ventilador = false;
    Serial.println("Ventilador Desactivado");
    delay(2500);
  }
}


////////Loop principal
void loop() {
  display.clearDisplay();
  presenciaHumanaCercana = digitalRead(PIR);  // Se revisa si hay presencia de un ser vivo cerca

  if (presenciaHumanaCercana && ciclos_Encendidos_Pantalla < 20) {
    display.ssd1306_command(SSD1306_DISPLAYON);  // Encender pantalla
    ciclos_Encendidos_Pantalla++;
  } else {
    display.ssd1306_command(SSD1306_DISPLAYOFF);  // Apagar pantalla si no hay quien la vea
    ciclos_Encendidos_Pantalla = 0;
  }
  // Comprobar conexion con broker MQTT
  if (!client.connected()) {
    reconnect();
  }
  refrescarDatosAmbientales();  //refresca los datos del sensor DHT
  if (modoAutomatico) {
    if (temperaturaActual > 22) {  //Si está activado el modo automático, revisar criterio de revisión
      ventilarArea(true);
    } else {
      ventilarArea(false);
    }
  }


  client.loop();  // Loop de cliente MQTT

  now = millis();
  if (now - ultimoMsj > 3000) {  //si el último mensaje se envió hace más de 3 segundos
    publicarDatosMQTT();
  }
  refrescarBarraEstado();
  display.display();
  delay(3500);
}  // Fin del void loop()



//----------Funciones de usuario
// Funcion Callback, aqui se pueden poner acciones si se reciben mensajes MQTT
void callback(char* topic1, byte* message, unsigned int length) {
  display.clearDisplay();
  Serial.println("Mensaje recibido: ");

  DynamicJsonDocument mensaje(128);
  deserializeJson(mensaje, message);
  bool ventilador = mensaje["ventilador"];
  bool modoAutomatico1 = mensaje["modoAutomatico"];

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }

  if (String(topic1) == topic_Actuadores) {  // Si se recibe el mensaje true, se enciende el led y el rele
    if (ventilador) {
      Serial.println("ActivarVentilador");
      ventilarArea(true);
      if (modoAutomatico) {
        modoAutomatico = false;
        Serial.println("Ventilador automático desactivado");
      }
    } else {
      Serial.println("DesactivarVentilador");
      ventilarArea(false);
      if (modoAutomatico) {
        modoAutomatico = false;
        Serial.println("Ventilador automático desactivado");
      }
    }
    if (modoAutomatico1) {
      if (!modoAutomatico) {
        modoAutomatico = true;
        Serial.println("Modo automático activado");
      }
    } else {
      if (modoAutomatico) {
        modoAutomatico = false;
        Serial.println("Modo automático desactivado");
      }
    }
  }  // fin de if (String(topic) == "iot/output")
  delay(1000);
}
// funcion para reconectar el cliente MQTT
void reconnect() {
  // Bucle hasta que se logre la conexión
  display.clearDisplay();
  while (!client.connected()) {

    display.setCursor(0, 0);  // Posición del texto en la pantalla (0, 0)
    display.print("Conectando MQTT");
    Serial.print("Intentando conexion MQTT");
    if (client.connect("IoTClient_Evega")) {
      display.setCursor(5, 25);
      Serial.print("Conexion exitosa");
      display.print("Conexion exitosa");
      display.drawBitmap(0, 15, icono_iot, 43, 20, WHITE);
      display.drawBitmap(40, 15, icono_iot, 20, 15, WHITE);
      // Suscribirse al tema de respuestas
      client.subscribe(topic_Actuadores);
      digitalWrite(LED_3, HIGH);
    } else {
      display.clearDisplay();
      digitalWrite(LED_3, LOW);
      display.setCursor(0, 50);
      display.print("Falló la comunicación, error rc=");
      display.setCursor(0, 60);
      display.print(client.state());
      Serial.print("Falló la comunicación, error rc=");
      Serial.print(client.state());
      display.setCursor(0, 70);
      display.print("Reintentando en 5 segundos");
      Serial.println("Reintentando en 5 segundos");
      display.display();
      delay(5000);
      Serial.println(client.connected());
      display.setCursor(0, 8);
      display.print(client.connected());
    }
  }
  display.display();
}

void refrescarBarraEstado() {
  if (WiFi.status() == WL_CONNECTED) {
    display.drawBitmap(0, 0, icono_wifi_2, 16, 15, WHITE);  //mostrar icono de ventilador en pantalla
    digitalWrite(LED_2, HIGH);                              // Una vez lograda la conexión se enciende el led sobre el ESP32
  }
  if (client.connected()) {
    display.setCursor(27, 0);  // Posición del texto en la pantalla (0, 0)
    display.print("MQTT");
    display.setCursor(20, 0);  // Posición del texto en la pantalla (0, 0)
    display.write(4);
    digitalWrite(BUILTIN_LED, HIGH);  // Se activa el led en el nodeMCU para indicar que hay conexión MQTT
  }
  if (ventilador) {
    display.drawBitmap(90, 0, icono_ventilador, 15, 15, WHITE);  //mostrar icono de ventilador en pantalla
  }
  if (modoAutomatico) {
    display.drawBitmap(106, 0, icono_automatico, 15, 15, WHITE);  //mostrar icono de ventilador en pantalla
  }
}
