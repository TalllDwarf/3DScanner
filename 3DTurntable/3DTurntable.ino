#include <LiquidCrystal.h>
#include <Stepper.h>

#define XPIN A0
#define YPIN A1
#define BUTTON 13

#define MAX_MOTOR_SPEED 10
#define STEPS 2049
#define FULL_REV (STEPS * 7)

LiquidCrystal lcd(8, 7, 5, 4, 3, 2);
Stepper stepper(STEPS, 12,11,10,9);

int X = 0;
char line1[16];

char serialIn = ' ';

unsigned char numberOfTurns = 0;
unsigned int singleStep = 0;

//The how many steps should each step do
int eachStep = 1;
int currentStep = 0;

void setup()
{
  Serial.begin(9600);

  pinMode(BUTTON, INPUT);

  stepper.setSpeed(MAX_MOTOR_SPEED);
  
  //setup lcds column and rows
  lcd.begin(16,2);

  //Print hello world
  lcd.print("  3D Turntable  ");
  Serial.println("3D Turntable");

  while(!Serial)
  {
    delay(10);
    lcd.setCursor(0,1);
    sprintf(line1, "Waiting for PC ");
    lcd.print(line1); 
  }
  
}

void StopStepper()
{
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
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
      if(singleStep != 0)
      {
        lcd.setCursor(0,1);
        sprintf(line1, "Turn:%-4d/%-4d", currentStep, numberOfTurns);
        lcd.print(line1);
        
        stepper.setSpeed(MAX_MOTOR_SPEED);
        stepper.step(singleStep);
        StopStepper();
        currentStep++;
        delay(10);
        if(currentStep >= numberOfTurns)
          Serial.write('F');
        else 
          Serial.write('D');
      }
      break;

      case 'r':
      case 'R':
        currentStep = 1;
        lcd.setCursor(0,1);
        sprintf(line1, "Reset       ");
        lcd.print(line1); 
      break;

      case 'i':
      case 'I':
        stepper.setSpeed(MAX_MOTOR_SPEED);
        stepper.step(singleStep);
        StopStepper();
        delay(10);
        Serial.write('D');
        lcd.setCursor(0,1);
        sprintf(line1, "Single Step ");
        lcd.print(line1); 
      break;

      case 't':
      case 'T':
        while(Serial.available() == 0) {}
       
        numberOfTurns = Serial.read();
        singleStep = FULL_REV / (unsigned int)numberOfTurns;
      break;
    }
  }
  else if(serialIn != 's' && serialIn != 'S')
  {
    if(digitalRead(BUTTON) == 0)
    {
      lcd.setCursor(0,1);
      sprintf(line1, "Full rotation  ");
      lcd.print(line1);
      
      stepper.setSpeed(MAX_MOTOR_SPEED);
      stepper.step(FULL_REV);
      StopStepper();

      lcd.setCursor(0,1);
      sprintf(line1, "Finished       ");
      lcd.print(line1);
    }
    else
    {
      X = analogRead(XPIN);
      X = min(1024, (X + 10));
  
      char steps = map(X, 10, 1024, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
  
      while(steps != 0)
      {
        lcd.setCursor(0,1);
        sprintf(line1, "Speed:%-3d%", steps);
        lcd.print(line1);
        stepper.setSpeed(abs(steps));

        if(steps > 0)
          stepper.step(10);
        else
          stepper.step(-10);

        X = analogRead(XPIN);
        X = min(1024, (X + 10));
        steps = map(X, 10, 1024, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);  
      }
      
        delay(100 - abs(steps)); 
        StopStepper();  
        lcd.setCursor(0,1);
        sprintf(line1, "Waiting        ");
        lcd.print(line1); 
    }
    delay(10);
  }
}
