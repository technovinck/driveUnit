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
#define UP 1
#define DOWN 2
#define LEFT 3 //9
#define RIGHT 4 //10
#define UP_LEFT 5
#define UP_RIGHT 6
#define DOWN_LEFT 7
#define DOWN_RIGHT 8
#define TURN_LEFT 9 //3
#define TURN_RIGHT 10 //4
#define STOP 0

#define TESTMODE_FL 25
#define TESTMODE_FR 26
#define TESTMODE_BL 27
#define TESTMODE_BR 28

#define FRONT_RIGHT_MOTOR 0
#define BACK_RIGHT_MOTOR 1
#define FRONT_LEFT_MOTOR 2
#define BACK_LEFT_MOTOR 3

#define FORWARD 1
#define BACKWARD -1

//enum Direction(UP=1, DOWN, LEFT, RIGHT, UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT, TURN_LEFT, TURN_RIGHT, STOP=0);
//int test = Direction[UP];


AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

WiFiClient espClient;
PubSubClient client(espClient);


// Globale Variable, um die aktuelle Geschwindigkeit zu speichern
int currentSpeed = 128;  // Standardgeschwindigkeit
bool individualControl = false; // Standardmäßig keine individuelle Steuerung

struct MOTOR_PINS
{
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS> motorPins =
    {
        {25, 33}, //BACK_RIGHT_MOTOR
        {17, 16}, //FRONT_RIGHT_MOTOR
        {27, 26}, //FRONT_LEFT_MOTOR
        {19, 18}, //BACK_LEFT_MOTOR
};
/*
std::vector<MOTOR_PINS> motorPins =
    {
        {18, 19}, //BACK_RIGHT_MOTOR
        {16, 17}, //FRONT_RIGHT_MOTOR
        {27, 26}, //FRONT_LEFT_MOTOR
        {25, 33}, //BACK_LEFT_MOTOR
};
*/

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
void executeMovement(int movement) {
  int speed = currentSpeed;  // default Geschwindigkeit setzen

  switch(movement) {
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
}

void processCarMovement(String inputValue)
{
  Serial.printf("Got value as %s %d\n", inputValue.c_str(), inputValue.toInt());

  if (inputValue.startsWith("SPEED:"))
  {
    // Extrahieren Sie die Geschwindigkeit und speichern Sie sie
    currentSpeed = inputValue.substring(6).toInt();
  }
  else
  {
    // Weiterhin die anderen Steuerbefehle behandeln
    switch (inputValue.toInt())
    {
      case UP:
      case DOWN:
      case LEFT:
      case RIGHT:
      case UP_LEFT:
      case UP_RIGHT:
      case DOWN_LEFT:
      case DOWN_RIGHT:
      case TURN_LEFT:
      case TURN_RIGHT:
      case TESTMODE_FL:
      case TESTMODE_FR:
      case TESTMODE_BL:
      case TESTMODE_BR:
      case STOP:
        // Wenn der Befehl einem bekannten Steuerbefehl entspricht, führe die Bewegung aus
        executeMovement(inputValue.toInt());
        break;
  
      default:
        // Behandeln Sie unbekannte Befehle hier
        break;
    }
  }
}

//Webserver Funktionen
void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found");
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,void *arg, uint8_t *data, size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      //client->text(getRelayPinsStatusJson(ALL_RELAY_PINS_INDEX));
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      processCarMovement("0");
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        processCarMovement(myData.c_str());       
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

// Funktion zum Empfangen von MQTT-Nachrichten
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Hier können Sie die Payload weiter verarbeiten
}

void reconnect() {
  // Wiederherstellen der MQTT-Verbindung
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("connected");
      // Hier kannst du die Nachricht senden
      client.publish("esp32/status", "Connected");
      // Abonnieren von Nachrichten
      client.subscribe("esp32/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Booting...");

  //WIFI part
  // Überprüfen, ob das WLAN-Netzwerk in der Nähe ist
  int networkCount = WiFi.scanNetworks();
  bool homeNetworkFound = false;

  Serial.println("Suche nach Heimnetzwerk");
  for (int i = 0; i < networkCount; ++i) {
      //Serial.print(".");
      if (WiFi.SSID(i) == ssid) {
          homeNetworkFound = true;
          Serial.print(WiFi.SSID(i));
          Serial.println(" gefunden!");
          break;
      } else {
        Serial.println(WiFi.SSID(i));
      }
  }
  
  if (homeNetworkFound) {
    // Verbindung zum Heimnetzwerk herstellen
    Serial.print("Connecting to SSID: ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);

    int attempts = 0;
    Serial.print("Verbindung zum WLAN herstellen...");
    while (WiFi.status() != WL_CONNECTED && attempts < 50) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Verbunden mit dem WLAN");
        //Hostnamen nach dem Verbindungsaufbau setzen
        WiFi.setHostname(hostname);
        Serial.print("Hostname: ");
        Serial.println(hostname);
        Serial.print("Signalstärke: ");
        Serial.println(WiFi.RSSI());
        delay(1000); // Wartezeit nach der WLAN-Verbindung
    } else {
      Serial.println("Verbindung konnte nicht hergestellt werden.");
    }
  } else {
      // Wenn das WLAN-Netzwerk nicht in der Nähe ist, Hotspot erstellen
      Serial.println("Suche nach Heimnetzwerk fehlgeschlagen, erstelle Hotspot.");
      WiFi.softAP(hotspotSSID, hotspotPassword);
      Serial.println("Hotspot erstellt");
      Serial.println(hostname);
      Serial.print(" im Hotspot-Modus!");

    //initialisiere Webserver
    server.on("/", HTTP_GET, handleRoot);
    server.onNotFound(handleNotFound);

    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    server.begin();
    Serial.println("HTTP server started");

  }


  if (WiFi.status() == WL_CONNECTED){

    // Initialisiere ArduinoOTA
    ArduinoOTA.onStart([]() {
        Serial.println("Starte OTA-Aktualisierung");
    });

    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA-Aktualisierung abgeschlossen");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Fortschritt: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA-Fehler [%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Authentifizierung fehlgeschlagen");
        else if (error == OTA_BEGIN_ERROR) Serial.println("OTA-Begin fehlgeschlagen");
        else if (error == OTA_CONNECT_ERROR) Serial.println("OTA-Verbindung fehlgeschlagen");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA-Empfangsfehler");
        else if (error == OTA_END_ERROR) Serial.println("OTA-Endfehler");
    });

    ArduinoOTA.begin();
  } else {
    Serial.println("Nix zu tun, da keine WIFI-Verbindung hergestellt wurde");
  }

client.setServer(mqttServer, mqttPort);
client.setCallback(callback);


  setUpPinModes();
} 

void loop() 
{
  ArduinoOTA.handle();
  ws.cleanupClients();

  // Überprüfen, ob eine Verbindung zum MQTT-Broker hergestellt ist
  if (!client.connected()) {
    reconnect();
  }
  // MQTT-Nachrichten verarbeiten
  client.loop();
}