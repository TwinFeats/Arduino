#include <Arduino.h>
#include <ButtonDebounce.h>
#include <LiquidCrystal.h>
#include <NeoPixelBrightnessBus.h>
#include <TM1637TinyDisplay.h>
#include <arduino-timer.h>
//#include <Keypad.h>
#include <Tone.h>
#include "DFRobotDFPlayerMini.h"

NeoGamma<NeoGammaTableMethod> colorGamma;

RgbColor white(255, 255, 255);
RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor blue(0, 0, 255);
RgbColor yellow(255, 255, 0);
RgbColor cyan(28, 255, 255);
RgbColor pink(255, 0, 255);
RgbColor black(0, 0, 0);
RgbColor coral = colorGamma.Correct(RgbColor(255, 127, 80));
RgbColor purple = colorGamma.Correct(RgbColor(138, 43, 226));
RgbColor olive = colorGamma.Correct(RgbColor(128, 128, 0));
RgbColor rosyBrown = colorGamma.Correct(RgbColor(188, 143, 143));
RgbColor yellowGreen = colorGamma.Correct(RgbColor(154, 205, 50));
RgbColor beige = colorGamma.Correct(RgbColor(255, 235, 205));
RgbColor darkSeaGreen = colorGamma.Correct(RgbColor(143, 188, 143));
RgbColor orange = colorGamma.Correct(RgbColor(255,165,0));
RgbColor turquoise = colorGamma.Correct(RgbColor(175, 238, 238));
RgbColor plum = colorGamma.Correct(RgbColor(221, 160, 221));

LiquidCrystal lcd(52, 50, 42, 44, 46, 48);

int oneShotCount = 0;
Timer<10, millis, int> oneShotTimers;

//Lock combo is 4219

/*
 * PINS:
 * 2 - Mastermind LED
 * 3 - Blackbox LED (inner)
 * 4 - Audio RX
 * 5 - Audio TX
 * 6 - Panel 1 indicator
 * 7 - Panel 2 indicator
 * 8 - Panel 3 indicator
 * 9 - Panel 4 indicator
 * 10 - Tone OUT
 * 11 - 
 * 12 - 
 * 13 - 
 *
 * 14 - 
 * 15 - 
 * 16 - 
 * 17 - 
 * 
 * 18 - DFMini TX
 * 19 - DFMini RX
 *
 * 22 - Mastermind Button 1
 * 23 - Mastermind Button 2
 * 24 - Mastermind Button 3
 * 25 - Mastermind Button 4
 * 26 - Mastermind Button 5
 * 27 - Mastermind ENTER
 * 28 - Switch 1
 * 29 - Switch 2
 * 30 - Switch 3
 * 31 - Switch 4
 * 32 - Switch 5
 * 33 - Switch 6
 * 34 - Tone 1 button
 * 35 - Tone 2 button
 * 36 - Tone 3 button
 * 37 - Tone 4 button
 * 38 - Tone 5 button
 * 39 - DEAD
 * 40 - Tone Play
 * 41 - Countdown DIO
 * 42 - LCD D4
 * 43 - Countdown CLK
 * 44 - LCD D5
 * 45 - Blackbox BEAM button
 * 46 - LCD D6
 * 47 - BlackBox GUESS button
 * 48 - LCD D7
 * 49 - Case Open/Close
 * 50 - LCD E
 * 51 - BlackBox place marker button
 * 52 - LCD RS
 * 53 - Blackbox LED (outer)
 *
 * A1 - LED Brightness
 * A2 - Blackbox beam X
 * A3 - Blackbox beam Y
 * A4 - Blackbox marker X
 * A5 - Blackbox marker Y
 * A6 - Keypad
 * 
 */

/* -------------------- DFPlayer -----------------------------*/
DFRobotDFPlayerMini mp3Player;
//DO NOT change these constants, some logic depends on the ordering
//case closed during game
#define TRACK_COUNTDOWN_STILL_RUNNING           1
//case is first opened by plaer 
#define TRACK_INTRO                             2
//game power up successful
#define TRACK_POWERED_UP                        3
//modem powered up
#define TRACK_MODEM_ON                          4
//firewall powered up
#define TRACK_FIREWALL_ON                       5
//control room powered up
#define TRACK_CONTROL_ROOM_ON                   6
//reactor core powered up
#define TRACK_REACTOR_UP                        7

#define TRACK_INCOMING_MSG                      8
#define TRACK_MODEM_ACQUIRED                    9
#define TRACK_FIREWALL_BREECHED                 10
#define TRACK_CONTROL_ROOM_ACCESS_GRANTED       11
#define TRACK_REACTOR_DEACTIVATED               12
#define TRACK_CLOSING_MSG                       13
#define TRACK_FUNCTION_INACCESSIBLE             14
//played to indicate wrong answer
#define TRACK_WRONG                             15


void initMP3Player() {
  Serial1.begin(9600);
  mp3Player.begin(Serial1);
  mp3Player.volume(30);
  mp3Player.play(3);
}

bool playTrackContinue(int track) {
  mp3Player.play(track);
  return false;
}

bool playTrack(int track) {
  mp3Player.stop();
  oneShotTimers.in(20, playTrackContinue, track);
  return false;
}

void playInfoThenTrack(int track) {
  mp3Player.play(TRACK_INCOMING_MSG);
  oneShotTimers.in(10000, playTrack, track);
}
/* --------------------END DFPlayer-------------------------*/

/* -------------------- GAME STATE ---------------------------*/
boolean isGameOver = false;

byte arrowUp[8] = {
  B00000,
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00000,
};

byte arrowDown[8] = {
  B00000,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
  B00000,
};

#define CASE    20
#define SWITCH1 28
#define SWITCH2 29
#define SWITCH3 30
#define SWITCH4 31
#define SWITCH5 32
#define SWITCH6 33

//case is first supplied power
#define INITIAL         0
//This is not case power, but the power state for the game
#define POWER_OFF       1
#define POWER_ON        2
//aka Tones module
#define MODEM           3
//aka Mastermind Module
#define FIREWALL        4
//aka Keypad module
#define CONTROL_ROOM    5
//aka Blackbox module
#define REACTOR_CORE    6

#define POWER_SWITCHES        0
#define MODEM_SWITCHES        1
#define FIREWALL_SWITCHES     2
#define CONTROL_ROOM_SWITCHES 3
#define REACTOR_CORE_SWITCHES 4

uint8_t GAMES[5] = {POWER_ON, MODEM, FIREWALL, CONTROL_ROOM, REACTOR_CORE};
const char *gameNames[5] = {"Power", "Modem", "Firewall", "Control room", "Reactor core"};

uint8_t gameState = INITIAL;
uint8_t switchState[6] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
uint8_t switchesGame[5][6];

ButtonDebounce switch1(SWITCH1, 100);
ButtonDebounce switch2(SWITCH2, 100);
ButtonDebounce switch3(SWITCH3, 100);
ButtonDebounce switch4(SWITCH4, 100);
ButtonDebounce switch5(SWITCH5, 100);
ButtonDebounce switch6(SWITCH6, 100);

void checkSwitches() {
  for (int g=0;g<5;g++) {
    int found = -1;
    for (int s=0;s<6;s++) {
      if (switchesGame[g][s] != switchState[s]) {
        found = g;
      } break;
    }
    if (found != -1) {
      if (gameState == POWER_OFF && found != 0) return;
      if (gameState != found-1) {
        playInfoThenTrack(TRACK_FUNCTION_INACCESSIBLE);
        return;
      }
      gameState = GAMES[found];
      playInfoThenTrack(found+3);   //+3 because track 3 is the power up track
      break;
    }
  }
}

void switch1Pressed(const int state) {
  switchState[0] = state;
  checkSwitches();
}

void switch2Pressed(const int state) {
  switchState[1] = state;
  checkSwitches();
}

void switch3Pressed(const int state) {
  switchState[2] = state;
  checkSwitches();
}

void switch4Pressed(const int state) {
  switchState[3] = state;
  checkSwitches();
}

void switch5Pressed(const int state) {
  switchState[4] = state;
  checkSwitches();
}

void switch6Pressed(const int state) {
  switchState[5] = state;
  checkSwitches();
}

void reportSwitches() {
  lcd.clear();
  for (int g=0;g<5;g++) {
    lcd.setCursor(0,0);
    lcd.print(gameNames[g]);
    lcd.setCursor(0,1);
    for (int s=0;s<6;s++) {
      lcd.print(switchesGame[g][s] == HIGH?"\2":"\1");
      lcd.print(" ");
    }
    delay(10000);
  }
  lcd.clear();
}

bool checkForDupSwitch(int g) {
  for (int i=0;i<g;i++) {
    bool found = true;
    for (int s=0;s<6;s++) {
      if (switchesGame[i][s] != switchesGame[g][s]) {
        found = false;
        break;
      }
    }
    if (found) return true;
    found = true;
    //check for all off, which is invalid since it is starting state
    for (int s=0;s<6;s++) {
      if (switchesGame[i][s] != HIGH) {
        found = false;
        break;
      }
    }
    if (found) return true;
  }
  return false;
}

void initGameState() {
    for (int g=0;g<5;g++) {
      do {
        for (int s=0;s<6;s++) {
          switchesGame[g][s] = random(2);
        }
      } while (checkForDupSwitch(g));
    }
    switch1.setCallback(switch1Pressed);
    switch2.setCallback(switch2Pressed);
    switch3.setCallback(switch3Pressed);
    switch4.setCallback(switch4Pressed);
    switch5.setCallback(switch5Pressed);
    switch6.setCallback(switch6Pressed);
}

/* -----------------END GAME STATE ----------------------------*/

/* ------------------- CASE ----------------------------------*/
#define caseOpen    49

ButtonDebounce caseSwitch(caseOpen, 100);

void caseOpenClose(const int state) {
  if (gameState == INITIAL) {
    if (state == LOW) {
      //case was closed
      gameState = POWER_OFF;
    } else {
      //this shouldn't happen as case is open when connected to power, so ignore and wait for case to close
    }
  } else {
    if (state == LOW) {
      //tried closing case while playing
      mp3Player.play(TRACK_COUNTDOWN_STILL_RUNNING);
    } else {
      if (gameState == POWER_OFF) {
        mp3Player.play(TRACK_INTRO);
      }
    }
  }
}

void initCase() {
  caseSwitch.setCallback(caseOpenClose);
}

/* --------------------- MASTERMIND ---------------------------*/
#define MasterMindButton1 22
#define MasterMindButton2 23
#define MasterMindButton3 24
#define MasterMindButton4 25
#define MasterMindButton5 26
#define MasterMindEnter 27
#define MasterMindLED 2
#define Brightness 1

struct CLUE {
  uint8_t correct;
  uint8_t incorrect;
};

int lastBrightness = 0;

boolean isMasterMindRunning = true;
NeoPixelBrightnessBus<NeoRgbFeature, Neo400KbpsMethod> mastermindLights(
    5, MasterMindLED);

RgbColor colors[8] = {white, red, green, blue, yellow, cyan, pink, black};
char colorNames[8] = {'W', 'R', 'G', 'B', 'Y', 'C', 'P', 'E'};

RgbColor code[5], guess[5] = {black, black, black, black, black};
RgbColor guessCopy[5], codeCopy[5];
RgbColor bad(1, 1, 1), bad2(2, 2, 2);

uint8_t mmLights[5] = {7, 7, 7, 7, 7};

CLUE clue;
char guessColorNames[6] = {'E', 'E', 'E', 'E', 'E', '\0'};

ButtonDebounce mm1(MasterMindButton1, 100);
ButtonDebounce mm2(MasterMindButton2, 100);
ButtonDebounce mm3(MasterMindButton3, 100);
ButtonDebounce mm4(MasterMindButton4, 100);
ButtonDebounce mm5(MasterMindButton5, 100);
ButtonDebounce mmEnter(MasterMindEnter, 100);

void initCode() {
   for (uint8_t i=0;i<5;i++) {
     code[i] = colors[random(8)];
   }
}

boolean compareRGB(RgbColor color1, RgbColor color2) {
  return color1.R == color2.R && color1.G == color2.G && color1.B == color2.B;
}

void mastermindComplete() {
  lcd.setCursor(0,1);
  lcd.print("Firewall down!");
}

/* updates clue */
void evaluateGuess() {
  for (int i = 0; i < 5; i++) {
    guessCopy[i] = guess[i];
    codeCopy[i] = code[i];
  }
  clue.correct = 0;
  clue.incorrect = 0;
  for (int i = 0; i < 5; i++) {
    if (compareRGB(codeCopy[i], guessCopy[i])) {
      clue.correct++;
      guessCopy[i] = bad;
      codeCopy[i] = bad2;
    }
  }
  int i = 0;
next:
  while (i < 5) {
    for (int j = 0; j < 5; j++) {
      if (compareRGB(codeCopy[i], guessCopy[j])) {
        clue.incorrect++;
        guessCopy[j] = bad;
        codeCopy[i] = bad2;
        i++;
        goto next;
      }
    }
    i++;
  }
}
void debugColor() {
//  Serial.println(guessColorNames);
//  Serial.flush();
}

/* Updated guessColorNames */
void convertColorsToNames(RgbColor g[5]) {
  for (int i = 0; i < 5; i++) {
    for (int c = 0; c < 8; c++) {
      if (compareRGB(g[i], colors[c])) {
        guessColorNames[i] = colorNames[c];
        break;
      }
    }
  }
}

void showClue() {
  lcd.setCursor(0, 0);
  lcd.print(clue.correct);
  lcd.print(" exactly right");
  lcd.setCursor(0, 1);
  if (clue.correct == 5) {
    lcd.print("                ");
//    lcd.print("MODEM ACQUIRED");
  } else {
    lcd.print(clue.incorrect);
    lcd.print(" need reorder");
  }
}

void mmNextLight(int index) {
  mmLights[index] = (mmLights[index] + 1) % 8;
  guess[index] = colors[mmLights[index]];
  mastermindLights.SetPixelColor(index, guess[index]);
  mastermindLights.Show();
}

// state is HIGH/LOW
void mm1Pressed(const int state) {
  if (!isGameOver && isMasterMindRunning && gameState == FIREWALL && state == LOW) {
    mmNextLight(0);
  }
}

void mm2Pressed(const int state) {
  if (!isGameOver && isMasterMindRunning && gameState == FIREWALL && state == LOW) {
    mmNextLight(1);
  }
}

void mm3Pressed(const int state) {
  if (!isGameOver && isMasterMindRunning && gameState == FIREWALL && state == LOW) {
    mmNextLight(2);
  }
}

void mm4Pressed(const int state) {
  if (!isGameOver && isMasterMindRunning && gameState == FIREWALL && state == LOW) {
    mmNextLight(3);
  }
}

void mm5Pressed(const int state) {
  if (!isGameOver && isMasterMindRunning && gameState == FIREWALL && state == LOW) {
    mmNextLight(4);
  }
}

void mmEnterPressed(const int state) {
  if (!isGameOver && isMasterMindRunning && gameState == FIREWALL && state == LOW) {
    evaluateGuess();
    showClue();
    if (clue.correct == 5) {
      playInfoThenTrack(TRACK_MODEM_ACQUIRED);
      gameState++;
    }
  }
}

void initMasterMind() {
  pinMode(MasterMindButton1, INPUT_PULLUP);
  pinMode(MasterMindButton2, INPUT_PULLUP);
  pinMode(MasterMindButton3, INPUT_PULLUP);
  pinMode(MasterMindButton4, INPUT_PULLUP);
  pinMode(MasterMindButton5, INPUT_PULLUP);
  pinMode(MasterMindEnter, INPUT_PULLUP);

  mastermindLights.Begin();
  mastermindLights.Show();  // init lights off
  initCode();
  convertColorsToNames(code);
  debugColor();
  mm1.setCallback(mm1Pressed);
  mm2.setCallback(mm2Pressed);
  mm3.setCallback(mm3Pressed);
  mm4.setCallback(mm4Pressed);
  mm5.setCallback(mm5Pressed);
  mmEnter.setCallback(mmEnterPressed);
}

/* ------------------- END MASTERMIND------------------- */

/*
 * ----------------Countdown timer ------------------------
 */

#define COUNTDOWN_CLK 43
#define COUNTDOWN_DIO 41
Timer<1> timer;
TM1637TinyDisplay clock(COUNTDOWN_CLK, COUNTDOWN_DIO);

int countdownSecs = 60 * 60;
boolean countdownRunning = false;  // starts when case is closed then opened
                                   // (case starts open when powered on)

int convertSecsToTimeRemaining(int secs) {
  int mins = secs / 60;
  secs = secs % 60;
  mins = mins * 100;  // shift over 2 decimal digits
  return mins + secs;
}
/* --------------------END countdown timer------------------ */

/* ------------------- KEYPAD --------------------*/
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

const char *letters = "1234A456B789C*0#D";

byte rowPins[ROWS] = {10, 11, 12, 13}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {15, 15, 16, 16}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
//Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
Timer<1> keypadTimer;

char keypadCode[5];
char keypadEntered[5];
int keypadCount = 0;

char getKey() {
  int val = analogRead(6);
  if (val >= 1000) return '1'; //1
  if (val >= 900) return '2'; //2
  if (val >= 820) return '3'; //3
  if (val >= 750) return 'A'; //4
  if (val >= 660) return '4'; //5
  if (val >= 620) return '5'; //6
  if (val >= 585) return '6'; //7
  if (val >= 540) return 'B'; //8
  if (val >= 500) return '7'; //9
  if (val >= 475) return '8'; //10
  if (val >= 455) return '9'; //11
  if (val >= 425) return 'C'; //12
  if (val >= 360) return '*'; //13
  if (val >= 300) return '0'; //14
  if (val >= 255) return '#'; //15
  if (val >= 200) return 'D'; //16
  return 0;
}

bool handleKeypad(void *t) {
  if (gameState != CONTROL_ROOM) return true;
  char customKey = getKey();
  if (customKey){
    keypadEntered[keypadCount++] = customKey;
    if (keypadCount == 5){
      for (int i=0;i<5;i++) {
        if (keypadEntered[i] != keypadCode[i]) {
          mp3Player.play(TRACK_WRONG);
          return true;
        }
      }
      playInfoThenTrack(TRACK_CONTROL_ROOM_ACCESS_GRANTED);
      gameState++;
    }
  }
  return true;
}

void reportKeycode() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Keycode");
  lcd.setCursor(0,1);
  for (int s=0;s<5;s++) {
    lcd.print(keypadCode[s]);
    lcd.print(" ");
  }
  delay(10000);
  lcd.clear();
}

void initKeypad() {
  for (int i=0;i<5;i++) {
    keypadCode[i] = letters[random(16)];
  }
  keypadTimer.every(200, handleKeypad);
}

/* ---------------- END KEYPAD ---------------------*/

/* ----------------- TONES -------------------------*/
#define TONE_BUTTON_1 34
#define TONE_BUTTON_2 35
#define TONE_BUTTON_3 36
#define TONE_BUTTON_4 37
#define TONE_BUTTON_5 38
#define TONE_PIN      10
#define TONE_PLAY     40

Tone tonePlayer;
ButtonDebounce tone1(TONE_BUTTON_1, 100);
ButtonDebounce tone2(TONE_BUTTON_2, 100);
ButtonDebounce tone3(TONE_BUTTON_3, 100);
ButtonDebounce tone4(TONE_BUTTON_4, 100);
ButtonDebounce tone5(TONE_BUTTON_5, 100);
ButtonDebounce tonePlay(TONE_PLAY, 100);

int notes[] = {NOTE_C6, NOTE_D6, NOTE_E6, NOTE_F6, NOTE_G6};

#define NOTES_LENGTH 15
uint8_t numNotesPlayed = 0;
int song[NOTES_LENGTH];
int notesPlayed[NOTES_LENGTH];
boolean modemComplete = false;

void checkNotes() {
  if (numNotesPlayed != NOTES_LENGTH) return;
  for (int i=0;i<NOTES_LENGTH;i++) {
    if (notesPlayed[i] != song[i]) return;
  }
  playInfoThenTrack(TRACK_MODEM_ACQUIRED);
  modemComplete = true;
  gameState++;
}

void tone1Pressed(const int state) {
  if (gameState == MODEM && numNotesPlayed < NOTES_LENGTH) {
    tonePlayer.play(notes[0]);
    notes[numNotesPlayed++] = 0;
    checkNotes();
  }
}

void tone2Pressed(const int state) {
  if (gameState == MODEM && numNotesPlayed < NOTES_LENGTH) {
    tonePlayer.play(notes[1]);
    notes[numNotesPlayed++] = 0;
    checkNotes();
  }
}

void tone3Pressed(const int state) {
  if (gameState == MODEM && numNotesPlayed < NOTES_LENGTH) {
    tonePlayer.play(notes[2]);
    notes[numNotesPlayed++] = 0;
    checkNotes();
  }
}

void tone4Pressed(const int state) {
  if (gameState == MODEM && numNotesPlayed < NOTES_LENGTH) {
    tonePlayer.play(notes[3]);
    notes[numNotesPlayed++] = 0;
    checkNotes();
  }
}

void tone5Pressed(const int state) {
  if (gameState == MODEM && numNotesPlayed < NOTES_LENGTH) {
    tonePlayer.play(notes[4]);
    notes[numNotesPlayed++] = 0;
    checkNotes();
  }
}

bool playNextNote(int note) {
  Serial.print("playNextNote ");
  Serial.println(note);
  tonePlayer.play(notes[note++]);
  if (note < 5) {
    oneShotTimers.in(1000, playNextNote, note);
  } else {
    Serial.println("stop");
    numNotesPlayed = 0;
    tonePlayer.stop();
  }
  return false;
}

void tonePlayPressed(const int state) {
  if (gameState == MODEM) {
    oneShotTimers.in(1000, playNextNote, 0);
  }
}

void initTone() {
  pinMode(TONE_BUTTON_1, INPUT_PULLUP);
  pinMode(TONE_BUTTON_2, INPUT_PULLUP);
  pinMode(TONE_BUTTON_3, INPUT_PULLUP);
  pinMode(TONE_BUTTON_4, INPUT_PULLUP);
  pinMode(TONE_BUTTON_5, INPUT_PULLUP);
  pinMode(TONE_PLAY, INPUT_PULLUP);
  tone1.setCallback(tone1Pressed);
  tone2.setCallback(tone2Pressed);
  tone3.setCallback(tone3Pressed);
  tone4.setCallback(tone4Pressed);
  tone5.setCallback(tone5Pressed);
  tonePlay.setCallback(tonePlayPressed);
  tonePlayer.begin(TONE_PIN);
  for (int i=0;i<NOTES_LENGTH;i++) {
    song[i] = random(5);
  }
  oneShotTimers.in(1000, playNextNote, 0);
}
/* --------------END TONES -------------------------*/

/* ----------------- BLACKBOX ----------------------*/
#define BLACKBOX_BEAM_X   2
#define BLACKBOX_BEAM_Y   3
#define BLACKBOX_MARKER_X 4
#define BLACKBOX_MARKER_Y 5
#define BLACKBOX_OUTER_LED 53
#define BLACKBOX_INNER_LED 3
#define BLACKBOX_BEAM_BUTTON 45
#define BLACKBOX_GUESS_BUTTON 47
#define BLACKBOX_MARKER_BUTTON 51

NeoPixelBrightnessBus<NeoRgbFeature, Neo400KbpsMethod> blackboxBeamLights(53, BLACKBOX_OUTER_LED);
NeoPixelBrightnessBus<NeoGrbFeature, Neo400KbpsMethod> blackboxMarkerLights(5, BLACKBOX_INNER_LED);
ButtonDebounce bbBeamButton(BLACKBOX_BEAM_BUTTON, 100);
ButtonDebounce bbGuessButton(BLACKBOX_GUESS_BUTTON, 100);
ButtonDebounce bbMarkerButton(BLACKBOX_MARKER_BUTTON, 100);
Timer<1> bbBeamJoystickTimer;
Timer<1> bbMarkerJoystickTimer;

int currentBeamLight = 0, currentMarkerLight = 0;

void bbBeamPressed(const int state) {
  if (gameState != REACTOR_CORE) return;
  if (blackboxBeamLights.GetPixelColor(currentBeamLight) == black || blackboxBeamLights.GetPixelColor(currentBeamLight) == white) {

  }
}

void bbGuessPressed(const int state) {
  if (gameState != REACTOR_CORE) return;

}

void bbMarkerPressed(const int state) {
  if (gameState != REACTOR_CORE) return;

}

bool checkBeamJoystick(void *t) {
  if (gameState != REACTOR_CORE) return true;
  int oldCurrentBeamLight = currentBeamLight;
  RgbColor oldColor = blackboxBeamLights.GetPixelColor(currentBeamLight);
  int x = analogRead(BLACKBOX_BEAM_X);
  int y = analogRead(BLACKBOX_BEAM_Y);
  int diffX = 512-x;
  int diffY = 512-y;
  if (diffX < -50) {        //moving left
    if (currentBeamLight > 8 && currentBeamLight < 16) {
      currentBeamLight--;
    } else if (currentBeamLight > 23 && currentBeamLight < 31) {
      currentBeamLight++;
    }
  } else if (diffX > 50) {  //moving right
    if (currentBeamLight > 7 && currentBeamLight < 15) {
      currentBeamLight++;
    } else if (currentBeamLight > 24) {
      currentBeamLight--;
    }
  }
  if (diffY < -50) {        //moving down
    if (currentBeamLight > 0 && currentBeamLight < 8) {
      currentBeamLight--;
    } else if (currentBeamLight > 15 && currentBeamLight < 23) {
      currentBeamLight++;
    }
  } else if (diffY > 50) {  //moving up
    if (currentBeamLight < 7) {
      currentBeamLight++;
    } else if (currentBeamLight > 16 && currentBeamLight < 24) {
      currentBeamLight--;
    }
  }
  if (oldCurrentBeamLight != currentBeamLight) {
    blackboxBeamLights.SetPixelColor(oldCurrentBeamLight, oldColor);  //is default color black?
    blackboxBeamLights.SetPixelColor(currentBeamLight, white);
  }
  return true;
}

// origin is lower right corner!
bool checkMarkerJoystick(void *t) {
  if (gameState != REACTOR_CORE) return true;
  int oldCurrentMarkerLight = currentMarkerLight;
  int row = currentMarkerLight / 8;
  int col = 7 - (currentMarkerLight % 8);
  RgbColor oldColor = blackboxMarkerLights.GetPixelColor(currentMarkerLight);
  int x = analogRead(BLACKBOX_MARKER_X);
  int y = analogRead(BLACKBOX_MARKER_Y);
  int diffX = 512-x;
  int diffY = 512-y;
  if (diffX < -50) { // left
    if (col < 7) {
      currentMarkerLight += 1;
    }

  } else if (diffX > 50) { // right
    if (col > 0) {
      currentMarkerLight += 1;
    }
  }
  if (diffY < -50) { // down
    if (row > 0) {
      currentMarkerLight -= 8;
    }

  } else if (diffY > 50) { // up
    if (row < 8) {
      currentMarkerLight += 8;
    }
  }
  if (oldCurrentMarkerLight != currentMarkerLight) {
    blackboxMarkerLights.SetPixelColor(oldCurrentMarkerLight, oldColor);  //is default color black?
    blackboxMarkerLights.SetPixelColor(currentMarkerLight, white);
  }

  return true;
}

void initBlackbox() {
  blackboxMarkerLights.Begin();
  blackboxMarkerLights.Show();
  blackboxBeamLights.Begin();
  blackboxBeamLights.Show();
  bbBeamButton.setCallback(bbBeamPressed);
  bbGuessButton.setCallback(bbGuessPressed);
  bbMarkerButton.setCallback(bbMarkerPressed);
  bbBeamJoystickTimer.every(200, checkBeamJoystick);
  bbMarkerJoystickTimer.every(200, checkMarkerJoystick);
}
/* --------------END BLACKBOX ----------------------*/

Timer<1> brightnessTimer;

bool checkBrightness(void *t) {
  int val = analogRead(Brightness);
  if (val < 64) val = 64;
  if (abs(lastBrightness - val) > 32) {
    lastBrightness = val;
    mastermindLights.SetBrightness(val / 4);
//    Serial.println((int)(val / 4));
    mastermindLights.Show();
  }
  return true;
}

void gameOver() {
    isGameOver = true;
    for (int i=0;i<5;i++) {
      guess[i] = black;
    }
    mastermindLights.Show();
}

bool blinkingCountdown = false;

bool updateCountdown(void *t) {
  if (isGameOver) {
    if (blinkingCountdown) {
      clock.showNumberDec(0, 0b01000000, true);
    } else {
      clock.showString("    ");
    }
    blinkingCountdown = !blinkingCountdown;
    return true;
  }
  if (!countdownRunning) return true;
  countdownSecs--;
  if (countdownSecs < 0) {
    countdownRunning = false;
    countdownSecs = 0;
    gameOver();
  }
  int num = convertSecsToTimeRemaining(countdownSecs);
  //  Serial.println(num);
  clock.showNumberDec(num, 0b01000000, true);
  return true;
}

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;  // wait for serial attach
    Serial.println("Starting");
  lcd.begin(16, 2);
  lcd.clear();
  lcd.createChar(1, arrowUp);
  lcd.createChar(2, arrowDown);
  randomSeed(analogRead(0));
  initGameState();
  initMasterMind();
  clock.setBrightness(7);
  countdownRunning = true;
  timer.every(1000, updateCountdown);
  brightnessTimer.every(100, checkBrightness);
  initTone();
  initKeypad();
  initMP3Player();
  initBlackbox();
//  reportSwitches();
//  reportKeycode();
}

void loop() {
  mm1.update();
  mm2.update();
  mm3.update();
  mm4.update();
  mm5.update();
  mmEnter.update();

  oneShotTimers.tick();
  timer.tick();
  brightnessTimer.tick();
  keypadTimer.tick();
  bbBeamJoystickTimer.tick();
  bbMarkerJoystickTimer.tick();

  if (isGameOver) {

  }
}