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

    void Turn(float fTargetAngle, bool bIsLeft)
    {
        float fCurrentAngle = 0;
        float fTime;
        float fOldTime;

        if (bIsLeft)
            m_Motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
        else
            m_Motors.setSpeeds(-TURN_SPEED, TURN_SPEED);

        while (fCurrentAngle < fTargetAngle)
        {
            ReadMeasurements();
            fTime = millis();
            fCurrentAngle += abs(m_Imu.g.z * ((fTime - fOldTime) / 1000));
            fOldTime = fTime;
        }
        m_Motors.setSpeeds(0, 0);
        delay(100);
    }

    void Calibrate()
    {
        // On White
        Serial.println("STARTING CALIBRATION");
        PlaceOnSensors();

        Serial.println("ON BLACK");
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

    void PlaySound()
    {
        m_Buzzer.play("!fa");
    }

    // nDuration is in seconds
    void MoveForwards(int nSpeed, int nDuration)
    {
        m_Motors.setSpeeds(nSpeed, nSpeed);
        delay(nDuration * 1000);
        m_Motors.setSpeeds(0, 0);
    }

    void TurnLeft(float fDegrees)
    {
        Turn(fDegrees, true);
    }

    void TurnRight(float fDegrees)
    {
        Turn(fDegrees, false);
    }

    void WaitForPress()
    {
        m_Button.waitForButton();
    }

    bool IsPressed()
    {
        return m_Button.getSingleDebouncedRelease();
    }

    void SetSpeed(int nLeftSpeed, int nRightSpeed)
    {
        m_Motors.setSpeeds(nLeftSpeed, nRightSpeed);
    }

    void HitBorder()
    {
        // Checks how reflective the surface is (1000 black)
        m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
        if (m_rgucReflectanceSensorReadings[0] > 500 || m_rgucReflectanceSensorReadings[5] > 500)
        {
            SetSpeed(0, 0);
            delay(100);
            // SlowDown();

            if (m_rgucReflectanceSensorReadings[0] > 500)
                TurnLeft(random(180));
            else if (m_rgucReflectanceSensorReadings[5] > 500)
                TurnRight(random(180));

            delay(100);
            SetSpeed(SPEED, SPEED);
        }
    }

    bool IsCollided()
    {
        // Check if net acceleration is over a certain threshold
        m_Imu.read();
        long xAcceleration = (long)m_Imu.a.x * (long)m_Imu.a.x;
        long yAcceleration = (long)m_Imu.a.y * (long)m_Imu.a.y;
        long netAcceleration = (long)xAcceleration + (long)yAcceleration;
        long test = (long)XY_ACCELERATION_THRESHOLD * (long)XY_ACCELERATION_THRESHOLD;
        Serial.println(netAcceleration);
        return (netAcceleration >= test);
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
        // pZumoRobot->SetSpeed(0, 0);
        // Serial.println("COLLISION!");
    }
}

// 78771250
// 5760000