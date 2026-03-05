#include <FEH.h>
#include <Arduino.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);


FEHMotor rightMotor(FEHMotor::Motor3, 9.0);
FEHMotor leftMotor(FEHMotor::Motor1, 9.0);
DigitalEncoder left_encoder(FEHIO::Pin10);
DigitalEncoder right_encoder(FEHIO::Pin8);

void moveBot(int distanceInches, int direction)
{
    right_encoder.ResetCounts();
    left_encoder.ResetCounts();

    int distanceCounts = (distanceInches * 318)/7.85; // Convert inches to encoder counts
    rightMotor.SetPercent(direction * 40);
    leftMotor.SetPercent(-1* direction * 40);
    while((left_encoder.Counts() + right_encoder.Counts()) / 2. < distanceCounts);
 
    leftMotor.Stop();
    rightMotor.Stop();

}
void turnRight(int percent, int counts)
{
    //Reset encoder counts

    right_encoder.ResetCounts();

    left_encoder.ResetCounts();


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
void ERCMain()
{
    
    moveBot(30, -1);
    Sleep(2500);
    moveBot(27.75, 1);
    Sleep(2500);
    turnRight(20, (6.55*318)/7.85); //find turn count radius for this
    Sleep(2500);
    moveBot(37, -1);
    Sleep(2500);
    moveBot(37, 1);   
    Sleep(2500); 
    

    
    

}