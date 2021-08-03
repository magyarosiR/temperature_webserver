// Import required libraries
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>


#include <OneWire.h>
#include <DallasTemperature.h>

// Data connected to GPIO 13
#define ONE_WIRE_BUS 13

// LED Green and Red connected to GPIO 4 and 27
int LED_GREEN = 4;
int LED_RED = 27;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Own network credentials to ESP32 can connect to local network.
const char* ssid = "TP-LINK_18D4";
const char* password = "06397579";

// Define Output and State for connection between LED and WEB.
// To check the parameters passed on the URL (GPIO number and its state)
const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Global variable initialization
float temp = 0.0;

// Return value in C degrees
String readDSTemperatureC() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus

  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);
  
  if(tempC == -127.00) {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC); 
  }

  temp = tempC;

  return String(tempC);
}

String readDSTemperatureF() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
  float tempF = sensors.getTempFByIndex(0);

  if(int(tempF) == -196){
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Fahrenheit: ");
    Serial.println(tempF);
  }
  return String(tempF);
}


// WEBserver with HTML and JavaScript
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Current temperature</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Calibri;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .ds-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body style="background-color:#d2fdcc;">
  <h1>The current temperature in my Room:</h1>
  <p>
    <i class="fas fa-thermometer-half" style="color:#DC143C;"></i> 
    <span class="ds-labels">Temperature Celsius</span> 
    <span id="temperaturec">%TEMPERATUREC%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-half" style="color:#DC143C;"></i> 
    <span class="ds-labels">Temperature Fahrenheit</span>
    <span id="temperaturef">%TEMPERATUREF%</span>
    <sup class="units">&deg;F</sup>
  </p>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { 
    xhr.open("GET", "/update?output="+element.id+"&state=0", true);}
  xhr.send();
}
</script>

</body>

<p style="font-size:20px;"> The restart will begin in <span id="countdowntimer">10 </span> seconds</p>

<script type="text/javascript">
    var timeleft = 10;
    var downloadTimer = setInterval(function(){
    timeleft--;
    document.getElementById("countdowntimer").textContent = timeleft;
    if(timeleft == 0){
        timeleft = 10;
    }
    },1000);
    
</script>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturec").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturec", true);
  xhttp.send();
}, 10000) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperaturef").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperaturef", true);
  xhttp.send();
}, 10000) ;
</script>
</html>)rawliteral";

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}


// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATUREC"){
    return readDSTemperatureC();
  }
  else if(var == "TEMPERATUREF"){
    return readDSTemperatureF();
  }

  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Always on: </h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(LED_GREEN) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

void setup(){
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  pinMode(LED_RED, OUTPUT);

  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println();
  
  // Start up the DS18B20 library
  sensors.begin();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page, refreshing
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperaturec", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDSTemperatureC().c_str());
  });
  server.on("/temperaturef", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readDSTemperatureF().c_str());
  });


  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();

}
// the loop function runs over and over again forever
void loop() {

  if(temp>=25.00)
    digitalWrite(LED_RED, HIGH);
  else
    digitalWrite(LED_RED, LOW);
}
