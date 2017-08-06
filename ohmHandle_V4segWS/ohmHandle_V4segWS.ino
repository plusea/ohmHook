/*
  ohmHook - measuring and manipulting resistance by hand
  for the E-Textile Tooling workshop at Eyeo 2016
  >> www.github.com/plusea/ohmHook

  displays an analog value on an Avago HCMS-39xx display controlled by ATtiny84
  original HCMS-39xx display code by Tom Igoe, created 12 June 2008
  modified by Hannah Perner-Wilson Apr 2016
*/

#include <LedDisplay.h>
#include <stdint.h>

// Define pins for the LED display:
#define dataPin 1              // connects to the display's data in
#define registerSelect 2       // the display's register select pin 
#define clockPin 3             // the display's clock pin
#define enable 8               // the display's chip enable pin
#define reset 9                // the display's reset pin
#define displayLength 4        // number of characters in the display
LedDisplay myDisplay = LedDisplay(dataPin, registerSelect, clockPin, enable, reset, displayLength); // create am instance of the LED display library:
float brightness = 15;         // screen brightness

#define buttonPin 5
#define potMinus 6
#define analogPin 7
#define speakerPin 10
#define potValue 500000
#define nSamplesMax 20         // maximum number of samples; must be less than 32 to prevent overflow


// Conversion of ADC value to mV. Should be calibrated for each device?
// Should also be made independent of battery voltage by reading 1.1V reference voltage first?
#define analogValueToMilliVolt(x) ((x * 23) / 10)

enum mode_e {
  mode_analog_read_init,
  mode_analog_read,
  mode_synth_init,
  mode_synth,
  mode_resistance_init,
  mode_resistance,
  mode_volt_init,
  mode_volt,
  mode_brightness,
  mode_jokes_init,
  mode_jokes,
  mode_max
};

int buttonState = 0;     // variable for reading the button status
int lastButtonState = 0; // stores the previous state of the button
mode_e mode = mode_analog_read_init;            // mode will store the current blinking mode (0 - 3)
int myDirection = 1;           // direction of scrolling.  -1 = left, 1 = right.
int analogValue;
uint32_t potBotValue;
uint32_t resistanceValue;
float soundValue;


void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(analogPin, INPUT);
  pinMode(potMinus, OUTPUT);
  digitalWrite(potMinus, LOW);
  pinMode(speakerPin, OUTPUT);

  noise(speakerPin, 100, 200);
  noise(speakerPin, 20000, 200);
  delay(10);

  myDisplay.begin();    // initialize the display library:
  myDisplay.setBrightness(brightness);    // set the brightness of the display:
  myDisplay.clear();
}

void handle_analog_read_init(void) {
  pinMode(potMinus, INPUT);
  mode = mode_analog_read;
}

void handle_analog_read(void) {
  analogValue = analogRead(analogPin);
  delay(10);
  // DISPLAY //
  myDisplay.setCursor(0);  // set the cursor to 1:
  if (analogValue < 1000 && analogValue > 99) myDisplay.print(" ");
  if (analogValue < 100 && analogValue > 9) myDisplay.print("  ");
  if (analogValue < 10) myDisplay.print("   ");
  myDisplay.print(analogValue, DEC);
}

void handle_synth_init(void) {
  myDisplay.setCursor(0);  // set the cursor to 1:
  myDisplay.print("SYNT");
  mode = mode_synth;
}

void handle_synth(void) {
  analogValue = analogRead(analogPin);
  if(analogValue < 1010){
    soundValue = map(analogValue, 0, 1023, 0, 20000);
    noise(speakerPin, soundValue, 1);
  }
}

void handle_resistance_init(void) {
  uint8_t n;
  
  pinMode(potMinus, OUTPUT);
  digitalWrite(potMinus, LOW);
  analogRead(analogPin); // always discard first measurement after switching channel
  
  analogValue = 0;
  for (n = 0; n < nSamplesMax; n++)
    analogValue += analogRead(analogPin);

  potBotValue = map(analogValue, 0, 1023 * nSamplesMax, 0, potValue);
  potBotValue = constrain(potBotValue, 0, potValue);
  
  mode = mode_resistance;
}

void handle_resistance(void) {
  char unit = 'R';
  
  analogValue = analogRead(analogPin);
  resistanceValue = (uint32_t) ((potValue - potBotValue) * analogValue / (1023 - analogValue));
  resistanceValue = constrain(resistanceValue, 0, potValue);
  
  // DISPLAY //
  myDisplay.setCursor(0);  // set the cursor to 1

  if (resistanceValue >= 1000) {
    resistanceValue = resistanceValue / 1000;
    unit = 'K';
  }
  if (resistanceValue < 10) {
    myDisplay.print(" ");
  }
  if (resistanceValue < 100) {
    myDisplay.print(" ");
  }
  myDisplay.print(resistanceValue);
  myDisplay.print(unit);
}

void handle_volt_init(void) {
  pinMode(potMinus, INPUT);
  mode = mode_volt;
}

void handle_volt(void) {
  int tmp, approx, digit;
  
  analogValue = analogValueToMilliVolt(analogRead(analogPin));
  
  // DISPLAY //
  myDisplay.setCursor(0);  // set the cursor to 1:
  
  digit = analogValue / 1000;
  approx = digit * 1000;
  myDisplay.print(digit, DEC);
  
  myDisplay.print("v");
  
  digit = (analogValue - approx) / 100;
  approx += digit * 100;
  myDisplay.print(digit, DEC);
  
  digit = (analogValue - approx) / 10;
  approx += digit * 10;
  myDisplay.print(digit, DEC);
}

void handle_brightness(void) {
  pinMode(potMinus, OUTPUT);
  digitalWrite(potMinus, LOW);
  brightness = map(analogRead(analogPin), 0, 1023, 1, 15);
  brightness = constrain(brightness, 1, 15);
  myDisplay.setCursor(0);  // set the cursor to 1:
  myDisplay.setBrightness(brightness);    // set the brightness of the display
  //myDisplay.print(brightness, DEC);
  myDisplay.print("LITE");
}

void handle_jokes_init(void) {
  myDisplay.clear();
  myDisplay.setCursor(0);
  analogValue = analogRead(analogPin);
  if (analogValue & 0x01)
    myDisplay.setString("resistance smells funny!");
  else
    myDisplay.setString("electrons are nomnom!");
  mode = mode_jokes;
}

void handle_jokes(void) {
  if ((myDisplay.getCursor() > displayLength) ||
      (myDisplay.getCursor() <= -(myDisplay.stringLength()))) {
    myDirection = -myDirection;
    delay(500);
  }
  myDisplay.scroll(myDirection);
  delay(100);
}

void loop() {
  // BUTTON //
  buttonState = digitalRead(buttonPin); // read the state of the pushbutton

  // detect the event of the button going from pressed to not pressed,
  // and increment the animation mode when this is detected
  if (lastButtonState == HIGH && buttonState == LOW) {
    mode = mode + 1;
  }

  lastButtonState = buttonState;  // store current button state as the last button state to use next time through loop()
  if (mode >= mode_max) mode = 0;

  ///// MODE: ANALOG READ /////
  if (mode == mode_analog_read_init) {
    handle_analog_read_init();
  }
  if (mode == mode_analog_read) {
    handle_analog_read();
  }

  ///// MODE: SYNTH /////
  if (mode == mode_synth_init) {
    handle_synth_init();
  }
  if (mode == mode_synth) {
    handle_synth();
  }

  ///// MODE: POT RESISTANCE VALUE /////
  if (mode == mode_resistance_init) {
    handle_resistance_init();
  }
  if (mode == mode_resistance) {
    handle_resistance();
  }

  ///// MODE: VOLT /////
  if (mode == mode_volt_init) {
    handle_volt_init();
  }
  if (mode == mode_volt) {
    handle_volt();
  }

  ///// MODE: BRIGHTNESS /////
  if (mode == mode_brightness) {
    handle_brightness();
  }

  ///// MODE: JOKES /////
  if (mode == mode_jokes_init) {
    handle_jokes_init();
  }
  if (mode == mode_jokes) {
    handle_jokes();
  }
}

void noise (unsigned char noisePin, int frequencyInHertz, long timeInMilliseconds) {
  int x;
  long delayAmount = (long)(1000000 / frequencyInHertz);
  long loopTime = (long)((timeInMilliseconds * 1000) / (delayAmount * 2));
  for (x = 0; x < loopTime; x++)
  {
    digitalWrite(noisePin, HIGH);
    delayMicroseconds(delayAmount);
    digitalWrite(noisePin, LOW);
    delayMicroseconds(delayAmount);
  }
}

