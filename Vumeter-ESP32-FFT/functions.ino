// Callback function to mainScreenTimer
// Set the flag to true
void mainScreenOnTimer(){
  mainScreenFlag = true;
}

// Display the mainScreen
void displayMainScreen()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.drawRect(20, 10, 90, 15, WHITE);
  display.setCursor(30, 15);
  display.println("IP practice");
  display.display();
}

// Chage the audio source pin
// and show a message in the display 
void changeAudioIn() {
  if (AUDIO_IN_PIN == 34)
  {
    AUDIO_IN_PIN = 35;
    display.clearDisplay();
    display.setCursor(40, 15);
    display.println("Microphone");
    display.display();
  }
  else{
    AUDIO_IN_PIN = 34;
    display.clearDisplay();
    display.setCursor(50, 15);
    display.println("Aux In");
    display.display();
  }

  // Restart HW timer to 0 to show
  // mainScreen after 3000000 ms (3 seg)
  timerRestart(mainScreenTimer);
}

// Change pattern
void changePattern() {
  display.clearDisplay();

  // If LEDs brightness is 0, show "error" message
  if(FastLED.getBrightness() == 0)
  {
    display.setCursor(45, 10);
    display.println("LEDs off");
    display.setCursor(5, 20);
    display.println("Turn on LEDs, please");
  }
  else
  {// If brightness is bigger than 0, disable
   // autoChangePatterns and change to next pattern
    autoChangePatterns = false;
    buttonPushPatternsCounter = (buttonPushPatternsCounter + 1) % 6;

    display.clearDisplay();
    display.setCursor(10, 15);
    switch (buttonPushPatternsCounter) {
      case 0:
        display.println("Pattern: rainbowBars");
        break;
      case 1:
        display.println("Pattern: noBars");
        break;
      case 2:
        display.println("Pattern: purpleBars");
        break;
      case 3:
        display.println("Pattern: centerBars");
        break;
      case 4:
        display.println("Pattern: changingBars");
        break;
      case 5:
        display.println("Pattern: waterfallBars");
        break;
    }
  }
  display.display();

  // Restart HW timer to 0 to show
  // mainScreen after 3000000 ms (3 seg)
  timerRestart(mainScreenTimer);
}

// Start changing pattern automatically
void startAutoChangePatterns() {
  display.clearDisplay();
  display.setCursor(0, 15);

  // If LEDs brightness is 0, show "error" message
  if(FastLED.getBrightness() == 0)
  {
    display.setCursor(45, 10);
    display.println("LEDs off");
    display.setCursor(5, 20);
    display.println("Turn on LEDs, please");
  }
  else
  {// Set autoChangePatterns to true and 
   // show a message to display
    autoChangePatterns = true;
    display.println("autoChangePattern: on");
  }

  display.display();

  // Restart HW timer to 0 to show
  // mainScreen after 3000000 ms (3 seg)
  timerRestart(mainScreenTimer);
}

// Change LEDs brightness
void brightnessButton() {
  display.clearDisplay();

  // If LEDs brightness is 0, show "error" message
  if(FastLED.getBrightness() == 0)
  {
    display.setCursor(45, 10);
    display.println("LEDs off");
    display.setCursor(5, 20);
    display.println("Turn on LEDs, please");
  }
  else
  {// Change brightness to LEDS and show a message to display
    if (FastLED.getBrightness() == BRIGHTNESS_SETTINGS[2])
    {
      FastLED.setBrightness(BRIGHTNESS_SETTINGS[0]);
    }
    else if (FastLED.getBrightness() == BRIGHTNESS_SETTINGS[0])
    {
      FastLED.setBrightness(BRIGHTNESS_SETTINGS[1]);
    }
    else if (FastLED.getBrightness() == BRIGHTNESS_SETTINGS[1])
    {
      FastLED.setBrightness(BRIGHTNESS_SETTINGS[2]);
    }
    
    display.setCursor(20, 15);
    display.println((String)"Brightness: " + FastLED.getBrightness());
  }

  display.display();

  // Restart HW timer to 0 to show
  // mainScreen after 3000000 ms (3 seg)
  timerRestart(mainScreenTimer);
}

// Turn LEDs on or off according to their previous state
void brightnessOnOff(){
  display.clearDisplay();

  // If LEDs are on, turn to off saving the
  // actual brightness to restore when turn on
  if (FastLED.getBrightness() != 0 )
  {
    restoreBrightness = FastLED.getBrightness();
    FastLED.setBrightness(0);
    autoChangePatterns = false;

    display.setCursor(10, 15);
    display.println("Powering off LEDs");
  }
  else
  {// Re-enable LEDs with the previous saved brightness
    FastLED.setBrightness(restoreBrightness);
    display.setCursor(10, 15);
    display.println("Powering on LEDs");
  }

  display.display();

  // Restart HW timer to 0 to show
  // mainScreen after 3000000 ms (3 seg)
  timerRestart(mainScreenTimer);
}

////////////////////
// PATTERNS BELOW //
////////////////////
void rainbowBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix->drawPixel(x, y, CHSV((x / BAR_WIDTH) * (255 / NUM_BANDS), 255, 255));
    }
  }
}

void purpleBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix->drawPixel(x, y, ColorFromPalette(purplePal, y * (255 / (barHeight + 1))));
    }
  }
}

void changingBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix->drawPixel(x, y, CHSV(y * (255 / kMatrixHeight) + colorTimer, 255, 255));
    }
  }
}

void centerBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    int yStart = ((kMatrixHeight - barHeight) / 2 );
    for (int y = yStart; y <= (yStart + barHeight); y++) {
      int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
      matrix->drawPixel(x, y, ColorFromPalette(heatPal, colorIndex));
    }
  }
}

void whitePeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix->drawPixel(x, peakHeight, CHSV(0,0,255));
  }
}

void outrunPeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix->drawPixel(x, peakHeight, ColorFromPalette(outrunPal, peakHeight * (255 / kMatrixHeight)));
  }
}

void waterfall(int band) {
  int xStart = BAR_WIDTH * band;
  double highestBandValue = 60000;        // Set this to calibrate your waterfall

  // Draw bottom line
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix->drawPixel(x, 0, CHSV(constrain(map(bandValues[band],0,highestBandValue,160,0),0,160), 255, 255));
  }

  // Move screen up starting at 2nd row from top
  if (band == NUM_BANDS - 1){
    for (int y = kMatrixHeight - 2; y >= 0; y--) {
      for (int x = 0; x < kMatrixWidth; x++) {
        int pixelIndexY = matrix->XY(x, y + 1);
        int pixelIndex = matrix->XY(x, y);
        leds[pixelIndexY] = leds[pixelIndex];
      }
    }
  }
}
