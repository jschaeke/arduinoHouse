
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

  The current state is stored in EEPROM and restored on bootup

*/
#include <Ethernet.h>
#include <PubSubClient.h>
#include <EEPROM.h>

// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE };
byte server[] = { 192, 168, 1, 30 };
byte ip[]     = { 192, 168, 1, 98 };

EthernetClient ethernetClient;
PubSubClient client(ethernetClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//MutuallyExclude the 'on' state between pairs of relays (for shutter to prevent up and down at the same time)
//ask R1 to turn on while R2 = on -> turns off R2 first (and broadcasts 'domogik/in/relay/r2 0')
//ask R2 to turn on while R1 = on -> turns off R1 first (and broadcasts 'domogik/in/relay/r1 0')
bool isMutuallyExclude = true;

const char* outTopic = "domogik/relayClient";
const char* inTopic = "domogik/in/relay/#";
const char* outRelayTopic = "domogik/in/relay/r";

bool relayStates[] = {
  HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH
};

int relayPins[] = {
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53
};       // an array of pin numbers to which LEDs are attached
int pinCount = 32;

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

void switchRelay(char* switchState, int pos) {
  // Switch on the LED if an 1 was received as first character
  if (switchState == '0') {
    digitalWrite(relayPins[pos], HIGH);
    relayStates[pos] = HIGH;
    EEPROM.update(pos, relayStates[pos]);    // Write state to EEPROM
  } else if (switchState == '1') {
    digitalWrite(relayPins[pos], LOW);
    relayStates[pos] = LOW;
    EEPROM.update(pos, relayStates[pos]);    // Write state to EEPROM
  }
}

void publishRelayState(int relayNbr, char* state) {
  char outputTopicBuff[100];
  strcpy(outputTopicBuff, outRelayTopic);
  char relaybuffer[5];
  sprintf(relaybuffer, "%d", relayNbr);
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

}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("RelayboardClient")) {
      Serial.println("connected");
      client.publish(outTopic, "RelayBoard booted");
      for (int thisPin = 0; thisPin < pinCount; thisPin++) {
        char* state = relayStates[thisPin] == HIGH ? "0" : "1";
        publishRelayState(thisPin + 1, state);
      }
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
    pinMode(relayPins[thisPin], OUTPUT);
  }
  pinMode(13, OUTPUT);

  for (int thisPin = 0; thisPin < pinCount; thisPin++) {
    relayStates[thisPin] = EEPROM.read(thisPin);
  }
  //restore state relayboard
  for (int thisPin = 0; thisPin < pinCount; thisPin++) {
    digitalWrite(relayPins[thisPin], relayStates[thisPin]);
  }

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
  client.loop();
}

