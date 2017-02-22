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

#include <Bounce2.h>

// These are the pins connected to the Wiegand D0 and D1 signals.
// Ensure your board supports external Interruptions on these pins
#define PIN_D0 2
#define PIN_D1 3

// The object that handles the wiegand protocol
Wiegand wiegand;

Bounce debouncerDeur = Bounce();
Bounce debouncerWC = Bounce();


// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
byte server[] = { 192, 168, 1, 30 };
byte ip[]     = { 192, 168, 1, 99 };

//
#define ONE_WIRE_BUS 6
#define ONE_WIRE_BUS2 7
#define DEUR_PIN 8
#define DEUR_DETECT 5
int lastDeurState = HIGH;
#define WC_DETECT 9

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
OneWire oneWire2(ONE_WIRE_BUS2);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
DallasTemperature sensors2(&oneWire2);

#define DEUR_OPEN_TOPIC "domogik/in/deur"
#define intervalReading 40000L

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
  Serial.println("sesam open u");
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
  if (String(topic).equals(DEUR_OPEN_TOPIC)) {
    if ((char)payload[0] == '1') {
      openDoor(topic);
    }

    if ((char)payload[0] == '0') {
      Serial.println("Sesam sluit");
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
  sensors2.begin();

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

  sensors2.setResolution(thermometer6, resolution);
  sensors2.setResolution(thermometer5, resolution);
  sensors2.setResolution(thermometer10, resolution);
  sensors2.setResolution(thermometer11, resolution);
  sensors2.setResolution(thermometer13, resolution);
  sensors2.setResolution(thermometer14, resolution);
  sensors2.setResolution(thermometer15, resolution);
  sensors2.setResolution(thermometer16, resolution);

  //input
  pinMode(DEUR_DETECT, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  debouncerDeur.attach(DEUR_DETECT);
  debouncerDeur.interval(50);

  //input
  pinMode(WC_DETECT, INPUT);
  digitalWrite(WC_DETECT,LOW);
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
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("domogik/helloWorld","1");
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
  sensors.requestTemperatures();
  delay(1000);

  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == -127.00 or tempC < -15 or tempC > 59) {
    if (ctr < 3) {
      printTemperature(sensors, deviceAddress, topic, calibration, ++ctr);
    }
    else
    {

      Serial.println(String("Error reading:").concat(topic));
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

long count = 0;

// Notifies when a reader has been connected or disconnected.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onStateChange()`
void stateChanged(bool plugged, const char* message) {
  Serial.print(message);
  Serial.println(plugged ? "CONNECTED" : "DISCONNECTED");
}

void receivedData(uint8_t* data, uint8_t bits, const char* message) {
  Serial.print(message);

  uint8_t bytes = (bits + 7) / 8;
  long code = 0;
  for (int i = 0; i < bytes; i++) {
    code = (code * 1000) + data[i];
  }
  
  Serial.println();
  char buf[15];
  //ex readout : 113086150
  ltoa(code, buf, 10);
  Serial.println(buf);
  String strCode = String(buf);
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
    Serial.println(String("deur detect 1"));

  }
  if ( debouncerDeur.rose() ) {
    client.publish("domogik/deurdetect", "0");
    Serial.println(String("deur detect 0"));
  }
  debouncerWC.update();

  if ( debouncerWC.rose() ) {
    client.publish("domogik/wcdetect", "1");
    Serial.println(String("wc detect 1"));

  }
  if ( debouncerWC.fell() ) {
    client.publish("domogik/wcdetect", "0");
    Serial.println(String("wc detect 0"));
  }

  
  if (count == 1 * intervalReading) {
    printTemperature(sensors, thermometer7, "domogik/berging", 0.5, 0);
  }
  if (count == 2 * intervalReading) {
    printTemperature(sensors, thermometerT, "domogik/tempT",0, 0);
    //second bus
  }
  if (count == 3 * intervalReading) {
    printTemperature(sensors2, thermometer5, "domogik/traponder", -0.4, 0);
  }
  if (count == 4 * intervalReading) {
    printTemperature(sensors2, thermometer10, "domogik/inkom",0, 0);//inkom
  }
  if (count == 5 * intervalReading) {
    printTemperature(sensors2, thermometer11, "domogik/wconder",0.3, 0);//wc onder
  }
  if (count == 6* intervalReading) {
    printTemperature(sensors2, thermometer9, "domogik/paulien", 0, 0);//paulien
  }
  if (count == 7 * intervalReading) {
    printTemperature(sensors2, thermometer3, "domogik/wcboven",0.3, 0);//wc boven
  }
  if (count == 8 * intervalReading) {
    printTemperature(sensors2, thermometer4, "domogik/keuken",-0.8, 0);
  }
  if (count == 9 * intervalReading) {
    printTemperature(sensors2, thermometer12, "domogik/living",0.2, 0);
  }
  if (count == 10 * intervalReading) {
    printTemperature(sensors2, thermometer6, "domogik/trapboven",0.4, 0);
  }
  if (count == 11 * intervalReading) {
    printTemperature(sensors2, thermometer13, "domogik/slpkberg",0, 0);
  }
  if (count == 12 * intervalReading) {
    printTemperature(sensors2, thermometer15, "domogik/badk",0.8, 0);
  }
  if (count == 13 * intervalReading) {
    printTemperature(sensors2, thermometer16, "domogik/vide",1.65, 0);
  }
  if (count == 14 * intervalReading) {
    printTemperature(sensors2, thermometer14, "domogik/masterslpk",0.3, 0);
    count = 0;
  }

  count = count + 1;
  client.loop();
}


