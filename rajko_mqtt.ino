// Include required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>


// Define the PWM strenght
#define M1_PWM_FACTOR 1
#define M2_PWM_FACTOR 1
// Define the pin for PWM output
#define M1_levo 2   // D0
#define M1_desno 3  //D1
#define M2_levo 4   // D2
#define M2_desno 5  //D3
// Define the pin for sensor
#define SENS_SICK 10  //D10
#define SENS_CINC 9   //D9

const char* ssid = "S10";
const char* password = "kolemehos";
const char* mqtt_server = "192.168.137.125";

char step = 0;  // 1 - accelorate, 2 - moving, 3 - stopping

char dutyCycle = 0;

char onlyOnce = 0;

long int t1, t2;

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //ovde dodati kod za obradu poruke i aktivaciju sime
  if (onlyOnce == 0 && *payload == 'T') {
    step = 1;
    onlyOnce = 1;
    t1 = millis();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/test");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Enable WiFi
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // Set the PWM pin as an output
  pinMode(M1_levo, OUTPUT);
  pinMode(M1_desno, OUTPUT);
  pinMode(M2_levo, OUTPUT);
  pinMode(M2_desno, OUTPUT);
  //Set the sensor pin as an input
  pinMode(SENS_SICK, INPUT);
  //  pinMode(SENS_CINC, INPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if(step != 0){
    t2 = millis();
  }

  // merenja vremena
  if((t2 - t1) > 10000){
    step = 0;
  }

  //iskljucivanje motora
  if (step == 0) {
    analogWrite(M1_desno, 0);
    analogWrite(M2_desno, 0);
    analogWrite(M1_levo, 0);
    analogWrite(M2_levo, 0);
    // delay(1500);
    // step = 1;
  }

  //detekcija sa sick senzora
  if (digitalRead(SENS_SICK) == HIGH) {
    step = 3;
  } else if (step == 3) step = 1;

  //ubrzavanje -- za menjanje nagiba rampe menjati delay
  if (step == 1) {
    if (dutyCycle < 255) {
      dutyCycle++;
    }
    analogWrite(M1_levo, dutyCycle * M1_PWM_FACTOR);  // Set PWM duty cycle
    analogWrite(M2_levo, dutyCycle * M2_PWM_FACTOR);  // Set PWM duty cycle
    delay(6);                                         // Wait for a short duration
  }

  //kocenje -- za menjanje nagiba rampe menjati delay
  if (step == 3) {
    analogWrite(M1_levo, dutyCycle * M1_PWM_FACTOR);  // Set PWM duty cycle
    analogWrite(M2_levo, dutyCycle * M2_PWM_FACTOR);  // Set PWM duty cycle
    delay(2);
    if (dutyCycle > 0) {
      dutyCycle--;
    }
  }
}
