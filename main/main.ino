#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   32

#define OLED_RESET      -1
#define SCREEN_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int pixelPin = 12;
const int trigPin = 25; // Trigger Pin of Ultrasonic Sensor
const int echoPin = 26; // Echo Pin of Ultrasonic Sensor

bool logMessage = false;

char sbuf[10];

const int filter_len = 5;
long filterArray[filter_len] = {0, 0, 0, 0, 0};

const int buffer_len = 7;
int buffer[buffer_len] = {0, 0, 0, 0, 0, 0, 0};
int buffer_p = 0;

#define N_PIXELS  13

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, pixelPin, NEO_GRB + NEO_KHZ800);

#define FOREVER         for (;;)

#define O   strip.Color(0, 0, 0)
#define R   strip.Color(80, 0, 0)
#define G   strip.Color(0, 80, 0)

uint32_t bars[15][N_PIXELS] = {
  {O, O, O, O, O, O, O, O, O, O, O, O, O}, // 0
  {G, O, O, O, O, O, O, O, O, O, O, O, G}, // 1
  {G, G, O, O, O, O, O, O, O, O, O, G, G}, // 2
  {G, G, G, O, O, O, O, O, O, O, G, G, G}, // 3
  {G, G, G, G, O, O, O, O, O, G, G, G, G}, // 4
  {G, G, G, G, G, O, O, O, G, G, G, G, G}, // 5
  {G, G, G, G, G, G, O, G, G, G, G, G, G}, // 6
  {G, G, G, G, G, G, G, G, G, G, G, G, G}, // 7
  {G, G, G, G, G, G, R, G, G, G, G, G, G}, // 8
  {G, G, G, G, G, R, R, R, G, G, G, G, G}, // 9
  {G, G, G, G, R, R, R, R, R, G, G, G, G}, // 10
  {G, G, G, R, R, R, R, R, R, R, G, G, G}, // 11
  {G, G, R, R, R, R, R, R, R, R, R, G, G}, // 12
  {G, R, R, R, R, R, R, R, R, R, R, R, G}, // 13
  {R, R, R, R, R, R, R, R, R, R, R, R, R}, // 14
};

void setup() {
  Serial.begin(115200); // Starting Serial Terminal
  Serial.println(F("Power on!"));

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed! STOP."));
    FOREVER;
  }

  display.clearDisplay();
  display.display();

  strip.begin();
  strip.setBrightness(50);
  strip.show();

  pinMode(trigPin, OUTPUT);
  digitalWrite(trigPin, LOW);
  pinMode(echoPin, INPUT);
}

void loop() {
  long duration, distance;

  duration = get_filtered_duration();

  distance = add_and_get_average(microsecondsToCentimeters(duration));

  draw_distance(distance);

  delay(50);
}

long get_filtered_duration() {
  // Store multiple measurements
  for (uint8_t i = 0; i < filter_len; i++) {
    trigger();
    filterArray[i] = pulseIn(echoPin, HIGH);
    delay(20);
  }

  // Sort the measurements in ascending order
  for (uint8_t i = 0; i < filter_len-1; i++) {
    for (uint8_t j = i + 1; j < filter_len; j++) {
      if (filterArray[i] > filterArray[j]) {
        const long temp = filterArray[i];
        filterArray[i] = filterArray[j];
        filterArray[j] = temp;
      }
    }
  }

  // Discard outliers and calculate average
  long sum = 0;
  for (uint8_t i = 1; i < 4; i++) {
    sum += filterArray[i];
  }
  return sum / 3;
}

void bar(uint8_t bar_num) {
  for (uint16_t i = 0; i < N_PIXELS; i++) {
    strip.setPixelColor(i, bars[bar_num][i]);
  }
  strip.show();
}

void trigger() {
   digitalWrite(trigPin, HIGH);
   delayMicroseconds(10);
   digitalWrite(trigPin, LOW);
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

int add_and_get_average(const int cm) {
  buffer[buffer_p++] = cm;
  if (buffer_p >= buffer_len) {
    buffer_p = 0; 
  }

  int avg = 0;
  for (uint8_t i = 0; i < buffer_len; i++) {
    avg += buffer[i];
  }
  return avg / buffer_len;
}

void draw_distance(const int cm) {
  int previous_cm = 0;

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(4);

  if (cm > 150) {
    display.setCursor(8,4);
    sprintf(sbuf, "%3s", "SLOW");
    display.print(F(sbuf));
  } else if (cm < 11) {
    display.setCursor(8,4);
    display.print(F("STOP!"));
  } else {
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(16,4); // (12,11)
    sprintf(sbuf, "%3d", cm);
    display.print(F(sbuf));
  
    display.setTextSize(2);
    display.setCursor(100,4);
    display.print(F("cm"));
  }

  display.display();

  // Calculate the index into the bar table...
  int8_t bar_ptr = (-3 * cm + 175) / 10;
  if (bar_ptr < 0) {
    bar_ptr = 0;
  } else if (bar_ptr > 14) {
    bar_ptr = 14;
  }
  // .. and show the bar
  bar(bar_ptr);

  if (previous_cm != cm) {
    if (cm <= 56) {
      Serial.print(cm);
      Serial.println("cm");
    }
    previous_cm = cm;
  }
}
