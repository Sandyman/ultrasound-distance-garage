#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_SCREEN_WIDTH    128
#define OLED_SCREEN_HEIGHT   32

#define OLED_RESET           -1
#define OLED_SCREEN_ADDRESS  0x3C

Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &Wire, OLED_RESET);

const int TrigPin = 25; // Trigger Pin of Ultrasonic Sensor
const int EchoPin = 26; // Echo Pin of Ultrasonic Sensor

bool logMessage = false;

char sbuf[10];

const int filter_len = 5;
long filterArray[filter_len] = {0, 0, 0, 0, 0};

const int buffer_len = 7;
int buffer[buffer_len] = {0, 0, 0, 0, 0, 0, 0};
int buffer_p = 0;

#define N_PIXELS  13

const int PixelPin = 12;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, PixelPin, NEO_GRB + NEO_KHZ800);

#define FOREVER         for (;;)

#define O   strip.Color(0, 0, 0)
#define R   strip.Color(255, 0, 0)
#define G   strip.Color(0, 255, 0)

uint32_t bars[15][N_PIXELS] = {
  {O, O, O, O, O, O, O, O, O, O, O, O, O}, //  0 - too far
  {G, O, O, O, O, O, O, O, O, O, O, O, G}, //  1
  {G, G, O, O, O, O, O, O, O, O, O, G, G}, //  2
  {G, G, G, O, O, O, O, O, O, O, G, G, G}, //  3
  {G, G, G, G, O, O, O, O, O, G, G, G, G}, //  4
  {G, G, G, G, G, O, O, O, G, G, G, G, G}, //  5
  {G, G, G, G, G, G, O, G, G, G, G, G, G}, //  6
  {G, G, G, G, G, G, G, G, G, G, G, G, G}, //  7
  {G, G, G, G, G, G, R, G, G, G, G, G, G}, //  8
  {G, G, G, G, G, R, R, R, G, G, G, G, G}, //  9
  {G, G, G, G, R, R, R, R, R, G, G, G, G}, // 10
  {G, G, G, R, R, R, R, R, R, R, G, G, G}, // 11
  {G, G, R, R, R, R, R, R, R, R, R, G, G}, // 12
  {G, R, R, R, R, R, R, R, R, R, R, R, G}, // 13
  {R, R, R, R, R, R, R, R, R, R, R, R, R}, // 14 - too close!
};

void setup() {
  Serial.begin(115200); // Starting Serial Terminal
  Serial.println(F("Power on!"));

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed! STOP."));
    FOREVER;
  }

  display.clearDisplay();
  display.display();

  strip.begin();
  strip.setBrightness(10);
  strip.show();

  pinMode(TrigPin, OUTPUT);
  digitalWrite(TrigPin, LOW);
  pinMode(EchoPin, INPUT);

  FOREVER {
    long duration, distance;

    duration = get_filtered_duration();
    distance = add_and_get_average(microsecondsToCentimeters(duration));
    show_distance(distance);

    delay(20);
  }
}

// Not used - preparing for deep sleep
void loop() {}

void ultrasound_trigger() {
   digitalWrite(TrigPin, HIGH);
   delayMicroseconds(10);
   digitalWrite(TrigPin, LOW);
}

long get_filtered_duration() {
  // Store multiple measurements
  for (uint8_t i = 0; i < filter_len; i++) {
    ultrasound_trigger();
    filterArray[i] = pulseIn(EchoPin, HIGH);
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

void display_setup() {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(4);
}

void display_slow() {
  display.setCursor(8,4);
  sprintf(sbuf, "%3s", "SLOW");
  display.print(F(sbuf));

  display.display();
}

void display_stop() {
  display.setCursor(8,4);
  display.print(F("STOP!"));

  display.display();
}

void display_distance(const int cm) {
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16,4);
  sprintf(sbuf, "%3d", cm);
  display.print(F(sbuf));

  display.setTextSize(2);
  display.setCursor(100,4);
  display.print(F("cm"));

  display.display();
}

void draw_bar(uint8_t bar_num) {
  for (uint16_t i = 0; i < N_PIXELS; i++) {
    strip.setPixelColor(i, bars[bar_num][i]);
  }
  strip.show();
}

void display_bar(const int cm) {
  // Calculate the index into the bar table...
  int8_t bar_ptr = (-3 * cm + 175) / 10;
  if (bar_ptr < 0) {
    bar_ptr = 0;
  } else if (bar_ptr > 14) {
    bar_ptr = 14;
  }

  // .. and draw the bar
  draw_bar(bar_ptr);
}

void show_distance(const int cm) {
  static int previous_cm = 0;

  display_setup();
  if (cm > 150) {
    display_slow();
  } else if (cm < 11) {
    display_stop();
  } else {
    display_distance(cm);
  }

  display_bar(cm);

  if (previous_cm != cm) {
    if (cm <= 56) {
      Serial.print(cm);
      Serial.println("cm");
    }
    previous_cm = cm;
  }
}
