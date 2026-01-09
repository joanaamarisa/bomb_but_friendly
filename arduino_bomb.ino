#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// -------------------------- LCD --------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// -------------------- Keypad 4x4 -------------------------
const byte ROWS = 4; 
const byte COLS = 4;  

char keys[ROWS][COLS] = {
  {'1','2','3','A'}, 
  {'4','5','6','B'}, 
  {'7','8','9','C'}, 
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {2, 3, 4, 5}; 
byte colPins[COLS] = {6, 7, 8, 9}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ------------------- SHARED PINS -------------------
const int redBtn = A3; 
const int blueBtn = A2; 
const int greenBtn = A1;

const int ledRed = 10; 
const int ledYellow = 11; 
const int ledGreen = 12;

const int buzzer = 13;

// ------------------- GAME VARIABLES -------------------
enum GameState { 
  IDENTIFICATION, 
  CRACK_CODE, 
  SIMON_SAYS, 
  CUT_WIRE, 
  EXPLODED, 
  DEFUSED,
  DONE 
};

GameState currentState = IDENTIFICATION;

String secretCode = ""; 
String userInput = ""; 
String studentID = "";

int simonSequence[5]; 
int simonPos = 0;
int sounds[3] = {262, 330, 392};

// -------------------------- SETUP --------------------------
void setup() {
  Serial.begin(115200);
  
  // entropy for random numbers
  randomSeed(analogRead(0)); 
  
  lcd.init(); 
  lcd.backlight();
  
  pinMode(redBtn, INPUT); 
  pinMode(blueBtn, INPUT); 
  pinMode(greenBtn, INPUT);
  
  pinMode(ledRed, OUTPUT); 
  pinMode(ledYellow, OUTPUT); 
  pinMode(ledGreen, OUTPUT);
  
  pinMode(buzzer, OUTPUT);
  
  startSequence();
}

// -------------------------- LOOP --------------------------
void loop() {
  checkIncomingSerial();
  
  // main game progression logic
  switch (currentState) {
    case IDENTIFICATION: 
      handleID(); 
      break;
    case CRACK_CODE:     
      handleCrack(); 
      break;
    case SIMON_SAYS:     
      handleSimon(); 
      break;
    case CUT_WIRE:       
      handleCut();   
      break;
    case EXPLODED:       
      handleExplosion(); 
      break;
    case DEFUSED:
      break;  
    case DONE:           
      break;
  }
}

// ------------------- HELPERS -------------------

void startSequence() {
  studentID = ""; 
  userInput = ""; 
  secretCode = ""; 
  simonPos = 0;
  
  digitalWrite(ledRed, LOW); 
  digitalWrite(ledGreen, LOW); 
  noTone(buzzer);
  
  lcd.clear(); 
  lcd.print("BOMB ACTIVE");
  
  delay(1500); 
  
  lcd.clear(); 
  lcd.print("Agent ID:");
  
  currentState = IDENTIFICATION;
}

void checkIncomingSerial() {
  if (Serial.available() > 0) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    
    // explosion signal from website timer
    if (msg == "BOOM") {
      currentState = EXPLODED;
    }
    
    // reset signal from website button
    if (msg == "RESET") {
      startSequence(); 
    }
  }
}

// -------------------------
// PHASE 0: IDENTIFICATION
// -------------------------
void handleID() {
  char key = keypad.getKey();

  if (key) {
    // backspace
    if (key == '*' && studentID.length() > 0) {
      studentID.remove(studentID.length() - 1);
      lcd.setCursor(studentID.length(), 1);
      lcd.print(" "); 
      return;
    }
  }
  
  if (key >= '0' && key <= '9' && studentID.length() < 8) {
    studentID += key;
    lcd.setCursor(studentID.length() - 1, 1); 
    lcd.print(key);
    
    if (studentID.length() == 8) {
      // calculate code based on last 4 digits
      long last4 = studentID.substring(4).toInt();
      long res = (last4 % 2 == 0) ? (last4 * 2) : (last4 / 2);
      secretCode = String(res);
      
      while (secretCode.length() < 4) {
        secretCode = "0" + secretCode;
      }
      
      if (secretCode.length() > 4) {
        secretCode = secretCode.substring(0, 4);
      }
      
      Serial.println("START"); 
      currentState = CRACK_CODE;
      
      lcd.clear(); 
      lcd.print("Access Code:"); 
      
      delay(1000); 
      lcd.clear();
    }
  }
}

// -------------------------
// PHASE 1: CRACK THE CODE
// -------------------------
void handleCrack() {
  lcd.setCursor(0, 0); 
  lcd.print("Insert Code:");
  
  char key = keypad.getKey();
  
  if (key) {
    // backspace
    if (key == '*' && userInput.length() > 0) {
      userInput.remove(userInput.length() - 1);
      lcd.setCursor(4 + userInput.length(), 1); 
      lcd.print(" "); 
      return;
    }

    userInput += key;
    lcd.setCursor(0, 1); 
    lcd.print("In: ");
    lcd.print(userInput);
    
    if (userInput.length() == 4) {
      if (userInput == secretCode) {
        winFX(); 
        currentState = SIMON_SAYS;
        
        for (int i = 0; i < 5; i++) {
          simonSequence[i] = random(0, 3);
        }
        
        lcd.clear(); 
        lcd.print("Simon Says..."); 
        delay(1500);
      } 
      else {
        failFX("-10s"); 
        Serial.println("PENALTY:10");
        userInput = ""; 
        lcd.clear();
      }
    }
  }
}

// -------------------------
// PHASE 2: SIMON SAYS
// -------------------------
void handleSimon() {
  lcd.clear(); 
  lcd.print("Observe..."); 
  delay(800);
  
  for (int i = 0; i <= simonPos; i++) {
    checkIncomingSerial();
    
    if (currentState != SIMON_SAYS) {
      return;
    }
    
    playSimonStep(simonSequence[i], 400);
  }
  
  lcd.clear(); 
  lcd.print("Your turn!");
  
  for (int i = 0; i <= simonPos; i++) {
    int btn = -1;
    
    while (btn == -1) { 
      checkIncomingSerial();
      
      if (currentState != SIMON_SAYS) {
        return;
      }
      
      btn = getBtn(); 
    }
    
    playSimonStep(btn, 300);
    
    if (btn != simonSequence[i]) {
      failFX("-10s"); 
      Serial.println("PENALTY:10");
      simonPos = 0; 
      return; 
    }
    
    while (getBtn() != -1) {
      // wait for release
    }
  }
  
  simonPos++;
  
  if (simonPos == 5) {
    winFX(); 
    currentState = CUT_WIRE;
    
    lcd.clear(); 
    lcd.print("Final Cut!"); 
    delay(1500);
  } 
  else { 
    delay(500); 
  }
}

// -------------------------
// PHASE 3: CUT THE WIRE
// -------------------------
void handleCut() {
  lcd.setCursor(0, 0); 
  lcd.print("STATION UNSTABLE");
  lcd.setCursor(0, 1); 
  lcd.print("CHECK WEBSITE...");
  
  int btn = getBtn();
  if (btn != -1) {
    Serial.println("TIME_REQ");
    delay(600); // waits for esp to process

    if (btn == 0) { // red button -> EVEN
        Serial.println("WIN"); 
        lcd.clear(); lcd.print("BOMB DEFUSED");
        currentState = DONE;
    } 
    else if (btn == 1) { // blue button -> ODD
        Serial.println("WIN");
        lcd.clear(); lcd.print("BOMB DEFUSED");
        currentState = DONE;
    }
    else {
        Serial.println("PENALTY:15");
        failFX("-15s");
    }
    
    while (getBtn() != -1);
  }
}

// -------------------------
// EXPLOSION
// -------------------------
void handleExplosion() {
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print("   BOOM!!!    ");
  
  lcd.setCursor(0, 1); 
  lcd.print(" GAME OVER ");
  
  digitalWrite(ledRed, HIGH);
  tone(buzzer, 100); 
  
  delay(5000); 
  
  digitalWrite(ledRed, LOW);
  noTone(buzzer);
  
  currentState = DONE; 
}


int getBtn() {
  if (digitalRead(redBtn) == HIGH) {
    return 0;
  }
  
  if (digitalRead(blueBtn) == HIGH) {
    return 1;
  }
  
  if (digitalRead(greenBtn) == HIGH) {
    return 2;
  }
  
  return -1;
}

void playSimonStep(int idx, int dur) {
  int led = (idx == 0) ? ledRed : (idx == 1) ? ledYellow : ledGreen;
  
  tone(buzzer, sounds[idx], dur);
  digitalWrite(led, HIGH); 
  
  delay(dur); 
  
  digitalWrite(led, LOW); 
  noTone(buzzer);
}

void winFX() {
  digitalWrite(ledGreen, HIGH); 
  tone(buzzer, 800, 200); 
  delay(250); 
  digitalWrite(ledGreen, LOW);
}

void failFX(String p) {
  lcd.clear(); 
  lcd.print("ERROR! ");
  lcd.print(p);
  
  digitalWrite(ledRed, HIGH); 
  tone(buzzer, 150, 600); 
  
  delay(1000); 
  digitalWrite(ledRed, LOW);
}