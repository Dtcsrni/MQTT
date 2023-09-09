/*
 Basic ESP32 MQTT example
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#define BUILTIN_LED 33

#define SCREEN_WIDTH 128 // Ancho de la pantalla OLED, en píxeles
#define SCREEN_HEIGHT 64 // Alto de la pantalla OLED, en píxeles

// Declaración para una pantalla SSD1306 conectada a través de I2C (pines SDA, SCL)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 2     // Pin digital conectado al sensor DHT

// Descomenta el tipo de sensor que estás utilizando:
#define DHTTYPE    DHT11     // DHT 11 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)
// Update these with values suitable for your network.

const char* ssid = "ArmsysTech";
const char* password = "sjmahpe122512";
const char* mqtt_server = "mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
DHT dht(DHTPIN, DHTTYPE);

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void generarMensajeConfort(int temperatura, int humedad) {
  // Establecer la posición del cursor en la pantalla para mostrar el mensaje de confort

  display.setCursor(2, 17);

  // Determinar el mensaje de confort según la temperatura
  if (  temperatura <= 0) {
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

  display.setCursor(2, 28);

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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Esp32Erick-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "Hola Esp32 Erick");
      // ... and resubscribe
      client.subscribe("inTopic/esp/Erick");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

    Serial.begin(115200);

  dht.begin();

  // Inicializar la pantalla SSD1306
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Fallo en la asignación de SSD1306"));
    for (;;) {
      // Se repite indefinidamente si falla la inicialización de la pantalla
    }
  }
  delay(200); 
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(40, 30); // Posición del texto en la pantalla (0, 0)
  display.print("Erick Vega");
   display.display(); 
    delay(2000);  // Espera 2 segundos entre lecturas
  display.clearDisplay();
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
   // Retardo de 5 segundos

  // Leer temperatura y humedad
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Verificar si se pudieron leer los valores correctamente
 

  // Limpiar la pantalla
  display.clearDisplay();

  // Mostrar temperatura en la pantalla OLED
 
  display.setTextSize(1);
  display.setCursor(2, 0);
  display.print("Temperatura: ");
  display.setTextSize(1);
  display.setCursor(80, 0);
  display.print(t);
  display.print(" ");
  display.cp437(true);
  display.write(167);
  display.print("C");
  // Mostrar humedad en la pantalla OLED
  display.setCursor(2, 8);
  display.print("Humedad: ");
  display.setCursor(80, 8);
  display.print(h);
  display.print(" %");

  // Generar el mensaje de confort y mostrarlo en la pantalla
if (!isnan(h) && !isnan(t)) {
  generarMensajeConfort(round(t), round(h));
    
  } else{

     display.setCursor(2, 25);
     display.print("Sin datos");

  }
  

  // Mostrar en la pantalla OLED
  display.display();

  long now = millis();
  if (now - lastMsg > 3000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "Temperatura: %lf C", t);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic/esp/Erick", msg);

    snprintf (msg, 50, "Humedad: %lf  \%", h);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("outTopic/esp/Erick", msg);
  }
}
