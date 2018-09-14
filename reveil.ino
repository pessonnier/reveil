
#include <Wire.h>  
#include "SSD1306.h"
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h> 

const char* ssid     = "Livebox-B7B0";
const char* password = "pTXMsVp3CGZiQj6tJy";
WiFiServer server(80);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 7200, 60000);

// SSD1306  display(ADDRESS, SDA, SDC);
SSD1306  display(0x3c, 21, 22);
int nbmessages = 23;
int message = 0;
String text = "Bienveune dans\nle monde du\nreveil";
String pouetpouet[] = {
  "tu es un\nglacon fondu", 
  "tu es un\ngarçon tordu", 
  "tu es moche", 
  "tu es méchant",
  "Akilan enerve\npapa",
  "tu es gentil", 
  "tu es une fille", 
  "tu es gros", 
  "tu est\ngourmand",
  "tu es une\nyoutubeuse",
  "tu pues", 
  "papa n'est pas\njoyeux", 
  "papa veux-tu\njouer avec nous?",
  "Akilan veut\nfaire caca", 
  "tu sent le\ncamembert", 
  "tu est la\nreine",
  "tu es belle", 
  "tu va faire\npipi dans ta\nculotte", 
  "tu peux voler",
  "tu sens le\nmashmaloo", 
  "tu sens \nl'ordinateur", 
  "vas aux toilettes\nsi non tu es puni!", 
  "papa a une\ntête de tigre", 
  "papa est sur\ngoogle" };
#define buzzer 4  
#define BTN_vert 2
#define BTN_rouge 13
#define DIODE1 32
#define DIODE2 33

// reglage du buzzer
int freq = 0;
int channel = 1;
int resolution = 8;
int reveil_heure = 7;
int reveil_min = 15;
int last_buzz_on = -120000;
int serial_time_periode = 10000;
int serial_time_last = 0;
boolean buzz_on = false;

boolean flip = false;
int cpt = 0;

// capteur de température
#include "DHT.h"
#define DHTPIN 14     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors
#define DHTALIM 27
#define REPAREDHT true
#define DHTFORCE false
DHT dht(DHTPIN, DHTTYPE);
volatile boolean bttRougePressed = false;
boolean ecranDispo = true;
const int delayAfficheMeteo = 7000;
long tAfficheMeteo = 0;
void IRAM_ATTR clicRouge () {
  bttRougePressed = true;
}

// météo
const String endpoint = "http://api.openweathermap.org/data/2.5/weather?id=2998632&APPID=32ff58959308fa084757734628f88703&units=metric";
const String label[10] = {"beau", "petits nuages", "nuages", "gros nuages", "pluie neige", "pluie", "orage", "neige", "brouillard", "sais pas"};
int code2pos(int code) {
  switch (code) {
    case 19 : return 4;
    case 10 : return 5;
    case 11 : return 6;
    case 13 : return 7;
    case 50 : return 8;
  }
  if (code <= 4) {
    return code -1;
  }
  return 9;
}


float humidite;
float tempPiece;
void temperature () {
  float h = dht.readHumidity();
  float t = dht.readTemperature(false, DHTFORCE);
  float hic = dht.computeHeatIndex(t, h, false);
  int cpt = 0;
  while (isnan(t) && REPAREDHT) {
    cpt ++;
    if (cpt > 5) {
      humidite = 0;
      tempPiece = 0;
      Serial.println("DHT abandon");
      return;
    }
    h = dht.readHumidity();
    t = dht.readTemperature();
    hic = dht.computeHeatIndex(t, h, false);
    Serial.print("DHT bug\n");
    digitalWrite(DHTALIM, LOW);
    delay(500);
    digitalWrite(DHTALIM, HIGH);
    delay(500);
  }
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C ");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.print(" *C ");
  humidite = h;
  tempPiece = t;
}

void setup() {
  // void attachInterrupt(uint8_t pin, void (*)(void), int mode);
  // FALLING
  // https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/esp32-hal-gpio.h
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(20);
  Serial.println("alu");
  Serial.println("pin DHT " + String(DHTPIN));
  Serial.println("pin DHT alim " + String(DHTALIM));
  display.init();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, text);
  display.display();

  // cnx au wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  //display.drawString(0,30, "Wifi: " + String(WiFi.localIP()));
  //display.display();

  // horloge
  timeClient.begin();

  // buzzer
  //ledcSetup(channel, freq, resolution);
  //ledcAttachPin(buzzer, channel);
  pinMode(buzzer,OUTPUT);

  // initialise la temprerature
  temperature();
  
  // Set BTN_STOP_ALARM to input mode
  pinMode(BTN_vert, INPUT_PULLUP);
  pinMode(BTN_rouge, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN_rouge), clicRouge, FALLING);
  
  pinMode(DIODE1, OUTPUT);
  pinMode(DIODE2, OUTPUT);
  digitalWrite(DIODE1, HIGH);
  digitalWrite(DIODE2, HIGH);

  pinMode(DHTALIM, OUTPUT);
  digitalWrite(DHTALIM, HIGH);
//  pinMode(6, OUTPUT);
//  digitalWrite(6, LOW); // HIGH fait un reset
//  pinMode(7, OUTPUT);
//  digitalWrite(7, HIGH);
//  pinMode(8, OUTPUT);
//  digitalWrite(8, HIGH);      
  Serial.println("rev 6");
  Serial.printf("ok");
}

String stemp;
String stmin;
String stmax;
String sdesc;
String txtcode;
void decodeMeteo(String ow) {
  int pfin = ow.length();
  int ptemp = ow.indexOf("\"temp\":")+7;
  int ptempf = ow.indexOf(",", ptemp);
  int ptempmin = ow.indexOf("\"temp_min\":")+11;
  int ptempminf = ow.indexOf(",", ptempmin);
  int ptempmax = ow.indexOf("\"temp_max\":")+11;
  int ptempmaxf = ow.indexOf("}", ptempmax);
  int pdesc = ow.indexOf("\"description\":")+15;
  int pdescf = ow.indexOf("\"", pdesc);
  int pcode = ow.indexOf("\"icon\":")+8;
  int pcodef = pcode+2;
  
  stemp = ow.substring(ptemp, ptempf);
  stmin = ow.substring(ptempmin, ptempminf);
  stmax = ow.substring(ptempmax, ptempmaxf);
  sdesc = ow.substring(pdesc, pdescf);
  String scode = ow.substring(pcode, pcodef);
  
  float temp = stemp.toFloat();
  float tmin = stmin.toFloat();
  float tmax = stmax.toFloat();
  int codeIcon = scode.toInt();
  txtcode = label[code2pos(codeIcon)];
}

void meteo () {
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin(endpoint); //Specify the URL
    int httpCode = http.GET();  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
        decodeMeteo(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
}

void affiche(String message) {
  timeClient.update();
  if (millis() - serial_time_last > serial_time_periode) {
    serial_time_last = millis();
    Serial.println(timeClient.getFormattedTime());
    Serial.println(timeClient.getHours());
    Serial.println(timeClient.getMinutes());
  }
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, timeClient.getFormattedTime());
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, message);
  display.display();
}

void afficheMeteo() {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "t : " + String(tempPiece));
  display.drawString(64, 0, "max : " + stmax);
  display.drawString(0, 16, "D : " + stemp);
  display.drawString(64, 16, "min : " + stmin);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 24, txtcode);
  display.display();
}

void reveil() {
  Serial.println("reveil");
  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, LOW);
  delay(100);
  digitalWrite(buzzer, HIGH);
  delay(100);
  digitalWrite(buzzer, LOW);
  delay(500);
}

void buzz() {
  Serial.println("buzz hi");
//  ledcWrite(channel, 125);
//  ledcWrite(channel, 255);
//  for (int thisNote = 0; thisNote < 8; thisNote++) {
//    ledcWriteTone(channel, melody[thisNote]);
//    delay(melodyDurations[thisNote]+1);
//  ledcWriteTone(channel, 1000);
  digitalWrite(buzzer, HIGH);
  delay(50);
  digitalWrite(buzzer, LOW);
  delay(50);
  digitalWrite(buzzer, HIGH);
  delay(50);
  digitalWrite(buzzer, LOW);
//  ledcWriteTone(channel, 0);
//  }
}

void loop() {
  // put your main code here, to run repeatedly:
  message++;
  if (message == nbmessages) {
    message = 0;
  }
  if (ecranDispo) {
    affiche(text);
  }
  
  if (digitalRead(BTN_vert) == LOW) {
    text = pouetpouet[message];
    cpt++;
    printf("VERT %d\n", cpt);
    flip = !flip;
    if (flip) {
      digitalWrite(DIODE1, LOW);
      digitalWrite(DIODE2, HIGH);
    } else {
      digitalWrite(DIODE1, HIGH);
      digitalWrite(DIODE2, LOW);
    }
  }
  if ((digitalRead(BTN_rouge) == LOW) || bttRougePressed) {
    Serial.println("ROUGE");
    buzz();
    buzz_on = false;
    meteo();
    temperature();
    display.init();
    afficheMeteo();
    bttRougePressed = false;
    ecranDispo = false;
    tAfficheMeteo = millis();
    //delay(7000);
  }
  if (millis() - tAfficheMeteo > delayAfficheMeteo) {
    ecranDispo = true;
  }
  if ((timeClient.getHours() == reveil_heure) && (timeClient.getMinutes() == reveil_min)) {
    if (!buzz_on) {
      Serial.println("c'est l'heure");
      Serial.println(millis() - last_buzz_on);
    }
    if (millis() - last_buzz_on > 120000) {
      Serial.println("allumage");
      last_buzz_on = millis();
      buzz_on = true;
    }
  }
  if ((timeClient.getHours() == reveil_heure) && (timeClient.getMinutes() == reveil_min +1)) {
    if (buzz_on) {
      Serial.println("ce n'est plus l'heure");
      buzz_on = false;
    }
  }
  if (buzz_on) {
    reveil();
  }
  delay(100);
}
