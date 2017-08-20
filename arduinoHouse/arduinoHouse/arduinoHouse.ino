/*
  Publishing in the callback

  - connects to an MQTT server
  - subscribes to the topic "inTopic"
  - when a message is received, republishes it to "outTopic"

  This example shows how to publish messages within the
  callback function. The callback function header needs to
  be declared before the PubSubClient constructor and the
  actual callback defined afterwards.
  This ensures the client reference in the callback function
  is valid.

*/
//MQTT
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
// Onewire
#include "OneWire.h"
#include "DallasTemperature.h"
#include <Wiegand.h>
// include the AnalogMultiButton library
#include <AnalogMultiButton.h>

#include <Bounce2.h>
#include <AsyncDelay.h>


// These are the pins connected to the Wiegand D0 and D1 signals.
// Ensure your board supports external Interruptions on these pins
#define PIN_D0 2
#define PIN_D1 3

// The object that handles the wiegand protocol
Wiegand wiegand;

Bounce debouncerDeur = Bounce();
Bounce debouncerWC = Bounce();

AsyncDelay delay_30s;
AsyncDelay delay_50s;
AsyncDelay delay_55s;
AsyncDelay delay_60s;

// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
byte server[] = { 192, 168, 1, 30 };
byte ip[]     = { 192, 168, 1, 99 };

//
#define ONE_WIRE_BUS 7
#define DEUR_PIN 8
#define DEUR_DETECT 5
int lastDeurState = HIGH;
#define WC_DETECT 9

// 5V --> 2.4K resistor --> A0  --> BUTTON_AN1 --> GND
//                              |--> 1.2K resistor --> BUTTON_AN2 --> GND
//                                                |--> 1.2K resistor --> BUTTON_AN3 --> GND
const int BUTTONS_PIN = A0;
// set how many buttons you have connected
const int BUTTONS_TOTAL = 3;
// find out what the value of analogRead is when you press each of your buttons and put them in this array
// you can find this out by putting Serial.println(analogRead(BUTTONS_PIN)); in your loop() and opening the serial monitor to see the values
// make sure they are in order of smallest to largest
const int BUTTONS_VALUES[BUTTONS_TOTAL] = {0, 340, 508};
const int BUTTON_AN1 = 0;
const int BUTTON_AN2 = 1;
const int BUTTON_AN3 = 2;
// make an AnalogMultiButton object, pass in the pin, total and values array
AnalogMultiButton buttons(BUTTONS_PIN, BUTTONS_TOTAL, BUTTONS_VALUES);

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

#define DEUR_OPEN_TOPIC "domogik/in/deur"
bool isNextReadsensors = true;

// Assign the unique addresses of your 1-Wire temp sensors.
// See the tutorial on how to obtain these addresses:
// http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html

DeviceAddress thermometer1 = {
  0x28, 0x02, 0x81, 0x35, 0x05, 0x00, 0x00, 0x8A
};
DeviceAddress thermometer2 = {
  0x28, 0x27, 0x79, 0x27, 0x05, 0x00, 0x00, 0x70
};
DeviceAddress thermometer3 = {
  0x28, 0x64, 0xE7, 0x27, 0x05, 0x00, 0x00, 0x49
};
DeviceAddress thermometer4 = {
  0x28, 0xB3, 0x86, 0x27, 0x05, 0x00, 0x00, 0xD4
};
DeviceAddress thermometer5 = {
  0x28, 0xD5, 0x32, 0x27, 0x05, 0x00, 0x00, 0xF5
};
DeviceAddress thermometer6 = {
  0x28, 0x1A, 0x8A, 0x27, 0x05, 0x00, 0x00, 0x3F
};
DeviceAddress thermometer7 = {
  0x28, 0x79, 0xDB, 0x27, 0x05, 0x00, 0x00, 0xFD
};
DeviceAddress thermometer8 = {
  0x28, 0xBE, 0x86, 0x27, 0x05, 0x00, 0x00, 0x9E
};
DeviceAddress thermometer9 = {
  0x28, 0xE5, 0xF2, 0x27, 0x05, 0x00, 0x00, 0x3A
};
DeviceAddress thermometer10 = {
  0x28, 0x09, 0x70, 0x27, 0x05, 0x00, 0x00, 0x26
};
DeviceAddress thermometer11 = {
  0x28, 0x7C, 0x95, 0x27, 0x05, 0x00, 0x00, 0x5D
};
DeviceAddress thermometer12 = {
  0x28, 0x3D, 0xB0, 0x35, 0x05, 0x00, 0x00, 0x0A
};
DeviceAddress thermometerT = {
  0x28, 0xA4, 0xD9, 0x27, 0x05, 0x00, 0x00, 0xF0
};
DeviceAddress thermometer13 = {
  0x28, 0xFF, 0x38, 0xDB, 0x50, 0x16, 0x03, 0xB8
};
DeviceAddress thermometer14 = { //4
  0x28, 0xFF, 0xCF, 0x07, 0x54, 0x16, 0x04, 0x41
};
//2
DeviceAddress thermometer15 = {
  0x28, 0xFF, 0xAF, 0xC2, 0x50, 0x16, 0x03, 0xF5
};
DeviceAddress thermometer16 = { //3
  0x28, 0xFF, 0x16, 0x69, 0x52, 0x16, 0x04, 0xB8
};
DeviceAddress thermometer17 = { //1
  0x28, 0xFF, 0x97, 0x05, 0x50, 0x16, 0x03, 0xF0
};

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);


void openDoor(char const * topic) {
  digitalWrite(DEUR_PIN, LOW);
  delay(1500);
  digitalWrite(DEUR_PIN, HIGH);
  client.publish(topic, 0);
}

// Callback function
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println(); // See more at: http://www.esp8266.com/viewtopic.php?f=29&t=8746#sthash.73Btjb9a.dpuf
  Serial.println((char)payload[0]);
  if (strcmp(topic, DEUR_OPEN_TOPIC) == 0) {
    if ((char)payload[0] == '1') {
      openDoor(topic);
    }

    if ((char)payload[0] == '0') {
      digitalWrite(DEUR_PIN, HIGH);
    }
  }
}

void setup()
{
  digitalWrite(DEUR_PIN, HIGH);
  pinMode(DEUR_PIN, OUTPUT);

  Ethernet.begin(mac, ip);
  if (client.connect("arduinoClient")) {
    // client.publish("outTopic","hello world");
    client.subscribe("domogik/in/#");
  }
  // start serial port
  Serial.begin(9600);
  // Start up the library
  sensors.begin();

  // set the resolution to 10 bit (good enough?)
  int resolution = 10;
  sensors.setResolution(thermometer1, resolution);
  sensors.setResolution(thermometer2, resolution);
  sensors.setResolution(thermometer3, resolution);
  sensors.setResolution(thermometer4, resolution);
  sensors.setResolution(thermometer7, resolution);
  sensors.setResolution(thermometer8, resolution);
  sensors.setResolution(thermometer9, resolution);
  sensors.setResolution(thermometer12, resolution);
  sensors.setResolution(thermometerT, resolution);

  sensors.setResolution(thermometer6, resolution);
  sensors.setResolution(thermometer5, resolution);
  sensors.setResolution(thermometer10, resolution);
  sensors.setResolution(thermometer11, resolution);
  sensors.setResolution(thermometer13, resolution);
  sensors.setResolution(thermometer14, resolution);
  sensors.setResolution(thermometer15, resolution);
  sensors.setResolution(thermometer16, resolution);

  //input
  pinMode(DEUR_DETECT, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  debouncerDeur.attach(DEUR_DETECT);
  debouncerDeur.interval(50);

  //input
  pinMode(WC_DETECT, INPUT);
  digitalWrite(WC_DETECT, LOW);
  // After setting up the button, setup the Bounce instance :
  debouncerWC.attach(WC_DETECT);
  debouncerWC.interval(10);


  //Install listeners and initialize Wiegand reader
  wiegand.onReceive(receivedData, "Card readed: ");
  wiegand.onStateChange(stateChanged, "State changed: ");
  wiegand.begin(WIEGAND_LENGTH_AUTO);

  //initialize pins as INPUT and attaches interruptions
  pinMode(PIN_D0, INPUT);
  pinMode(PIN_D1, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_D0), pinStateChanged, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_D1), pinStateChanged, CHANGE);

  //Sends the initial pin state to the Wiegand library
  pinStateChanged();
  delay_30s.start(30000, AsyncDelay::MILLIS);
  delay_50s.start(50000, AsyncDelay::MILLIS);
  delay_55s.start(55000, AsyncDelay::MILLIS);
  delay_60s.start(60000, AsyncDelay::MILLIS);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("domogik/arduinoHouse","booted");
      // ... and resubscribe
      client.subscribe("domogik/in/#");
    } else {
      Serial.print("failed, rc=");
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void printTemperature(DallasTemperature sensors, DeviceAddress deviceAddress, char const * topic, float calibration, int ctr)
{
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == -127.00 or tempC < -15 or tempC > 59) {
    if (ctr < 3) {
      printTemperature(sensors, deviceAddress, topic, calibration, ++ctr);
    }
  }
  else {
    Serial.println(topic);
    Serial.println(tempC);
    float tempCcalib = calibration + tempC;
    Serial.println(tempCcalib);
    char buffer[12];
    char outstr[15];
    dtostrf(tempCcalib, 4, 2, outstr);
    client.publish(topic, outstr);
  }
}

// When any of the pins have changed, update the state of the wiegand library
void pinStateChanged() {
  wiegand.setPin0State(digitalRead(PIN_D0));
  wiegand.setPin1State(digitalRead(PIN_D1));
}

// Notifies when a reader has been connected or disconnected.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onStateChange()`
void stateChanged(bool plugged, const char* message) {
  Serial.print(message);
  Serial.println(plugged ? "CONNECTED" : "DISCONNECTED");
}

void receivedData(uint8_t* data, uint8_t bits, const char* message) {
  uint8_t bytes = (bits + 7) / 8;
  long code = 0;
  for (int i = 0; i < bytes; i++) {
    code = (code * 1000) + data[i];
  }
  char buf[15];
  //ex readout : 113086150
  ltoa(code, buf, 10);
  client.publish("domogik/cardreader", buf);
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }

  // Every few milliseconds, check for pending messages on the wiegand reader
  // This executes with interruptions disabled, since the Wiegand library is not thread-safe
  noInterrupts();
  wiegand.flush();
  interrupts();

  // Update the Bounce instance :
  debouncerDeur.update();

  if ( debouncerDeur.fell() ) {
    client.publish("domogik/deurdetect", "1");
  }
  if ( debouncerDeur.rose() ) {
    client.publish("domogik/deurdetect", "0");
  }
  debouncerWC.update();

  if ( debouncerWC.rose() ) {
    client.publish("domogik/wcdetect", "1");
  }
  if ( debouncerWC.fell() ) {
    client.publish("domogik/wcdetect", "0");
  }

  if (buttons.onPress(BUTTON_AN1))
  {
    client.publish("domogik/buttonan1", "1");
  }
  if (buttons.onRelease(BUTTON_AN1))
  {
    client.publish("domogik/buttonan1", "0");
  }
  if (buttons.onPress(BUTTON_AN2))
  {
    client.publish("domogik/buttonan2", "1");
  }
  if (buttons.onRelease(BUTTON_AN2))
  {
    client.publish("domogik/buttonan2", "0");
  }
  if (buttons.onPress(BUTTON_AN3))
  {
    client.publish("domogik/buttonan3", "1");
  }
  if (buttons.onRelease(BUTTON_AN3))
  {
    client.publish("domogik/buttonan3", "0");
  }

  if (delay_30s.isExpired()) {
    if (isNextReadsensors) {
      sensors.requestTemperatures();
      isNextReadsensors = false;
    }
    delay_30s.repeat(); // Count from when the delay expired, not now
  }
  if (delay_50s.isExpired()) {
    printTemperature(sensors, thermometer7, "domogik/berging", 0.5, 0);
    printTemperature(sensors, thermometerT, "domogik/tempT", 0, 0);
    printTemperature(sensors, thermometer5, "domogik/traponder", -0.4, 0);
    printTemperature(sensors, thermometer10, "domogik/inkom", 0, 0); //inkom
    delay_50s.repeat();
  }
  if (delay_55s.isExpired()) {
    printTemperature(sensors, thermometer11, "domogik/wconder", 0.3, 0); //wc onder
    printTemperature(sensors, thermometer9, "domogik/paulien", 0, 0);//paulien
    printTemperature(sensors, thermometer3, "domogik/wcboven", 0.3, 0); //wc boven
    printTemperature(sensors, thermometer4, "domogik/keuken", -0.8, 0);
    printTemperature(sensors, thermometer12, "domogik/living", 0.2, 0);
    delay_55s.repeat();
  }
  if (delay_60s.isExpired()) {
    printTemperature(sensors, thermometer6, "domogik/trapboven", 0.4, 0);
    printTemperature(sensors, thermometer13, "domogik/slpkberg", 0, 0);
    printTemperature(sensors, thermometer15, "domogik/badk", 0.8, 0);
    printTemperature(sensors, thermometer16, "domogik/vide", 0, 0);
    printTemperature(sensors, thermometer14, "domogik/masterslpk", 0.3, 0);

    isNextReadsensors = true;
    delay_30s.start(30000, AsyncDelay::MILLIS);
    delay_60s.repeat();
  }
  client.loop();
}


