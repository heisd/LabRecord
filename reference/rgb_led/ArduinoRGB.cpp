// Define the pin where the built-in RGB LED is connected
#include <Adafruit_NeoPixel.h>
//Define the number of LEDs in the strip (usually 1 for built-in LED)
#define LED_PIN 48
// Create an instance of the Adafruit_NeoPixel
#define NUM_LEDS 1
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
void setup() {
    // Initialize the NeoPixel library
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}
void loop() {
    // Cycle through some colors
    strip.setPixelColor(0, strip.Color(255, 0, 0)); // Red
    strip.show();
    delay(1000);
    strip.setPixelColor(0, strip.Color(0, 255, 0)); // Green
    strip.show();
    delay(1000);
    strip.setPixelColor(0, strip.Color(0, 0, 255)); // Blue
    strip.show();
    delay(1000);
}
