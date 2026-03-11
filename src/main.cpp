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

boolean isRed = false;
boolean isBlue = false;

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

void moveForward(float distanceInches)
{
    int direction = 1;
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    int distanceCounts = (distanceInches * 318)/7.85; // Convert inches to encoder counts
    rightMotor.SetPercent(direction * 40);
    leftMotor.SetPercent(-1* direction * 40);
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < distanceCounts);
 
    leftMotor.Stop();
    rightMotor.Stop();

}
void moveBackward(float distanceInches)
{
    int direction = -1;
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    int distanceCounts = (distanceInches * 318)/7.85; // Convert inches to encoder counts
    rightMotor.SetPercent(direction * 40);
    leftMotor.SetPercent(-1* direction * 40);
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
void followLine(int drivePercent, float threshold)
{
    (void)threshold; // threshold superseded by calibrated ranges below

    int slowPercent = drivePercent / 2;
    if(slowPercent < 10)
    {
        slowPercent = 10;
    }

    const int forwardLeft = -drivePercent;   // left motor spins negative to drive forward
    const int forwardRight = drivePercent;   // right motor spins positive to drive forward
    const int slowLeft = -slowPercent;
    const int slowRight = slowPercent;

    const float WHITE_MIN = 2.0f;
    const float WHITE_MAX = 3.0f;
    const float BLACK_MIN = 3.8f;
    const float BLACK_MAX = 4.5f;

    auto inRange = [](float value, float min, float max)
    {
        return value > min && value < max;
    };

    while(true)
    {
        const float leftValue = leftOpto.Value();
        const float middleValue = middleOpto.Value();
        const float rightValue = rightOpto.Value();

        const bool leftOnWhite = inRange(leftValue, WHITE_MIN, WHITE_MAX);
        const bool rightOnWhite = inRange(rightValue, WHITE_MIN, WHITE_MAX);
        const bool middleOnBlack = inRange(middleValue, BLACK_MIN, BLACK_MAX);
        const bool rightOnBlack = inRange(rightValue, BLACK_MIN, BLACK_MAX);

        LCD.Clear();
        LCD.Write("Left: ");
        LCD.Write(leftValue);
        LCD.Write(" Middle: ");
        LCD.Write(middleValue);
        LCD.Write(" Right: ");
        LCD.WriteLine(rightValue);

        if(!leftOnWhite && !rightOnWhite)
        {
            leftMotor.Stop();
            rightMotor.Stop();
            break;
        }

        if(leftOnWhite && middleOnBlack)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(leftOnWhite && rightOnBlack)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(!leftOnWhite)
        {
            leftMotor.SetPercent(slowLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(!middleOnBlack)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(slowRight);
        }
        else
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(forwardRight);
        }

        Sleep(25);
    }

    leftMotor.Stop();
    rightMotor.Stop();
}

void clickBlueButton(){
    moveBackward(3); // back up from wall
    turnLeft45(20);
    moveForward(4);
    turnRight45(20);
    moveForward(8);
}
void clickRedButton(){
    moveBackward(3); // back up from wall
    turnRight45(20);
    moveForward(4);
    turnLeft45(20);
    moveForward(8);
}

void checkLight(){
    while (true) {
       if(light_sensor.Value() < .4) {
           clickRedButton();
           isRed = true;
           isBlue = false;
           break;
       }
       else if(light_sensor.Value() < .75) {
           clickBlueButton();
           isBlue = true;
           isRed = false;
           break;
       }
   }
}
void ERCMain()
{
    /** 
     * Pending change final instructions
     * 
    waitForTouchStart("Tap screen to start Milestone 2!"); // tap screen
    waitForStartLight(); //checks if cds cell (red filter) detects red light
    moveForward(2.5); // clicks button
    Sleep(1000);
    turnLeft90(20); // turn left to exterior wall
    Sleep(1000);
    turnLeft45(20); // turn left to ramp
    Sleep(1000);
    moveForward(24); // drive FULLY up ramp, no parts hanging behind
    Sleep(1000);
    lineFollow() // til ending when all black
    turnLeft90(20); // turn left to face humidifier
    Sleep(1000);
    moveForward(4); //lineup to humidifier light
    checkLight(); // check cds cell to see if red or blue light, click corresponding button

    If Blue:{
        find way back to ramp from blue position}
    else if Red{
        find way back to ramp from red position}

    Move back down ramp, no parts hanging behind
    
    //finish rest after this stuff works...

    */
   
   
   followLine(50, 0.5);



   
} 