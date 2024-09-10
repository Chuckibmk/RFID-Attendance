#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// set the LCD address to 0x27 for 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define SS_PIN 10 // sda in rfid
#define  RST_PIN 9 // rst in rfid

#define LED_G 5
#define LED_R 4
#define LED_B 3

#define BUZZER 2

MFRC522 mfrc522(SS_PIN, RST_PIN); // create mfrc522 instance

struct Profile{
  String name;
  String uid;
};

Profile interns[] = {
  {"INTERN 1", "23 5F 89 1A"},
  {"INTERN 2", "23 5F 89 1A"},
};

const int numStudents = sizeof(interns) / sizeof(interns[0]);

void setup() {
  Serial.begin(9600); // Initiate a serial communication
  SPI.begin(); //Initiate SPI bus
  mfrc522.PCD_Init(); 
  lcd.init();
  lcd.backlight(); // turn on the blacklight and print a message.
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);  
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);
}
void setColor(int redValue, int greenValue, int blueValue){
  analogWrite(LED_R, redValue);
  analogWrite(LED_G, greenValue);
  analogWrite(LED_B, blueValue);
}
void loop() {
  // look for new cards
  if (! mfrc522.PICC_IsNewCardPresent()){
    lcd.setCursor(3,0);
    lcd.print("SHOW YOUR");
    lcd.setCursor(4,1);
    lcd.print("ID CARD");
    return;
  }else{
    lcd.clear();
  }
  // select one of the cards
  if(!mfrc522.PICC_ReadCardSerial()){
    return;
  }
  // show UID on serial monitor
  Serial.print("UID tag :");
  String content = "";
  byte letter;
  for(byte i = 0; i < mfrc522.uid.size; i++){
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);

    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  content.toUpperCase();
  for(int i = 0; i < numStudents; i++){
    if(content.substring(1) == interns[i].uid) // Change here the uid of the cards you want to give access
    {
      lcd.print("STUDENT 01");
      lcd.setCursor(0,1);
      lcd.print("PRESENT");
      // digitalWrite(LED_G, HIGH);
      setColor(0,  255, 0);
      tone(BUZZER, 500);
      delay(1000);
      // digitalWrite(LED_G, LOW);
      setColor(0,  0, 0);
      noTone(BUZZER);
      lcd.clear();
    }else
    {
      lcd.print("UNAUTHORIZE");
      lcd.setCursor(0,1);
      lcd.print("ACCESS");
      // digitalWrite(LED_R, HIGH);
      setColor(255, 0, 0);
      tone(BUZZER, 300);
      delay(2000);
      // digitalWrite(LED_R, LOW);
      setColor(0, 0, 0);
      noTone(BUZZER);
      lcd.clear();
    }
  }
}
