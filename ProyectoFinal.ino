#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <DHT.h>
#include <DHT_U.h>

#define PHOTOCELL A0
#define LED_1 D8
#define LED_2 D7
#define SENSOR_DHT D5
#define ECHO D4
#define TRIG D3
#define SENSOR_PIR D6

DHT dht(SENSOR_DHT, DHT11);

int ldr;
int temp;
int hum;
int distancia;
int tiempo;
int valPIR;
int pirState = LOW;

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "Totalplay-35A1"
#define WLAN_PASS       "35A14626M5xZ82ct"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME  "KappitaRoss"
#define AIO_KEY       "aio_pNAU000BvNRWK3tZxsgRojIEXc0g"

/**************************** Global State  *********************************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/


// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/photocell");
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperature");
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humidity");
Adafruit_MQTT_Publish distance = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/distance");
Adafruit_MQTT_Publish PIR = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pir");


// Setup a feed called 'onoff' for subscribing to changes to the button
Adafruit_MQTT_Subscribe led1 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/led1", MQTT_QOS_1);
Adafruit_MQTT_Subscribe led2 = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/led2", MQTT_QOS_1);

/*************************** Sketch Code ************************************/

void setup() {

  pinMode(SENSOR_PIR, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(TRIG, OUTPUT);

  dht.begin();
    
  Serial.begin(9600);
  delay(2000);

  Serial.println(F("Adafruit MQTT demo"));

  conectarWiFi();

  led1.setCallback(led1callback);
  led2.setCallback(led2callback);

  mqtt.subscribe(&led1);
  mqtt.subscribe(&led2);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();
  
  ultrasonico();
  fotocelda();
  temperatura();
  humedad();
  sensorPIR();
  Serial.println(F("\n- - - - - - - - - - - - - -"));
  
  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(500);
  delay(10000);
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
 // if(! mqtt.ping()) {
 //   mqtt.disconnect();
 // }
 
}

void ultrasonico(){
   digitalWrite(TRIG, HIGH);
   delay(1);
   digitalWrite(TRIG, LOW);
   tiempo = pulseIn(ECHO, HIGH);
   distancia = tiempo/58.2;
   Serial.print(F("\nDistancia medida :"));
   Serial.print(distancia);
   Serial.print(" cm");
   if (! distance.publish(distancia)) {
    Serial.println(F("\nFailed"));
  } else {
    Serial.println(F("\nOK!"));
  }
}

void fotocelda(){
  ldr = analogRead(PHOTOCELL);
  Serial.print(F("\nLuminosidad en la fotocelda : "));
  Serial.print(ldr);
  if (! photocell.publish(ldr)) {
    Serial.println(F("\nFailed"));
  } else {
    Serial.println(F("\nOK!"));
  }
}

void temperatura(){
  
  temp = dht.readTemperature();
  Serial.print(F("\nTemperatura : "));
  Serial.print(temp);
  Serial.print(" °C");
  if (! temperature.publish(temp)) {
    Serial.println(F("\nFailed"));
  } else {
    Serial.println(F("\nOK!"));
  }
}

void humedad(){
   hum = dht.readHumidity();
  Serial.print(F("\nPorcentaje de humedad : "));
  Serial.print(hum);
  Serial.print(" %");
  if (! humidity.publish(hum)) {
    Serial.println(F("\nFailed"));
  } else {
    Serial.println(F("\nOK!"));
  }
}

void sensorPIR(){
   valPIR = digitalRead(SENSOR_PIR);
   if (valPIR == HIGH)   //si está activado
   { 
      if (pirState == LOW)  //si previamente estaba apagado
      {
        //digitalWrite(LED_2, HIGH);  //LED ON
        Serial.print(F("\nSensor PIR activado"));
        pirState = HIGH;
      }else{
        //digitalWrite(LED_2, LOW); // LED OFF
        Serial.print(F("\nSensor PIR desactivado"));
        pirState = LOW;
      }
      if (! PIR.publish(pirState)) {
        Serial.println(F("\nFailed"));
      } else {
        Serial.println(F("\nOK!"));
      }
   }
}

void led1callback(char *data, uint16_t len) {
  Serial.print("Hey we're in the led 1 callback, the button value is: ");
  Serial.println(data);

     String message = String(data);
      message.trim();
      if (message == "ON") {digitalWrite(LED_1, HIGH);}
      if (message == "OFF") {digitalWrite(LED_1, LOW);}
  
}

void led2callback(char *data, uint16_t len) {
  Serial.print("Hey we're in the led 2 callback, the button value is: ");
  Serial.println(data);

     String message = String(data);
      message.trim();
      if (message == "ON") {digitalWrite(LED_2, HIGH);}
      if (message == "OFF") {digitalWrite(LED_2, LOW);}
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 10 seconds...");
       mqtt.disconnect();
       delay(12000);  // wait 10 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void conectarWiFi(){
  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
}
