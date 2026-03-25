#include <FEHLCD.h>
#include <FEHIO.h>
#include <FEHUtility.h>
#include <FEHRCS.h>
#include <FEHSD.h>
#include <FEH.h>

FEHMotor rightMotor(FEHMotor::Motor0, 9.0);
FEHMotor leftMotor(FEHMotor::Motor2, 9.0);
DigitalEncoder left_encoder(FEHIO::Pin10);
DigitalEncoder right_encoder(FEHIO::Pin8);

void ERCMain()
{
    // Declare variables.
    // For touchscreen functionality.
    int touch_x, touch_y;
    
    RCSPose *pose;
    RCS.DisableRateLimit();
    RCS.InitializeTouchMenu("Z1TESTING");

    while (!LCD.Touch(&touch_x, &touch_y))
        {
            // Request position data from RCS
            pose = RCS.RequestPosition();

            // Clear previous position data from LCD
            LCD.SetFontColor(BLACK);
            LCD.FillRectangle(130, 20, 100, 60);

            LCD.SetFontColor(WHITE);
            LCD.WriteRC("X Position:", 2, 0);
            LCD.WriteRC("Y Position:", 3, 0);
            LCD.WriteRC("Heading:", 4, 0);

            // Write the RCS data to the LCD screen
            LCD.WriteRC(pose->x, 2, 12);
            LCD.WriteRC(pose->y, 3, 12);
            LCD.WriteRC(pose->heading, 4, 12);
        }
        while (!LCD.Touch(&touch_x, &touch_y));
        
    LCD.Clear();
    LCD.WriteLine("Drive to B!");
    while (!LCD.Touch(&touch_x, &touch_y));
    while (LCD.Touch(&touch_x, &touch_y));
    while( RCS.RequestPosition()->y < 55){
        // Drive forward until Y position is at least 55
        rightMotor.SetPercent(30);
        leftMotor.SetPercent(30);
        Sleep(0.3);
    }
    rightMotor.Stop();
    leftMotor.Stop();

    left_encoder.ResetCounts();
    right_encoder.ResetCounts();
    while( (left_encoder.Counts()+right_encoder.Counts())/2 < (70*2.48) ){
        // Drive forward until encoders reach 1000 counts
        rightMotor.SetPercent(30);
        leftMotor.SetPercent(-30);
        Sleep(0.3);
    }
    while( RCS.RequestPosition()->x > 18 ){
        // Drive forward until X position is at least 10
        rightMotor.SetPercent(30);
        leftMotor.SetPercent(30);
        Sleep(0.3);
    }
    rightMotor.Stop();
    leftMotor.Stop();

    
    left_encoder.ResetCounts();
    right_encoder.ResetCounts();
    while( (left_encoder.Counts()+right_encoder.Counts())/2 < (70*2.48) ){
        // Drive forward until encoders reach 1000 counts
        rightMotor.SetPercent(30);
        leftMotor.SetPercent(-30);
        Sleep(0.3);
    }

    while( RCS.RequestPosition()->y > 45 ){
        // Drive forward until Y position is at least 55
        rightMotor.SetPercent(30);
        leftMotor.SetPercent(30);
        Sleep(0.3);
    }
    LCD.WriteLine("Done with while!");
    rightMotor.Stop();
    leftMotor.Stop();

    
}