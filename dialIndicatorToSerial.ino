/*
 MIT License

 Copyright (c) 2017 Paweł Stawicki

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */

// pins - voltage on outputs are ~1.5V so we use the ADC
#define DataPin A0
#define ClkPin  A1
#define LedPin  13

// Dial Indicator resolution: 100 - 0.01mm, 1000 - 0.001mm
//#define Resolution 100
#define Resolution 1000

// UART speed
#define UARTBaudRate 115200

// ADC threshold, ADC values greater than this are interpreted as logical 1, see loop()
#define ADC_Threshold 140

// data format
#define DATA_BITS_LEN 24
#define INCH_BIT 23
#define SIGN_BIT 20
#define START_BIT -1 // -1 - no start bit

// data capture and decode functions
bool getRawBit() {
    bool data;
    while (analogRead(ClkPin) > ADC_Threshold)
        ;
    while (analogRead(ClkPin) < ADC_Threshold)
        ;
    data = analogRead(DataPin) > ADC_Threshold;
    return data;
}

long getRawData() {
    long out = 0;
    for (int i = 0; i < DATA_BITS_LEN; i++) {
        out |= getRawBit() ? 1L << DATA_BITS_LEN : 0L;
        out >>= 1;
    }
    return out;
}

long getValue(bool &inch) {
    long out = getRawData();
    inch = out & (1L << INCH_BIT);
    bool sign = out & (1L << SIGN_BIT);
    out &= (1L << SIGN_BIT) - 1L;
    out >>= (START_BIT+1);
    if (sign)
        out = -out;
    return out;
}

// printing functions
void printBits(long v) {
    char buf[DATA_BITS_LEN + 1];
    for (int i = DATA_BITS_LEN - 1; i >= 0; i--) {
        buf[i] = v & 1 ? '1' : '0';
        v >>= 1;
    }
    buf[DATA_BITS_LEN] = 0;
    Serial.print(buf);
}

void prettyPrintValue(long value, bool inch) {
    double v = value;
#if Resolution == 100
    if (inch) {
        Serial.print(v / 2000, 4);
        Serial.print(" in");
    } else {
        Serial.print(v / 100, 2);
        Serial.print(" mm");
    }
#else
    if (inch) {
        Serial.print(v / 20000, 5);
        Serial.print(" in");
    } else {
        Serial.print(v / 1000, 3);
        Serial.print(" mm");
    }
#endif
}

void toggleLed() {
#ifdef LedPin
  static bool state = false;
  state = !state;
  digitalWrite(LedPin, state);
#endif
}

// Arduino setup and main loop

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

void setup() {
    // set ADC prescale to 16 (set ADC clock to 1MHz)
    // this gives as a sampling rate of ~77kSps
    sbi(ADCSRA, ADPS2);
    cbi(ADCSRA, ADPS1);
    cbi(ADCSRA, ADPS0);

    Serial.begin(UARTBaudRate);
#ifdef LedPin
    pinMode(LedPin, OUTPUT);
#endif
}

void loop() {
    bool inch;
    long value;

    value = getValue(inch);

    // print time
    Serial.print(millis());
    Serial.print(" ms ");

    // pretty print measured value
    prettyPrintValue(value, inch);

    // toggle LED
    toggleLed();

    // uncomment if you are interested in optimal ADC_Threshold value
    //Serial.print(' '); Serial.print(analogRead(ClkPin)/2);

    // uncomment if you are interested in raw data bits
    //Serial.print(' '); printBits(getRawData());

    // uncomment if you are interested in raw data
    //Serial.print(' '); Serial.print(value);

    Serial.println();
}

