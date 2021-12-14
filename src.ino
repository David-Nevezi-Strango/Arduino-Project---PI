
 /* Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */
 //Libraries used for this project
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <ArduinoJson.h>
#include <SD.h>
//pins used for this project
#define REDLED          2     //RED LED pin
#define YELLOWLED       3     //YELLOW LED pin
#define GREENLED        4     //GREEN LED pin
#define SENSORDOOR      A1    //Door Sensor pin
#define SENSORBUTTON    A0    //Button pin
#define BUZZER          5     //BUZZER pin
#define VOLUME          450   //Volume for the buzzer
#define SS_SD           6     //MicroSD Card Slot pin
#define RST_PIN         9     //RFID     // Configurable, see typical pin layout above
#define SS_PIN          10    //RFID     // Configurable, see typical pin layout above
const int rs = 8, en = 7, d4 = A2, d5 = A3, d6 = A4, d7 = A5; //pins for lcd

// variables will change:
int sensorDoorState = 0;         // variable for reading the sensor status
int sensorButtonState = 0;

boolean err = false; //error boolean
boolean doorOpened = false; //door boolean to check if door got opened via button or succesfull authentification

// initialize the libraries and variables for JSON and MicroSD Card Reader
File myFile;
StaticJsonDocument<256> json;
String jsonBuffer;
String uidString;

// initialize the libraries for RFID
MFRC522 rfid(SS_PIN, RST_PIN);
// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
   // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.print("Begin");
  lcd.setCursor(0,1);
  lcd.print("Initialization");
  delay(500);
  //initalize pins for primitive modules like buzzer, buttons, leds and door sensor
  pinMode(BUZZER, OUTPUT);
  pinMode(SENSORBUTTON, INPUT_PULLUP);
  pinMode(SENSORDOOR, INPUT_PULLUP);
  pinMode(REDLED, OUTPUT);
  pinMode(YELLOWLED, OUTPUT);
  pinMode(GREENLED, OUTPUT);
  Serial.begin(9600);    // Initialize serial communications with the PC -- for debug purposes
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  rfid.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details 
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  lcd.clear();
  Serial.println("Initializing connection to SD");
  lcd.print("SD Init");
  lcd.setCursor(0,1);
  pinMode(SS_SD, OUTPUT); 
  if (!SD.begin(SS_SD)) {
    Serial.println("Initialization to SD card failed");
    lcd.print("Failed");
    err = true;
    delay(500);
    return; 
  }

  Serial.println("Initialization succesfull");
  lcd.print("Successfull");
  delay(500);
  lcd.clear();
  lcd.print("Opening JSON");
  lcd.setCursor(0,1);
  myFile = SD.open("members.txt");
  if (myFile) {//parsing the JSON: reading the contents of the text file as string and then parsing it for JSON syntax
    Serial.println("Information from the json: "); 
    int i =0;
    while (myFile.available()) { //reading the contents character by character until ';' is found
     char c=myFile.read();
     if (c==';'){ //if end of JSON has been found
        Serial.println(jsonBuffer); //print out the contents on the Debug Screen
        deserializeJson(json, jsonBuffer); //deserializing the contents of the buffer into the JSON Object 
        DeserializationError error = deserializeJson(json, jsonBuffer); //error variable if errors has occured during parsing
        if (error) {
          Serial.println("Deserialization failed");
          lcd.clear();
          lcd.println("Deserialization");
          lcd.print("failed");
          err = true;
          delay(500);
          return;
          }
     }
     jsonBuffer.concat(c); //concatenating the characters into a JSON Buffer
    }
    myFile.close();
    
  }
  else { //if the system failed to open the JSON
    Serial.println("Failed to open json.txt");
    lcd.print("Failed");
    err = true;
    delay(500);
    return;
  }
  lcd.print("Successfull");
  delay(500);
}
void readRFID() {
    //function to read the hex code of the cards and also to verify if it is admitted or not
    rfid.PICC_ReadCardSerial();
    //UID of Card in a String variable
    uidString = String(rfid.uid.uidByte[0]) + " " + String(rfid.uid.uidByte[1]) + " " + 
    String(rfid.uid.uidByte[2]) + " " + String(rfid.uid.uidByte[3]);
    Serial.println(uidString);
    delay(500);
    //if UID is admitted 
    if(json.containsKey(uidString)){
      digitalWrite(GREENLED,HIGH);
      printMember();
      doorOpened = true;
      delay(2000);
      digitalWrite(GREENLED,LOW);
    }else{    
      printAccesDenied();
    }
    //flush uid String
    uidString.remove(0);
}
void printMember(){
    //lcd function to print admitted members using admitted cards and flash the green led
    Serial.print("Tag UID: ");
    Serial.println(uidString);
    Serial.println(json[uidString].as<const char*>());
    lcd.clear();
    lcd.print("WELCOME");
    lcd.setCursor(0, 1);
    lcd.print(json[uidString].as<const char*>());
  
}
void printIntruder(){
    //lcd function to print intruder alert, send buzzer signal and flash the red led
    lcd.clear();
    lcd.print("INTRUDER!!");
    digitalWrite(REDLED,HIGH);
    tone(BUZZER,VOLUME);
    delay(500);
    noTone(BUZZER);
    digitalWrite(REDLED,LOW);
}
void printAccesDenied(){
    //function to print access denied for unadmitted cards, send buzzer signal and flash the yellow led
    if(sensorDoorState ==0){
      //if door is not opened
      lcd.clear();
      lcd.println("ACCESS DENIED!!!");
      Serial.println("ACCESS DENIED!!!");
      digitalWrite(YELLOWLED,HIGH);
      tone(BUZZER,VOLUME);
      delay(500);
      noTone(BUZZER);
      digitalWrite(YELLOWLED,LOW);
    }else{
      printIntruder();
    }
    

}
void doorButtonOpened(){
    //function to deactivate system for 2 seconds if door button is pressed and light up the green led
    lcd.println("DOOR UNLOCKED!!!");
    Serial.println("DOOR UNLOCKED!!!");
    digitalWrite(GREENLED,HIGH);
    doorOpened = true;
    delay(2000);
    digitalWrite(GREENLED,LOW);
    lcd.clear();
}
void loop() {
  //if no error has occured
  if(!err){
  //read the state of button and door sensor
    sensorButtonState = analogRead(SENSORBUTTON);
    sensorDoorState = digitalRead(SENSORDOOR);
    if(doorOpened && sensorDoorState == 0){
        //if door is closed after the button was pressed or succesfull authentification, reset the boolean to false
        doorOpened = false;
    }
     if(sensorButtonState >512){
      //if door button was pressed, let the user open the door without starting the alarm
        doorButtonOpened();
     }else if(rfid.PICC_IsNewCardPresent()) {
      //read card data if RFID sensor is sensing a card
        readRFID();
     }else if(sensorDoorState !=0 && !doorOpened){
      //detecting intruder: opening the door without authentification
      printIntruder();
      return;
     }
    
    lcd.clear();
    Serial.print(sensorButtonState); //Debug: print the state of the door button
    Serial.print(" ");
    Serial.println(sensorDoorState); //Debug: print the state of the door
   }
}
