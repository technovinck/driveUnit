#include <Arduino.h>
#include "webpage.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h> // MQTT-Bibliothek
#include <config.h> //enthält Zugangsdaten für das Heimnetz

//definieren der Richtungen um den Code lesbarer zu machen (UP wird dann durch 1 ersetzt)
//      _______
// FL1-|   O>  |-FR3
//     |   _   |
//     |O |_| O|
//     |       |
// BL0-|_______|-BR2
//    
//********************
#define FRONT_RIGHT_MOTOR 0
#define BACK_RIGHT_MOTOR 1
#define FRONT_LEFT_MOTOR 2
#define BACK_LEFT_MOTOR 3

// Definiere die Richtungen und Aktionen als numerische Werte
#define STOP 0
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define UP_LEFT 5
#define UP_RIGHT 6
#define DOWN_LEFT 7
#define DOWN_RIGHT 8
#define TURN_LEFT 9
#define TURN_RIGHT 10
#define TESTMODE_FL 25
#define TESTMODE_FR 26
#define TESTMODE_BL 27
#define TESTMODE_BR 28

#define FORWARD 1
#define BACKWARD -1

// Globale Variable, um die aktuelle Geschwindigkeit zu speichern
uint8_t currentSpeed = 100;  // Standardgeschwindigkeit
bool individualControl = false; // Standardmäßig keine individuelle Steuerung

struct MOTOR_PINS
{
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS> motorPins =
    {
        {25, 33}, //FRONT_LEFT_MOTOR FL <
        {17, 16}, //FRONT_RIGHT_MOTOR BL
        {27, 26}, //BACK_RIGHT_MOTOR
        {19, 18}, //BACK_LEFT_MOTOR
};

struct TopicMapping {
  const char* name;
  u_int8_t value;
};

const TopicMapping topicMappings[] = {
  {"up", 1},
  {"down", 2},
  {"left", 3},
  {"right", 4},
  {"up_left", 5},
  {"up_right", 6},
  {"down_left", 7},
  {"down_right", 8},
  {"turn_left", 9},
  {"turn_right", 10},
  {"stop", 0},
  {"testmode_fl", 25},
  {"testmode_fr", 26},
  {"testmode_bl", 27},
  {"testmode_br", 28}
};

const int numTopicMappings = sizeof(topicMappings) / sizeof(topicMappings[0]);

int getTopicValue(const char* topicName) {
  for (int i = 0; i < numTopicMappings; ++i) {
    if (strcmp(topicMappings[i].name, topicName) == 0) {
      return topicMappings[i].value;
    }
  }
  // Wenn das Thema nicht gefunden wurde, gib -1 zurück oder handle den Fehler entsprechend
  return -1;
}

WiFiClient espClient;
PubSubClient client(espClient);

// Muesam Funktionen
void setUpPinModes()
{
  for (int i = 0; i < motorPins.size(); i++)
  {
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);
  }
}

void setMotorSpeeds(int speedFrontRight, int speedBackRight, int speedFrontLeft, int speedBackLeft)
{
  analogWrite(motorPins[FRONT_RIGHT_MOTOR].pinIN1, speedFrontRight);
  analogWrite(motorPins[BACK_RIGHT_MOTOR].pinIN1, speedBackRight);
  analogWrite(motorPins[FRONT_LEFT_MOTOR].pinIN1, speedFrontLeft);
  analogWrite(motorPins[BACK_LEFT_MOTOR].pinIN1, speedBackLeft);
}

/// @brief Funktion um einen Motor mit einer bestimmten Geschwindigkeit vorwärts, 
///        rückwärts zu drehen oder zu stoppen, der index bestimmt den Motor
/// @param motorIndex //vorne Links = 1,vorne rechts = 3,hinten links = 0,hinten rechts = 2
/// @param direction  //(FORWARD, BACKWARD, STOP)
/// @param speed      //z.B. 128, wenn Wert zu gering kein Verfahren mehr möglich 
void rotateMotor(int motorIndex, int direction, int speed) {

  int in1 = motorPins[motorIndex].pinIN1;
  int in2 = motorPins[motorIndex].pinIN2;

  switch (direction) {
    case FORWARD:
      analogWrite(in1, speed);
      analogWrite(in2, 0);  // PWM für die Gegenrichtung auf 0 setzen
      break;
    case BACKWARD:
      analogWrite(in1, 0);   // PWM für die Vorwärtsrichtung auf 0 setzen
      analogWrite(in2, speed);
      break;
    case STOP:
      analogWrite(in1, 0);  // PWM auf 0 setzen, um den Motor zu stoppen
      analogWrite(in2, 0);  // PWM auf 0 setzen, um den Motor zu stoppen
      break;
    default:
      // Fügen Sie hier eine Fehlerbehandlung für ungültige Richtungen hinzu, wenn gewünscht
      break;
  }
}

void rotateMotors(int frontLeft, int frontRight, int backLeft, int backRight, int motorSpeed) {
  Serial.print(frontLeft);
  Serial.print(frontRight);
  Serial.print(backLeft);
  Serial.println(backRight);
  rotateMotor(FRONT_RIGHT_MOTOR, frontRight, motorSpeed);
  rotateMotor(BACK_RIGHT_MOTOR, backRight, motorSpeed);
  rotateMotor(FRONT_LEFT_MOTOR, frontLeft, motorSpeed);
  rotateMotor(BACK_LEFT_MOTOR, backLeft, motorSpeed);
}

/**
 * @brief Diese Funktion bestimmt die Drehung der Räder je nach Bewegungsrichtung.
 *
 * //vorne Links = 1,vorne rechts = 3,hinten links = 0,hinten rechts = 2
 *
 * @param movement
 */
void executeMovement(u_int8_t direction, u_int16_t travelTime ) {
  int speed = currentSpeed;  // default Geschwindigkeit setzen

  // Wenn STOP empfangen wird, stoppe sofort die Motoren und kehre zurück
  //if (direction == STOP) {
  //  rotateMotors(STOP, STOP, STOP, STOP, speed);
  //  return;
  //}

  switch(direction) {
    case UP:
      rotateMotors(FORWARD, FORWARD, FORWARD, FORWARD, speed);
      break;
    case DOWN:
      rotateMotors(BACKWARD, BACKWARD, BACKWARD, BACKWARD, speed);
      break;
    case LEFT:
      rotateMotors(BACKWARD, FORWARD, FORWARD, BACKWARD, speed);
      break;
    case RIGHT:
      rotateMotors(FORWARD, BACKWARD, BACKWARD, FORWARD, speed);
      break;
    case UP_LEFT:
      rotateMotors(STOP, FORWARD, FORWARD, STOP, speed);
      break;
    case UP_RIGHT:
      rotateMotors(FORWARD, STOP, STOP, FORWARD, speed);
      break;
    case DOWN_LEFT:
      rotateMotors(BACKWARD, STOP, STOP, BACKWARD, speed);
      break;
    case DOWN_RIGHT:
      rotateMotors(STOP, BACKWARD, BACKWARD, STOP, speed);
      break;
    case TURN_LEFT:
      rotateMotors(BACKWARD, FORWARD, BACKWARD, FORWARD, speed);
      break;
    case TURN_RIGHT:
      rotateMotors(FORWARD, BACKWARD, FORWARD, BACKWARD, speed);
      break;
    case TESTMODE_FL:
      rotateMotors(STOP, FORWARD, STOP, STOP, speed);
      break;
    case TESTMODE_FR:
      rotateMotors(STOP, STOP, STOP, FORWARD, speed);
      break;
    case TESTMODE_BL:
      rotateMotors(FORWARD, STOP, STOP, STOP, speed);
      break;
    case TESTMODE_BR:
      rotateMotors(STOP, STOP, FORWARD, STOP, speed);
      break;
    case STOP:
      rotateMotors(STOP, STOP, STOP, STOP, speed);
      break;
  }
  // Fahrzeit
  //delay(travelTime);

  // Stoppe die Motoren wenn Zeit abgelaufen
  //rotateMotors(STOP, STOP, STOP, STOP, speed);

}

/// @brief Behandlung der MQTT Nachrichten-Themen
/// @param topic 
/// @param payload 
/// @param length 
void callback(char* topic, byte* payload, unsigned int length) {
  // Hier können weitere Verarbeitungen der MQTT-Nachrichten hinzugefügt werden
  // (z. B. Logging, Datenverarbeitung, usw.)
  Serial.print("Topic: ");
  Serial.println(topic);
  // Führe die entsprechende Aktion basierend auf dem MQTT-Thema aus
  if (strcmp(String(topic).substring(10,14).c_str(), "move") == 0) {
    // Extrahiere die Richtung aus dem Thema
    String directionString = String(topic).substring(15);
    const char* direction = directionString.c_str(); // Richtung als const char*

    Serial.print("Extracted Direction: ");
    Serial.println(direction);
    u_int8_t dirNum= getTopicValue(direction);
    Serial.print("Direction = ");
    Serial.print(direction);
    Serial.print(" : ");
    Serial.println(dirNum);

    // Extrahiere die Länge der Fahrzeit aus der Payload
    u_int16_t travelTime = atoi((char *)payload);
    
    // Führe die Bewegung aus
    executeMovement(dirNum, travelTime);
  }
  if (strcmp(String(topic).substring(10,15).c_str(), "speed") == 0) {
  // Extrahiere die Geschwindigkeit aus dem Payload
  currentSpeed = atoi((char *)payload);
  client.publish("driveUnit/status", "Set speed to: ");
  Serial.println(currentSpeed); 
  }
}


void reconnect() {
  // Wiederherstellen der MQTT-Verbindung
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
      // Hier kannst du die Nachricht senden
      client.publish("driveUnit/status", "Connected");
      // Abonnieren von Nachrichten
      client.subscribe("driveUnit/move/#");
      client.subscribe("driveUnit/speed/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Verzögerung, bevor ein erneuter Verbindungsversuch unternommen wird
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  ArduinoOTA.begin();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  setUpPinModes();
}

void loop() {
  ArduinoOTA.handle();

  // Überprüfen, ob eine Verbindung zum MQTT-Broker hergestellt ist
  if (!client.connected()) {
    reconnect(); // Wenn keine Verbindung besteht, versuche erneut eine Verbindung herzustellen
  } else {
    // MQTT-Nachrichten verarbeiten, wenn eine Verbindung besteht
    client.loop();
  }
}