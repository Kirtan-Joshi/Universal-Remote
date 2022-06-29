//***********INSTALLING THE LIBRARIES**********//
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>      //ESP8266 WIFI LIBRARY PROVIDES ESP8266 SPECIFIC WIFI ROUTINES AND WE ARE CALLING IT TO CONNECT TO THE NETWORK
#include<PubSubClient.h>      //PubSubClient ALLOWS A CLIENT TO PUBLISH/SUBSCRIBE MESSAGING WITH A MQTT SUPPORT SERVER
#include <Arduino.h>
#include <Hash.h>
#include <FS.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>


//********** ASSIGNING WIFI CREDENTIALS TO ESP **********//
String old_ssid = "redmi";   //PROVIDE SSID TO WHICH ESP WILL TRY TO CONNECT
String old_pwd = "Msdhonicsk7";           //PROVIDE PASSWORD OF THE SSID NETWORK


//********** DECLARING INPUT PARAMETER FOR WEB SERVER ***********//
const char* input_ssid = "ssid";        //THIS PARAMETER IS USED TO STORE INPUT SSID VALUE
const char* input_pwd = "pwd";          //THIS PARAMETER IS USED TO STORE INPUT PASSWORD VALUE


//********** DECLARING VARIABLES **********//
int n;                  //DEFINE VARIABLE 'n' AS INTEGER TYPE FOR STORING THE NUMBER OF NEARBY NETWORKS
String network[100];    //DEFINE 'network' AS STRING ARRAY OF MAX LENGTH = 100 FOR STORING THE SSID NAME OF NEARBY NETWORKS
String new_ssid;        //DEFINE 'new_ssid' AS STRING TYPE FOR STORING THE VALUE OF ENTERED SSID
String new_pwd;         //DEFINE 'new_pwd' AS STRING TYPE FOR STORING THE VALUE OF ENTERED PASSWORD


//********** CREATE AsyncWebServer OBJECT ON PORT 80 **********//
AsyncWebServer server(80);
  

//********** PROVIDING MQTT BROKER NAME **********//
const char* mqtt_server = "broker.mqtt-dashboard.com";


//********** CREATE espClient AS MQTT CLIENT  **********//
WiFiClient espClient;
PubSubClient client(espClient);


//********** DECLARE VARIABLES FOR IR TRANSMITTER **********//
const uint16_t kIrLed = D2;            //DEFINE PIN NUMBER TO WHICH IR TRANSMITTER IS CONNECTED
IRsend irsend(kIrLed);                 //SETS IR TRANSMITTING PIN AS A IR TRANSMITTER TO SEND THE IR SIGNAL

//***********************************************************************************************************//
//***************  HTML CODE OF WEB PAGE WHICH WILL TAKE SSID & PASSWORD AS INPUT FROM USER  ****************//
//***********************************************************************************************************//
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<style>
    h2 {
        align-content: center;
    }
    input[type=text],
    input[type=password] {
        width: 100%;
        padding: 5px 5px;
        margin: 4px 0;
        display: inline-block;
        border: 1px solid #ccc;
        box-sizing: border-box;
    }
    .main-block {
        margin-top: 150px;
        margin-bottom: auto;
        margin-left: auto;
        margin-right: auto;
        max-width: 340px;
        /* min-height: 460px; */
        padding: 10px 0;
        /* margin: auto; */
        border-radius: 5px;
        border: solid 1px #ccc;
        box-shadow: 1px 2px 5px rgba(0, 0, 0, .31);
        background: #ebebeb;
    }

    /* //set a style for the buttons// */

    button {
        background-color: #4CAF50;
        color: white;
        padding: 14px 20px;
        margin: 4px 0;
        border: none;
        cursor: pointer;
        width: 100%;
    }
    /*   set padding to the container */

    .container {
        padding: 16px;
    }
</style>

<body style="background-image: linear-gradient(to right,rgba(251, 192, 192), rgba(219, 28, 28, 0), rgb(251, 192, 192));">
      <div style=" margin-top: 150px; margin-bottom: auto; margin-left: auto; margin-right: auto;max-width: 340px; padding: 10px 0;border-radius: 5px;border: solid 1px #ccc;box-shadow: 1px 2px 5px rgba(0, 0, 0, .31);background: #ebebeb;">
        <h2 style=text-align:center>Enter your SSID and Password</h2>
        <!--Step 1 : Adding HTML-->
        <form action="/get">
            <div class="container">
                <label for="ssid"><b>SSID :</b></label>
  %OPTIONPLACEHOLDER%
  <br> 
                <label for="pwd" style="font-weight:bold ;">Password: </label>
                <input type="password" value="pwd" name="pwd" id="pwd"><br><br>
                <input type="checkbox" id="check" onclick="myFunction()"> <label for="check">Show Password</label>

                <script>
                    function myFunction() {
                        var x = document.getElementById("pwd");
                        if (x.type === "password") {
                            x.type = "text";
                        } else {
                            x.type = "password";
                        }
                    }
                </script>


                <!-- <label><b>Password :</b></label>
     <input type="password" placeholder="Enter Password" name="pwd" required>
                <hr> -->
                <br>
                <button type="submit">submit</button>
            </div>
    </div>

    </form>

</body>

</html>
)rawliteral";

//********** REPLACES OPTIONPLACEHOLDER WITH OPTION SECTION IN THE WEB PAGE **********//
String processor(const String& var){
  if(var == "OPTIONPLACEHOLDER"){
    String opts = "";
    opts += "<select name=\"ssid\" id=\"ssid\">" ;
    for(int i = 0; i < n; i++){
      opts += "<option required> " + network[i] +"</option>";
    }
    opts += "</select>"; 
    return opts;
  }
  return String();
}

//********** CREATE A FUNCTION TO SEARCH NEARBY NETWORKS AND STORING IT IN 'network' VARIABLE **********//
void myNetwork() {
  Serial.println(""); 
  Serial.println("Searching Wifi......");           //PRINT 'Searching Wifi......' ON SERIAL MONITOR
  n = WiFi.scanNetworks();                          //TO STORE THE NUMBER OF NEARBY NETWORKS IN 'n'       
  Serial.print("Networks Found:");                  //PRINT 'Networks Found:' ON SERIAL MONITOR
  Serial.print(n);                                  //PRINT NUMBER OF NETWORK ON SERIAL MONITOR
  Serial.println("");        
  for (int i = 0 ; i < n; i++) {                    
    Serial.print(i + 1);                         
    Serial.print(".");
    network[i] = WiFi.SSID(i).c_str();              //STORES THE NAME OF ith NETWORK IN ith POSITION OF 'network' VARIABLE IN STRING FORM 
    Serial.print(network[i]);                       //PRINT THE ith VALUE OF 'network' VARIABLE ON SERIAL MONITOR
    Serial.println("");                             //PRINT IN NEW LINE 
  }
    Serial.println("");
}

//********** CREATE A FUNCTION TO SEND 404 ERROR IF PAGE NOT FOUND **********//
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");      //SEND HTTP STATUS CODE 404 AND THE RESPONSE THAT THE PAGE NOT FOUND
}


//********** CREATE A STRING FOR READING THE DATA FROM EEPROM **********//
String read_file(fs::FS &fs, const char * path){    
  Serial.print("");
  Serial.printf("Reading file: %s\r\n", path);      //PRINT "Reading file:'NAME OF FILE'" ON SERIAL MONITOR 
  File file = fs.open(path, "r");                   //OPEN THE FILE 
  if(!file || file.isDirectory()){                  //IF FILE IS NOT OPENED OR IT IS NOT A FILE TYPE THEN EXECUTE THIS LOOP
    Serial.println("Empty file/Failed to open file"); //PRINT "Empty file/Failed to open file" ON SERIAL MONITOR 
    return String();                                //RETURNS THE STRING 
  }
  Serial.println("- read from file:");              //PRINT "- read from file:" ON SERIAL MONITOR
  String fileContent;                               //DECLARE A STRING VARIABLE NAMED 'fileContent'
  while(file.available()){                          //EXECUTE THIS LOOP UNTIL FILE IS AVAILABLE    
    fileContent+=String((char)file.read());         //STORE THE CONTENT OF FILE IN 'fileContent' VARIABLE
  }
  file.close();                                     //CLOSE THE FILE
  Serial.println(fileContent);                      //PRINT THE VALUE OF 'fileContent' VARIABLE ON SERIAL MONITOR
  return fileContent;                               //RETURN THE VALUE OF 'fileContent' VARIABLE
}


//********** CREATE A STRING FOR WRITING THE DATA IN EEPROM **********//
void write_file(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);           //PRINT "Writing file:'NAME OF FILE'" ON SERIAL MONITOR
  File file = fs.open(path, "w");                        //OPEN THE FILE
  if(!file){                                             //IF FILE IS NOT AVAILABLE THE EXECUTE THIS LOOP
    Serial.println("Failed to open file for writing");   //PRINT "Failed to open file for writing" ON SERIAL MONITOR
    return;
  }
  if(file.print(message)){                               //IF THE MESSAGE WHICH IS RECEIVED FROM WEB PAGE IS WRITTEN IN FILE THEN EXECUTE THE LOOP
    Serial.println("SUCCESS in writing file");           //PRINT "SUCCESS in writing file" ON SERIAL MONITOR
  }
  else {                                                 //IF THE MESSAGE WHICH IS RECEIVED FROM WEB PAGE WAS NOT WRITTEN IN FILE THEN EXECUTE THE LOOP
    Serial.println("FAILED to write file");              //PRINT "FAILED to write file" ON SERIAL MONITOR
  }
  file.close();                                          //CLOSE THE FILE
}





//********** CREATE A FUNCTION TO SETUP THE WIFI OF ESP **********//
void setup_wifi(){
 String myssid = read_file(SPIFFS, "/ssid.txt");      //THE CONTENT OF 'ssid.txt' FILE IS GIVEN TO 'myssid' STRING VARIABLE
  Serial.print("SSID : ");                            //PRINT "SSID : " ON SERIAL MONITOR
  Serial.println(myssid);                             //PRINT THE VALUE OF myssid VARIABLE ON SERIAL MONITOR
  
  String mypwd = read_file(SPIFFS, "/pwd.txt");       //THE CONTENT OF 'pwd.txt' FILE IS GIVEN TO 'mypwd' STRING VARIABLE
  Serial.print("PWD : ");                             //PRINT 'PWD : ' ON SERIAL MONITOR 
  Serial.println(mypwd);                              //PRINT THE VALUE OF mypwd VARIABLE ON SERIAL MONITOR
df:  while(WiFi.status() != WL_CONNECTED){            //THIS LOOP WILL EXECUTE UNTIL ESP CONNECTS TO WIFI
      Serial.println("");                             
      Serial.print("Configuring access point : ");   
      for(int i = 0; i < 1; i++){
       WiFi.begin(old_ssid,old_pwd);                  //ESP STARTS CONNECTING WITH old_ssid
       Serial.print(old_ssid);
       for(int i = 0; i < 20; i++){
         Serial.print(".");
         if(WiFi.status() == WL_CONNECTED){          //IF ESP IS CONNECTED WITH WIFI THEN EXECUTE THIS LOOP
          Serial.println("");
          Serial.print("Connected with : ");         //PRINT "Connected with : " ON SERIAL MONITOR
          Serial.print(old_ssid);                    //PRINT old_ssid ON SERIAL MONITOR 
          Serial.println("");                        //NEW LINE 
          Serial.print("IP Address: ");              //PRINT "IP Address: " ON SERIAL MONITOR 
          Serial.print(WiFi.localIP());              //PRINT THE IP ADDRESS OF CONNECTED WIFI ON SERIAL MONITOR
          goto df;                                   //GO TO df LINE
       }
       delay(500);                                   //DELAY OF 500 MILISECONDS
       }
     }
      for(int j = 0; j < 1 ;j++){
        Serial.println("");
        Serial.print("Configuring access point : ");
        WiFi.begin(myssid,mypwd);                    //ESP STARTS CONNECTING WITH myssid
        Serial.print(myssid);
        for(int j = 0; j < 20 ;j++){
          Serial.print(".");
          if(WiFi.status() == WL_CONNECTED){        //IF ESP IS CONNECTED WITH WIFI THEN EXECUTE THIS LOOP
            Serial.println("");
            Serial.print("Connected with : ");      //PRINT "Connected with : " ON SERIAL MONITOR
            Serial.print(myssid);                   //PRINT myssid ON SERIAL MONITOR 
            Serial.println("");                     //NEW LINE
            Serial.print("IP Address: ");           //PRINT "IP Address: " ON SERIAL MONITOR
            Serial.print(WiFi.localIP());           //PRINT THE IP ADDRESS OF CONNECTED WIFI ON SERIAL MONITOR
            goto df;                                //GO TO df LINE
          }
        delay(500);                                 //DELAY OF 500 MILISECONDS
        } 
    }
   delay(1000);     
  }
}

//********** CREATE A FUNCTION TO RECEIVE MESSAGES FROM MQTT BROKER **********//
void callback(char* topic,byte* message, unsigned int length) 
{
  Serial.print("Message arrived [");   //PRINT "Message arrived [" ON SERIAL MONITOR 
  Serial.print(topic);                 //PRINT topic NAME ON SERIAL MONITOR 
  Serial.print("] ");                  //PRINT ']' ON SERIAL MONITOR           
  for (int i = 0; i < length; i++)     
  {
    Serial.print((char)message[i]);   //PRINT THE MESSAGE ON SERIAL MONITOR WHICH IS RECEIVED BY ESP CLIENT 
    irsend.sendNEC((char)message[i]); //SEND THE IR SIGNAL FROM IR TRANSMITTER
  }
  Serial.println();
}


//********** CREATE A FUNCTION TO CONNECT WITH MQTT CLIENT **********//
void reconnect()
{
df:  int counter = 0;                                 //SET THE COUNTER VALUE ZERO  
     while (!client.connected())                      //THIS LOOP WILL EXECUTE UNTIL THE ESP CONNECTS WITH MQTT BROKER
     {
       if (counter==5)                                //IF THE COUNTER VALUE EQUALS TO 5 THEN THIS LOOP WILL BE EXECUTED 
       {
         ESP.restart();                               //RESTART THE ESP MODULE
       }
     counter += 1;                                    //INCREMENT THE COUNTER VALUE BY 1 
     Serial.print("Attempting MQTT connection..."); //PRINT "Attempting MQTT connection..." ON SERIAL MONITOR
    // ATTEMPTING TO CONNECT ESP WITH MQTT BROKER
   
     if (client.connect("7cf38fe7-7a58-4ce0-8dc7-2e0377635c8d","RENESYS" , "123456"))     //IF CLIENT IS CONNECTED WITH MQTT BROKER THEN EXECUTE THIS LOOP 
     {
      Serial.println("");
      Serial.print("connected");                   //PRINT "connected" ON SERIAL MONITOR
      client.subscribe("SMIT/mqtt");               //ESP CLIENT WILL SUBSCRIBE TO "SMIT/mqtt" TOPIC
      goto df;
     } 
     else                                           //IF ESP CLIENT IS NOT CONNECTED WOTH MQTT BROKER THEN EXECUTE THIS LOOP
     {
      Serial.print("failed, rc=");                 //PRINT "failed, rc=" ON SERIAL MONITOR    
      Serial.print(client.state());                //PRINT THE STATUS OF ESP AND MQTT BROKER CONNECTION
      Serial.println(" try again in 5 seconds");  //PRINT "try again in 5 seconds" ON SERIAL MONITOR
      delay(5000);    //DELAY OF 5 SECONDS
      // WAITS FOR 5 SECONDS BEFORE RETRYING
     }
   } 
} 

//******************************************************************************//
//******************************  SETUP FUNCTION  ******************************//
//******************************************************************************// 
void setup() {
  Serial.begin(9600);                  //INITIALIZE SERIAL MONITOR AND SETS THE BAUD RATE OF 9600
  if(!SPIFFS.begin()){                 //IF SPIFFS DOES NOT STARTED THEN EXECUTE THIS LOOP
      Serial.println("An Error has occurred while mounting SPIFFS");  //PRINT "An Error has occurred while mounting SPIFFS" ON SERIAL MONITOR
      return;
    }

  setup_wifi();                        //CALL THE 'setup_wifi' FUNCTION
  myNetwork();                         //CALL 'myNetwork' FUNCTION

//*********** START THE WEB SERVER **********//
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);    //SENDS THE HTTP STATUS CODE 200 AND SEND THE HTML CODE FOR WEB PAGE BY DECLARING THE CONTENT TYPE IS HTML
  });

  // SEND A GET REQUEST TO <ESP_IP>/update?input_ssid=<new_ssid>&input_pwd=<new_pwd>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {

    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(input_ssid) && request->hasParam(input_pwd)) {       //IF GET REQUEST HAS THE VALUE OF BOTH PARAMETER THEN EXECUTE THIS LOOP
      new_ssid = request->getParam(input_ssid)->value();                       //STORE THE VALUE OF 'input_ssid' PARAMETER IN 'new_ssid' VARIABLE
      new_pwd = request->getParam(input_pwd)->value();                         //STORE THE VALUE OF 'input_pwd' PARAMETER IN 'new_pwd' VARIABLE
      write_file(SPIFFS, "/ssid.txt", new_ssid.c_str());                       //WRITE THE VALUE OF 'new_ssid' IN 'ssid.txt' FILE
      write_file(SPIFFS, "/pwd.txt", new_pwd.c_str());                         //WRITE THE VALUE OF 'new_pwd' IN 'pwd.txt' FILE
      Serial.println("ssid : ");                                               //PRINT "ssid : " ON SERIAL MONITOR 
      Serial.print(new_ssid);                                                  //PRINT THE VALUE OF 'new_ssid' ON SERIAL MONITOR
      Serial.print("");
      Serial.println("pwd :");                                                 //PRINT "pwd : " ON SERIAL MONITOR
      Serial.print(new_pwd);                                                   //PRINT THE VALUE OF 'new_pwd' ON SERIAL MONITOR
      
    }
    else {                                                                     //IF GET REQUEST DOES NOT HAVE THE VALUE OF BOTH PARAMETER THEN EXECUTE THIS LOOP
      new_ssid = "No message sent";                                            //GIVE "No message sent" VALUE TO new_ssid                                  
      new_pwd = "No message sent";                                             //GIVE "No message sent" VALUE TO new_pwd 
      Serial.println("ssid : ");                                               //PRINT "ssid : " ON SERIAL MONITOR 
      Serial.print(new_ssid);                                                  //PRINT THE VALUE OF 'new_ssid' ON SERIAL MONITOR
      Serial.println("pwd :");                                                 //PRINT "pwd : " ON SERIAL MONITOR
      Serial.print(new_pwd);                                                   //PRINT THE VALUE OF 'new_pwd' ON SERIAL MONITOR
    }

    request->send(200, "text/html", "<br><a href=\"/\">Return to Home Page</a>");  //SEND THE REQUEST WITH HTTP STATUS CODE 200(OK) AND THE LINK OF PREVIOUS PAGE WITH HTML CONTENT TYPE  
  });
  server.onNotFound(notFound);                             //IF THE WEB PAGE WAS NOT FOUND THEN CALL THE notFound() FUNCTION
  server.begin();                                          //START THE SERVER
  
  client.setServer(mqtt_server, 1883); //PROVIDES MQTT SERVER NAME AND PORT NUMBER TO ESP CLIENT  
  client.setCallback(callback);        //CALL THE callback FUNCTION 
  irsend.begin();                      //INITIALIZ IR TRANSMITTER FOR TRANSMITTING IR SIGNAL 
}

//*****************************************************************************//
//******************************  LOOP FUNCTION  ******************************//
//*****************************************************************************//
void loop() {
if(WiFi.status() == WL_CONNECTED){    //IF ESP IS CONNECTED WITH WIFI THEN EXECUTE THIS LOOP
  if(!client.connected()){           //IF ESP CLIENT IS NOT CONNECTED WITH MQTT BROKER THE EXECUTE THIS LOOP           
      reconnect();                    //CALL THE reconnect FUNCTION
    }
  client.loop();                   
  delay(1000);                       //DELAY OF 1 SECONDS
 }                         
}
