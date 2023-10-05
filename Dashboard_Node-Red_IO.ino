/*
El sketch utiliza cadenas JSON para enviar  los valores analógicos leídos de 2 potenciómetros y 2 motones, y recibe el estado de 6 leds 
Autor: Erick Renato Vega Ceron
*/
#include <ArduinoJson.h>

#define espera 3000

const uint8_t potenciometro1 = A0;
const uint8_t potenciometro2 = A1;

const int btnPIN1 = 6;
const int btnPIN2 = 7;

String logSerial = "";
int pin[] = { 8, 9, 10, 11, 12, 13 };
int Estado = 0;
int PinElegido = 0;
int tamanoArray = 0;
int comandosTotales = 0;
int estado = 0;
int led = 0;
int pot1 = 0;
int pot2 = 0;
int pot1Anterior = 0;
int pot2Anterior = 0;
bool boton1 = false;
bool boton2 = false;
bool boton1Anterior = false;
bool boton2Anterior = false;
bool cambios = false;
String jsonComandos = "";
String comandoLed = "";
String mensajeSerial = "";

void setup() {
  Serial.begin(9600);
  tamanoArray = sizeof(pin) / sizeof(pin[0]);
  //Definir GPIO LED
  for (int i = 0; i < tamanoArray; i++) {
    pinMode(pin[i], OUTPUT);
  }
  //Definir GPIO de botones
  pinMode(btnPIN1, INPUT_PULLUP);
  pinMode(btnPIN2, INPUT_PULLUP);
  pinMode(potenciometro1, INPUT);
  pinMode(potenciometro2, INPUT);
}

void loop() {

  if (Serial.available() > 0) {
    mensajeSerial = Serial.readString();
    if (mensajeSerial.length() <= 2) {
      comandoLed = mensajeSerial;
    } else {
      comandoLed = deserealizarJSON(mensajeSerial);  //se obtiene la cadena de LED a mostrar
    }
    procesarComandoLED(comandoLed);  //se procesa la cadena obtenida  para mostrar los LED
  }
  if (leerValoresAnalogicos()) {
    serializarJSON();
  }
}

bool leerValoresAnalogicos() {
  pot1Anterior = pot1;
  pot2Anterior = pot2;
  boton1Anterior = boton1;
  boton2Anterior = boton2;
  pot1 = map(analogRead(potenciometro1), 0, 1023, 0.05, 1000);
  pot2 = map(analogRead(potenciometro2), 0, 1023, 0.05, 1000);
  boton1 = !digitalRead(btnPIN1);
  boton2 = !digitalRead(btnPIN2);
  if (pot1 != pot1Anterior || pot2 != pot2Anterior || boton1 != boton1Anterior || boton2 != boton2Anterior)
    cambios = true;
  else
    cambios = false;

  return cambios;
}

void procesarComandoLED(String texto) {
  int caracteresXComando = 3;
  int contadorCaracteres = 0;

  for (int i = 0; i < texto.length(); i++) {  ///Se recorre cada caracter del mensaje
    contadorCaracteres++;

    if (isDigit(texto.charAt(i))) {  //si es un digito
      if (contadorCaracteres == 1)   //revisar el primer digito
        PinElegido = String(texto.charAt(i)).toInt();
      if (contadorCaracteres == 2) {
        if (String(texto.charAt(i)).toInt() == 1 || (String(texto.charAt(i - 1)).toInt() == 7 && String(texto.charAt(i + 1)).toInt() == 1)) {
          Estado = 3;
        } else if (String(texto.charAt(i)).toInt() == 0 || (String(texto.charAt(i - 1)).toInt() == 7 && String(texto.charAt(i + 1)).toInt() == 0)) {
          Estado = 2;
        }
      }
      if (contadorCaracteres == 3 && Estado != 3 && Estado != 2) {
        Estado = String(texto.charAt(i)).toInt();
      }
    }
    if (contadorCaracteres == caracteresXComando) {
      contadorCaracteres = -1;
      ejecutarComandoLED();
    }
  }
}
void ejecutarComandoLED() {
  if (Estado == 0) {
    digitalWrite(pin[PinElegido - 1], LOW);
  }
  if (Estado == 1) {
    digitalWrite(pin[PinElegido - 1], HIGH);
  }
  if (Estado == 2) {
    for (int i = 0; i < tamanoArray; i++) {
      digitalWrite(pin[i], LOW);
    }
  }
  if (Estado == 3) {
    for (int i = 0; i < tamanoArray; i++) {
      digitalWrite(pin[i], HIGH);
    }
  }
  Estado = 0;
}
String deserealizarJSON(String json) {
  String comandoLed = "";
  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, json);

  if (error) { return; }  //si hay error se finaliza programa

  comandoLed = doc["comandoled"].as<String>();

  return comandoLed;
}

void serializarJSON() {
  String json;
  StaticJsonDocument<300> doc;
  doc["pot1"] = pot1;
  doc["pot2"] = pot2;
  doc["btn1"] = boton1;
  doc["btn2"] = boton2;
  serializeJson(doc, json);
  Serial.println(json);
}
