#include <LiquidCrystal.h>
#include <MFRC522.h> // for the RFID
#include <SPI.h> // for the RFID and SD card module
#include <SD.h> // for the SD card
#include <RTClib.h> // for the RTC

LiquidCrystal lcd(3, 2, A0, A1, A2, A3); // initialize the library with the numbers of the interface pins
//   The circuit:
//  * LCD RS pin to digital pin 3
//  * LCD Enable pin to digital pin 2
//  * LCD D4 pin to digital pin A0
//  * LCD D5 pin to digital pin A1
//  * LCD D6 pin to digital pin A2
//  * LCD D7 pin to digital pin A3

// define pins for RFID
#define CS_RFID 10
#define RST_RFID 9
// define select pin for SD card module
#define CS_SD 4 
 
// Create a file to store the data
File myFile;
 
// Instance of the class for RFID
MFRC522 rfid(CS_RFID, RST_RFID); 
 
// Variable to hold the tag's UID
String uidString;
 
// Instance of the class for RTC
RTC_DS1307 rtc;
 
// Define check in time
const int checkInHour = 14;
const int checkInMinute = 30;
 
//Variable to hold user check in
int userCheckInHour;
int userCheckInMinute;
 
// Pins for LEDs and buzzer
const int redLED1 = 6;
const int redLED2 = 7;
const int buzzer = 5;
 
void setup() {
  
  // Set LEDs and buzzer as outputs
  pinMode(redLED1, OUTPUT);  
  pinMode(redLED2, OUTPUT);
  pinMode(buzzer, OUTPUT);
  
  // Init Serial port
  Serial.begin(9600); // open the serial port at 9600 bps:
  lcd.begin(16,2); // starting the 16*2 LCD display
  while(!Serial); // Adding this line makes the board pause until you open the serial port, so you get to see that initial bit of data. Needed for native USB port only like Leonardo/Micro/Zero
  
  SPI.begin(); // starting the serial peripherial interface
  rfid.PCD_Init(); // Init MFRC522 card (PCD means: proximity coupling device)
 
  // Setup for the SD card
  Serial.print("Initializing SD card...");
  lcd.print("Initializing ");
  lcd.setCursor(0, 1); // set the cursor to column 0, line 1
  lcd.print("SD card...");
  delay(2000); // wait for 3 seconds
  lcd.clear(); // clear the lcd
  
  if(!SD.begin(CS_SD)) { // SD.begin(ss_pin) initializes the SD library and card. It initializes the SPI bus, which is used for communication between Arduino and SD card. For SPI interface, CS_SD is the pin connected to the SS (slave select) pin in our case.  ss_pin is optional, if not set, the hardware SS pin is used.
    Serial.println("initialization failed!");
    lcd.print("Initialization ");
    lcd.setCursor(0, 1);
    lcd.print("failed!");
    return; // end of setup
  }
  Serial.println("initialization done.");
  lcd.print("Initialization ");
  lcd.setCursor(0, 1);
  lcd.print("Done...");
 
  // Setup for the RTC  
  if(!rtc.begin()) { // real time clock check
    Serial.println("Couldn't find RTC");
    lcd.clear();
    lcd.print("Couldn't find RTC");
    while(1);                                          // when an error is caught, this method will fire and we will get time to reset the Arduino, hence stopping program to get into any more trouble
  }
  else {
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  if(!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    lcd.clear();
    lcd.print("RTC Not Running!");
  }
}
 
void loop() {
  //look for new cards
  delay(1000);
  lcd.clear();
  lcd.print("Tap your ID Card");
  if(rfid.PICC_IsNewCardPresent()) { // Proximity Integrated Circuit Card
    readRFID();
    logCard();
    verifyCheckIn();
  }
  delay(10);
}
 
void readRFID() {
  rfid.PICC_ReadCardSerial(); // reading serial number of the card present 
  lcd.clear();
  Serial.print("Tag UID: "); // Unified Information Devices
  lcd.print("Tag UID: ");
  for(int i=0;i<4;i++){ 
    uidString+=String(rfid.uid.uidByte[i]);
    uidString+=i!=3?" ":"";
  }
  // for (byte i = 0; i < 4; i ++) {
  //   uidString += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX) + (i != 3 ? ":" : "" );
  // } // to get the result in nn:nn:nn:nn format
  Serial.println(uidString);
  lcd.setCursor(0, 1);
  lcd.print(uidString);
  delay(1000);
 
  // Sound the buzzer when a card is read
  tone(buzzer, 2000); // it sends a 2kHz sound signal to pin 9
  delay(1000); // pause the program for 1000 ms
  noTone(buzzer); // stop the signal sound
  delay(200);
}
 
void logCard() {
  // Enables SD card chip select pin
  digitalWrite(CS_SD,LOW);
  
  // Open file
  myFile=SD.open("DATA.txt", FILE_WRITE);
 
  // If the file opened ok, write to it
  if (myFile) {
    Serial.println("File opened ok");
    lcd.clear();
    lcd.print("File opened ok");
    delay(1000);
    myFile.print(uidString);
    myFile.print(", ");   
    
    // Save time on SD card
    DateTime now = rtc.now();
    myFile.print(now.year(), DEC);
    myFile.print('/');
    myFile.print(now.month(), DEC);
    myFile.print('/');
    myFile.print(now.day(), DEC);
    myFile.print(',');
    myFile.print(now.hour(), DEC);
    myFile.print(':');
    myFile.println(now.minute(), DEC);
    
    // Print time on Serial monitor
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.println(now.minute(), DEC);
    Serial.println("sucessfully written on SD card");
 
    // Print time on lcd 
    lcd.clear();
    lcd.print(now.year(), DEC);
    lcd.print(':');
    lcd.print(now.month(), DEC);
    lcd.print(':');
    lcd.print(now.day(), DEC);
    lcd.print(' ');
    lcd.setCursor(11, 0);
    lcd.print(now.hour(), DEC);
    lcd.print(':');
    lcd.print(now.minute(), DEC);
    lcd.setCursor(0, 1);
    lcd.print("Written on SD...");
    delay(1000);
    
    myFile.close();
 
    // Save check in time;
    userCheckInHour = now.hour();
    userCheckInMinute = now.minute();
  }
  else {
    
    Serial.println("error opening data.txt");  
    lcd.clear();
    lcd.print("error opening data.txt");
  }
  // Disables SD card chip select pin  
  digitalWrite(CS_SD,HIGH);
}
 
void verifyCheckIn(){
  if((userCheckInHour < checkInHour)||((userCheckInHour==checkInHour) && (userCheckInMinute <= checkInMinute))){
    digitalWrite(redLED1, HIGH);
    delay(2000);
    digitalWrite(redLED1,LOW);
    Serial.println("You're on time!");
    lcd.clear();
    lcd.print("You're on time!");
  }
  else{
    digitalWrite(redLED2, HIGH);
    delay(2000);
    digitalWrite(redLED2,LOW);
    Serial.println("You are late...");
    lcd.clear();
    lcd.print("You are Late...");
    delay(2000);
    lcd.clear();
    
  }
  
}
