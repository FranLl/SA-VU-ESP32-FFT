// (Heavily) adapted from https://github.com/G6EJD/ESP32-8266-Audio-Spectrum-Display/blob/master/ESP32_Spectrum_Display_02.ino
// Adjusted to allow brightness changes on press+hold, Auto-cycle for 3 button presses within 2 seconds
// Edited to add Neomatrix support for easier compatibility with different layouts.

// Libraries...
#include <FastLED_NeoMatrix.h>        // ...to control the LEDs matrix
#include <arduinoFFT.h>               // ...to do Fast Fourier Transform for Arduino
#include <EasyButton.h>               // ...to control switches and buttons
#include <Wire.h>                     // ...to allow you to communicate with I2C/TWI devices
#include <Adafruit_GFX.h>             // ...of core graphics for all our displays
#include <Adafruit_SSD1306.h>         // ...for Monochrome OLEDs based on SSD1306 drivers


// Wave configuration
#define SAMPLES                1024                       // Number of samples we get from the electrical signal. Must be a power of 2
#define SAMPLING_FREQ         40000                       // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE              1000                       // Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define NUM_BANDS                16                       // To change this, you will need to change the bunch of if statements describing the mapping from bins to bands
#define NOISE                  2000                       // Used as a crude noise filter, values below this are ignored

// ESP32 configuration
#define AUDIO_IN_BTN_PIN          4                       // Connect a push button to this pin to change audio in
#define PATTERN_BTN_PIN           2                       // Connect a push button to this pin to change patterns
#define BRIGHTNESS_BTN_PIN        0                       // Connect a push button to this pin to change LED brightness
#define LED_PIN                   5                       // LED strip data
int AUDIO_IN_PIN =              34;                       // Pin of the electrical signal input. Aux in = 34, mic in = 35
#define LONG_PRESS_MS           1000                       // Number of ms to count as a long press

// LEDs configuration
#define COLOR_ORDER             GRB                       // If colours look wrong, play with this
#define CHIPSET             WS2812B                       // LED strip type
#define MAX_MILLIAMPS          2000                       // Careful with the amount of power here if running off USB port
const int BRIGHTNESS_SETTINGS[3] = {5, 70, 200};          // 3 Integer array for 3 brightness settings (based on pressing+holding BTN_PIN)
#define LED_VOLTS                 5                       // Usually 5 or 12
const uint8_t kMatrixWidth =     32;                      // Matrix width
const uint8_t kMatrixHeight =     8;                      // Matrix height. I use 1 led more to hide the bottom white row
#define NUM_LEDS       (kMatrixWidth * kMatrixHeight)     // Total number of LEDs
#define BAR_WIDTH      (kMatrixWidth  / (NUM_BANDS - 1))  // If width >= 8 light 1 LED width per bar, >= 16 light 2 LEDs width bar etc
#define TOP            (kMatrixHeight - 0)                // Don't allow the bars to go offscreen

// Display configuration
#define SCREEN_WIDTH 128                                  // OLED display width, in pixels
#define SCREEN_HEIGHT 32                                  // OLED display height, in pixels. For our use the screen is so small that we have to indicate that it is smaller so that the font appears larger.
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an ESP32:             21(SDA),  22(SCL)
#define OLED_RESET     -1                                 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C                               // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

hw_timer_t *mainScreenTimer = NULL;                       // Timer to show the mainScreen
bool mainScreenFlag = false;                              // Flag to show the mainScreen when timer reach the time

// Sampling and FFT stuff
unsigned int sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
byte peak[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};              // The length of these arrays must be >= NUM_BANDS
int oldBarHeights[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};      // The length of these arrays must be >= NUM_BANDS
int bandValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};         // The length of these arrays must be >= NUM_BANDS
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;
arduinoFFT FFT = arduinoFFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);

// Button stuff
int buttonPushPatternsCounter = 0;                          // Select pattern between 0 and 5
bool autoChangePatterns = false;                            // Change patterns automatically
int restoreBrightness = 0;                                  // LED brightness value before poweroff
EasyButton audioInBtn(AUDIO_IN_BTN_PIN);                    // Button to change audio in
EasyButton patternBtn(PATTERN_BTN_PIN);                     // Button to change patterns
EasyButton brightnessBtn(BRIGHTNESS_BTN_PIN);               // Button to change brightness

// FastLED stuff
CRGB leds[NUM_LEDS];
DEFINE_GRADIENT_PALETTE( purple_gp ) {
                                      0,   0, 212, 255,   //blue
                                      255, 179,   0, 255 }; //purple
DEFINE_GRADIENT_PALETTE( outrun_gp ) {
                                      0, 141,   0, 100,   //purple
                                      127, 255, 192,   0,   //yellow
                                      255,   0,   5, 255 };  //blue
DEFINE_GRADIENT_PALETTE( greenblue_gp ) {
                                        0,   0, 255,  60,   //green
                                      64,   0, 236, 255,   //cyan
                                      128,   0,   5, 255,   //blue
                                      192,   0, 236, 255,   //cyan
                                      255,   0, 255,  60 }; //green
DEFINE_GRADIENT_PALETTE( redyellow_gp ) {
                                        0,   200, 200,  200,   //white
                                      64,   255, 218,    0,   //yellow
                                      128,   231,   0,    0,   //red
                                      192,   255, 218,    0,   //yellow
                                      255,   200, 200,  200 }; //white
CRGBPalette16 purplePal = purple_gp;
CRGBPalette16 outrunPal = outrun_gp;
CRGBPalette16 greenbluePal = greenblue_gp;
CRGBPalette16 heatPal = redyellow_gp;
uint8_t colorTimer = 0;

// FastLED_NeoMaxtrix - see https://github.com/marcmerlin/FastLED_NeoMatrix for Tiled Matrixes, Zig-Zag and so forth
/* 8x8 matrix
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, kMatrixWidth, kMatrixHeight,
  NEO_MATRIX_TOP        + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS       + NEO_MATRIX_PROGRESSIVE);
*/
// 32x8 matrix
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(
  leds, kMatrixWidth, kMatrixHeight,
  NEO_MATRIX_TOP        + NEO_MATRIX_LEFT +
  NEO_MATRIX_COLUMNS       + NEO_MATRIX_ZIGZAG);


// Setup function
void setup() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);
  FastLED.setBrightness(BRIGHTNESS_SETTINGS[0]);                                                  // Set initial brightness value
  FastLED.clear();

  // Initialize buttons
  audioInBtn.begin();
  patternBtn.begin();
  brightnessBtn.begin();

  // Set on pressed functions
  audioInBtn.onPressed(changeAudioIn);
  patternBtn.onPressed(changePattern);
  brightnessBtn.onPressed(brightnessButton);

  // Set on pressed for functions
  patternBtn.onPressedFor(LONG_PRESS_MS, startAutoChangePatterns);
  brightnessBtn.onPressedFor(LONG_PRESS_MS, brightnessOnOff);

  // Start display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    // Address 0x3D for 128x64. For my screen 0x3D don't work
  
  // Timer to show the mainScreen
  mainScreenTimer = timerBegin(0, 80, true);
  timerAttachInterrupt(mainScreenTimer, &mainScreenOnTimer, true);
  timerAlarmWrite(mainScreenTimer, 3000000, true);
  timerAlarmEnable(mainScreenTimer);
}


// Loop function
void loop() {
  // Don't clear screen if waterfall pattern, be sure to change this is you change the patterns / order
  if (buttonPushPatternsCounter != 5) FastLED.clear();

  audioInBtn.read();
  patternBtn.read();
  brightnessBtn.read();

  // Reset bandValues[]
  for (int i = 0; i<NUM_BANDS; i++){
    bandValues[i] = 0;
  }

  // Sample the audio pin
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    vReal[i] = analogRead(AUDIO_IN_PIN); // A conversion takes about 9.7uS on an ESP32
    vImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) { /* chill */ }
  }

  // Compute FFT
  FFT.DCRemoval();                             // Because ESP32 doesnt read negative values, for that, we add the 1'65 (3'3V/2) to audio signal and remove it with this function
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(FFT_FORWARD);
  FFT.ComplexToMagnitude();

  // Analyse FFT results
  for (int i = 2; i < (SAMPLES/2); i++){       // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
    if (vReal[i] > NOISE) {                    // Add a crude noise filter
      /*8 bands, 12kHz top band
      if (i<=3 )           bandValues[0]  += (int)vReal[i];
      if (i>3   && i<=6  ) bandValues[1]  += (int)vReal[i];
      if (i>6   && i<=13 ) bandValues[2]  += (int)vReal[i];
      if (i>13  && i<=27 ) bandValues[3]  += (int)vReal[i];
      if (i>27  && i<=55 ) bandValues[4]  += (int)vReal[i];
      if (i>55  && i<=112) bandValues[5]  += (int)vReal[i];
      if (i>112 && i<=229) bandValues[6]  += (int)vReal[i];
      if (i>229          ) bandValues[7]  += (int)vReal[i];*/

      //16 bands, 12kHz top band
      if (i<=2 )           bandValues[0]  += (int)vReal[i];
      if (i>2   && i<=3  ) bandValues[1]  += (int)vReal[i];
      if (i>3   && i<=5  ) bandValues[2]  += (int)vReal[i];
      if (i>5   && i<=7  ) bandValues[3]  += (int)vReal[i];
      if (i>7   && i<=9  ) bandValues[4]  += (int)vReal[i];
      if (i>9   && i<=13 ) bandValues[5]  += (int)vReal[i];
      if (i>13  && i<=18 ) bandValues[6]  += (int)vReal[i];
      if (i>18  && i<=25 ) bandValues[7]  += (int)vReal[i];
      if (i>25  && i<=36 ) bandValues[8]  += (int)vReal[i];
      if (i>36  && i<=50 ) bandValues[9]  += (int)vReal[i];
      if (i>50  && i<=69 ) bandValues[10] += (int)vReal[i];
      if (i>69  && i<=97 ) bandValues[11] += (int)vReal[i];
      if (i>97  && i<=135) bandValues[12] += (int)vReal[i];
      if (i>135 && i<=189) bandValues[13] += (int)vReal[i];
      if (i>189 && i<=264) bandValues[14] += (int)vReal[i];
      if (i>264          ) bandValues[15] += (int)vReal[i];
    }
  }

  // Process the FFT data into bar heights
  for (byte band = 0; band < NUM_BANDS; band++) {
    // Scale the bars for the display
    int barHeight = bandValues[band] / AMPLITUDE;
    if (barHeight > TOP) barHeight = TOP;

    // Small amount of averaging between frames
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;

    // Move peak up
    if (barHeight > peak[band]) {
      peak[band] = min(TOP, barHeight);
    }

    // Draw bars
    switch (buttonPushPatternsCounter) {
      case 0:
        rainbowBars(band, barHeight);
        break;
      case 1:
        // No bars on this one
        break;
      case 2:
        purpleBars(band, barHeight);
        break;
      case 3:
        centerBars(band, barHeight);
        break;
      case 4:
        changingBars(band, barHeight);
        break;
      case 5:
        waterfall(band);
        break;
    }

    // Draw peaks
    switch (buttonPushPatternsCounter) {
      case 0:
        whitePeak(band);
        break;
      case 1:
        outrunPeak(band);
        break;
      case 2:
        whitePeak(band);
        break;
      case 3:
        // No peaks
        break;
      case 4:
        // No peaks
        break;
      case 5:
        // No peaks
        break;
    }

    // Save oldBarHeights for averaging later
    oldBarHeights[band] = barHeight;
  }

  // Decay peak
  EVERY_N_MILLISECONDS(60) {
    for (byte band = 0; band < NUM_BANDS; band++)
      if (peak[band] > 0) peak[band] -= 1;
    colorTimer++;
  }

  // Used in some of the patterns
  EVERY_N_MILLISECONDS(10) {
    colorTimer++;
  }

  EVERY_N_SECONDS(10) {
    if (autoChangePatterns) buttonPushPatternsCounter = (buttonPushPatternsCounter + 1) % 6;
  }

  FastLED.show();

  // Show main screen
  if ( mainScreenFlag )
  {
    displayMainScreen();
    mainScreenFlag = false;
  }
}
