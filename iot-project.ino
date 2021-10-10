//Not-A-Helmet

//Libraries
#include <ESP8266WiFi.h>;
#include <DHT.h>;
#include <ESP8266HTTPClient.h>

//Pins
int dht11=D1;
int lm393sound=D2;
int mq2=A0;
int led=D5;
int pushButton=D6;
int motor=D8;
uint8_t interruptPin=D6;

//Sensor thresholds
int mq2thres=900;

//Network credentials
const char* ssid = "BLAH"; 
const char* password = "BLAH"; 
String myWriteAPIKey= "BLAH";
const char* thingSpeakAddress="api.thingspeak.com";

//Initializing objects
DHT dht(dht11,DHT11);     
WiFiClient client;

//danger flag
int dngr=0;

void connectwifi() {
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");     
  } 
  digitalWrite(led,HIGH);
  Serial.println("\nConnected to WiFi"); 
}

int interr=0;

ICACHE_RAM_ATTR void alert()
{
  interr=!interr;
  digitalWrite(led,interr );
}

void setup() {
  Serial.begin(9600);
  
  connectwifi();
  dht.begin();
  
  pinMode(led, OUTPUT);
  pinMode(D8, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  digitalWrite(led,LOW);
  attachInterrupt(digitalPinToInterrupt(interruptPin),alert,FALLING);
}


void broadcast(int h,int t,int noiselvl,int gasconc,int dngr){
  if (client.connect(thingSpeakAddress, 80)) 
  {
    String msg="key=";
    msg += myWriteAPIKey;
    msg += "&field1=";
    msg += String(t);
    msg += "&field2=";
    msg += String(h);
    msg += "&field3=";
    msg += String(noiselvl);
    msg += "&field4=";
    msg += String(gasconc);
    msg += "&field5=";
    msg += String(dngr);
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + myWriteAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(String(msg.length()));
    client.print("\n\n");
    client.print(msg);
    
    String ret=client.readString();
    Serial.println(ret);
    client.stop();
  }
}

void danger(int h,int t,int noiselvl,int gasconc) {
  dngr=1;
  while(dngr==1)
  { 
    digitalWrite(led,HIGH);
    broadcast(h,t,noiselvl,gasconc,1);
    h = dht.readHumidity();
    noiselvl = digitalRead(lm393sound);
    gasconc= analogRead(mq2);
    
    if(!checkReadings(h,t,noiselvl,gasconc))
      dngr=0;
    delay(9000);
    serialprnt(h,t,noiselvl,gasconc,dngr);
    int val= (gasconc-400)*624/1024;
    analogWrite(motor,val);
  }
  digitalWrite(led,LOW);
}

bool checkReadings(int h,int t,boolean noiselvl,int gasconc){
  if(h>90||t>50||noiselvl==HIGH||gasconc>mq2thres)
    return 1;
  return 0;
}

void serialprnt(int h,int t,int noiselvl,int gasconc,boolean dngr){
  Serial.print("Humidity=");
  Serial.print(h);
  Serial.print("\nTemperature=");
  Serial.print(t); 
  Serial.print("\nNoiselvl=");
  Serial.print(noiselvl);
  Serial.print("\ngasconc=");
  Serial.print(gasconc);
  Serial.print("\ndngr=");
  Serial.print(dngr);
}

void emergency(int h,int t,boolean noiselvl,int gasconc) {
  broadcast(h,t,noiselvl,gasconc,1);
}


const char * headerKeys[] = {"feeds"} ;
const size_t numberOfHeaders = 1;
int gadim()
{ 
  HTTPClient http;
  http.begin("https://api.thingspeak.com/channels/1168012/fields/1.json?api_key=9TCHQENG0INJWE5E&results=2");
  http.collectHeaders(headerKeys, numberOfHeaders);
  http.GET();
  for(int i = 0; i< http.headers(); i++){
        Serial.println(http.header(i));
  }
}

void loop() {
  
  delay(10000);
  
  //dht11 readings
  double h = dht.readHumidity();
  double t = dht.readTemperature();

  //lm393 readings
  int noiselvl=0;
  noiselvl = digitalRead(lm393sound);
  
  //mq2 readings
  int gasconc= analogRead(mq2);

  serialprnt(h,t,noiselvl,gasconc,0);
  
/*
  if(gadim()==1)
    interr=1;
  else
    interr=0;
  */
  
  if(interr==1)
    emergency(h,t,noiselvl,gasconc);
  else{
  digitalWrite(led,LOW);
  //Checking if any of the readings are dangerous
  if(checkReadings(h,t,noiselvl,gasconc))
    danger(h,t,noiselvl,gasconc);
  else
    broadcast(h,t,noiselvl,gasconc,0);  
  }
    
  digitalWrite(led,interr );
  int val= (gasconc-400)*624/1024;
  analogWrite(motor,val);
  
}
