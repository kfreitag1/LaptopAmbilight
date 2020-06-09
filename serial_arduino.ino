#include <FastLED.h>
#include <ESP8266WiFi.h>

// CONSTANTS
#define DATA_PIN 6
#define INPUT_PIN D4
#define MAX_NUM_LEDS 127
#define CURRENT_DRAW 300
#define ACKNOWLEDGE 6

// VARIABLES
CRGBArray<MAX_NUM_LEDS> leds = {};
CRGBArray<MAX_NUM_LEDS> tempLeds = {};

byte NUM_LEDS;
byte FADE_SPEED;

byte reset_counter = 0;
bool preinit_tag = false;
byte preinit_sin_counter = 0;
bool startup_complete = false;
int startup_counter = 0;
bool started_init = false;
bool completed_init = false;
byte current_init_index = 0;

int ledCounter = 0;
byte pixelIndex = 0;

void resetAll() {
  reset_counter = 0;
  preinit_tag = false;
  preinit_sin_counter = 0;
  startup_complete = false;
  startup_counter = 0;
  started_init = false;
  completed_init = false;
  current_init_index = 0;
  ledCounter = 0;
  pixelIndex = 0;
}

// Helper function that blends one uint8_t toward another by a given amount
void nblendU8TowardU8( uint8_t& cur, const uint8_t target, uint8_t amount)
{
  if( cur == target) return;
  
  if( cur < target ) {
    uint8_t delta = target - cur;
    delta = scale8_video( delta, amount);
    cur += delta;
  } else {
    uint8_t delta = cur - target;
    delta = scale8_video( delta, amount);
    cur -= delta;
  }
}

// Blend one CRGB color toward another CRGB color by a given amount.
// Blending is linear, and done in the RGB color space.
// This function modifies 'cur' in place.
CRGB fadeTowardColor( CRGB& cur, const CRGB& target, uint8_t amount)
{
  nblendU8TowardU8( cur.red,   target.red,   amount);
  nblendU8TowardU8( cur.green, target.green, amount);
  nblendU8TowardU8( cur.blue,  target.blue,  amount);
  return cur;
}

// The main data gathering initialization loop
void initializationLoop() {
  switch(current_init_index) {
    case 0:
      NUM_LEDS = Serial.read();
      break;
    case 1:
      FADE_SPEED = Serial.read();

      // Finished initialization
      completed_init = true;
      Serial.print(NUM_LEDS);

      FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
      FastLED.setMaxPowerInVoltsAndMilliamps(3.3, CURRENT_DRAW);

      fill_solid(leds, MAX_NUM_LEDS, CRGB::Black);
      FastLED.show();
      FastLED.delay(10);
      break;
    default: 
//          digitalWrite(INPUT_PIN, LOW);
      break;
  }
  current_init_index++;
}

// Main function to read the date from the serial and push it to the LEDs
void readAndSetLEDs() {
  reset_counter++;
  
  while(Serial.available()){
    tempLeds[ledCounter][pixelIndex] = Serial.read();
  
    pixelIndex++;
    if(pixelIndex >= 3) {
      pixelIndex = 0;
      ledCounter++;
      
      if(ledCounter >= NUM_LEDS) {
        ledCounter = 0;
        reset_counter = 0;
      }
    }
  }

  for(int i = 0; i < NUM_LEDS; i++) {
    fadeTowardColor(leds[i], tempLeds[i], startup_complete ? FADE_SPEED : 1);
  }
}

// SETUP FUNCTION
void setup() {
  Serial.setTimeout(500);
  Serial.begin(115200);
  while(!Serial){};

  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  
  pinMode(INPUT_PIN, OUTPUT);
  digitalWrite(INPUT_PIN, HIGH);
}

// LOOP FUNCTION
void loop() {
  if(completed_init) {
    
    // Main loop functions
    if(!startup_complete){ startup_counter++; if(startup_counter >= 50){ startup_complete = true; }}

    readAndSetLEDs();

    FastLED.show();
    FastLED.delay(10);

    if(reset_counter >= 200) {
      resetAll();
    }
  } else {
    
    // Initialization
    if(started_init and Serial.available()) {
      
      // During initialization
      initializationLoop();
    } else if(Serial.available() and Serial.readStringUntil('\n') == "init") {
      
      // Start initialization
      started_init = true;
      Serial.print(ACKNOWLEDGE);
    } else {
      
      // Pre-initialization stuff
      if(!preinit_tag){
        FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, MAX_NUM_LEDS);
        FastLED.setMaxPowerInVoltsAndMilliamps(3.3, CURRENT_DRAW);
  
        fill_solid(leds, MAX_NUM_LEDS, CRGB::Black);
        FastLED.show();
      }
    }
  }
}
