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

void moveBackward(float distanceInches, int powerPercent = 40)
{
    int direction = 1;
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    int distanceCounts = (distanceInches * 318)/7.85; // Convert inches to encoder counts
    rightMotor.SetPercent(direction * powerPercent);
    leftMotor.SetPercent(-1* direction * powerPercent);
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < distanceCounts);
 
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

        

        if((leftOnBlack && rightOnBlack) || (middleOnBlack && rightOnWhite) || (middleOnBlack && leftOnWhite))
        {
            leftMotor.Stop();
            rightMotor.Stop();
            break;
        }

        else if(leftOnWhite)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(forwardRight);
        }
        else if(leftOnGray)
        {
            leftMotor.SetPercent(forwardLeft);
            rightMotor.SetPercent(slowRight);
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
   if (isRed) {
       LCD.WriteLine("Red Light Detected!");
   }
   else if (isBlue) {
       LCD.WriteLine("Blue Light Detected!");
   }
}

void oldERCMain(){
    //waitForTouchStart("Tap screen to start Milestone 2!"); // tap screen
    waitForStartLight(); //checks if cds cell (red filter) detects red light
    moveBackward(3, 50); // clicks button
    Sleep(250);
    

    moveForward(2.5);
    turnRight45(20);
    moveForward(1);
    turnRight45(20);
    moveForward(4);
    turnLeft(50, 20);
    moveForward(36, 50);

    turnLeft(30, 20);
    moveForward(1);
    turnLeft(30, 20);
    moveForward(1);
    turnLeft(30, 20);
    moveBackward(20);

    Sleep(1000);
    followLine(20);
    moveForward(4);
    checkLight();
}
void ERCMain()
{
    
    
    waitForTouchStart("Tap screen to start Milestone 2!"); // tap screen
    waitForStartLight(); //checks if cds cell (red filter) detects red light
    moveBackward(3, 50); // clicks button
    Sleep(250);
    
    waitForTouchStart("Tap screen to continue Milestone 2!"); // tap screen
    moveForward(25, 50);

    waitForTouchStart("Tap screen to continue Milestone 2!"); // tap screen
    checkLight();
    

    
    
    

    
    



   
} 