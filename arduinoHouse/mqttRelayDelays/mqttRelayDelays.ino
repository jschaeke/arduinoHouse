
/*
   MQTT 32 output pins for driving 2 16-channel relay boards on an arduino mega with ethernet shield.

   NOTE: the topic is hardcoded: domogik/in/relay/r + relay number : e.g

   mosquitto_pub -t domogik/in/relay/r1 -m 1  -> turn on
   mosquitto_pub -t domogik/in/relay/r1 -m 0  -> turn off
   mosquitto_pub -t domogik/in/relay/r1 -m 2  -> toggle; so turn on again

  By Jeroen Schaeken
  Derived from http://www.esp8266.com/viewtopic.php?f=29&t=8746

  It connects to an MQTT server then:
  - on 0 switches off relay
  - on 1 switches on relay
  - on 2 switches the state of the relay

  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.

  There is a 400ms between 2 mutuallyExcluded relays

  Pins 2->7 are used as input (switch) detection. Their state are published on domogik/relayArduinoPin1

  A MQ2 smoke sensor on A0
  https://create.arduino.cc/projecthub/Aritro/smoke-detection-using-mq-2-gas-sensor-79c54a


*/
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include <AsyncDelay.h>

// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
byte server[] = { 192, 168, 1, 30 };
byte ip[]     = { 192, 168, 1, 98 };

EthernetClient ethernetClient;
PubSubClient client(ethernetClient);
long lastMsg = 0;
char msg[50];
int value = 0;
AsyncDelay delay_400ms;
AsyncDelay delay_30s;
AsyncDelay delay_5s;

//MutuallyExclude the 'on' state between pairs of relays (for shutter to prevent up and down at the same time)
//ask R1 to turn on while R2 = on -> turns off R2 first (and broadcasts 'domogik/in/relay/r2 0')
//ask R2 to turn on while R1 = on -> turns off R1 first (and broadcasts 'domogik/in/relay/r1 0')
bool isMutuallyExclude = true;

const char* outTopic = "domogik/relayClient";
const char* inTopic = "domogik/in/relay/#";
const char* outRelayTopic = "domogik/in/relay/r";
const char* outPinTopic = "domogik/relayArduinoPin";

bool relayStates[] = {
  HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH
};

int relayPins[] = {
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53
};       // an array of pin numbers to which LEDs are attached
int pinCount = 32;

#define PIN_DETECT_1 2
#define PIN_DETECT_2 3
#define PIN_DETECT_3 4
#define PIN_DETECT_4 5
#define PIN_DETECT_5 6
#define PIN_DETECT_6 7
#define SMOKE_PIN A0

Bounce debounce1 = Bounce();
Bounce debounce2 = Bounce();
Bounce debounce3 = Bounce();
Bounce debounce4 = Bounce();
Bounce debounce5 = Bounce();
Bounce debounce6 = Bounce();

boolean doSwitch = false;
boolean doAllOff = false;

void setup_ethernet() {

  delay(10);
  // We start by connecting to a WiFi network
  Ethernet.begin(mac, ip);

  digitalWrite(13, LOW);
  delay(500);
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
  delay(500);
  digitalWrite(13, HIGH);

}

void delayTimers() {
  delay_400ms.start(400, AsyncDelay::MILLIS);
  delay_30s.start(30000, AsyncDelay::MILLIS);
  delay_5s.start(5000, AsyncDelay::MILLIS);
}

void  publishStates() {
  for (int thisPin = 0; thisPin < pinCount; thisPin++) {
    char* state = relayStates[thisPin] == HIGH ? "0" : "1";
    publishRelayState(thisPin + 1, state);
  }
}

void switchRelay(char* switchState, int pos) {
  if (switchState == '0') {
    //Turning off is safe to be done immediately
    digitalWrite(relayPins[pos], HIGH);
    relayStates[pos] = HIGH;
  } else if (switchState == '1') {
    //turning on is done in setStates
    //digitalWrite(relayPins[pos], LOW);
    relayStates[pos] = LOW;
  }
}

void setStates() {
  if (doSwitch) {
    for (int thisPin = 0; thisPin < pinCount; thisPin++) {
      digitalWrite(relayPins[thisPin], relayStates[thisPin]);
    }
    doSwitch = false;
    doAllOff = true;
    delay_30s.start(30000, AsyncDelay::MILLIS);
  }
}

void turnAllOff() {
  if (doAllOff) {
    doAllOff = false;
    for (int thisPin = 0; thisPin < pinCount; thisPin++) {
      relayStates[thisPin] = HIGH;
      publishRelayState(thisPin + 1, "0");
      switchRelay('0', thisPin);
    }
  }
}
void publishRelayState(int relayNbr, char* state) {
  char outputTopicBuff[100];
  strcpy(outputTopicBuff, outRelayTopic);
  char relaybuffer[5];
  sprintf(relaybuffer, "%d", relayNbr);
  strcat(outputTopicBuff, relaybuffer);
  client.publish(outputTopicBuff, state, true);
}

void publishPinState(int pinNbr, char* state) {
  char outputTopicBuff[100];
  strcpy(outputTopicBuff, outPinTopic);
  char relaybuffer[5];
  sprintf(relaybuffer, "%d", pinNbr);
  strcat(outputTopicBuff, relaybuffer);
  client.publish(outputTopicBuff, state);
}

void mutuallyExcludePair(char* switchState, int pos) {
  if (pos % 2 == 0 && switchState == '1') //even
  {
    if (pinCount > (pos + 1) && relayStates[pos + 1] == LOW) {
      switchRelay('0', pos + 1);
      publishRelayState(pos + 2, "0");
    }
  }
  else if (pos % 2 == 1 && switchState == '1') //odd
  {
    if ((pos - 1) >= 0 && relayStates[pos - 1] == LOW) {
      switchRelay('0', pos - 1);
      publishRelayState(pos, "0");
    }
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
  //Comes in as domogik/in/relay/r1 ... domogik/in/relay/r16
  String topicString = String(topic);
  int relayNumber = topicString.substring(18, 20).toInt();
  int posInArray = relayNumber - 1;
  char* switchState = (char)payload[0];

  if (switchState == '2') {
    if (relayStates[posInArray] == HIGH) {
      switchState = '1';
    }
    else {
      switchState = '0';
    }
  }

  if (isMutuallyExclude) {
    mutuallyExcludePair(switchState, posInArray);
  }

  switchRelay(switchState, posInArray);
  if (switchState == '1') {
    doSwitch = true;
  }

  delayTimers();
}


void reconnect() {
  while (!client.connected()) {
    if (client.connect("RelayboardClient")) {
      Serial.println("connected");
      client.publish(outTopic, "RelayBoard booted");
      publishStates();
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for (int i = 0; i < 5000; i++) {
        delay(1);
      }
    }
  }
}


void setup() {

  //Relayboard
  for (int thisPin = 0; thisPin < pinCount; thisPin++) {
    digitalWrite(relayPins[thisPin], HIGH);
    pinMode(relayPins[thisPin], OUTPUT);
  }
  pinMode(13, OUTPUT);


  //init state relayboard
  //turnAllOff();

  delayTimers();

  //input
  pinMode(PIN_DETECT_1, INPUT_PULLUP);
  pinMode(PIN_DETECT_2, INPUT_PULLUP);
  pinMode(PIN_DETECT_3, INPUT_PULLUP);
  pinMode(PIN_DETECT_4, INPUT_PULLUP);
  pinMode(PIN_DETECT_5, INPUT_PULLUP);
  pinMode(PIN_DETECT_6, INPUT_PULLUP);
  pinMode(SMOKE_PIN, INPUT);
  debounce1.attach(PIN_DETECT_1);
  debounce2.attach(PIN_DETECT_2);
  debounce3.attach(PIN_DETECT_3);
  debounce4.attach(PIN_DETECT_4);
  debounce5.attach(PIN_DETECT_5);
  debounce6.attach(PIN_DETECT_6);
  debounce1.interval(50);
  debounce2.interval(50);
  debounce3.interval(50);
  debounce4.interval(50);
  debounce5.interval(50);
  debounce6.interval(50);


  digitalWrite(13, LOW);
  delay(500);
  digitalWrite(13, HIGH);
  delay(500);

  Serial.begin(115200);
  setup_ethernet();
  client.setServer(server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }

  if (delay_5s.isExpired()) {
    int analogSensor = analogRead(SMOKE_PIN);
    char bufferSensor[6];
    sprintf(bufferSensor, "%d", analogSensor);
    client.publish("domogik/smoke", bufferSensor);
    delay_5s.repeat();
  }


  if (delay_400ms.isExpired()) {
    setStates();
    delay_400ms.repeat();
  }
  if (delay_30s.isExpired()) {
    turnAllOff();
    delay_30s.repeat();
  }
  debounce1.update();
  if ( debounce1.fell() ) {
    publishPinState(1, "1");
  }
  if ( debounce1.rose() ) {
    publishPinState(1, "0");
  }
  debounce2.update();
  if ( debounce2.fell() ) {
    publishPinState(2, "1");
  }
  if ( debounce2.rose() ) {
    publishPinState(2, "0");
  }
  debounce3.update();
  if ( debounce3.fell() ) {
    publishPinState(3, "1");
  }
  if ( debounce3.rose() ) {
    publishPinState(3, "0");
  }
  debounce4.update();
  if ( debounce4.fell() ) {
    publishPinState(4, "1");
  }
  if ( debounce4.rose() ) {
    publishPinState(4, "0");
  }
  debounce5.update();
  if ( debounce5.fell() ) {
    publishPinState(5, "1");
  }
  if ( debounce5.rose() ) {
    publishPinState(5, "0");
  }
  debounce6.update();
  if ( debounce6.fell() ) {
    publishPinState(6, "1");
  }
  if ( debounce6.rose() ) {
    publishPinState(6, "0");
  }

  client.loop();
}

