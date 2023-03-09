#include "OLED_Driver.h"
#include "GUI_paint.h"
#include "DEV_Config.h"
#include "Debug.h"
#include "ImageData.h"

#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>

#define RST_PIN         9          // Configurable, see typical pin layout above
#define SS_PIN          2 
#define BUZZER          4

//IS REVERSED X cu Y ca ii orientat mai bine asa joystickul
#define JOYSTICK_X      A6
#define JOYSTICK_Y      A5
#define JOYSTICK_BTN    3

unsigned int xValueJoy = 0;
unsigned int yValueJoy = 0;


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

struct point{
  unsigned int x;
  unsigned int y;
};

//    PIEXELS DISPLAY
//  LATIME    96  de la 0 x 5 pana la 0 x 19 incepe ultimul
      //in modul box de la 1 la 18
//  INALTIME  64  de la 0 x 5 pana la 0 x 12 incepe ultimul
      // in modul de BOX de la 4 la 12


//    UID CARDS
//  31022 -> player 1
//  8481  -> player 2


//    MEMORY ZONES
//  Player 1  ->  x = 1
//  Player 2  ->  x = 5
//  HIGH SCORE: x10x11 -> 2 bytes number
//  at 50 and 51 is the seed 50 first byte

unsigned int currentPlayer = 1;
unsigned int newPlayer = 1;

point snakeLocation[50];
int snakeLength = 4;

bool isThereFood = true;
point foodLocation;

unsigned int score = 0;
unsigned int highScore = 0;
int direction = 0;

void drawPoint(int startPointX, int startPointY, UWORD color, DOT_PIXEL pixelSize, DOT_STYLE style)
{
  Paint_DrawPoint(5 * startPointX, 5 * startPointY, color, pixelSize, style);
}

void movingLine (){
  Paint_DrawLine(30, 18, 51, 18, RED, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
  delay(500);
  point startPoint;
  startPoint.x = 10;
  startPoint.y = 6;
  point endPoint;
  endPoint.x = 17;
  endPoint.y = 6;


  while(1)
  {
    drawPoint(startPoint.x, startPoint.y, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    startPoint.x+=1;
    endPoint.x+=1;
    drawPoint(endPoint.x, endPoint.y, RED, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    delay(500);
  }

}

void writeScore(int score)
{
  Paint_DrawChar(87, 0, ((score%10) +'0'), &Font12, MAGENTA, BLACK);
  if(score > 9)
  {
  Paint_DrawChar(80, 0, ((score/10) +'0'), &Font12, MAGENTA, BLACK);
  }
}

void drawSnake()
{
  drawPoint(snakeLocation[0].x, snakeLocation[0].y, YELLOW, DOT_PIXEL_3X3, DOT_STYLE_DFT);
  for(int i = 1 ; i < snakeLength; i++)
  {
      drawPoint(snakeLocation[i].x, snakeLocation[i].y, GREEN, DOT_PIXEL_3X3, DOT_STYLE_DFT);
  }
  drawPoint(foodLocation.x, foodLocation.y, MAGENTA, DOT_PIXEL_3X3, DOT_STYLE_DFT);
}

void drawDeathScreen()
{
    Paint_DrawString_EN(3, 5, "You Lost", &Font16, RED, WHITE);
    Paint_DrawString_EN(27, 22, "Again?", &Font12, BLACK, WHITE);
    Paint_DrawString_EN(31, 37, ">YES<", &Font12, DARKGREEN, WHITE);
    Paint_DrawString_EN(35, 52, ">NO<", &Font12, BLACK, DARKGREEN);
    bool yes = false;

    tone(BUZZER, 1000); // Send 1KHz sound signal...
    delay(300);        // ...for 1 sec
    noTone(BUZZER);     // Stop sound...
    delay(100);
    tone(BUZZER, 1000); // Send 1KHz sound signal...
    delay(300);        // ...for 1 sec
    noTone(BUZZER);
    Serial.println("Score");
    Serial.println(score);

    Serial.println("HighScore");
    Serial.println(highScore);
    if(score > highScore)
    {
      if(currentPlayer == 1)
      {
        EEPROM.update(110, score/255);
        EEPROM.update(111, score%255);
      }
      else
      {
        EEPROM.update(510, score/255);
        EEPROM.update(511, score%255);
      }
      highScore = score;
    }

    while(digitalRead(JOYSTICK_BTN) == HIGH)
    {
      yValueJoy = 1024-analogRead(JOYSTICK_Y);
      if(yValueJoy < 300 || yValueJoy > 800)
      {
          yes = !yes;
          if(!yes)
          {
            Paint_DrawString_EN(31, 37, ">YES<", &Font12, DARKGREEN, WHITE);
            Paint_DrawString_EN(35, 52, ">NO<", &Font12, BLACK, DARKGREEN);
          }
          else
          {
            Paint_DrawString_EN(31, 37, ">YES<", &Font12, BLACK, DARKGREEN);
            Paint_DrawString_EN(35, 52, ">NO<", &Font12, DARKGREEN, WHITE);
          }
          delay(300);
      }
    }
    //Serial.println(digitalRead(JOYSTICK_BTN));
    if(!yes)
    {
      resetGame();
    }
    else
    {
      drawMainMenu();
    }
}

void checkIfDead()
{
    bool alive = true;
    for(int i = 1; i < snakeLength && alive; i++)
    {
      if(snakeLocation[0].x == snakeLocation[i].x && snakeLocation[0].y == snakeLocation[i].y)
      {
        alive = false;
        drawDeathScreen();
      }
    }
    if(snakeLocation[0].x == 0 || snakeLocation[0].x == 19 || snakeLocation[0].y == 3 || snakeLocation[0].y == 13)
    {
      drawDeathScreen();
    }
}

void moveSnake(int directionLocal)
{
    if(isThereFood == false)
    {
      drawPoint(foodLocation.x, foodLocation.y, MAGENTA, DOT_PIXEL_3X3, DOT_STYLE_DFT);
      isThereFood = true;
    }
    point newPoint;

    if(snakeLocation[0].x == foodLocation.x && snakeLocation[0].y == foodLocation.y)
    {
      score++;
      Serial.println("SAAA MANCAT MANCAREA");
      isThereFood = false;
      snakeLocation[snakeLength].x = snakeLocation[snakeLength - 1].x;
      snakeLocation[snakeLength].y = snakeLocation[snakeLength - 1].y;
    }
    else
    {
      drawPoint(snakeLocation[snakeLength - 1].x, snakeLocation[snakeLength - 1].y, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    }

    // 0 -> right
    if(directionLocal == 0)
    {
        newPoint.x = snakeLocation[0].x + 1;
        newPoint.y = snakeLocation[0].y;
    }

    // 1 -> left
    else if(directionLocal == 1)
    {
        newPoint.x = snakeLocation[0].x - 1;
        newPoint.y = snakeLocation[0].y;
    }

    //  2 -> up
    else if(directionLocal == 2)
    {
        newPoint.x = snakeLocation[0].x;
        newPoint.y = snakeLocation[0].y - 1;
    }

  //    down
    else
    {
      newPoint.x = snakeLocation[0].x;
      newPoint.y = snakeLocation[0].y + 1;
    }
    
    for(int i = snakeLength - 1; i > 0; i--)
    {
      Serial.println(i);
      snakeLocation[i].x = snakeLocation[i-1].x;
      snakeLocation[i].y = snakeLocation[i-1].y;
    }
  
    snakeLocation[0].x = newPoint.x;
    snakeLocation[0].y = newPoint.y;

    drawPoint(snakeLocation[1].x, snakeLocation[1].y, GREEN, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    drawPoint(snakeLocation[0].x, snakeLocation[0].y, YELLOW, DOT_PIXEL_3X3, DOT_STYLE_DFT);

    if(snakeLocation[0].x == foodLocation.x && snakeLocation[0].y == foodLocation.y)
    {
      snakeLength++;
    }

    checkIfDead();
}

void getNewFoodLocation()
{
  bool ok = true;
  do
    {
      ok = true;
      foodLocation.x = random(1, 19);
      foodLocation.y = random(4, 12);
      for(int i = 0; i < snakeLength && ok==true; i++)
      {
        if(foodLocation.x == snakeLocation[i].x && foodLocation.y == snakeLocation[i].y)
        {
            Serial.println("FAILED");
            ok = false;
        }
      }
    }while(ok==false);
}

int waitAndDetermineOutput(int oldDirection)
{
  if(isThereFood == false)
  {
    getNewFoodLocation();
  }
  int direction = oldDirection;
  for(int i = 1; i <= 55; i++)
    {    
      Driver_Delay_ms(6);
      // because of the orientation of the joystick
      xValueJoy = 1024-analogRead(JOYSTICK_X);
      yValueJoy = 1024-analogRead(JOYSTICK_Y);
      if(xValueJoy > 800)
      {
        direction = 0;
      }
      else if(xValueJoy < 300)
      {
        direction = 1;
      }
      else if(yValueJoy > 800)
      {
        direction = 2;
      }
      else if(yValueJoy < 300)
      {
        direction = 3;
      }
    }
    if(direction + oldDirection == 1 || direction+oldDirection == 5)
      direction = oldDirection;
  return direction;
}

void resetGame()
{
  score = 0;
  snakeLength = 4;
  isThereFood = true;
  direction = 0;

  OLED_0in95_rgb_Clear();  
  int firstByte = EEPROM.read(50);
  int secondByte = EEPROM.read(51);
  int seed = firstByte*8 + secondByte;
  randomSeed(seed);
  seed++;
  EEPROM.update(50, seed/255);
  EEPROM.update(51, seed%255);

  Serial.println("seeeeed");
  Serial.println(seed);

  Paint_DrawLine(0, 64, 94, 64, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(0, 16, 94, 16, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(1, 16, 1, 64, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(94, 16, 94, 64, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  Paint_DrawString_EN(30, 2, "SNAKE", &Font12, BLACK, GREEN);
  //drawPoint(12, 12, GREEN, DOT_PIXEL_3X3, DOT_STYLE_DFT);
  //movingLine();
  snakeLocation[3].x = 3;
  snakeLocation[3].y = 6;
  snakeLocation[2].x = 4;
  snakeLocation[2].y = 6;
  snakeLocation[1].x = 5;
  snakeLocation[1].y = 6;
  snakeLocation[0].x = 6;
  snakeLocation[0].y = 6;
  foodLocation.x = random(8, 19);
  foodLocation.y = random(7, 13);
  drawSnake();
  //drawDeathScreen();
  delay(200);
}


signed long getID(){
              if ( ! mfrc522.PICC_IsNewCardPresent()) {
                        return 0;
            }
            // Select one of the cards
            if ( ! mfrc522.PICC_ReadCardSerial()) {
                        return 0;
            }
  unsigned long hex_num;
  hex_num =  mfrc522.uid.uidByte[0] << 24;
  hex_num += mfrc522.uid.uidByte[1] << 16;
  hex_num += mfrc522.uid.uidByte[2] <<  8;
  hex_num += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); // Stop reading
  return hex_num;
}

void drawMainMenu()
{
  OLED_0in95_rgb_Clear();  
  delay(100);
  if(currentPlayer == 1)
  {
    Paint_DrawString_EN(15, 12, ">PLAY<", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(52, 52, "Andrei", &Font12, BLACK, GREEN);
    highScore = EEPROM.read(110)*255 + EEPROM.read(111);
  }
  else
  {
    Paint_DrawString_EN(15, 12, ">PLAY<", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(62, 52, "Ale", &Font12, BLACK, PINK);
    highScore = EEPROM.read(510)*255 + EEPROM.read(511);
  }
  Paint_DrawString_EN(6, 30, "Highscore: ", &Font12, BLACK, GRAY);
  char highScoreString[3];
  itoa(highScore, highScoreString, 10);
  Paint_DrawString_EN(76, 30, highScoreString, &Font12, BLACK, GRAY);
  while(digitalRead(JOYSTICK_BTN) == HIGH)
    {
      delay(5);
    }
  resetGame();

}

void tryToLogIn()
{
  int x = getID();
  while(x != 31022 && x != 8481)
  {
    x = getID();
  }
  if(x == 31022)
  {
    currentPlayer = 1;
  }
  else
  {
    currentPlayer = 2;
  }
  drawMainMenu();
}

void drawLogIn()
{
  OLED_0in95_rgb_Clear();  
  Paint_DrawString_EN(13, 5, "SNAKE", &Font20, BLACK, GREEN);
  Paint_DrawString_EN(7, 27, "Present card", &Font12, BLACK, WHITE);
  int firstPara[11] = {36, 37, 38, 38, 39, 39, 39, 38, 38, 37, 36};
  for(int i = 0 ; i < 11; i++)
  {
    Paint_DrawPoint(firstPara[i],  i+47, WHITE, DOT_PIXEL_1X1, DOT_STYLE_DFT);
  }
  int secondPara[16] = {45, 46, 46, 47, 47, 47, 48, 48, 48, 48, 47, 47, 47, 46, 46, 45};
  for(int i = 0 ; i < 16; i++)
  {
    Paint_DrawPoint(secondPara[i],  i+44, WHITE, DOT_PIXEL_1X1, DOT_STYLE_DFT);
  }
  int thirdPara[24] = {54, 55, 55, 56, 56, 57, 57, 58, 58, 58, 59, 59, 59, 59, 58, 58, 58, 57, 57, 56, 56, 55, 55, 54};
  for(int i = 0 ; i < 24; i++)
  {
    Paint_DrawPoint(thirdPara[i],  i+40, WHITE, DOT_PIXEL_1X1, DOT_STYLE_DFT);
  }
  tryToLogIn();
}

void setup() {

System_Init();
pinMode(JOYSTICK_BTN, INPUT_PULLUP);
pinMode(BUZZER, OUTPUT);

  Serial.begin(115200);
  while (!Serial);                       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
            SPI.begin();                             // Init SPI bus
            mfrc522.PCD_Init();              // Init MFRC522
            delay(4);                                             // Optional delay. Some board do need more time after init to be ready, see Readme
            mfrc522.PCD_DumpVersionToSerial();      // Show details of PCD - MFRC522 Card Reader details
            Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  if(USE_IIC) {
    Serial.print("Only USE_SPI_4W, Please revise DEV_config.h !!!");
    return 0;
  }
  
  Serial.print(F("OLED_Init()...\r\n"));
  OLED_0in95_rgb_Init();
  Driver_Delay_ms(500); 
  OLED_0in95_rgb_Clear();  

  //1.Create a new image size
  UBYTE *BlackImage;
  Serial.print("Paint_NewImage\r\n");
  Paint_NewImage(BlackImage, OLED_0in95_RGB_WIDTH, OLED_0in95_RGB_HEIGHT, 0, BLACK);  
  Paint_SetScale(65);

  Paint_DrawString_EN(20, 0, "Pusik  Ale", &Font16, BLACK, RED);
  Serial.println(EEPROM.length());
  Driver_Delay_ms(1000);  
  drawLogIn(); 
}

void loop() {
            direction = waitAndDetermineOutput(direction);
            moveSnake(direction);
            writeScore(score);
            // Dump debug info about the card; PICC_HaltA() is automatically called
            /*if(getID() == 31022)
            {
                newPlayer = 1;
            }
            else if(getID() == 8481)
                  {
                    newPlayer = 2;
                  }
            if(currentPlayer != newPlayer)
            {
              currentPlayer = newPlayer;
              Serial.println(currentPlayer);
              Serial.println(EEPROM.read(currentPlayer*100+20));
              Serial.println(EEPROM.read(currentPlayer*100+20)*10 +  EEPROM.read(currentPlayer*100+21));
            }*/
}
