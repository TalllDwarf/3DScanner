#include <LiquidCrystal.h>
#include <Stepper.h>

#define XPIN A0
#define YPIN A1

#define MAX_STEPS 5

#define STEPS 64

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
Stepper stepper(STEPS, 7, 8, 9, 10);

int X = 0, Y = 0;
char line1[16];

char serialIn = ' ';

//The how many steps should each step do
byte eachStep = 1;

void setup()
{
  Serial.begin(9600);
  
  //setup lcds column and rows
  lcd.begin(16,2);

  //Print hello world
  lcd.print("3D Turntable");
  lcd.setCursor(0,1);
  lcd.print("Connecting to PC");

  while(!Serial)
  {delay(10);}
  lcd.clear();

  lcd.setCursor(0,0);
  Serial.println("3D Turntable");
}

void loop()
{
  if(Serial.available() > 0)
  {
    serialIn = Serial.read();

    switch(serialIn)
    {
      case 's':
      case 'S':
        stepper.step(eachStep);
        delay(1);
        Serial.write('D');
      break;

      case 'e':
      case 'E':
        while(Serial.available() == 0)
        {delay(1);}
        eachStep = Serial.read();
      break;

      case 't':
      case 'T':
        Serial.write(STEPS);
      break;
    }
  }
  else if(serialIn != 's' && serialIn != 'S')
  {
    X = analogRead(XPIN);
    X = min(1024, (X + 10));
    
    //Y = analogRead(YPIN);    
    lcd.setCursor(0,1);

    char steps = map(X, 10, 1024, -MAX_STEPS, MAX_STEPS);

    sprintf(line1, "Step:%-2d X:%-4d", steps, X);
    
    lcd.print(line1);
    delay(10);
  }
}
