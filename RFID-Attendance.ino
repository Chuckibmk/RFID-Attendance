#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// set the LCD address to 0x27 for 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
#define SS_PIN 10 // sda in rfid
#define  RST_PIN 9 // rst in rfid

#define LED_G 5
#define LED_R 4
#define LED_B 3

#define BUZZER 2
#define MAX_PROFILES 10

MFRC522 mfrc522(SS_PIN, RST_PIN); // create mfrc522 instance

struct Profile{
  String name;
  String uid;
};

Profile profiles[MAX_PROFILES];
int numProfiles = 0;

// Profile profiles[] = {
//   {"INTERN 1", "23 5F 89 1A"},
//   {"INTERN 2", "23 5F 89 1A"},
// };

// int numProfiles = sizeof(profiles) / sizeof(profiles[0]);

Servo myservo;

int pos = 0;

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
  myservo.attach(7);

  // Add sample profiles for initial testing
  // profiles[0] = {"STUDENT 1", "23 5F 89 1A"};
  // profiles[1] = {"STUDENT 2", "24 5F 89 1B"};
  // numProfiles = 2;
}
void setColor(int redValue, int greenValue, int blueValue){
  analogWrite(LED_R, redValue);
  analogWrite(LED_G, greenValue);
  analogWrite(LED_B, blueValue);
}
void loop() {
  // Check for incoming serial commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n'); // Read command from Python
    command.trim(); // Trim any leading or trailing whitespace
    handleCommand(command); // Process the command
  }

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
  for(int i = 0; i < numProfiles; i++){
    if(content.substring(1) == profiles[i].uid) // Change here the uid of the cards you want to give access
    {
      // lcd.print("STUDENT 01");
      lcd.print(profiles[i].name);
      lcd.setCursor(0,1);
      lcd.print("PRESENT");
      authenticateProfile(profiles[i].name);
      // show Green light
      myservo.write(120);
      setColor(0,  255, 0);
      tone(BUZZER, 500);
      delay(1000);
      // off the bulb
      myservo.write(0);
      setColor(0,  0, 0);
      noTone(BUZZER);
      lcd.clear();
      return;
    }      
  }
  lcd.print("UNAUTHORIZE");
  lcd.setCursor(0,1);
  lcd.print("ACCESS");
  // show Red Light
  
  setColor(255, 0, 0);
  tone(BUZZER, 300);
  delay(2000);
  // off the bulb
  setColor(0, 0, 0);
  noTone(BUZZER);
  lcd.clear();
}

// Function to handle commands from Python
void handleCommand(String command) {
  char cmd = command.charAt(0); // First character is the command

  if (cmd == 'r') {
    sendProfiles(); // Send the profiles back to Python
  }
  else if (cmd == 'c') {
    createProfile(command); // Create a new profile
  }
  else if (cmd == 'u') {
    updateProfile(command); // Update an existing profile
  }
  else if (cmd == 'd') {
    deleteProfile(command); // Delete a profile
  }
}

// Function to send profiles to Python

void sendProfiles() {
  Serial.println("START_PROFILES"); // Start marker
  delay(100);
  for (int i = 0; i < numProfiles; i++) {
    Serial.print(profiles[i].name);
    Serial.print(",");
    Serial.println(profiles[i].uid);
  }
  delay(100);
  Serial.println("END_PROFILES"); // End marker
}

// Function to create a new profile
void createProfile(String command) {
  if (numProfiles >= MAX_PROFILES) {
    Serial.println("Error: Max profiles reached");
    return;
  }

  int commaIndex = command.indexOf(',');
  if (commaIndex == -1) return;

  String name = command.substring(1, commaIndex);
  String uid = command.substring(commaIndex + 1);

  profiles[numProfiles].name = name;
  profiles[numProfiles].uid = uid;
  numProfiles++;

  Serial.println("Profile created");
}

// Function to update a profile
void updateProfile(String command) {
  int firstComma = command.indexOf(',');
  int secondComma = command.indexOf(',', firstComma + 1);
  
  if (firstComma == -1 || secondComma == -1) return;
  
  int index = command.substring(1, firstComma).toInt();
  if (index >= numProfiles || index < 0) {
    Serial.println("Error: Invalid profile index");
    return;
  }
  
  String name = command.substring(firstComma + 1, secondComma);
  String uid = command.substring(secondComma + 1);
  
  profiles[index].name = name;
  profiles[index].uid = uid;
  
  Serial.println("Profile updated");
}

// Function to delete a profile
void deleteProfile(String command) {
  int index = command.substring(1).toInt();
  if (index >= numProfiles || index < 0) {
    Serial.println("Error: Invalid profile index");
    return;
  }

  // Shift profiles down to remove the one at index
  for (int i = index; i < numProfiles - 1; i++) {
    profiles[i] = profiles[i + 1];
  }
  numProfiles--;

  Serial.println("Profile deleted");
}

// Check if the UID matches a profile and inform the Python script
void authenticateProfile(String name) {
  // for (int i = 0; i < numProfiles - 1; i++) {
  //   // if (profiles[i].indexOf(uid) != -1) {
  //     Serial.print("AUTH_SUCCESS");
  //     Serial.print(",");
  //     Serial.println(name);
  //     // Serial.println("," + profiles[i]);
  //     return;
  //   // }
  // }
  Serial.println("AUTH_SUCCESS," + name);
}