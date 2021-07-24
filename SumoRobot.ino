#include <ZumoShield.h>
#include <Wire.h>

constexpr long XY_ACCELERATION_THRESHOLD = 100000000;
constexpr float TURN_SPEED = 150;
constexpr float SEARCH_SPEED = 200;
constexpr float ACCELERATED_SPEED = 300;
constexpr float WAIT_DURATION = 100;
class CZumoRobot
{
public:
    enum EState
    {
        Search,
        Fight,
        Flight,
        RightTurn,
        LeftTurn,
        Stop,
    };

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

    // template <typename T>
    void TurnRight(float tDegrees)
    {
        Turn(tDegrees, false);
    }

    // template <typename T>
    // void TurnLeft(float tDegrees)
    // {
    //     Turn(tDegrees, true);
    // }

    void TurnLeft(float tDegrees)
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

    void SetCurrentState(EState eNextState)
    {
        eCurrentState = eNextState;
    }

    EState GetCurrentState()
    {
        return eCurrentState;
    }

    void HitBorder()
    {
        m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
        if (m_rgucReflectanceSensorReadings[5] > 500)
            eCurrentState = RightTurn;
        else if (m_rgucReflectanceSensorReadings[0] > 500)
            eCurrentState = LeftTurn;
    }

    void IsCollided()
    {
        m_Imu.read();
        long xAcceleration = (long)m_Imu.a.x * (long)m_Imu.a.x;
        long yAcceleration = (long)m_Imu.a.y * (long)m_Imu.a.y;
        long netAcceleration = (long)xAcceleration + (long)yAcceleration;
        if (netAcceleration >= XY_ACCELERATION_THRESHOLD)
        {
            eCurrentState = Fight;
            // int nOption = random(2);
            // if (nOption == 1)
            // {
            //     eCurrentState = Fight;
            // }
            // else
            // {
            //     eCurrentState = Flight;
            // }
        }
    }

    float GetWaitDuration()
    {
        return fWaitDuration;
    }

    void SetWaitDuration(float fFutureWaitDuration)
    {
        fWaitDuration = fFutureWaitDuration;
    }

private:
    ZumoIMU m_Imu;
    ZumoMotors m_Motors;
    Pushbutton m_Button;
    ZumoReflectanceSensorArray m_ReflectanceSensors;
    ZumoBuzzer m_Buzzer;
    unsigned int m_rgucReflectanceSensorReadings[6];
    EState eCurrentState;
    float fWaitDuration = 0;

    void ReadMeasurements()
    {
        m_Imu.read();

        // Format Gyro Readings
        m_Imu.g.x *= 0.00875;
        m_Imu.g.y *= 0.00875;
        m_Imu.g.z *= 0.00875;
    }

    float CalculateAngularVelocity()
    {
        m_Imu.read();
        return abs(m_Imu.g.z * 0.00875);
    }

    // template <typename T>
    void Turn(float tTargetAngle, bool bIsLeft)
    {
        float tCurrentAngle = 0;
        float tTime = 0;
        float tOldTime = 0;
        float tAngularVelocity = 0;

        if (bIsLeft)
            m_Motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
        else
            m_Motors.setSpeeds(-TURN_SPEED, TURN_SPEED);

        float tTest = 0;
        int i = 0;
        while (true)
        {
            tTime = millis();
            m_Imu.read();
            tAngularVelocity = abs(m_Imu.g.z * 0.00875);
            tCurrentAngle += tAngularVelocity * ((tTime - tOldTime)) / 1000;
            tOldTime = tTime;

            if (i == 0)
                tTest = tCurrentAngle + tTargetAngle;

            if (tCurrentAngle >= tTest)
                break;

            i++;
        }

        // Once turned go back to search
        eCurrentState = Search;
        SetSpeed(0, 0);
    }

    void Calibrate()
    {
        Serial.println("PLACE ON WHITE");
        PlaceOnSensors();

        Serial.println("PLACE ON BLACK");
        PlaySound();
        PlaceOnSensors();

        Serial.println("CALIBRATED");
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
};

CZumoRobot *pZumoRobot;

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    pZumoRobot = new CZumoRobot;

    pZumoRobot->WaitForPress();
    Serial.println("STARTING");
    // pZumoRobot->SetCurrentState(pZumoRobot->Search);
}

void loop()
{
    pZumoRobot->MoveForwards(200, 2);
    // pZumoRobot->SetWaitDuration(0);

    // // pZumoRobot->IsCollided();
    // pZumoRobot->HitBorder();

    // switch (pZumoRobot->GetCurrentState())
    // {
    // case pZumoRobot->Search:
    //     // Serial.println("SEARCH");
    //     pZumoRobot->SetSpeed(SEARCH_SPEED, SEARCH_SPEED);
    //     break;
    // case pZumoRobot->Fight:
    //     Serial.println("FIGHT");
    //     pZumoRobot->SetSpeed(ACCELERATED_SPEED, ACCELERATED_SPEED);
    //     pZumoRobot->SetWaitDuration(500);
    //     break;
    // case pZumoRobot->LeftTurn:
    //     Serial.println("LEFT TURN");
    //     pZumoRobot->PlaySound();
    //     pZumoRobot->TurnLeft(random(180));
    //     // pZumoRobot->SetSpeed(0, 0);
    //     // pZumoRobot->SetWaitDuration(WAIT_DURATION);
    //     break;
    // case pZumoRobot->RightTurn:
    //     pZumoRobot->PlaySound();
    //     Serial.println("RIGHT TURN");
    //     pZumoRobot->TurnRight(random(180));
    //     // pZumoRobot->SetSpeed(0, 0);
    //     // pZumoRobot->SetWaitDuration(WAIT_DURATION);
    //     break;
    // case pZumoRobot->Flight:
    //     Serial.println("FLIGHT");
    //     int nOption = random(2);
    //     if (nOption == 1)
    //         pZumoRobot->TurnLeft(random(180));
    //     else
    //         pZumoRobot->TurnRight(random(180));
    //     pZumoRobot->SetSpeed(0, 0);
    //     pZumoRobot->SetWaitDuration(WAIT_DURATION);
    //     break;
    // default:
    //     pZumoRobot->SetSpeed(0, 0);
    //     break;
    // }
    // delay(pZumoRobot->GetWaitDuration());
}
