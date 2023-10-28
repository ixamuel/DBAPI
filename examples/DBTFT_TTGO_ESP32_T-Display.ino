#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyXML.h>
#include <TFT_eSPI.h> // Include the graphics library

const char* ssid = "SSID";
const char* password = "password";
const char* DB_Client_Id = "5edc7e052decd1a1d863575651d33382";
const char* DB_Api_Key = "fccfbd3abe12d4d38f5a53b5a48b1f82";

TFT_eSPI tft = TFT_eSPI(); // Create an instance of the library

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi.");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient

    http.begin("https://apis.deutschebahn.com/db-api-marketplace/apis/timetables/v1/fchg/8004799");  //Specify request destination
    http.addHeader("Accept", "application/xml"); //Specify request headers
    http.addHeader("DB-Client-Id", DB_Client_Id);
    http.addHeader("DB-Api_Key", DB_Api_Key);

    int httpCode = http.GET(); //Send the request

    if (httpCode > 0) { //Check the returning code
      String payload = http.getString();   //Get the request response payload

      TinyXMLDocument doc; // Create a TinyXMLDocument object
      if (doc.Parse(payload.c_str()) == XML_SUCCESS) { // Parse the XML payload
        XMLElement* timetable = doc.RootElement();

        int trainCount = 0;
        for (XMLElement* s = timetable->FirstChildElement("s"); s != NULL; s = s->NextSiblingElement("s")) {
          XMLElement* ar = s->FirstChildElement("ar");
          XMLElement* dp = s->FirstChildElement("dp");
          if (ar && dp && ar->IntAttribute("l") == 74 && dp->IntAttribute("l") == 74) {
            String arTime = ar->Attribute("ct");
            String dpTime = dp->Attribute("ct");

            tft.setCursor(0, trainCount * 20);
            tft.println("RB74 " + arTime.substring(6,8) + ":" + arTime.substring(8,10) + " " + dpTime.substring(6,8) + ":" + dpTime.substring(8,10));

            trainCount++;
            if (trainCount >= 3) break;
          }
        }
      } else {
        Serial.println("Failed to parse XML!");
      }
    }

    delay(10000); // Wait for 10 seconds before next request
  }
}
