
#include "DisplayFunctions.h"
#include <Pangodream_18650_CL.h>
#include "GatewayLora.h"

#define I2C_ADDRESS_OF_SCREEN   0x3C

// String SECRET_SSID = "Trio";
// String SECRET_PASS = "DanaAmalAida";

// #define SECRET_SSID "TechPublic"
// #define SECRET_PASS ""
#define SECRET_SSID "Trio"
#define SECRET_PASS "DanaAmalAida"
// #define SECRET_SSID "wrong"
// #define SECRET_PASS "wrong"


std::list <PacketInfo> PacketsBuffer = std::list <PacketInfo>();
std::map <String, ANIMAL_TYPE> Nodes = std::map <String, ANIMAL_TYPE>(); //key = device_name (node)
                                           // data = animal_type (in node)

SSD1306 display(I2C_ADDRESS_OF_SCREEN, OLED_SDA, OLED_SCL);
Pangodream_18650_CL BL;

void onReceive(int packetSize) {
  String message = "";

  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  Serial.print("Gateway Receive: ");
  Serial.println(message);

  fillPacketsBuffer(&PacketsBuffer ,message,LoRa.packetRssi());
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.printf("Connecting to %s ", SECRET_SSID);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // Configure the LED an an output
  pinMode(LED_BUILTIN, OUTPUT);

  // Configure OLED by setting the OLED Reset HIGH, LOW, and then back HIGH
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, HIGH);
  delay(100);
  digitalWrite(OLED_RST, LOW);
  delay(100);
  digitalWrite(OLED_RST, HIGH);

  display.init();
  display.flipScreenVertically();

  showLogo(&display);
  delay(2000);

  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoRa Transmitter");
  display.display();
  delay(2000);


  //Initianting SD card
  spi.begin(SD_SCK,SD_MISO,SD_MOSI,SD_CS);
  delay(200);
  if (!SD.begin(SD_CS,spi)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  init_sdcard_log(&timeinfo);

  // Configure the LoRA radio
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  if (!LoRa.begin(LORA_BAND*1E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);
  }
  Serial.println("LoRa init succeeded.");
  
  LoRa.onReceive(onReceive);
  LoRa_rxMode();

}

void loop() {
  static int counter = 0;

  long rssi = WiFi.RSSI();

  display.clear();
  displayWifi(&display,rssi,(WiFi.status()!= WL_CONNECTED));
  displayNodeCount(&display,4,3,2);
  displayBattery(BL.getBatteryChargeLevel(),&display);

  // fillPacketsBuffer(&PacketsBuffer,"A5,050,123,78.2,0.67,3.3,11.3,73");

  //print on incoming nodes
  if(!PacketsBuffer.empty())
    displayPacketsBuffer(&PacketsBuffer,&display);
  
  // toggle the led to give a visual indication the packet was sent
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);

  counter++;
  delay(1500);
}
















