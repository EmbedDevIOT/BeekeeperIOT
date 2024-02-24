#include "Config.h"

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5        /* Time ESP32 will go to sleep (in seconds) */

// GPIO PINs
#define SET_PIN 18 // кнопкa Выбор
#define PL_PIN 19  // кнопкa Плюс
#define MN_PIN 5   // кнопкa Минус

#define RELAY 23 // Реле

#define DS_SENS 4 // ds18b20

#define TX_PIN 17 // tx
#define RX_PIN 16 // rx

#define HX_DT 25  // tx
#define HX_CLK 26 // rx

GlobalConfig Config;

// номер телефона в международном формате
String phone = "";
// часы
// DS3231 Clock;
bool h12 = false;
bool PM;
bool Century = false;
int16_t year, month, date, dow, hrs, mins, secs;
int16_t yearSet, monthSet, dateSet, dowSet, hourSet, minSet, secSet;

#define OLED_SPI_SPEED 8000000ul
RTC_DATA_ATTR int bootCount = 0;

GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> oled;
HX711 scale; //
SoftwareSerial SIM800(TX_PIN, RX_PIN);
OneWire DS(DS_SENS);
#define SEALEVELPRESSURE_HPA (1013.25)
// Adafruit_BME280 bme;

double vectors[8][3] = {{20, 20, 20}, {-20, 20, 20}, {-20, -20, 20}, {20, -20, 20}, {20, 20, -20}, {-20, 20, -20}, {-20, -20, -20}, {20, -20, -20}};
double perspective = 100.0f;
int deltaX, deltaY, deltaZ, iter = 0;
uint32_t timer;

void StartingInfo();

// int translateX(double x, double z);
// void rotateX(int angle);
// void rotateY(int angle);
// void rotateZ(int angle);
// void drawVectors();
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("Wakeup caused by timer");
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

void setup()
{
  Config.firmware = "0.2";
  Wire.begin();
  scale.begin(HX_DT, HX_CLK); // HX
  oled.init();                // инициализация
  StartingInfo();
}

void loop()
{
    if (scale.is_ready()) {
    scale.set_scale();    
    Serial.println("Tare... remove any weights from the scale.");
    delay(5000);
    scale.tare();
    Serial.println("Tare done...");
    Serial.print("Place a known weight on the scale...");
    delay(5000);
    long reading = scale.get_units(10);
    Serial.print("Result: ");
    Serial.println(reading);
  } 
  else {
    Serial.println("HX711 not found.");
  }
  delay(1000);
}

int translateX(double x, double z)
{
  return (int)((x + 64) + (z * (x / perspective)));
}

int translateY(double y, double z)
{
  return (int)((y + 32) + (z * (y / perspective)));
}

void rotateX(int angle)
{
  double rad, cosa, sina, Yn, Zn;
  rad = angle * PI / 180;
  cosa = cos(rad);
  sina = sin(rad);
  for (int i = 0; i < 8; i++)
  {
    Yn = (vectors[i][1] * cosa) - (vectors[i][2] * sina);
    Zn = (vectors[i][1] * sina) + (vectors[i][2] * cosa);
    vectors[i][1] = Yn;
    vectors[i][2] = Zn;
  }
}

void rotateY(int angle)
{
  double rad, cosa, sina, Xn, Zn;
  rad = angle * PI / 180;
  cosa = cos(rad);
  sina = sin(rad);
  for (int i = 0; i < 8; i++)
  {
    Xn = (vectors[i][0] * cosa) - (vectors[i][2] * sina);
    Zn = (vectors[i][0] * sina) + (vectors[i][2] * cosa);
    vectors[i][0] = Xn;
    vectors[i][2] = Zn;
  }
}

void rotateZ(int angle)
{
  double rad, cosa, sina, Xn, Yn;
  rad = angle * PI / 180;
  cosa = cos(rad);
  sina = sin(rad);
  for (int i = 0; i < 8; i++)
  {
    Xn = (vectors[i][0] * cosa) - (vectors[i][1] * sina);
    Yn = (vectors[i][0] * sina) + (vectors[i][1] * cosa);
    vectors[i][0] = Xn;
    vectors[i][1] = Yn;
  }
}

void drawVectors()
{
  oled.line(translateX(vectors[0][0], vectors[0][2]), translateY(vectors[0][1], vectors[0][2]), translateX(vectors[1][0], vectors[1][2]), translateY(vectors[1][1], vectors[1][2]));
  oled.line(translateX(vectors[1][0], vectors[1][2]), translateY(vectors[1][1], vectors[1][2]), translateX(vectors[2][0], vectors[2][2]), translateY(vectors[2][1], vectors[2][2]));
  oled.line(translateX(vectors[2][0], vectors[2][2]), translateY(vectors[2][1], vectors[2][2]), translateX(vectors[3][0], vectors[3][2]), translateY(vectors[3][1], vectors[3][2]));
  oled.line(translateX(vectors[3][0], vectors[3][2]), translateY(vectors[3][1], vectors[3][2]), translateX(vectors[0][0], vectors[0][2]), translateY(vectors[0][1], vectors[0][2]));
  oled.line(translateX(vectors[4][0], vectors[4][2]), translateY(vectors[4][1], vectors[4][2]), translateX(vectors[5][0], vectors[5][2]), translateY(vectors[5][1], vectors[5][2]));
  oled.line(translateX(vectors[5][0], vectors[5][2]), translateY(vectors[5][1], vectors[5][2]), translateX(vectors[6][0], vectors[6][2]), translateY(vectors[6][1], vectors[6][2]));
  oled.line(translateX(vectors[6][0], vectors[6][2]), translateY(vectors[6][1], vectors[6][2]), translateX(vectors[7][0], vectors[7][2]), translateY(vectors[7][1], vectors[7][2]));
  oled.line(translateX(vectors[7][0], vectors[7][2]), translateY(vectors[7][1], vectors[7][2]), translateX(vectors[4][0], vectors[4][2]), translateY(vectors[4][1], vectors[4][2]));
  oled.line(translateX(vectors[0][0], vectors[0][2]), translateY(vectors[0][1], vectors[0][2]), translateX(vectors[4][0], vectors[4][2]), translateY(vectors[4][1], vectors[4][2]));
  oled.line(translateX(vectors[1][0], vectors[1][2]), translateY(vectors[1][1], vectors[1][2]), translateX(vectors[5][0], vectors[5][2]), translateY(vectors[5][1], vectors[5][2]));
  oled.line(translateX(vectors[2][0], vectors[2][2]), translateY(vectors[2][1], vectors[2][2]), translateX(vectors[6][0], vectors[6][2]), translateY(vectors[6][1], vectors[6][2]));
  oled.line(translateX(vectors[3][0], vectors[3][2]), translateY(vectors[3][1], vectors[3][2]), translateX(vectors[7][0], vectors[7][2]), translateY(vectors[7][1], vectors[7][2]));
}

void StartingInfo()
{
  oled.clear();     // очистка
  oled.setScale(2); // масштаб текста (1..4)
  // oled.home();      // курсор в 0,0
  oled.setCursor(10, 0);
  oled.print("Beekeeper");
  delay(1000);
  oled.setScale(1);
  // курсор на начало 3 строки
  oled.setCursor(0, 3);
  oled.print("starting...");

  oled.clear();
  oled.update();
  byte textPos1 = 8;
  byte textPos2 = 32;

  oled.createBuffer(5, 0, 66, textPos2 + 8 + 2);

  oled.roundRect(5, textPos1 - 4, 65, textPos1 + 8 + 2, OLED_STROKE);
  oled.setCursorXY(10, textPos1);
  oled.print("SET MODE");

  oled.roundRect(5, textPos2 - 4, 65, textPos2 + 8 + 2, OLED_FILL);
  oled.setCursorXY(10, textPos2);
  oled.invertText(true);
  oled.print("LOL KEK");

  oled.sendBuffer();
  oled.update();
}