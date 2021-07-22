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

    void SetCurrentState(EState eNextState)
    {
        eCurrentState = eNextState;
    }

    EState GetCurrentState()
    {
        return eCurrentState;
    }

    void HitBorderLeft()
    {
        m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
        if (m_rgucReflectanceSensorReadings[0] > 500)
            eCurrentState = LeftTurn;
    }

    void HitBorderRight()
    {
        m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
        if (m_rgucReflectanceSensorReadings[5] > 500)
            eCurrentState = RightTurn;
    }

    void IsCollided()
    {
        m_Imu.read();
        long xAcceleration = (long)m_Imu.a.x * (long)m_Imu.a.x;
        long yAcceleration = (long)m_Imu.a.y * (long)m_Imu.a.y;
        long netAcceleration = (long)xAcceleration + (long)yAcceleration;
        if (netAcceleration >= XY_ACCELERATION_THRESHOLD)
        {
            int nOption = random(2);
            if (nOption == 1)
            {
                eCurrentState = Fight;
            }
            else
            {
                eCurrentState = Flight;
            }
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

        // Once turned go back to search
        eCurrentState = Search;
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
};

CZumoRobot *pZumoRobot;

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    pZumoRobot = new CZumoRobot;

    pZumoRobot->WaitForPress();
    Serial.println("STARTING");
    pZumoRobot->SetCurrentState(pZumoRobot->Search);
}

void loop()
{
    pZumoRobot->SetWaitDuration(0);
    pZumoRobot->SetCurrentState(pZumoRobot->Search);

    pZumoRobot->HitBorderLeft();
    pZumoRobot->HitBorderRight();
    pZumoRobot->IsCollided();
    switch (pZumoRobot->GetCurrentState())
    {
    case pZumoRobot->Search:
        pZumoRobot->SetSpeed(SEARCH_SPEED, SEARCH_SPEED);
        break;
    case pZumoRobot->Fight:
        pZumoRobot->SetSpeed(ACCELERATED_SPEED, ACCELERATED_SPEED);
        pZumoRobot->SetWaitDuration(500);
        break;
    case pZumoRobot->Flight:
        int nOption = random(2);
        if (nOption == 1)
            pZumoRobot->TurnLeft(random(180));
        else
            pZumoRobot->TurnRight(random(180));
        pZumoRobot->SetSpeed(0, 0);
        pZumoRobot->SetWaitDuration(WAIT_DURATION);
        break;
    case pZumoRobot->RightTurn:
        pZumoRobot->TurnRight(random(180));
        pZumoRobot->SetSpeed(0, 0);
        pZumoRobot->SetWaitDuration(WAIT_DURATION);
        break;
    case pZumoRobot->LeftTurn:
        pZumoRobot->TurnLeft(random(180));
        pZumoRobot->SetSpeed(0, 0);
        pZumoRobot->SetWaitDuration(WAIT_DURATION);
        break;
    default:
        pZumoRobot->SetSpeed(SEARCH_SPEED, SEARCH_SPEED);
        break;
    }
    delay(pZumoRobot->GetWaitDuration());
}
