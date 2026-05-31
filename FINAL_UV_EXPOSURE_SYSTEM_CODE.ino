#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =======================
// Pins
// =======================
#define BTN_INC 2
#define BTN_DEC 3
#define BTN_EXP 4
#define AUX_OUT 6

// Current sensing
#define SHUNT_PIN A0

// Photodiode TIA output
#define PD_PIN A1

// =======================
// Shunt resistor
// =======================
float Rshunt = 5.0;
float correction = 1.0;

// =======================
// Photodiode parameters
// =======================
float feedbackResistor = 110000.0; // 110k
float responsivity = 0.08;         // A/W @ 405nm

// =======================
// Timer variables
// =======================
float setSeconds = 35.0;
float totalSeconds = 35.0;

bool isRunning = false;

unsigned long exposureStartTime = 0;

// =======================
// LCD refresh timer
// =======================
unsigned long lcdTimer = 0;
const int lcdInterval = 200;

// =======================
// Double click detection
// =======================
unsigned long lastIncPress = 0;
unsigned long lastDecPress = 0;

const int doubleClickDelay = 300;

// =======================
// Filtered values
// =======================
float filteredCurrent = 0;
float filteredPower = 0;

// =======================
// Read Average
// =======================
float readAverage(int pin) {

  float sum = 0;

  for (int i = 0; i < 10; i++) {

    sum += analogRead(pin);

    delay(2);
  }

  return sum / 10.0;
}

// =======================
// Time Display
// =======================
void printTime(float totalSec) {

  lcd.print(totalSec, 1);
  lcd.print("s   ");
}

// =======================
// Setup
// =======================
void setup() {

  pinMode(BTN_INC, INPUT_PULLUP);
  pinMode(BTN_DEC, INPUT_PULLUP);
  pinMode(BTN_EXP, INPUT_PULLUP);

  pinMode(AUX_OUT, OUTPUT);

  digitalWrite(AUX_OUT, LOW);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("UV System Ready");

  delay(1000);

  lcd.clear();
}

// =======================
// Main Loop
// =======================
void loop() {

  // =======================
  // Read LED Current
  // =======================
  float shuntRaw = readAverage(SHUNT_PIN);

  float Vshunt =
    shuntRaw * (5.0 / 1023.0);

  float ledCurrent =
    (Vshunt / Rshunt) * correction;

  // Filter current
  filteredCurrent =
    0.8 * filteredCurrent +
    0.2 * ledCurrent;

  // =======================
  // Read Photodiode Voltage
  // =======================
  float pdRaw = readAverage(PD_PIN);

  float pdVoltage =
    pdRaw * (5.0 / 1023.0);

  // =======================
  // Optical Power Calculation
  // =======================

  // Photodiode current
  float Ipd =
    pdVoltage / feedbackResistor;

  // Optical power in Watts
  float opticalPower_W =
    Ipd / responsivity;

  // Convert to mW
  float opticalPower_mW =
    opticalPower_W * 1000.0;

  // Filter optical power
  filteredPower =
    0.8 * filteredPower +
    0.2 * opticalPower_mW;

  // =======================
  // SET MODE
  // =======================
  if (!isRunning) {

    digitalWrite(AUX_OUT, LOW);

    // LCD refresh control
    if (millis() - lcdTimer >= lcdInterval) {

      lcdTimer = millis();

      // -------------------
      // Top line
      // -------------------
      lcd.setCursor(0, 0);

      lcd.print("I:");
      lcd.print((int)(filteredCurrent * 1000));
      lcd.print("mA ");

      lcd.print("P:");
      lcd.print(filteredPower, 2);
      lcd.print("mW ");

      // -------------------
      // Bottom line
      // -------------------
      lcd.setCursor(0, 1);

      lcd.print("Set:");
      printTime(setSeconds);
    }

    // =======================
    // Increment Button
    // =======================
    if (digitalRead(BTN_INC) == LOW) {

      unsigned long now = millis();

      // Double press = +1 sec
      if (now - lastIncPress <
          doubleClickDelay) {

        setSeconds += 1.0;

        lastIncPress = 0;
      }

      // Single press = +0.1 sec
      else {

        setSeconds += 0.1;

        lastIncPress = now;
      }

      delay(150);
    }

    // =======================
    // Decrement Button
    // =======================
    if (digitalRead(BTN_DEC) == LOW) {

      unsigned long now = millis();

      // Double press = -1 sec
      if (now - lastDecPress <
          doubleClickDelay) {

        if (setSeconds >= 1.0)
          setSeconds -= 1.0;

        lastDecPress = 0;
      }

      // Single press = -0.1 sec
      else {

        if (setSeconds >= 0.1)
          setSeconds -= 0.1;

        lastDecPress = now;
      }

      delay(150);
    }

    // =======================
    // Start Exposure
    // =======================
    if (digitalRead(BTN_EXP) == LOW &&
        setSeconds > 0) {

      delay(200);

      totalSeconds = setSeconds;

      isRunning = true;

      exposureStartTime = millis();

      lcd.clear();
    }
  }

  // =======================
  // RUN MODE
  // =======================
  else {

    digitalWrite(AUX_OUT, HIGH);

    // =======================
    // Accurate Real-Time Timer
    // =======================
    float elapsedTime =
      (millis() - exposureStartTime)
      / 1000.0;

    float remainingTime =
      setSeconds - elapsedTime;

    if (remainingTime < 0.0)
      remainingTime = 0.0;

    totalSeconds = remainingTime;

    // =======================
    // LCD refresh control
    // =======================
    if (millis() - lcdTimer >= lcdInterval) {

      lcdTimer = millis();

      // -------------------
      // Top line
      // -------------------
      lcd.setCursor(0, 0);

      lcd.print("I:");
      lcd.print((int)(filteredCurrent * 1000));
      lcd.print("mA ");

      lcd.print("P:");
      lcd.print(filteredPower, 2);
      lcd.print("mW ");

      // -------------------
      // Bottom line
      // -------------------
      lcd.setCursor(0, 1);

      lcd.print("Run:");
      printTime(totalSeconds);
    }

    // =======================
    // Stop Exposure
    // =======================
    if (remainingTime <= 0.0) {

      totalSeconds = 0.0;

      isRunning = false;

      digitalWrite(AUX_OUT, LOW);

      lcd.clear();
    }
  }
}