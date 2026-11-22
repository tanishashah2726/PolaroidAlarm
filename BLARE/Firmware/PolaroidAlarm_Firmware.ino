#include <WiFi.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

const char* ssid     = "Journey2Mars";
const char* password = "Rocky$Rani";

#define TFT_SCLK D8
#define TFT_MOSI D10
#define TFT_RST  D4
#define TFT_DC   D5
#define TFT_CS   D6
#define TFT_BL   D9

#define KEY_1      D0
#define KEY_2      D1
#define KEY_3      D2
#define KEY_4      D3
#define BUZZER_PIN D7

class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
      : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}
  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int alarmHour     = 6;
int alarmMin      = 0;
bool editHour     = true;
bool alarmEnabled = true;
bool alarmRinging = false;
bool isSnoozed    = false;
bool flashVisible = true;
int lastMin       = -1;

unsigned long ringStartTime   = 0;
unsigned long snoozeStartTime = 0;
unsigned long flashTimer      = 0;

int melody[]            = { 494, 659, 784, 740, 659, 988, 880, 740, 659, 784, 740, 622, 698, 494, 0 };
int noteMs[]            = { 300, 450, 150, 300, 600, 300, 900, 900, 450, 150, 300, 600, 300, 900, 500 };
int noteCount           = 15;
int currentNote         = 0;
unsigned long noteTimer = 0;

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);

  pinMode(KEY_1, INPUT_PULLUP);
  pinMode(KEY_2, INPUT_PULLUP);
  pinMode(KEY_3, INPUT_PULLUP);
  pinMode(KEY_4, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  tft.init(76, 284);
  tft.setOffsets(82, 18);
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(10, 28);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Connecting WiFi...");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  configTzTime("EST5EDT,M3.2.0,M11.1.0", "pool.ntp.org");
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  struct tm t;
  if (!getLocalTime(&t)) return;

  if (t.tm_min != lastMin) {
    lastMin = t.tm_min;
    if (alarmEnabled && !isSnoozed && t.tm_hour == alarmHour && t.tm_min == alarmMin) {
      startAlarm(t);
    }
    drawScreen(t);
  }

  if (alarmRinging) {
    if (millis() - ringStartTime >= 60000) {
      alarmRinging = false;
      isSnoozed = true;
      flashVisible = true;
      snoozeStartTime = millis();
      noTone(BUZZER_PIN);
      drawScreen(t);
    } else if (millis() - flashTimer >= 1000) {
      flashTimer = millis();
      flashVisible = !flashVisible;
      drawScreen(t);
    }
  }

  if (isSnoozed && (millis() - snoozeStartTime >= 300000)) {
    isSnoozed = false;
    startAlarm(t);
  }

  if (digitalRead(KEY_1) == LOW) {
    if (editHour) alarmHour = (alarmHour + 1) % 24;
    else alarmMin = (alarmMin + 1) % 60;
    drawScreen(t);
    delay(200);
  }

  if (digitalRead(KEY_2) == LOW) {
    if (editHour) alarmHour = (alarmHour + 23) % 24;
    else alarmMin = (alarmMin + 59) % 60;
    drawScreen(t);
    delay(200);
  }

  if (digitalRead(KEY_3) == LOW) {
    editHour = !editHour;
    drawScreen(t);
    delay(200);
  }

  if (digitalRead(KEY_4) == LOW) {
    if (alarmRinging || isSnoozed) {
      alarmRinging = false;
      isSnoozed = false;
      flashVisible = true;
      noTone(BUZZER_PIN);
    } else {
      alarmEnabled = !alarmEnabled;
    }
    drawScreen(t);
    delay(200);
  }

  if (alarmRinging) {
    if (millis() - noteTimer >= noteMs[currentNote]) {
      noteTimer = millis();
      currentNote = (currentNote + 1) % noteCount;
      if (melody[currentNote] == 0) noTone(BUZZER_PIN);
      else tone(BUZZER_PIN, melody[currentNote]);
    }
  }
}

void startAlarm(struct tm t) {
  alarmRinging = true;
  flashVisible = true;
  ringStartTime = millis();
  flashTimer = millis();
  currentNote = 0;
  noteTimer = millis();
  tone(BUZZER_PIN, *melody);
  drawScreen(t);
}

void drawScreen(struct tm t) {
  const int BUF_LEN = 32;
  char buf[BUF_LEN];

  tft.setCursor(10, 6);
  tft.setTextSize(5);
  if (alarmRinging && !flashVisible) {
    tft.fillRect(10, 6, 160, 40, ST77XX_BLACK);
  } else {
    tft.setTextColor(alarmRinging ? ST77XX_RED : ST77XX_WHITE, ST77XX_BLACK);
    sprintf(buf, "%02d:%02d", t.tm_hour, t.tm_min);
    tft.print(buf);
  }

  tft.setCursor(10, 52);
  tft.setTextSize(2);
  if (alarmRinging || isSnoozed) {
    tft.fillRect(10, 52, 270, 20, ST77XX_BLACK);
  } else {
    tft.setTextColor(alarmEnabled ? ST77XX_GREEN : ST77XX_YELLOW, ST77XX_BLACK);
    sprintf(buf, "ALM %02d:%02d %s %s", alarmHour, alarmMin, alarmEnabled ? "ON " : "OFF", editHour ? "[HR] " : "[MIN]");
    tft.print(buf);
  }
}
