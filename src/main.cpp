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

AnalogInputPin leftOpto(FEHIO::Pin3);
AnalogInputPin middleOpto(FEHIO::Pin4);
AnalogInputPin rightOpto(FEHIO::Pin5);

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

    while(light_sensor.Value() > 3.5)
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
    int slowPercent = drivePercent / 2;
    if(slowPercent < 10)
    {
        slowPercent = 10;
    }

    const int forwardLeft = -drivePercent;   // left motor spins negative to drive forward
    const int forwardRight = drivePercent;   // right motor spins positive to drive forward
    const int slowLeft = -slowPercent;
    const int slowRight = slowPercent;

    while(true)
    {
        const float leftValue = leftOpto.Value();
        const float middleValue = middleOpto.Value();
        const float rightValue = rightOpto.Value();

        LCD.Clear();
        LCD.Write("Left: ");
        LCD.Write(leftValue);
        LCD.Write(" Middle: ");
        LCD.Write(middleValue);
        LCD.Write(" Right: ");
        LCD.WriteLine(rightValue);

        if(leftValue > threshold && middleValue > threshold && rightValue > threshold)
        {
            break;
        }

        if(leftValue > threshold)
        {
            leftMotor.SetPercent(slowLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(middleValue > threshold)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(rightValue > threshold)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(slowRight);
        }
        else
        {
            leftMotor.Stop();
            rightMotor.Stop();
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
           break;
       }
       else if(light_sensor.Value() < .75) {
           clickBlueButton();
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
    
    //finish rest after this stuff works...

    */
   
   
   while(true){
    LCD.Clear();
    LCD.Write("Left Opto: ");
    LCD.WriteLine(leftOpto.Value());
    LCD.Write("Middle Opto: ");
    LCD.WriteLine(middleOpto.Value());
    LCD.Write("Right Opto: ");
    LCD.WriteLine(rightOpto.Value());
   }



   
} 