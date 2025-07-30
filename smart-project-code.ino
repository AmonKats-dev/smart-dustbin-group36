#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Pin definitions
#define TRIG_PIN 5
#define ECHO_PIN 18
#define TRIG2_PIN 14
#define ECHO2_PIN 27
#define SERVO_GENERAL_PIN 4
#define SERVO_PLASTIC_PIN 12
#define BUZZER_PIN 13
#define PLASTIC_SENSOR_PIN 32

#define GSM_RX 16
#define GSM_TX 17
HardwareSerial sim800(2);

Servo servoGeneral;
Servo servoPlastic;

bool messageSent = false;

void setup() {
  Serial.begin(115200);
  sim800.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIG2_PIN, OUTPUT);
  pinMode(ECHO2_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PLASTIC_SENSOR_PIN, INPUT);

  servoGeneral.setPeriodHertz(50);
  servoGeneral.attach(SERVO_GENERAL_PIN, 500, 2400);
  servoGeneral.write(0);

  servoPlastic.setPeriodHertz(50);
  servoPlastic.attach(SERVO_PLASTIC_PIN, 500, 2400);
  servoPlastic.write(0);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found");
    while (true);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Smart Bin System");
  display.display();
  delay(3000);

  waitForNetwork();
}

void waitForNetwork() {
  Serial.println("Checking network...");
  for (int i = 0; i < 10; i++) {
    sim800.println("AT+CREG?");
    delay(500);
    while (sim800.available()) {
      String response = sim800.readString();
      Serial.println("Network: " + response);
      if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1) {
        Serial.println("Network connected!");
        return;
      }
    }
    delay(1000);
  }
  Serial.println("No network!");
}

long readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return duration * 0.034 / 2;
}

void sendSMS(const String &number, const String &message) {
  Serial.println("Sending SMS to " + number);
  sim800.println("AT+CMGF=1"); // Set text mode
  delay(1000);
  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(1000);
  sim800.print(message);
  delay(500);
  sim800.write(26); // Ctrl+Z to send SMS
  Serial.println("SMS sent.");
}

void loop() {
  long handDistance = readDistance(TRIG_PIN, ECHO_PIN);
  long binDistance = readDistance(TRIG2_PIN, ECHO2_PIN);
  bool plasticDetected = digitalRead(PLASTIC_SENSOR_PIN) == LOW;

  display.clearDisplay();
  display.setCursor(0, 0);

  if (handDistance > 0 && handDistance < 20) {
    servoGeneral.write(90);
    display.println("Bin: OPEN");
    delay(2000);
    servoGeneral.write(0);
  } else {
    display.println("Bin: CLOSED");
  }

  display.setCursor(0, 16);
  display.print("Hand Dist: ");
  display.print(handDistance);
  display.println(" cm");

  display.setCursor(0, 28);
  if (plasticDetected) {
    display.println("Plastic: YES");
    servoPlastic.write(90);
    delay(2000);
    servoPlastic.write(0);
  } else {
    display.println("Plastic: NO");
  }

  display.setCursor(0, 40);
  if (binDistance > 0 && binDistance < 3) {
    display.println("!!! BIN FULL !!!");
    tone(BUZZER_PIN, 1000);
    messageSent = false;
  } else if (binDistance >= 3 && binDistance <= 5) {
    display.println("Bin almost full");
    tone(BUZZER_PIN, 500);
    if (!messageSent) {
      sendSMS("+256755876969", "You have received points, keep recycling");
      messageSent = true;
    }
  } else {
    display.println("Bin OK");
    noTone(BUZZER_PIN);
    messageSent = false;
  }

  display.setCursor(0, 54);
  display.print("Bin Dist: ");
  display.print(binDistance);
  display.println(" cm");

  display.display();
  delay(500);
}
