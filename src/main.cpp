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

const int maxRCSRequestsPerCheck = 6; // Use the full RCS budget for heading fine-tuning.

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
    bool inTolerance = false;

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
            inTolerance = false;
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
                inTolerance = true;
                stopDriveMotors();
            }
            else
            {
                inTolerance = false;
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
        }

        Sleep(settleMs);
    }

    stopDriveMotors();
    return inTolerance;
}

bool pulseToRCSCheck(float targetHeading, int maxRequests = maxRCSRequestsPerCheck)
{
    if(maxRequests <= 0)
    {
        return true;
    }

    const float headingToleranceDegrees = 4.0f;
    const int minPulseMs = 12;
    const int maxPulseMs = 180;
    const int settleMs = 120;
    const int turnPowerPercent = 16;
    bool inTolerance = false;

    for(int request = 0; request < maxRequests; request++)
    {
        RCSPose pose;
        if(!getRCSPose(pose))
        {
            stopDriveMotors();
            return false;
        }

        const float headingError = signedHeadingError(targetHeading, pose.heading);
        if(fabs(headingError) <= headingToleranceDegrees)
        {
            inTolerance = true;
            stopDriveMotors();
        }
        else
        {
            inTolerance = false;
            float pulseScale;
            if(maxRequests >= 6)
            {
                if(request < 2)
                {
                    pulseScale = 1.0f;
                }
                else if(request < 4)
                {
                    pulseScale = 0.55f;
                }
                else
                {
                    pulseScale = 0.15f;
                }
            }
            else if(maxRequests <= 1)
            {
                pulseScale = 1.0f;
            }
            else
            {
                const float progress = (float)request / (float)(maxRequests - 1);
                pulseScale = 1.0f - (0.75f * progress);
            }

            int turnPulseMs = (int)(fabs(headingError) * 3.0f * pulseScale);
            if(turnPulseMs < minPulseMs)
            {
                turnPulseMs = minPulseMs;
            }
            if(turnPulseMs > maxPulseMs)
            {
                turnPulseMs = maxPulseMs;
            }

            pulseTurnForRCS(headingError, turnPowerPercent, turnPulseMs);
        }

        Sleep(settleMs);
    }

    stopDriveMotors();
    return inTolerance;
}

float normalizeHeadingDegrees(float heading)
{
    while(heading < 0.0f)
    {
        heading += 360.0f;
    }

    while(heading >= 360.0f)
    {
        heading -= 360.0f;
    }

    return heading;
}

void turnLeftTracked(float &expectedHeading, int angle, int percent = 40, int rcsChecks = maxRCSRequestsPerCheck)
{
    turnLeft(angle, percent);
    expectedHeading = normalizeHeadingDegrees(expectedHeading + angle);

    pulseToRCSCheck(expectedHeading, rcsChecks);
}

void turnRightTracked(float &expectedHeading, int angle, int percent = 40, int rcsChecks = maxRCSRequestsPerCheck)
{
    turnRight(angle, percent);
    expectedHeading = normalizeHeadingDegrees(expectedHeading - angle);

    pulseToRCSCheck(expectedHeading, rcsChecks);
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
    const unsigned long timeoutMs = 5000;
    const unsigned long startMs = millis();

    while(true)
    {
        if(millis() - startMs > timeoutMs)
        {
            break;
        }

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
            isRed = false;
            
            moveBackward(5);
            turnLeft(45,20);
            moveBackward(4);
            turnRight(45,20);
            moveForward(3);
            break;
        }

        Sleep(10);
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
    Sleep(5000);
    composterServo.SetDegree(88);
}

void turnComposterBackward()
{
    composterServo.SetDegree(50);
    Sleep(5000);
    composterServo.SetDegree(88);
}

void randomDriveSequence(int moveCount = 6)
{
    randomSeed(millis());

    for(int move = 0; move < moveCount; move++)
    {
        int angle = random(30, 91);
        int distance = random(3, 9);

        if(random(0, 2) == 0)
        {
            turnLeft(angle, 30);
        }
        else
        {
            turnRight(angle, 30);
        }

        Sleep(100);

        if(random(0, 2) == 0)
        {
            moveForward(distance, 30);
        }
        else
        {
            moveBackward(distance, 30);
        }

        Sleep(100);
    }
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

    float expectedHeading = 45.0f;
    

    moveForward(18);
    Sleep(100);

    turnLeftTracked(expectedHeading, 45, 40, 6);
    moveForward(5.5);

    moveLiftDown(distanceToLift);
    Sleep(300);
    moveLiftUp(4.6);
    moveForward(3);
    moveLiftUp(9);
    //finish apple bucket

    //move to cpomster
    moveBackward(7);
    turnRightTracked(expectedHeading, 45);
    moveBackward(11);
    turnRightTracked(expectedHeading, 45);
    moveBackward(4.3);
    turnRightTracked(expectedHeading, 90);
    moveBackward(4.07);
    //lined up with composter hopefully
    turnComposterBackward();
    Sleep(500);
    turnComposterForward();
    //turncomposter n shi



    moveForward(7);
    turnLeftTracked(expectedHeading, 45);
    moveForward(5);
    turnLeftTracked(expectedHeading, 45);

    moveForward(36, 60);
    Sleep(100);
    turnRightTracked(expectedHeading, 90, 40);
    Sleep(100);

    moveBackward(10);
    Sleep(100);
    turnLeftTracked(expectedHeading, 90, 40);
    Sleep(100);
    
    moveForward(11);
    
    Sleep(100);
    turnRightTracked(expectedHeading, 85, 40);
    Sleep(100);

    moveForward(7); //change back to 5
    moveLiftDown(1);
    Sleep(100);
    turnLeftTracked(expectedHeading, 90, 40);
    Sleep(100);

    moveBackward(11);
    Sleep(100);
    turnLeftTracked(expectedHeading, 45, 40);
    Sleep(100);

    moveForward(22);
    moveLiftDown(distanceToLift);
    moveBackward(5);
    moveForward(5);
    moveLiftUp(distanceToLift);
    Sleep(100);

    moveBackward(10);
    turnLeftTracked(expectedHeading, 45);
    moveForward(5);
    checkLight();
    Sleep(100);

    moveBackward(5);
    turnRightTracked(expectedHeading, 180);
    moveForward(7);
    turnRightTracked(expectedHeading, 90);
    moveForward(25);
    Sleep(400);

    randomDriveSequence();

    
}
