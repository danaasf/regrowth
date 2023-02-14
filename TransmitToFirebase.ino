#include <FirebaseESP32.h>
#include <WiFi.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// #define WIFI_SSID "Trio"
// #define WIFI_PASSWORD "DanaAmalAida"
#define WIFI_SSID "Pilot2"
#define WIFI_PASSWORD "carolteresafarah"
// #define WIFI_SSID "TechPublic"
// #define WIFI_PASSWORD ""

#define USER_EMAIL "iot.regrowth@gmail.com"
#define USER_PASSWORD "Regrowth123"

/* 2. Define the API Key */
#define API_KEY "0vh0uCSsK39x2AUAAavXKk8cRfkcGrFck3rpc6gf"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://regrowth-c498e-default-rtdb.europe-west1.firebasedatabase.app/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app


//Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String main="";

class PacketReceivedInfo{
public:
    String device_name;
    String rfid_reading; //animal tag (mainly used as number of the chicken, it's string because we have to check if its 0, because it indicates a low battery level, checking it is easier as string)
    int animal_weight; 
    float temperature;
    float humidity;
    double voltage; 
    double soc; //state of charge - battery
    int RSSI; //signal power 
    String time_received;
    bool low_battery_report;
  PacketReceivedInfo(String device_name, String rfid_reading, int animal_weight, float temperature, float humidity,double voltage, double soc, int RSSI, String time_received, bool low_battery_report): device_name(device_name), 
  rfid_reading(rfid_reading), animal_weight(animal_weight), temperature(temperature), humidity(humidity),voltage(voltage), soc(soc), RSSI(RSSI),time_received(time_received),low_battery_report(low_battery_report){}
};


unsigned long previousMillis = 0;
const long interval = 43200000; // interval at which to send data (12 hours in milliseconds)

void setup()
{
  Serial.begin(115200);
  delay(2000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);

}

void parseTime(String str, String &date, String &time, String &timeRange) {
  int hour = 0;
  int minute = 0;
  int second = 0;
  //Mon Feb 06 18:33:13 2023
  sscanf(str.c_str(), "%*s %*s %*d %d:%d:%d %*d", &hour, &minute, &second);
  date = str.c_str() + 4;
  date = date.substring(0, 6) + " " + date.substring(15,20);
  time = String(hour) + ":" + String(minute) + ":" + String(second);
  timeRange = (hour >= 12) ? "PM" : "AM";
}


void loop()
{  
  unsigned long currentMillis = millis();
  if ((currentMillis - previousMillis) %interval == 0) {

  if (Firebase.ready()) 
  { 
  String email= USER_EMAIL; 
  int atIndex = email.indexOf("@");
  String user = email.substring(0, atIndex);
  
  PacketReceivedInfo packet("A1","12836",2373,13,0.67,3.3,0.9,28,"Mon Feb 06 18:33:13 2023",false);
  String numOfNode= packet.device_name;


  /** Adding the node, to the node list, with the default animal type (chicken) **/
    String animalType;
    Firebase.getString(fbdo, "/test/Users/"+user+ "/Node/" +numOfNode);
    if (animalType == NULL) {
      animalType="Chicken";
      Firebase.setString (fbdo, "/test/Users/" +user+ "/Node/" +numOfNode, animalType);
    } 


  /** Actual parameters are taken from the class above - not all fields are needed **/ 


    int weight=packet.animal_weight;
    int activity=0; //initial value of the activity is 1 always, and once transmitted to the firebase it is added by one, so it is initialized to 0 - default state 
    float humidity=packet.humidity;
    double temperature= packet.temperature;
    String animalNumber= packet.rfid_reading;
    float connection=packet.RSSI;
    float battery=packet.soc;
    double voltage= packet.voltage;
    String date;
    String time;
    String timeRange;
    parseTime(packet.time_received, date, time, timeRange);
    String GWId= WiFi.macAddress();
    bool batteryLow= (((packet.rfid_reading).toInt())==0);


    /** Transmitting to the related fields **/ 
    
    Firebase.setFloat(fbdo, "/test/Users/" +user+ "/Data/Animal/" + animalType+ "/"+ animalNumber + "/" + date +"/Weight", weight);
    activity++;
    Firebase.setFloat(fbdo, "/test/Users/" +user+ "/Data/Animal/" + animalType+ "/"+ animalNumber + "/" + date +"/Activity", activity);
    int count= Firebase.getInt(fbdo, "/test/Users/"+user+ "/Date/Animal/" + animalType + "/Number");
    count++;
    Firebase.setInt (fbdo, "/test/Users/"+ user+ "/Data/Animal/"+ animalType+"/Number",count );
    Firebase.setDouble(fbdo, "/test/Users/" +user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" +date+ "/" +timeRange + "/Humidity", humidity);
    Firebase.setDouble(fbdo, "/test/Users/" +user + "/Enviroment/Animal/" + animalType + "/" + animalNumber + "/Date/" +date+ "/" +timeRange + "/Temperature" , temperature);

    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Battery", battery ); 
    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Connection", connection ); 
    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Tension", voltage ); 
    Firebase.setString (fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/Gateway", GWId ); 
    Firebase.setBool(fbdo, "/test/Users/" +user + "/Animal/" + animalType+ "/Node/" + numOfNode + "/BatteryLow", batteryLow ); 

    

    delay(200);
    Serial.printf("Get Weight  %s\n", Firebase.getFloat(fbdo, "/test/Users/UChicken/RfidTag/UCurrentDate/Weight") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    weight=fbdo.to<float>();
    delay(200);
    Serial.printf("Get Activity  %s\n", Firebase.getFloat(fbdo, "/test/Users/UChicken/RfidTag/UCurrentDate/Activity") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
    activity=fbdo.to<float>();
     delay(200);
    Serial.printf("Get Humidity  %s\n", Firebase.getFloat(fbdo, "/test/Users/Enviroment/CurrentDate/AM/Humidity") ? String(fbdo.to<double>()).c_str() : fbdo.errorReason().c_str());
    humidity=fbdo.to<double>();


//     Serial.printf("Get Device Name  %s\n", Firebase.getString(fbdo, "/test/device_name") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
//     device_name=fbdo.to<String>();
//    Serial.printf("Get Animal Name %s\n", Firebase.getString(fbdo, "/test/animal_ID") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
//     animal_ID=fbdo.to<String>();
//    Serial.printf("Get Animal Type  %s\n", Firebase.getString(fbdo, "/test/animal_type") ? String(fbdo.to<String>()).c_str() : fbdo.errorReason().c_str());
//     animal_type=fbdo.to<String>();
//     Serial.printf("Get Animal Weight  %s\n", Firebase.getInt(fbdo, "/test/animal_weight") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
//     animal_weight=fbdo.to<int>();
//      Serial.printf("Get Temperature  %s\n", Firebase.getInt(fbdo, "/test/temperature") ? String(fbdo.to<int>()).c_str() : fbdo.errorReason().c_str());
//     temperature=fbdo.to<int>();
//      Serial.printf("Get Humidity  %s\n", Firebase.getFloat(fbdo, "/test/humidity") ? String(fbdo.to<float>()).c_str() : fbdo.errorReason().c_str());
//     humidity=fbdo.to<float>();
     
     

  Serial.println(); 
//   Serial.print("name:");
//  Serial.print(device_name); 
//  Serial.print("ID:");
//  Serial.print(animal_ID);
//  Serial.print("  type: ");
//  Serial.print(animal_type);
   Serial.print("  weight: ");
  Serial.print(weight);
//   Serial.print("  temperature: ");
//  Serial.print(temperature);
//   Serial.print("  humidity: ");
//  Serial.print(humidity);
  
  Serial.println();
  Serial.println("------------------");
  Serial.println();
  

  delay(2500);
  }
  }
}
