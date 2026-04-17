#include <FEH.h>
#include <Arduino.h>
#include <FEHUtility.h>
#include <math.h>


// Declare things like Motors, Servos, etc. here
// For example:
// FEHMotor leftMotor(FEHMotor::Motor0, 6.0);
// FEHServo servo(FEHServo::Servo0);


FEHMotor rightMotor(FEHMotor::Motor3, 9.0);
FEHMotor leftMotor(FEHMotor::Motor1, 9.0);
DigitalEncoder left_encoder(FEHIO::Pin10);
DigitalEncoder right_encoder(FEHIO::Pin8);
AnalogInputPin light_sensor(FEHIO::Pin1);

FEHServo servo(FEHServo::Servo0);
FEHServo composterServo(FEHServo::Servo1);

boolean isRed = false;
boolean isBlue = false;

const int maxRCSRequestsPerCheck = 6; // 6 checks * 6 requests = 36 max RCS calls, leaving 14 calls of buffer.

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

float signedHeadingError(float targetHeading, float currentHeading)
{
    float error = targetHeading - currentHeading;

    while(error > 180.0f)
    {
        error -= 360.0f;
    }

    while(error < -180.0f)
    {
        error += 360.0f;
    }

    return error;
}

float headingTowardCoordinate(float dx, float dy)
{
    float heading = degrees(atan2(dx, dy));

    if(heading < 0.0f)
    {
        heading += 360.0f;
    }

    return heading;
}

bool getRCSPose(RCSPose &pose)
{
    RCSPose *latestPose = RCS.RequestPosition(true);
    if(latestPose == nullptr)
    {
        return false;
    }

    pose = *latestPose;
    return pose.x >= 0.0f && pose.y >= 0.0f && pose.heading >= 0.0f;
}

void stopDriveMotors()
{
    leftMotor.Stop();
    rightMotor.Stop();
}

void pulseForwardForRCS(int powerPercent, int pulseMs)
{
    leftMotor.SetPercent(powerPercent);
    rightMotor.SetPercent(-powerPercent);
    Sleep(pulseMs);
    stopDriveMotors();
}

void pulseTurnForRCS(float headingError, int powerPercent, int pulseMs)
{
    if(headingError > 0)
    {
        rightMotor.SetPercent(-powerPercent);
        leftMotor.SetPercent(-powerPercent);
    }
    else
    {
        rightMotor.SetPercent(powerPercent);
        leftMotor.SetPercent(powerPercent);
    }

    Sleep(pulseMs);
    stopDriveMotors();
}

bool pulseToRCSPose(
    float targetX,
    float targetY,
    float targetHeading,
    int maxRCSRequests = 3,
    float toleranceInches = 0.5f,
    float headingToleranceDegrees = 4.0f,
    int drivePowerPercent = 18,
    int turnPowerPercent = 16)
{
    const int minPulseMs = 70;
    const int maxDrivePulseMs = 220;
    const int maxTurnPulseMs = 180;
    const int settleMs = 150;

    for(int request = 0; request < maxRCSRequests; request++)
    {
        RCSPose pose;
        if(!getRCSPose(pose))
        {
            stopDriveMotors();
            return false;
        }

        const float dx = targetX - pose.x;
        const float dy = targetY - pose.y;
        const float distance = sqrt((dx * dx) + (dy * dy));

        if(distance > toleranceInches)
        {
            const float targetDriveHeading = headingTowardCoordinate(dx, dy);
            const float driveHeadingError = signedHeadingError(targetDriveHeading, pose.heading);

            if(fabs(driveHeadingError) > headingToleranceDegrees)
            {
                int turnPulseMs = (int)(fabs(driveHeadingError) * 3.0f);
                if(turnPulseMs < minPulseMs)
                {
                    turnPulseMs = minPulseMs;
                }
                if(turnPulseMs > maxTurnPulseMs)
                {
                    turnPulseMs = maxTurnPulseMs;
                }

                pulseTurnForRCS(driveHeadingError, turnPowerPercent, turnPulseMs);
            }
            else
            {
                int drivePulseMs = (int)(distance * 80.0f);
                if(drivePulseMs < minPulseMs)
                {
                    drivePulseMs = minPulseMs;
                }
                if(drivePulseMs > maxDrivePulseMs)
                {
                    drivePulseMs = maxDrivePulseMs;
                }

                pulseForwardForRCS(drivePowerPercent, drivePulseMs);
            }
        }
        else
        {
            const float finalHeadingError = signedHeadingError(targetHeading, pose.heading);
            if(fabs(finalHeadingError) <= headingToleranceDegrees)
            {
                stopDriveMotors();
                return true;
            }

            int turnPulseMs = (int)(fabs(finalHeadingError) * 3.0f);
            if(turnPulseMs < minPulseMs)
            {
                turnPulseMs = minPulseMs;
            }
            if(turnPulseMs > maxTurnPulseMs)
            {
                turnPulseMs = maxTurnPulseMs;
            }

            pulseTurnForRCS(finalHeadingError, turnPowerPercent, turnPulseMs);
        }

        Sleep(settleMs);
    }

    stopDriveMotors();
    return false;
}

bool pulseToRCSCheck(float targetX, float targetY, float targetHeading)
{
    return pulseToRCSPose(
        targetX,
        targetY,
        targetHeading,
        maxRCSRequestsPerCheck);
}

void clickBlueButton()
{
    moveBackward(3); // Back up from wall.
    turnRight(45, 20);
    moveForward(4);
    turnLeft(45, 20);
    moveForward(5);
}

void clickRedButton()
{
    moveBackward(3); // Back up from wall.
    turnLeft(45, 20);
    moveForward(4);
    turnRight(45, 20);
    moveForward(5);
}

void checkLight()
{
    while(true)
    {
        if(light_sensor.Value() < .6)
        {
            clickRedButton();
            isRed = true;
            isBlue = false;

            moveBackward(5);
            turnRight(45,20);
            moveBackward(4);
            turnLeft(45,20);
            moveForward(3);


            break;

        }
        else if(light_sensor.Value() < 1.1)
        {
            clickBlueButton();
            isBlue = true;
            isRed = false;
            
            moveBackward(5);
            turnLeft(45,20);
            moveBackward(4);
            turnRight(45,20);
            moveForward(3);
            break;
        }
    }
    
}

void checkLevers(){
    int correctLever = 1; //default to middle lever because we want to.
    
    if (correctLever == 0) // Left lever
    {
        turnLeft(45, 20);
        moveForward(6);
        moveLiftDown(7);
        moveBackward(2);
        moveLiftDown(3);
        moveForward(2);
        moveLiftUp();
        moveBackward(6);
        turnRight(45,20);

    }
    else if (correctLever == 1) // Middle lever
    {
        moveForward(4);
        moveLiftDown(7);
        moveBackward(2);
        moveLiftDown(3);
        moveForward(2);
        moveLiftUp();
        moveBackward(4);
    }
    else if (correctLever == 2) // Right lever
    {
        turnRight(45, 20);
        moveForward(6);
        moveLiftDown(7);
        moveBackward(2);
        moveLiftDown(3);
        moveForward(2);
        moveLiftUp();
        moveBackward(6);
        turnLeft(45,20);
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

    RCS.InitializeTouchMenu("30300G2YKL");
    waitForTouchStart("Click the screen when you're ready for your OFFICIAL run.!");

    waitForStartLight();

    //backing up to hit button
    leftMotor.SetPercent(-30);
    rightMotor.SetPercent(30);
    Sleep(750);
    leftMotor.Stop();
    rightMotor.Stop();
    Sleep(100);
    
    moveForward(3);
    
    turnRight(90);
    moveBackward(6);
    turnRight(45);
    moveBackward(3);

    // No RCS check here: composter is in the RCS deadzone.
    turnComposterForward();
    turnComposterBackward();
    moveForward(4);
    turnRight(90);
    moveBackward(11);
    turnRight(135);
    moveForward(4.5);
    turnLeft(50);
    moveForward(6);

    // RCS check: apple bucket.
    //pulseToRCSCheck(10.33f, 20.15f, 90.0f);
    moveLiftUp(4.7); // Example apple bucket height value. Replace with your measured value.
    moveForward(6);
    moveLiftUp(); // Lift up max.
    moveBackward(8);
    turnRight(45);
    moveBackward(11);
    turnRight(90);
    moveForward(8); // Line up with ramp.
    turnLeft(55);
    moveForward(14, 60);

    // RCS check: ramp.
    //pulseToRCSCheck(30.45f, 44.68f, 180.0f);
    turnLeft(90);
    moveForward(18); // Open window.
    moveBackward(9);
    turnRight(90);
    moveBackward(17.5);
    turnLeft(90);

    // RCS check: tall table.
    pulseToRCSCheck(25.80f, 63.54f, 270.0f);
    moveForward(2);
    moveLiftDown(4.0); // Example table/bucket height value. Replace with the measured value.
    moveBackward(3);
    turnLeft(45);
    moveBackward(10);
    turnLeft(90);

    // RCS check: middle of levers.
    pulseToRCSCheck(18.10f, 59.16f, 45.0f);
    checkLevers();
    turnLeft(135);
    moveForward(9);

    // RCS check: parallel with line of humidifier.
    pulseToRCSCheck(20.19f, 51.35f, 180.0f);
    turnRight(90);
    moveForward(6);

    // RCS check: line up with CdS cell.
    pulseToRCSCheck(13.00f, 50.82f, 90.0f);
    checkLight();// Do humidifier.
    turnRight(180);
    moveBackward(17);
    turnLeft(90);
    moveBackward(36);
}
