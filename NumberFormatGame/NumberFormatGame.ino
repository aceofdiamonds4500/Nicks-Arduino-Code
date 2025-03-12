/**
/   Project Name: The Number Format Game
/   Author: Nicholas Cieplensky
/   Date: February 26th, 2025
/   Parts Required:
/     x1 Arduino
/     x1 16x2 Liquid Crystal Display
/     x1 Keypad
/
/   Description: This is a simple number format conversion game, asking the user to convert one number format
/   to another (hex to binary, etc.). This keeps a score of how many the user gets right and displays a final
/   score at the end.
/   Note: I feel I could add a highscore board at the end, but I'll just keep track of it on paper because I
/   like keeping track of a highscore after the machine is off
**/

#include "Keypad.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const byte ROWS = 4; // number of rows
const byte COLS = 4; // number of columns
char keys[ROWS][COLS] = {
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};

byte rowPins[ROWS] = {7, 6, 5, 4};    // row    pinouts of the keypad R1 = D7, R2 = D6, R3 = D5, R4 = D4
byte colPins[COLS] = {3, 2, 1, 0};    // column pinouts of the keypad C1 = D3, C2 = D2, C3 = D1, C4 = D0
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int score = 0;            // score for users
const int numAmount = 10;  // upper bound for the arrays that also keeps track of how many unique questions there are
String answer = "";

const String hexHead = "HEX: 0x";   // ex. HEX: 0x[hex number]\n To BIN: [user input]
const String decHead = "DEC: ";
const String binHead = "BIN: ";
int headerLen = 0;                  // keeps track of length of header

int highscore[5] = {10,7,5,4,2}; 

// Very important that the conversions are parallel with position. hexNums[0] = decNums[0] = binNums[0], etc.
const String hexNums[numAmount] = 
{"AA",        "12",     "B6",        "14",     "C9",       "09",   "0D",   "1B",     "58",      "96"};
const String decNums[numAmount] = 
{"170",       "18",     "182",       "20",     "201",      "9",    "13",   "27",     "88",      "150"};
const String binNums[numAmount] = 
{"10101010",  "10010",  "10110110",  "10100",  "11001001", "1001", "1101", "11011",  "1011000", "10010110"};

void setup()
{
  lcd.begin(16,2);
  lcd.setCursor(0,0);
}
 
/**
/ Desc: This function clears the screen, then takes two arguments to print on a Liquid Crystal Display.
/ Params:
/   line1: prints line 1 on the LCD
/   line2: prints line 2
**/
void fullPagePrint(String line1, String line2){
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}


void loop()
{
  score = 0;
  fullPagePrint("Press * to play","DEC/HEX/BIN Game");
  
  char key = ' '; 

  while(key != '*'){
    key = keypad.getKey(); 
    if (key == '*'){
      break;
    }
    // else if (key == '#'){
    //   debugMode();
    //   break;
    // }
  }

  fullPagePrint("* to enter,","# to backspace");
  delay(3000);
  lcd.clear();

  while(gameLoop() == true) // "Score: " will continue to be displayed until a loss, then it skips to "Final score: "
  {
    lcd.clear();
    lcd.print("Score: ");
    lcd.print(score);
    delay(1500);
  }

  lcd.clear();
  lcd.print("Final Score: ");
  lcd.print(score);
  delay(1500);

  showHighScores();
  delay(3000);
}


/**
/ Desc: This is the main gameplay loop. It contains functionality for handling what happens when the user
/       gets the answer right or wrong and returns the status of success to loop().
/ Params:
/   bool success: determines if the user succeeds at answering correctly
/   char key: initializes a keypress as a character
/   String input: user input from getInput()
**/
boolean gameLoop(){
  bool success = true;  // true until user gets one wrong
  char key = ' ';

  printQuestion();

  lcd.cursor();

  String input = getInput();

  lcd.noCursor();
  lcd.clear();
  if(input != answer)
  {
    success = false;
    lcd.print("Incorrect!");
    delay(2000);
    fullPagePrint("Answer was: ", answer);
    // lcd.clear();               DEBUG
    // lcd.print("Input was: ");
    // lcd.setCursor(0,1);
    // lcd.print(input);
  }
  else
  {
    lcd.print("Correct!");
    lcd.setCursor(0,1);
    lcd.print("+1 point");
    score++;
  }

  delay(2000);
  input = "";
  return success;
}


/**
/ Desc: This function prints out a screen with a question and a field to answer in.
/       A question, for example:
/       BIN: 10110110
/       To HEX: 0x_
/ Params:
/   int numFormatQ: a random choice between hexNums, decNums, and binNums arrays
/   int numFormatA: another random number format choice, but different from numFormatQ
/   int numIdx: index of both arrays
**/
void printQuestion(){
  int numFormatQ = random(1,4);
  int numFormatA = random(1,4);
  while(numFormatA == numFormatQ)
  {
    numFormatA = random(1,4);
  }
  int numIdx = random(1,numAmount+1); //scales based on the amount of questions you add

  lcd.clear();
  switch(numFormatQ){                //display the question
    case 1:
      lcd.print(hexHead);
      lcd.print(hexNums[numIdx-1]);
      break;
    case 2:
      lcd.print(decHead);
      lcd.print(decNums[numIdx-1]);
      break;
    case 3:
      lcd.print(binHead);
      lcd.print(binNums[numIdx-1]);
      break;
    default:
      break;
  }

  lcd.setCursor(0,1);
  lcd.print("To ");
  switch(numFormatA){                //display the answer
    case 1:
      lcd.print(hexHead);
      headerLen = hexHead.length();
      answer = hexNums[numIdx-1];    //the answer will always be on the same index as the question's array, just in a different array
      break;
    case 2:
      lcd.print(decHead);
      headerLen = decHead.length();
      answer = decNums[numIdx-1];
      break;
    case 3:
      lcd.print(binHead);
      headerLen = decHead.length();
      answer = binNums[numIdx-1];
      break;
    default:
      break;
  }
}

/**
/ Desc: This function creates an input system for the user to answer 
/ Params:
/   char key: initializes a keypress as a character
/   String input: user input
**/
String getInput(){
  String input = "";
  char key = ' ';

  while(key != '*'){
    key = keypad.getKey();
    if(key == '#'){
      if(input.length() > 0)
      {
        input.remove(input.length()-1);
        lcd.setCursor(input.length()+headerLen+3,1);  //header length + length of "To " + length of input 
        lcd.print(" ");
        lcd.setCursor(input.length()+headerLen+3,1);   
      }
    }
    else if (key != NO_KEY && key != '*'){
      input.concat(key);
      lcd.print(key);
    }
  }
  return input;
}

//only use when you need to reset scores
void debugMode(){
  char key = ' '; 
  showHighScores();
  delay(3000);

  fullPagePrint("Reset scores?", "(*) YES, (#) NO");
  while(key != '#' || key != '*'){
    key = keypad.getKey(); 
    if (key == '*'){
      for (int i = 0 ; i < 5; i++) {
        EEPROM.write(i, highscore[i]);
      }
      fullPagePrint("Scores reset to", "default values.");
      delay(3000);
      loop();
    }
    else if(key == '#')
    {
      fullPagePrint("Scores not reset.","Returning...");
      delay(3000);
      loop();
    }

  }
}

void showHighScores(){
  if(score > EEPROM.read(5))
  {
    lcd.clear();
    lcd.print("New High Score!");
    delay(1000);
  }
  
  for(int i = 4; i >= 0; i--)
  {
    if(score > EEPROM.read(i))
    {
      if(i < 4) {EEPROM.write(i,i+1);}
      EEPROM.write(i,score);
    }
    else {break;}
  }

  lcd.clear();
  lcd.print("High scores: ");
  lcd.setCursor(0,1);
  for(int i = 0; i < 5; i++)
  {
    lcd.print(EEPROM.read(i));
    lcd.print(" ");
  }
}