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

void moveBackward(float distanceInches)
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
void moveForward(float distanceInches)
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
void followLine(int drivePercent)
{

    int slowPercent = 0.25*drivePercent;
    


    const int forwardLeft = drivePercent;   // left motor spins negative to drive forward
    const int forwardRight = -drivePercent;   // right motor spins positive to drive forward
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
        const float rightValue = rightOpto.Value();

        const bool leftOnWhite = inRange(leftValue, 2.0, 2.4);
        const bool rightOnWhite = inRange(rightValue, 2.4, 2.9);
        const bool middleOnBlack = inRange(middleValue, 4.1, 4.4);
        const bool rightOnBlack = inRange(rightValue, 4.1, 4.5);
        const bool leftOnGray = inRange(leftValue, 2.8, 3.25);
        const bool rightOnGray = inRange(rightValue, 3.2, 3.65);
        const bool leftOnBlack = inRange(leftValue, 4.1, 4.3);

        

        if(leftOnBlack && rightOnBlack)
        {
            leftMotor.Stop();
            rightMotor.Stop();
            break;
        }

        else if((leftOnWhite && rightOnWhite) || (middleOnBlack))
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(leftOnWhite && rightOnBlack)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(leftOnGray)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(slowRight);
        }
        else if(rightOnGray || leftOnBlack)
        {
            leftMotor.SetPercent(slowLeft);
            rightMotor.SetPercent(forwardRight);
        }
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
   
    waitForTouchStart("Tap screen to start Milestone 2!"); // tap screen
    waitForStartLight(); //checks if cds cell (red filter) detects red light
    moveBackward(2.5); // clicks button
    Sleep(250);
    moveForward(2.5);
    turnRight45(20);
    moveForward(1);
    turnRight45(20);
    moveForward(1.5);
    turnLeft45(20);
    moveForward(24);
    moveForward(3);
    turnLeft90(20);
    followLine(20);
    moveForward(4);
    checkLight();

    



   
} 