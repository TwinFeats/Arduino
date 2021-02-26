#include <Arduino.h>
#include <PJONSoftwareBitBang.h>
#include <LiquidCrystal.h>
#include <TM1637TinyDisplay.h>
#include <arduino-timer.h>
#include <Tone.h>
#include "DFRobotDFPlayerMini.h"

LiquidCrystal lcd(52, 50, 42, 44, 46, 48);

Timer<10, millis, int> oneShotTimers;

// Lock combo is 4219

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
 * 11 - DFPlayer busy in
 * 12 - Color sensor LED
 * 13 - Card reader button
 *
 * 14 -
 * 15 -
 * 16 -
 * 17 -
 *
 * 18 - DFMini TX
 * 19 - DFMini RX
 * 20 SDA - Color sensor
 * 21 SCL - Color sensor
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
 * A6 - MP3 volume
 *
 */
Timer<1> brightnessTimer;

bool checkBrightness(void *t) {
  int val = analogRead(A1);
  if (val < 64) val = 64;
  if (abs(lastBrightness - val) > 32) {
    lastBrightness = val;
    mastermindLights.SetBrightness(val / 4);
    mastermindLights.Show();
  }
  return true;
}

void gameOver() {
  isGameOver = true;
  for (int i = 0; i < 5; i++) {
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

void penalizeSeconds(int secs) {
  countdownSecs -= secs;
}

void setCountdown(int secs) {
  countdownSecs = secs;
}

/* --------------------END countdown timer------------------ */

/* -------------------- DFPlayer -----------------------------*/
#define mp3BusyPin 11

DFRobotDFPlayerMini mp3Player;
// DO NOT change these constants, some logic depends on the ordering
// case closed during game
#define TRACK_COUNTDOWN_STILL_RUNNING 1
// case is first opened by plaer
#define TRACK_INTRO 2
// game power up successful
#define TRACK_POWERED_UP 3
// modem powered up
#define TRACK_MODEM_ON 4
// firewall powered up
#define TRACK_FIREWALL_ON 5
// control room powered up
#define TRACK_CONTROL_ROOM_ON 6
// reactor core powered up
#define TRACK_REACTOR_UP 7

#define TRACK_INCOMING_MSG 8
#define TRACK_MODEM_ACQUIRED 9
#define TRACK_FIREWALL_BREECHED 10
#define TRACK_CONTROL_ROOM_ACCESS_GRANTED 11
#define TRACK_REACTOR_DEACTIVATED 12
#define TRACK_CLOSING_MSG 13
#define TRACK_FUNCTION_INACCESSIBLE 14
// played to indicate wrong answer
#define TRACK_WRONG 15
#define TRACK_HACK_ATTEMPT_DETECTED 16

Timer<1> mp3Timer;
int mp3Queue[10];
int mp3Count = 0;
boolean mp3Playing = false;

void checkMp3Queue() {
  if (mp3Count > 0) {
    mp3Playing = true;
    mp3Player.play(mp3Queue[0]);
    for (int i=1;i<mp3Count;i++) {
      mp3Queue[i-1] = mp3Queue[i];
    }
    mp3Count--;
  }
}

void playTrack(int track);
void overridePlay(int track) {
    mp3Count = 0;
    mp3Player.stop();
    mp3Playing = false;
    playTrack(track);
}

void playTrack(int track) {
  if (mp3Count < 10) {
    mp3Queue[mp3Count++] = track;
    if (!mp3Playing) {
      checkMp3Queue();
    }
  } else {
    overridePlay(TRACK_HACK_ATTEMPT_DETECTED);
    setCountdown(countdownSecs >> 1);
  }
}

void playInfoThenTrack(int track) {
  playTrack(TRACK_INCOMING_MSG);
  playTrack(track);
}

boolean checkMp3Busy(void* t) {
  mp3Playing = (digitalRead(mp3BusyPin) == LOW);
  if (!mp3Playing) {
    checkMp3Queue();
  }
  return true;
}

void initMP3Player() {
  Serial1.begin(9600);
  mp3Player.begin(Serial1);
  mp3Player.volume(30);
  mp3Player.play(3);
  pinMode(mp3BusyPin, INPUT);
  mp3Timer.every(1000, checkMp3Busy);
}


/* --------------------END DFPlayer-------------------------*/

/* -------------------- LCD ---------------------------------*/
Timer<1> lcdTimer;
int msgCount = 0;
char *msgLine1Queue[10];
char *msgLine2Queue[10];
char msgBuffer[17];

char *mallocStringLiteral(const char *str) {
  return (char *)malloc(strlen(str)+1);
}

bool checkMsgQueue(void* t) {
  if (msgCount > 0) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(msgLine1Queue[0]);
    lcd.setCursor(0, 1);
    lcd.print(msgLine2Queue[0]);
    if (msgLine1Queue[0] != NULL) {
      free(msgLine1Queue[0]);
    }
    if (msgLine2Queue[0] != NULL) {
      free(msgLine2Queue[0]);
    }

    for (int i=1;i<msgCount;i++) {
      msgLine1Queue[i-1] = msgLine1Queue[i];
      msgLine2Queue[i-1] = msgLine2Queue[i];
    }
    msgCount--;
    lcdTimer.in(5000, checkMsgQueue); //schedule next msg check
  }
  return false; //one-shot only
}

void queueMsg(char *line1, char *line2) {
  if (msgCount < 10) {
    msgLine1Queue[msgCount] = line1;
    msgLine2Queue[msgCount++] = line2;
    if (msgCount == 1) {  //only one msg, display it now
      checkMsgQueue(NULL);
    }
  } else {
    overridePlay(TRACK_HACK_ATTEMPT_DETECTED);
    setCountdown(countdownSecs >> 1);
  }
}

void initLcd() {
//  lcdTimer.every(3000, checkMsgQueue);
}

/* ---------------END LCD -----------------------------------*/

/* -------------------- GAME STATE ---------------------------*/
boolean isGameOver = false;

byte arrowUp[8] = {
    B00000, B00100, B01110, B10101, B00100, B00100, B00000,
};

byte arrowDown[8] = {
    B00000, B00100, B00100, B10101, B01110, B00100, B00000,
};

#define CASE 20
#define SWITCH1 28
#define SWITCH2 29
#define SWITCH3 30
#define SWITCH4 31
#define SWITCH5 32
#define SWITCH6 33

// case is first supplied power
#define INITIAL 0
// This is not case power, but the power state for the game
#define POWER_OFF 1
#define POWER_ON 2
// aka Tones module
#define MODEM 3
// aka Mastermind Module
#define FIREWALL 4
// aka Keypad module
#define CONTROL_ROOM 5
// aka Blackbox module
#define REACTOR_CORE 6

#define POWER_SWITCHES 0
#define MODEM_SWITCHES 1
#define FIREWALL_SWITCHES 2
#define CONTROL_ROOM_SWITCHES 3
#define REACTOR_CORE_SWITCHES 4

uint8_t GAMES[5] = {POWER_ON, MODEM, FIREWALL, CONTROL_ROOM, REACTOR_CORE};
const char *gameNames[5] = {"Power", "Modem", "Firewall", "Control room",
                            "Reactor core"};

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
  for (int g = 0; g < 5; g++) {
    int found = -1;
    for (int s = 0; s < 6; s++) {
      if (switchesGame[g][s] != switchState[s]) {
        found = g;
      }
      break;
    }
    if (found != -1) {
      if (gameState == POWER_OFF && found != 0) return;
      if (gameState != found - 1) { //ensure current state is the one prev to the current switches
        playInfoThenTrack(TRACK_FUNCTION_INACCESSIBLE);
        return;
      }
      gameState = GAMES[found];
      playInfoThenTrack(found + 3);  //+3 because track 3 is the power up track
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
  for (int g = 0; g < 5; g++) {
    lcd.setCursor(0, 0);
    lcd.print(gameNames[g]);
    lcd.setCursor(0, 1);
    for (int s = 0; s < 6; s++) {
      lcd.print(switchesGame[g][s] == HIGH ? "\2" : "\1");
      lcd.print(" ");
    }
    delay(10000);
  }
  lcd.clear();
}

bool checkForDupSwitch(int g) {
  for (int i = 0; i < g; i++) {
    bool found = true;
    for (int s = 0; s < 6; s++) {
      if (switchesGame[i][s] != switchesGame[g][s]) {
        found = false;
        break;
      }
    }
    if (found) return true;
    found = true;
    // check for all off, which is invalid since it is starting state
    for (int s = 0; s < 6; s++) {
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
  pinMode(SWITCH1, INPUT_PULLUP);
  pinMode(SWITCH2, INPUT_PULLUP);
  pinMode(SWITCH3, INPUT_PULLUP);
  pinMode(SWITCH4, INPUT_PULLUP);
  pinMode(SWITCH5, INPUT_PULLUP);
  pinMode(SWITCH6, INPUT_PULLUP);
  for (int g = 0; g < 5; g++) {
    do {
      for (int s = 0; s < 6; s++) {
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
#define caseOpen 49

ButtonDebounce caseSwitch(caseOpen, 100);

void caseOpenClose(const int state) {
  if (gameState == INITIAL) {
    if (state == LOW) {
      // case was closed
      gameState = POWER_OFF;
    } else {
      // this shouldn't happen as case is open when connected to power, so
      // ignore and wait for case to close
    }
  } else {
    if (state == LOW) {
      // tried closing case while playing
      mp3Player.play(TRACK_COUNTDOWN_STILL_RUNNING);
    } else {
      if (gameState == POWER_OFF) {
        mp3Player.play(TRACK_INTRO);
      }
    }
  }
}

void initCase() { caseSwitch.setCallback(caseOpenClose); }


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
  clock.setBrightness(7);
  countdownRunning = true;
  timer.every(1000, updateCountdown);
  brightnessTimer.every(100, checkBrightness);
  initTone();
  initMP3Player();

  gameState = REACTOR_CORE;
  //  reportSwitches();
  // reportKeycode();
}

void loop() {
  oneShotTimers.tick();
  timer.tick();
  brightnessTimer.tick();
  lcdTimer.tick();
  mp3Timer.tick();
  if (isGameOver) {
  }
}