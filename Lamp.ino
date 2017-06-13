#include <NewPing.h>
 
#define MAX_DISTANCE 200
#define NUM_OF_SONAR 5
#define INTERVAL 30
#define TOL 20
#define CYCLE_D 33

//Relay pins
#define REL1 40
#define REL2 41
#define REL3 42
#define REL4 43
#define REL5 44

//Sonar sensor objects 
NewPing sonar[NUM_OF_SONAR] ={
  NewPing(22, 23, MAX_DISTANCE),
  NewPing(24, 25, MAX_DISTANCE),
  NewPing(26, 27, MAX_DISTANCE),
  NewPing(28, 29, MAX_DISTANCE),
  NewPing(32, 33, MAX_DISTANCE),
};

#include <SPI.h>
#include <WiFi.h>

char ssid[] = "xxxx";      //network SSID (name)
char pass[] = "yyyyy";   //network password
int keyIndex = 0;    
int status = WL_IDLE_STATUS;
WiFiServer server(80);


float MMD[NUM_OF_SONAR]; //Maximum Moveable distance for the client
uint8_t OBJ[NUM_OF_SONAR];  //Objects in the room bed,chair
uint8_t LIGHT[NUM_OF_SONAR]={1,1,1,1,1}; 
int max_dis[]={0,0,0,0,0}; 
boolean CALIBRATED=false;

void setup() {
  Serial.begin(9600);      // initialize serial communication
  pinMode(9, OUTPUT);      // set the LED pin mode

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);       // don't continue
  }

  String fv = WiFi.firmwareVersion();
  if (fv != "1.1.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status

  //set the pins of the relay module
  pinMode(REL1,OUTPUT);
  pinMode(REL2,OUTPUT);
  pinMode(REL3,OUTPUT);
  pinMode(REL4,OUTPUT);
  pinMode(REL5,OUTPUT);  
  //switch off the pannels
   controlLEDpannel();  
}


void loop() {

WiFiClient client = server.available();   // listen for incoming clients

//findmaxdis(5);
//Serial.println("MAX DIST FOUND.!");
uint8_t current_loc=NUM_OF_SONAR;

while(! client && CALIBRATED ){

 
  Serial.println("Positioning....................");
  int readings[]={0,0,0,0,0};
  for(uint8_t i=0; i<NUM_OF_SONAR; i++){    
     readings[i]=(sonar[i].convert_cm(sonar[i].ping_median(5)));
     if(readings[i] != 0 && readings[i] <20){
          LIGHT[i]=0;      
     }else{
          LIGHT[i]=1; 
     }
     delay(CYCLE_D);  
   }
   controlLEDpannel();
} 
  
  if (client) {     
    Serial.println("Handle a client....................");
    Serial.println("new client");
    skipRequest(client);
    sendHTTPHeader(client);   //send the HTTP header including SSE response       
    while (client.connected()) {
      char c = client.read(); 
      if(!CALIBRATED){ //If the lamp is not calibrated then send event stream
        Serial.println("Calibration SSE....................");
        serverSentEvent(client);
      }
      String request = client.readStringUntil('\r');
      client.flush();
        if (request.indexOf("/@") != -1) { //Listen to the clients mannual control request  
           Serial.println("Client mannual Request..........");
           uint8_t start = (uint8_t)(request.indexOf("@")) + 1;      // Start val
           uint8_t ends = (uint8_t)(request.indexOf("HTTP/1.1"));   // end val
           processReqLED(request,start,ends);
           controlLEDpannel();
        }
        if (request.indexOf("/?") != -1) { //Check for client end responce for end of calibration
           Serial.println("Client request end of calibration..........");
           uint8_t start = (uint8_t)(request.indexOf("?")) + 1;      // Start val
           uint8_t ends = (uint8_t)(request.indexOf("HTTP/1.1"));   // end val
           processReq(request,start,ends);
           CALIBRATED=true;
        }      
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
      uint8_t i;
  for(i=0;i<NUM_OF_SONAR;i++){
   LIGHT[i]=1;
  } 
  }  
}

void controlLEDpannel(){ //control the lde pannel using the clients request
uint8_t i;
for(i=0;i<NUM_OF_SONAR;i++){
  if(LIGHT[i] == 1){
     digitalWrite(40+i,HIGH);  
  }else{
     digitalWrite(40+i,LOW);  
  }  
}  
}
void processReqLED(String request,uint8_t start,uint8_t ends){ //Process the request to control LED pannel
  uint8_t i,j=0;
  for(i=start;i<ends;i++){
     char buff[5];
     uint8_t c=0;
     while((request[i] != ':') && (request[i] != '\0')){       
       buff[c]=request[i];
       i++;
       c++;
     }
     buff[c]='\0';
     if(j<5){
        LIGHT[j]=atoi(buff);  //update the MMD- Maximum Moveble Distence 
        Serial.println(LIGHT[j]);
     }
     j++;  
  }   
}

void processReq(String request,uint8_t start,uint8_t ends){ //Process the request for the calibration data
  uint8_t i,j=0;
  for(i=start;i<ends;i++){
     char buff[5];
     uint8_t c=0;
     while((request[i] != ':') && (request[i] != '\0')){
       
       buff[c]=request[i];
       i++;
       c++;
     }
     buff[c]='\0';
     if(j<5){
        MMD[j]=atof(buff);  //update the MMD- Maximum Moveble Distence 
     }else{
        OBJ[j%5]=atoi(buff); //update object array
     }
     j++;  
  }

   Serial.println("Calibration finished...");
   
   for(i=0;i<=4;i++){
     Serial.print("sonar ");
     Serial.print(i+1);
     Serial.print(" : ");
     Serial.print(MMD[i]);
     Serial.print(" object code: ");
     Serial.println(OBJ[i]);
   }   
}

void skipRequest(WiFiClient client) {
  // an http request ends with a blank line
  boolean currentLineIsBlank = true;
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.write(c);
      // if you've gotten to the end of the line (received a newline
      // character) and the line is blank, the http request has ended,
      // so you can send a reply
      if (c == '\n' && currentLineIsBlank) {
        return;
      }
      if (c == '\n') {
        // you're starting a new line
        currentLineIsBlank = true;
      }
      else if (c != '\r') {
        // you've gotten a character on the current line
        currentLineIsBlank = false;
      }
    }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}


void sendHTTPHeader(WiFiClient client) { //HTTP header to the client

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/event-stream;charset=UTF-8");
  client.println("Connection: close");  // the connection will be closed after completion of the response
  client.println("Access-Control-Allow-Origin: *");  // allow any connection. We don't want Arduino to host all of the website ;-)
  client.println("Access-Control-Expose-Headers: *");
  client.println("Access-Control-Allow-Credentials: false");
  client.println("Cache-Control: no-cache");  // refresh the page automatically every 5 sec
  client.println();
  client.flush();

}

void serverSentEvent(WiFiClient client) { //Send server send event to the client side in JSON object notation
int s0,s1,s2,s3,s4;

s0=(sonar[0].convert_cm(sonar[0].ping_median(5)));
s1=(sonar[1].convert_cm(sonar[1].ping_median(5)));
s2=(sonar[2].convert_cm(sonar[2].ping_median(5)));
s3=(sonar[3].convert_cm(sonar[3].ping_median(5)));
s4=(sonar[4].convert_cm(sonar[4].ping_median(5)));

client.print("data: {");

  client.print("\"sonar1\":");
  client.print(s0);
  client.print(",");
  
  client.print("\"sonar2\":");
  client.print(s1);
  client.print(",");
  
  client.print("\"sonar3\":");
  client.print(s2);
  client.print(",");
  
  client.print("\"sonar4\":");
  client.print(s3);
  client.print(",");
  
  client.print("\"sonar5\":");
  client.print(s4);

client.print("}\n\n");
client.flush();
}

void sendInitialReadins(WiFiClient client) { //Send initial readings to the client
  findmaxdis(5);

  client.print("data: {");
  for (uint8_t i= 0;i < NUM_OF_SONAR; i++) {
  client.print("\"sonar");
  client.print(i+1);
  client.print("\":");
  client.print(max_dis[i]);
  Serial.print(max_dis[i]);
  Serial.print(" ");  
    if(i  !=  4){
      client.print(",");  
    } 
  }
  Serial.print("\n");
  client.print("}\n\n");  
  client.flush();
}

void findmaxdis(uint8_t rounds){  //Find maximum distance fo the room. i.e boundry of the room.
  
  //find maximum distance of the room a sensor can give at a given time
  //rounds : number o rounds
  for(uint8_t j=0; j < rounds; j++){
      for(uint8_t i =0; i<NUM_OF_SONAR; i++){
          delay(INTERVAL);
          max_dis[i]+=(sonar[i].convert_cm(sonar[i].ping_median(rounds)));
        if(j == rounds-1){
              max_dis[i]=max_dis[i]/rounds;
              if(max_dis[i]==0){
                 max_dis[i]=MAX_DISTANCE;
              }         
        }  
    }
  }
}

boolean  isvaliedmove(uint8_t prev, uint8_t curr){
  if(prev == NUM_OF_SONAR && ((curr == 0)||(curr == 1))){
     return true;  
  }else if(prev != NUM_OF_SONAR && ((prev-1 == curr)||(prev+1 == curr))){
     return true;
  }else{
     return false;
  }
} 

