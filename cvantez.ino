#include <Bounce2.h>
#include <LiquidCrystal.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define MIDI_BAUD_RATE 31250
#define IN_COUNT 6

const int CH_MAX = 16;
const int DISPLAY_ROWS = 2;
const int DISPLAY_COLS = 16;
const int CC_MAX = 127;
const int BAR_STATES = 5; 
const int BAR_ROW = 0;
const int BAR_COL = 12;
const int BAR_WIDTH = 4;
const int CC_CHAR_IND = 5;
const int CH_CHAR_IND = 6;
const int CV_CHAR_IND = 7;
const int MIDI_CHANNEL_OUT = 10;
const int NUM_CV_INS = 1; //
const char SIXTEEN_SPACES[] = "                ";
const int DEBOUNCE_MS = 5;
const int CC_COMMAND = 0xB0;
const int INIT_CH = 9; //10 - 1
const int MIDI_REFRESH_RATE_MILLIS = 50;
const int DISPLAY_REFRESH_RATE_MILLIS = 50;
//int MAX_BAR_SIZE = BAR_STATES * BAR_WIDTH;

const int NEXT_PIN = 6;
Bounce bounceNext = Bounce(); 

const int MODE_PIN = 7;
Bounce bounceMode = Bounce();

const int NUM_CC_OPTIONS = 48;

int invals[IN_COUNT];
int ccIndices[IN_COUNT];
int chs[IN_COUNT];
int activeIndex = 0;
bool secondRowIsDirty = true;

unsigned long lastMidiMillis = 0;
unsigned long lastDisplayMillis = 0; 

enum editState {

  CV_EDIT, CH_EDIT, CC_EDIT
  
};

int uiState = CC_EDIT;

byte ccs[NUM_CC_OPTIONS] = {
  255,
2,
64,
65,
3,
4,
5,
6,
66,
8,
9,
10,
11,
12,
13,
67,
14,
68,
69,
88,
70,
15,
71,
72,
73,
74,
16,
17,
75,
18,
76,
77,
19,
20,
78,
79,
21,
22,
80,
81,
23,
24,
82,
83,
84,
85,
86,
87
};

String descriptions[NUM_CC_OPTIONS] = {
"<nothing>",
"BD1 Attack",
"BD1 Decay",
"BD1 Pitch",
"BD1 Tune",
"BD1 Noise",
"BD1 Filter",
"BD1 Dist",
"BD1 Trigger",
"BD2 Decay",
"BD2 Tune",
"BD2 Tone",
"SD Tune",
"SD D-Tune",
"SD Snappy",
"SD SN Decay",
"SD Tone",
"SD Tone Decay",
"SD Pitch",
"RS Tune",
"CY Decay",
"CY Tone",
"CY Tune",
"OH Decay",
"HH Tune",
"HH Decay",
"CL Tune",
"CL Decay",
"CP Decay",
"CP Filter",
"CP Attack",
"CP Trigger",
"HT/C Tune",
"HT/C Decay",
"H Noise On/Off",
"H Tom/Conga",
"MT/C Tune",
"MT/C Decay",
"M Noise On/Off",
"M Tom/Conga",
"LT/C Tune",
"LT/C Decay",
"L Noise On/Off",
"L Tom/Conga",
"Tom Noise",
"CB Tune",
"CB Decay",
"MA Decay"
};


byte barCells[BAR_STATES][8] = {  
   { 
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  B10000,
  } ,
     { 
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  B11000,
  } ,
     { 
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  B11100,
  } ,
     { 
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  B11110,
  } ,
     { 
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  } 
};

byte ccChar[8] = {
  0b01100,
  0b10000,
  0b01100,
  0b00000,
  0b00011,
  0b00100,
  0b00011,
};

byte chChar[8] = {
  0b01100,
  0b10000,
  0b01100,
  0b00000,
  0b00101,
  0b00111,
  0b00101,
};

byte cvChar[8] = {
  0b01100,
  0b10000,
  0b01100,
  0b00000,
  0b00101,
  0b00101,
  0b00010
};

void setup() {

for (int i=0; i<IN_COUNT; i++)
{
  invals[i] = 0;
  ccIndices[i] = 0;
  chs[i] = INIT_CH;
  activeIndex = 0;
}
   pinMode(NEXT_PIN, INPUT_PULLUP);
   bounceNext.attach(NEXT_PIN);
   bounceNext.interval(DEBOUNCE_MS);

   pinMode(MODE_PIN, INPUT_PULLUP);
   bounceMode.attach(MODE_PIN);
   bounceMode.interval(DEBOUNCE_MS);

   Serial.begin(MIDI_BAUD_RATE); 
   initDisplay();
}

void initDisplay()
{


  lcd.createChar(CC_CHAR_IND, ccChar);
  lcd.createChar(CH_CHAR_IND, chChar);
  lcd.createChar(CV_CHAR_IND, cvChar);

 
  //define custom characters for bar chart
  for (int i=0; i<BAR_STATES; i++)
  {
    lcd.createChar(i, barCells[i]);
  }
  
 // set up the LCD's number of columns and rows:
  lcd.begin(DISPLAY_COLS, DISPLAY_ROWS);


  
  lcd.print("CVantez v0.01");
  
}



void outputMIDI()
{

  unsigned long currTime = millis();
  
  if (currTime > lastMidiMillis + MIDI_REFRESH_RATE_MILLIS)
  {
      for (int i=0; i< IN_COUNT; i++)
      {
        cc(chs[i], ccs[ccIndices[i]], invals[i]);
      }
      lastMidiMillis = currTime;
  }
  

}

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void noteOn(int cmd, int pitch, int velocity) {
  Serial.write(cmd);
  Serial.write(pitch);
  Serial.write(velocity);
}

void cc(int channel, int cc, int val)
{
  noteOn(channel | CC_COMMAND, cc, val);
}

void loop() {

  readAnalogInputs();
  scanButtons();
  outputMIDI();
  updateDisplay();
  
  
}

void readAnalogInputs()
{

  for (int i=0; i<IN_COUNT; i++)
  {
    invals[i] = analogRead((IN_COUNT-1)-i);
    invals[i] = invals[i] >> 3; //divide by 8 to turn [0, 1023] into [0, 127]
  }
}

void updateDisplay()
{
  
  unsigned long currTime = millis();
  
  if (currTime > lastDisplayMillis + DISPLAY_REFRESH_RATE_MILLIS)
  {
    lcd.setCursor(0, 0);
  
    lcd.write(byte(CV_CHAR_IND));
    lcd.print(activeIndex+1);
  
    lcd.print(" ");
    lcd.write(byte(CH_CHAR_IND));
  
    int outputCh = chs[activeIndex] + 1; 
    
    if (outputCh < 10)
    {
      lcd.print("0");
    }
  
    lcd.print(outputCh);
  
    lcd.print(" ");
    lcd.write(byte(CC_CHAR_IND));
  
    int activeCC = ccs[ccIndices[activeIndex]];
    if (activeCC < 100)
    {
      lcd.print("0");
    }
  
    if (activeCC < 10)
    {
      lcd.print("0");
    }
  
    lcd.print(activeCC);
    lcd.print (" ");


    if (secondRowIsDirty)
    {
    
    
    //blank the second row
    lcd.setCursor(0, 1);
    lcd.print(SIXTEEN_SPACES);


    lcd.setCursor(0, 1);
    lcd.print(descriptions[ccIndices[activeIndex]]);
    
    secondRowIsDirty = false;
    }
  
    drawBarGraph(invals[activeIndex], BAR_ROW, BAR_COL, BAR_WIDTH);
  
  
    if (isCCSelected())
    {
      lcd.setCursor(7,0);
      lcd.cursor();
    }
  
    if (isCHSelected())
    {
      lcd.setCursor(3,0);
      lcd.cursor();
    }
  
    if (isCVSelected())
    {
      lcd.setCursor(0,0);
      lcd.cursor();
    }

    lastDisplayMillis = currTime;
  }

}

void scanButtons()
{
  bounceNext.update(); 
  bounceMode.update();
  
  if (bounceNext.fell())
  {
    nextClick();
  }

  if (bounceMode.fell())
  {
    modeClick();
  }
  

}

void nextClick(){
   if (uiState == CC_EDIT)
   {
     ccIndices[activeIndex]++;
     if (ccIndices[activeIndex] >= NUM_CC_OPTIONS)
     {
        ccIndices[activeIndex] = 0;
     }
   } else if (uiState == CV_EDIT)
   {
      activeIndex++;
      if (activeIndex >= IN_COUNT)
      {
        activeIndex = 0;
      }
   } else { // CH_EDIT
      chs[activeIndex]++;
      if (chs[activeIndex] >= CH_MAX)
      {
        chs[activeIndex] = 0;
      }
   }
   secondRowIsDirty = true;
}

void modeClick() {
  if (uiState == CC_EDIT)
  {
     uiState = CV_EDIT; 
  } else if (uiState == CV_EDIT)
  {
     uiState = CH_EDIT;
  } else {
     uiState = CC_EDIT;
  }
  
  secondRowIsDirty = true;
}

bool isCCSelected()
{
  return uiState == CC_EDIT;
}
bool isCHSelected()
{
  return uiState == CH_EDIT;
}
bool isCVSelected()
{
  return uiState == CV_EDIT;
}

void drawBarGraph(int val, int row, int col, int width) {

  int barSize = width * BAR_STATES;
  
  int barval = ((val * 1.0) / (CC_MAX * 1.0)) * barSize;
  
  lcd.setCursor(col, row);

  int printedChars = 0;
  
  while (barval > 0)
  {
      if (barval >= BAR_STATES)
      { //write a full bar character
        barval = barval - BAR_STATES;
        lcd.write(byte(BAR_STATES-1));
      } else { //write the last, partial bar character
        lcd.write(byte(barval));
        barval = 0;
      }
      printedChars++;
  }

  while (printedChars < width)
  { //blank the rest of the line
      lcd.print(" ");
      printedChars++;
  }

  
}

