#include <FEH.h>
#include <Arduino.h>
#include <FEHUtility.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);


FEHMotor rightMotor(FEHMotor::Motor3, 9.0);
FEHMotor leftMotor(FEHMotor::Motor1, 9.0);
DigitalEncoder left_encoder(FEHIO::Pin10);
DigitalEncoder right_encoder(FEHIO::Pin8);
AnalogInputPin light_sensor(FEHIO::Pin1);

AnalogInputPin leftOpto(FEHIO::Pin14);
AnalogInputPin middleOpto(FEHIO::Pin13);
AnalogInputPin rightOpto(FEHIO::Pin12);

FEHServo servo(FEHServo::Servo0);
FEHServo composterServo(FEHServo::Servo1);

boolean isRed = false;
boolean isBlue = false;

int liftUp = 112;
int liftStop = 82;
int liftDown = 52;

float timeLift = 2530; // milliseconds, for top to bottom/or bottomtotop
float heightBottom = 3.0; //inches
float heightTop = 9.5; //inches
float distanceToLift = heightTop - heightBottom; //inches

void moveLiftDown(float distance) {
    
   
        servo.SetDegree(liftDown);
        Sleep((int)(timeLift * (distance/distanceToLift)));
        servo.SetDegree(liftStop);  
    
    
    

}


void moveLiftUp(float distance = distanceToLift){
    if (distance == distanceToLift ) {
        servo.SetDegree(liftUp);
        Sleep((int)(timeLift * (distance/distanceToLift)));
        servo.SetDegree(liftStop);  
    }
    else{
    servo.SetDegree(liftUp);
    Sleep((int)(timeLift * ((distance-3.0f)/distanceToLift)));
    servo.SetDegree(liftStop);  
    }
}

void waitForTouchStart(String message)
{
    int x;
    int y;

    
    LCD.WriteLine(message.c_str());

    while(!LCD.Touch(&x, &y))
    {
        Sleep(50);
    }

    while(LCD.Touch(&x, &y))
    {
        Sleep(50);
    }

    LCD.Clear();
}

void waitForStartLight()
{
    LCD.Clear();
    LCD.WriteLine("Click Start Button, then wait for light!");

    while(light_sensor.Value() > 0.5)
    {
        Sleep(50);
    }

    LCD.Clear();
}

void moveBackward(float distanceInches, int powerPercent = 40)
{   
    int direction = 1;
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    int distanceCounts = (distanceInches * 318)/7.85; // Convert inches to encoder counts
    rightMotor.SetPercent(direction * powerPercent);
    leftMotor.SetPercent(-1* direction * powerPercent);
    const unsigned long timeoutMs = 10000;
    const unsigned long startMs = millis();

    while(((left_encoder.Counts() + right_encoder.Counts()) / 2. < distanceCounts))
    {
        if(millis() - startMs > timeoutMs)
        {
            break;
        }
        Sleep(10);
    }
    
    leftMotor.Stop();
    rightMotor.Stop();

}
void moveForward(float distanceInches, int powerPercent = 40)
{
    int direction = -1;
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    int distanceCounts = (distanceInches * 318)/7.85; // Convert inches to encoder counts
    rightMotor.SetPercent(direction * powerPercent);
    leftMotor.SetPercent(-1* direction * powerPercent);
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < distanceCounts);
 
    leftMotor.Stop();
    rightMotor.Stop();

}
void turnRight90(int percent)
{
    //Reset encoder counts

    right_encoder.ResetCounts();

    left_encoder.ResetCounts();

    int counts = (6.55*318)/7.85;


    //Set both motors to desired percent

    //hint: set right motor backwards, left motor forwards
    rightMotor.SetPercent(percent);
    leftMotor.SetPercent(percent);
    //While the average of the left and right encoder is less than counts,
    //keep running motors
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts);  

    //Turn off motors
    rightMotor.Stop();
    leftMotor.Stop();

}
void turnLeft90(int percent)
{
    //Reset encoder counts

    right_encoder.ResetCounts();

    left_encoder.ResetCounts();

    int counts = (6.55*318)/7.85;


    //Set both motors to desired percent

    //hint: set right motor backwards, left motor forwards
    rightMotor.SetPercent(-1 * percent);
    leftMotor.SetPercent(-1 * percent);
    //While the average of the left and right encoder is less than counts,
    //keep running motors
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts);  

    //Turn off motors
    rightMotor.Stop();
    leftMotor.Stop();

}
void turnLeft45(int percent)
{
    //Reset encoder counts

    right_encoder.ResetCounts();

    left_encoder.ResetCounts();

    int counts = (0.5)*(6.55*318)/7.85;


    //Set both motors to desired percent

    //hint: set right motor backwards, left motor forwards
    rightMotor.SetPercent(-1 * percent);
    leftMotor.SetPercent(-1 * percent);
    //While the average of the left and right encoder is less than counts,
    //keep running motors
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts);  

    //Turn off motors
    rightMotor.Stop();
    leftMotor.Stop();

}





void turnRight45(int percent)
{
    //Reset encoder counts

    right_encoder.ResetCounts();

    left_encoder.ResetCounts();

    int counts = (0.5)*(6.55*318)/7.85;


    //Set both motors to desired percent

    //hint: set right motor backwards, left motor forwards
    rightMotor.SetPercent(percent);
    leftMotor.SetPercent(percent);
    //While the average of the left and right encoder is less than counts,
    //keep running motors
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts);  

    //Turn off motors
    rightMotor.Stop();
    leftMotor.Stop();

}
void turnRight(int angle, int percent = 40)
{
    //Reset encoder counts

    right_encoder.ResetCounts();

    left_encoder.ResetCounts();

    int counts = (angle/90.0)*(6.55*318)/7.85;


    //Set both motors to desired percent

    //hint: set right motor backwards, left motor forwards
    rightMotor.SetPercent(percent);
    leftMotor.SetPercent(percent);
    //While the average of the left and right encoder is less than counts,
    //keep running motors
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts);  

    //Turn off motors
    rightMotor.Stop();
    leftMotor.Stop();

}
void turnLeft(int angle, int percent = 40)
{
    //Reset encoder counts

    right_encoder.ResetCounts();

    left_encoder.ResetCounts();

    int counts = (angle/90.0)*(6.55*318)/7.85;


    //Set both motors to desired percent

    //hint: set right motor backwards, left motor forwards
    rightMotor.SetPercent(-1 * percent);
    leftMotor.SetPercent(-1 * percent);
    //While the average of the left and right encoder is less than counts,
    //keep running motors
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < counts);  

    //Turn off motors
    rightMotor.Stop();
    leftMotor.Stop();

}

void followLineBack(int drivePercent)
{

    int slowPercent = 0.15*drivePercent;
    


    const int frontLeft = drivePercent;   // left motor spins negative to drive forward
    const int frontRight = -drivePercent;   // right motor spins positive to drive forward
    const int slowLeft = slowPercent;
    const int slowRight = -slowPercent;

    auto inRange = [](float value, float min, float max)
    {
        return value > min && value < max;
    };

    while(true)
    {
        const float leftValue = leftOpto.Value();
        const float middleValue = middleOpto.Value();
        //const float rightValue = rightOpto.Value();
        //right opto is broken

        const bool leftOnWhite = inRange(leftValue, 2.0, 2.2);
        const bool middleOnBlack = inRange(middleValue, 4.35, 4.55);
        const bool leftOnGray = inRange(leftValue, 3.35, 3.65);
        const bool leftOnBlack = inRange(leftValue, 4.1, 4.3);

        

        if(light_sensor.Value() < 0.75)
        {
            leftMotor.Stop();
            rightMotor.Stop();
            break;
        }

        else if(leftOnWhite)
        {
            leftMotor.SetPercent(frontLeft);
            rightMotor.SetPercent(frontRight);
        }
        else if(leftOnGray)
        {
            leftMotor.SetPercent(frontLeft);
            rightMotor.SetPercent(slowRight);
        }
        else if (leftOnBlack)
        {
            leftMotor.SetPercent(slowLeft);
            rightMotor.SetPercent(frontRight);
        }
    }

    leftMotor.Stop();
    rightMotor.Stop();
}

void clickBlueButton(){
    moveForward(3); // back up from wall
    turnLeft45(20);
    moveBackward(4);
    turnRight45(20);
    moveBackward(8);
}
void clickRedButton(){
    moveForward(3); // back up from wall
    turnRight45(20);
    moveBackward(4);
    turnLeft45(20);
    moveBackward(8);
}

void checkLight(){
    while (true) {
       if(light_sensor.Value() < .6) {
           clickRedButton();
           isRed = true;
           isBlue = false;
           break;
       }
       else if(light_sensor.Value() < 1.1) {
           clickBlueButton();
           isBlue = true;
           isRed = false;
           break;
       }
   }
   if (isRed) {
       LCD.WriteLine("Red Light Detected!");
   }
   else if (isBlue) {
       LCD.WriteLine("Blue Light Detected!");
   }
}

void turnComposterForward()
{
    composterServo.SetDegree(126);
    Sleep(1500);
    composterServo.SetDegree(88);
}

void turnComposterBackward()
{
    composterServo.SetDegree(50);
    Sleep(1500);
    composterServo.SetDegree(88);
}

void ERCMain()
{
    RCS.InitializeTouchMenu("0300G2YKL");
    waitForStartLight();
    leftMotor.SetPercent(-30);
    rightMotor.SetPercent(30);
    Sleep(750);
    leftMotor.Stop();
    rightMotor.Stop();
    moveForward(3);
    turnRight(50, 40);
    moveBackward(5);
    turnRight(85, 40);
    moveBackward(7.5);
    turnComposterForward();
    Sleep(1500);
    turnComposterBackward();
    moveForward(13);



    /** 
    
    
    moveBackward(3);
    turnComposterForward();
    Sleep(5000);
    turnComposterBackward();
    moveBackward(8); //sldfjlkdfsj
    turnLeft(50, 40); 
    moveBackward(3);
    turnRight(50,40);
    moveBackward(3);
    moveForward(1);
    */

    




    
}
