
/*
   100K temp probe in series with 100k resistor where the resistor
   is connected to VCC and temp probe connected to GND.


   

   Notes:
   - User input to change set temperature
   - Display to see current temperature
   - When temp = 511 temp probe resistance is 100k



Example output to serial console:
------------- cnt: 1 -------------
Analog value: 537
i: 23.78 nA
Vo: 2.62 V
Power: 118.77 nW
Rt: 110.49 kOhm
Temperature: 22.94 C
   
*/
#include <Bounce2.h>

// Set I/O pins
  // Heater on
const int heatOn = 13;
  // Segment pins
int segmentPins[] = {1,2,3,4,5,6,7}; // for pins a,b,c,d,e,f,g
int decimalPin = 0;
  // Digit select pins
const int debounce = 10; // debounce time in ms
const int digit1 = 8;
const int digit2 = 9;
const int digit3 = 10;
  // Temperature adjustment inputs
const int downPin = 11;
const int upPin = 12;
Bounce downSw = Bounce(downPin, debounce);
Bounce upSw = Bounce(upPin, debounce);

  // Analog pin
const int anaPin = 0;

// Constants
const float vcc = 5; // This value is likely wrong so don't actually use it
const float r = 100; // kohm
const float hyst = 1.5;

// Variables
int ana = 0;
int heatState = 0;
int setTemp = 32; // Traget state
float temp = 0;
float lastTemp = 0;
float sendTemp = 0;
float vo = 0;
float rt = 0;
float i = 0;
// Arrays - Data points for sensor B57550G1104
float resistanceTable[] = {1.6419, 1.8548, 2.1008, 2.3861, 2.7179, 3.1050, 3.5581, 4.0901, 4.7170, 5.4584, 6.3383, 7.3867, 8.6407, 10.147, 11.963, 14.164, 16.841, 20.114, 24.136, 29.100, 35.262, 42.950, 52.598, 64.776, 80.239, 100.00, 125.42, 158.34, 201.27, 257.69, 332.40};
float temperatureTable[] = {150.00, 145.00, 140.00, 135.00, 130.00, 125.00, 120.00, 115.00, 110.00, 105.00, 100.00, 95.000, 90.000, 85.000, 80.000, 75.000, 70.000, 65.000, 60.000, 55.000, 50.000, 45.000, 40.000, 35.000, 30.000, 25.000, 20.000, 15.000, 10.000, 5.0000, 0.0000};
int arrSize = sizeof(resistanceTable) / sizeof(float); // calculate array size
// Count
long int cnt = 0;

void setup() {
  //Serial.begin(9600); // USB is always 12 Mbit/sec

  // Set I/O pin modes
    // Heat on
  pinMode(heatOn, OUTPUT);
    // Segments
  for (int i=0; i < 7; i++) {
     pinMode(segmentPins[i], OUTPUT);
  }
  pinMode(decimalPin, OUTPUT);
    // Digit select - set HIGH to prevent flickering at startup
  pinMode(digit1, OUTPUT);
  pinMode(digit2, OUTPUT);
  pinMode(digit3, OUTPUT);
  digitalWrite(digit1, HIGH);
  digitalWrite(digit2, HIGH);
  digitalWrite(digit3, HIGH);
    // Temp select inputs
  pinMode(downPin, INPUT_PULLUP); // Down
  pinMode(upPin, INPUT_PULLUP); // Up
}

/*
    This function is sourced from:
    http://playground.arduino.cc/Main/MultiMap
*/
// note: the in array should have increasing values
float FmultiMap(float val, float * _in, float * _out, uint8_t size) {
  // take care the value is within range
  // val = constrain(val, _in[0], _in[size-1]);
  if (val <= _in[0]) return _out[0];
  if (val >= _in[size-1]) return _out[size-1];

  // search right interval
  uint8_t pos = 1;  // _in[0] allready tested
  while(val > _in[pos]) pos++;

  // this will handle all exact "points" in the _in array
  if (val == _in[pos]) return _out[pos];

  // interpolate in the right segment for the rest
  return (val - _in[pos-1]) * (_out[pos] - _out[pos-1]) / (_in[pos] - _in[pos-1]) + _out[pos-1];
}

int displayReset() {
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], LOW);
  }
  digitalWrite(decimalPin, LOW);
  return(0);
}

// Function to print value to 3 digit 7 segment display
int SevenSegDisplay(uint8_t msd, uint8_t mid, uint8_t lsd, int decimal) {
  /*       _
           a
     _  |f   b|
    |_|   -g-   
    |_| |e   c|
           d
           ¯
  */   
  // Numbers 0 - 9 for 7 seg display
  int numMatrix[10][7] = {
    {1,1,1,1,1,1,0},
    {0,1,1,0,0,0,0},
    {1,1,0,1,1,0,1},
    {1,1,1,1,0,0,1},
    {0,1,1,0,0,1,1},
    {1,0,1,1,0,1,1},
    {1,0,1,1,1,1,1},
    {1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1},
    {1,1,1,0,0,1,1},
  };

  // Write MSD
  for (int i = 0; i < 7; i++) {
    if (numMatrix[msd][i] == 1) {
      digitalWrite(segmentPins[i], HIGH);
    }
  }
  if (decimal == 2) digitalWrite(decimalPin, HIGH);
  digitalWrite(digit1, LOW);
  delay(5);
  digitalWrite(digit1, HIGH);
  displayReset();

  // Write middle digit
  for (int i = 0; i < 7; i++) {
    if (numMatrix[mid][i] == 1) {
      digitalWrite(segmentPins[i], HIGH);
    }
  }
  if (decimal == 1) digitalWrite(decimalPin, HIGH);
  digitalWrite(digit2, LOW);
  delay(5);
  digitalWrite(digit2, HIGH);
  displayReset();

  // Write LSD digit
  for (int i = 0; i < 7; i++) {
    if (numMatrix[lsd][i] == 1) {
      digitalWrite(segmentPins[i], HIGH);
    }
  }
  if (decimal == 0) digitalWrite(decimalPin, HIGH);
  digitalWrite(digit3, LOW);
  delay(5);
  digitalWrite(digit3, HIGH);
  displayReset();
  
  return(0);
}


void loop() {
  // Get the temperature
    // Gather the data
  ana = analogRead(anaPin);
    // Do the maths
  vo = vcc * ana / 1024; // mV
  i = ((vcc - vo ) / r); // mA
  rt = r * ana / (1023 - ana); // kohm
  float pwr = vcc * vcc / (r + rt); // nW
  temp = FmultiMap(rt, resistanceTable, temperatureTable, arrSize);

  // Heating element logic
  if (lastTemp == 0) {
    lastTemp = temp;
    if (temp < setTemp) digitalWrite(heatOn, HIGH);
  }
  if (temp != lastTemp){
    if (heatState == 0 && temp >= (setTemp + hyst)) {
      digitalWrite(heatOn, LOW); 
      heatState = 1;
    }
    else if (heatState == 1 && temp <= (setTemp - hyst)) {
      digitalWrite(heatOn, HIGH);
      heatState = 0;
    }  
  }
  lastTemp = temp;

  // Print temperature to 7-segment display
    // Delay on displayed temperature to prevent jitters
  if (cnt >= 100) {
    sendTemp = temp; 
    cnt = 0;
  }
    // Isolate digits
  uint8_t hundredths,tenths,ones,tens,hundreds;
  hundreds = sendTemp / 100;
  tens = (sendTemp - hundreds*100) / 10;
  ones = (sendTemp - hundreds*100 - tens*10);
  tenths = (sendTemp - hundreds*100 - tens*10 - ones) * 10;
  hundredths = (sendTemp - hundreds*100 - tens*10 - ones) * 100 - tenths*10;
  
  cnt++;

  // Temperature adjustment buttons
  upSw.update();
  downSw.update();
  if (upSw.fallingEdge()) {
    setTemp = setTemp + 1;
    sendTemp = setTemp; // Display the set temp
    cnt = 0; // Reset cnt to 0 so set temp is displayed for some time
  }
  if (downSw.fallingEdge()) {
    setTemp = setTemp - 1;
    sendTemp = setTemp;
    cnt = 0;
  }
  hundreds = sendTemp / 100;
  tens = (sendTemp - hundreds*100) / 10;
  ones = (sendTemp - hundreds*100 - tens*10);
  tenths = (sendTemp - hundreds*100 - tens*10 - ones) * 10;
  hundredths = (sendTemp - hundreds*100 - tens*10 - ones) * 100 - tenths*10;
  // Determin decimal placement and call SevenSegDisplay function to print the temperature
  if (sendTemp < 10) {
    // Display 2 decimal points
    SevenSegDisplay(ones, tenths, hundredths, 2);
  }
  else if (sendTemp < 100 && sendTemp >= 10) {
    // Distplay 1 decimal point
    SevenSegDisplay(tens, ones, tenths, 1);
  }
  else {
    // No decimal points
    SevenSegDisplay(hundreds, tens, ones, 0);
  }

/*
  // Print to serial console
    // Count
  Serial.print("------------- ");
  Serial.print("cnt: ");
  Serial.print(cnt);
  Serial.println(" -------------");
    // Raw analog value
  Serial.print("Analog value: ");
  Serial.println(ana);
    // Calculated current draw - Unreliable, using vcc in calculation 
  Serial.print("i: ");
  Serial.print(i * 1000);
  Serial.println(" nA");
    // Calculated voltage across sensor
  Serial.print("Vo: ");
  Serial.print(vo);
  Serial.println(" V");
    // Calculated power draw - Unreliable, using vcc in calculation
  Serial.print("Power: ");
  Serial.print(pwr * 1000);
  Serial.println(" nW");
    // Calculated resistance of sensor
  Serial.print("Rt: ");
  Serial.print(rt);
  Serial.println(" kOhm");
    // Calculated temperature
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" C");

  Serial.println("Digits:");
  Serial.println(hundreds);
  Serial.println(tens);
  Serial.println(ones);
  Serial.println(tenths);
  Serial.println(hundredths);
  */
}








