#include <ZumoShield.h>
#include <Wire.h>

const long XY_ACCELERATION_THRESHOLD = 2400;
const float TURN_SPEED = 150;
const float SPEED = 200;
const float ACCELERATED_SPEED = 300;

class CZumoRobot
{
private:
    ZumoIMU m_Imu;
    ZumoMotors m_Motors;
    Pushbutton m_Button;
    ZumoReflectanceSensorArray m_ReflectanceSensors;
    ZumoBuzzer m_Buzzer;
    unsigned int m_rgucReflectanceSensorReadings[6];

    void ReadMeasurements()
    {
        m_Imu.read();

        // Format Gyro Readings
        m_Imu.g.x *= 0.00875;
        m_Imu.g.y *= 0.00875;
        m_Imu.g.z *= 0.00875;
    }

    template <typename T>
    void Turn(T tTargetAngle, bool bIsLeft)
    {
        T tCurrentAngle = 0;
        T tTime;
        T tOldTime;

        if (bIsLeft)
            m_Motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
        else
            m_Motors.setSpeeds(-TURN_SPEED, TURN_SPEED);

        while (tCurrentAngle < tTargetAngle)
        {
            ReadMeasurements();
            tTime = millis();
            tCurrentAngle += abs(m_Imu.g.z * ((tTime - tOldTime) / 1000));
            tOldTime = tTime;
        }
        m_Motors.setSpeeds(0, 0);
        delay(100);
    }

    void Calibrate()
    {
        // On White
        PlaceOnSensors();

        // On Black
        PlaySound();
        PlaceOnSensors();

        // Calibrated
        PlaySound();
    }

    void PlaceOnSensors()
    {
        float fTime = millis();
        float fThreshold = fTime + 5000;
        while (fTime < fThreshold)
        {
            m_ReflectanceSensors.calibrate();
            fTime = millis();
        }
    }

public:
    CZumoRobot() : m_Button(ZUMO_BUTTON)
    {
        m_Imu.init();
        m_Imu.enableDefault();
        m_ReflectanceSensors.init();

        m_Button.waitForPress();
        Calibrate();
    }

    void PlaySound(char *cNotes = "!fa")
    {
        m_Buzzer.play(cNotes);
    }

    void MoveForwards(int nSpeed, int nDuration) // nDuration is in seconds
    {
        m_Motors.setSpeeds(nSpeed, nSpeed);
        delay(nDuration * 1000);
        m_Motors.setSpeeds(0, 0);
    }

    template <typename T>
    void TurnRight(T tDegrees)
    {
        Turn(tDegrees, false);
    }

    template <typename T>
    void TurnLeft(T tDegrees)
    {
        Turn(tDegrees, true);
    }

    void WaitForPress()
    {
        m_Button.waitForButton();
    }

    bool IsPressed()
    {
        return m_Button.getSingleDebouncedRelease();
    }

    template <typename T>
    void SetSpeed(T tLeftSpeed, T tRightSpeed)
    {
        m_Motors.setSpeeds(tLeftSpeed, tRightSpeed);
    }

    void HitBorder()
    {
        // Checks how reflective the surface is (1000 black)
        m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
        if (m_rgucReflectanceSensorReadings[0] > 500 || m_rgucReflectanceSensorReadings[5] > 500)
        {
            SetSpeed(0, 0);
            delay(100);

            float test = random(180);
            if (m_rgucReflectanceSensorReadings[0] > 500)
                TurnLeft(test);
            else if (m_rgucReflectanceSensorReadings[5] > 500)
                TurnRight(test);

            delay(100);
            SetSpeed(SPEED, SPEED);
        }
    }

    bool IsCollided()
    {
        m_Imu.read();
        long xAcceleration = (long)m_Imu.a.x * (long)m_Imu.a.x;
        long yAcceleration = (long)m_Imu.a.y * (long)m_Imu.a.y;
        long netAcceleration = (long)xAcceleration + (long)yAcceleration;
        return (netAcceleration >= 100000000);
    }
};

CZumoRobot *pZumoRobot;

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    pZumoRobot = new CZumoRobot;

    pZumoRobot->WaitForPress();
    Serial.println("STARTING");
    pZumoRobot->SetSpeed(SPEED, SPEED);
}

void loop()
{
    pZumoRobot->HitBorder();
    if (pZumoRobot->IsCollided())
    {
        Serial.println("COLLISION!");
        int nOption = random(2);
        if (nOption == 1)
            pZumoRobot->SetSpeed(ACCELERATED_SPEED, ACCELERATED_SPEED);
        else
        {
            nOption = random(2);
            if (nOption == 1)
                pZumoRobot->TurnLeft(random(180));
            else
                pZumoRobot->TurnRight(random(180));
        }
    }
}
