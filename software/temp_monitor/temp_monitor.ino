/*
Project: RFR New Member Project
Group: 20
Authors: Arjun, Sid, Sai
Date: 10/18/26 v0.1 Code Skeleton

Hardware Used:Arduino Nano, CD4051BE (8 Channels), temperature sensing diodes
Outputs: Maximum temperature over an I2C bus, and a blinking red LED above 60C

File Description:
1. The code sets each pin on the MUX to a binary number, which connects that sensor to the 
input pin on the Nano.
2. The Nano's ADC turns the analog voltage from the diode into a digital number. The code will 
read it several times and find the average to smooth out noise.
3. Using the diode's calibration (-2mV per C), converts voltage to temperature.
4. Repeats this process for 8 diodes, storing each temp.
5. Finds hottest diode.
6. If the hottest diode is > 60, send a fault - fault is turned off when temp <= 57C
7. Blinks red LED if fault is active
8. When the I2C master requests data, the NAno sends max temp as well as the fautl status.
*/

#include <Wire.h>

const int PIN_A = 2;
const int PIN_B = 3;
const int PIN_C = 4;
const int PIN_ADC = A0;
const int PIN_LED = 7;

const float V_REF = 1.1;
const int NUM_SAMPLES = 8, NUM_CHANNELS = 8;
const float SLOPE = -0.002;

const float FAULT_TEMP = 60.0;
const float CLEAR_TEMP = 57.0;

const unsigned long BLINK_MS = 250;

const float V_CAL[NUM_CHANNELS] = {0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6, 0.6};
const float TEMP_CAL[NUM_CHANNELS] = {25.0, 25.0, 25.0, 25.0, 25.0, 25.0, 25.0, 25.0};

const int I2C_ADDRESS = 0x08;

// Global // volatile -> force to read from ram
float temps[NUM_CHANNELS];
volatile float maxTemp;
volatile bool fault = false;
bool ledOn = false;
unsigned long last_toggled = 0;

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  analogReference(INTERNAL);

  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onI2CRequest);
}

void loop() {
  scanAllChannels();
  maxTemp = findMaxTemp();
  updateFault();
  updateLed();
}


// HELPER FUNCTIONS

// Connects the sensor to the Nano's A0 pin
void selectChannel(int channel){
  digitalWrite(PIN_A, channel >> 0 & 1);
  digitalWrite(PIN_B, channel >> 1 & 1);
  digitalWrite(PIN_C, channel >> 2 & 1);
}

float readVoltage(int channel){
  selectChannel(channel);
  delayMicroseconds(10); // Settling time
  analogRead(PIN_ADC);

  float sum = 0;
  for(int i = 0; i < NUM_SAMPLES; i++){
    sum += analogRead(PIN_ADC);
  }
  sum /= NUM_SAMPLES;
  return (sum*V_REF) / 1024;
}

float voltageToTemp(int channel, float voltage){
  return TEMP_CAL[channel] + (voltage - V_CAL[channel])/SLOPE;
}

void scanAllChannels(){
  for(int i = 0; i < NUM_CHANNELS; i++){
    float v = readVoltage(i);
    temps[i] = voltageToTemp(i, v);
  }
}

float findMaxTemp(){
  float highest = temps[0];
  for(int i = 1; i < NUM_CHANNELS; i++)
    if(temps[i] > highest) highest = temps[i];
  return highest;
}

void updateFault(){
  if(maxTemp > FAULT_TEMP) fault = true;
  else if(maxTemp <= CLEAR_TEMP) fault = false;
}

void updateLed(){
  if(!fault){ 
    ledOn = false;
    digitalWrite(PIN_LED, LOW); 
  }
  else if(millis() - last_toggled >= BLINK_MS){
    ledOn = !ledOn;
    last_toggled = millis();
    digitalWrite(PIN_LED, ledOn);
  } 
}

void onI2CRequest(){
  // TODO
}