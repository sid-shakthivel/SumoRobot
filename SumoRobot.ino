#include <ZumoShield.h>
#include <Wire.h>

constexpr float XY_ACCELERATION_THRESHOLD = 100000000;
constexpr float TURN_SPEED = 150;
constexpr float SPEED = 150;
constexpr float ACCELERATED_SPEED = 300;
constexpr float MILISECONDS = 1000;

class CZumoRobot
{
public:
    enum EState
    {
        Search,
        Fight,
        Flight,
        LeftTurn,
        RightTurn
    };

    CZumoRobot() : m_Button(ZUMO_BUTTON)
    {
        m_Imu.init();
        m_Imu.enableDefault();
        m_ReflectanceSensors.init();

        m_Button.waitForPress();
        Calibrate();
    }

    void MoveForwards(int nSpeed, int nDuration)
    {
        m_Motors.setSpeeds(nSpeed, nSpeed);
        delay(nDuration * 1000);
        m_Motors.setSpeeds(0, 0);
    }

    void SetSpeed(int nLeftSpeed, int nRightSpeed)
    {
        m_Motors.setSpeeds(nLeftSpeed, nRightSpeed);
    }

    void SetCurrentState(EState eNextState)
    {
        eCurrentState = eNextState;
    }

    EState GetCurrentState()
    {
        switch (eCurrentState)
        {
        case Search:
            // Serial.println("SEARCH");
            break;
        case Fight:
            Serial.println("FIGHT");
            break;
        case Flight:
            Serial.println("FLIGHT");
            break;
        case RightTurn:
            Serial.println("RIGHT TURN");
            break;
        case LeftTurn:
            Serial.println("LEFT TURN");
            break;
        default:
            Serial.println("DEFAULT");
            break;
        }
        return eCurrentState;
    }

    void SetWaitDuration(float fNextWaitDuration)
    {
        fWaitDuration = fNextWaitDuration;
    }

    float GetWaitDuration()
    {
        return fWaitDuration;
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
        return m_Button.waitForPress();
    }

    bool IsPressed()
    {
        return m_Button.getSingleDebouncedPress();
    }

    void IsHitBorder()
    {
        m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
        if (m_rgucReflectanceSensorReadings[0] > 500)
            SetCurrentState(LeftTurn);
        else if (m_rgucReflectanceSensorReadings[5] > 500)
            SetCurrentState(RightTurn);
    }

    void IsCollided()
    {
        ReadMeasurements();
        unsigned long ulXAcceleration = (unsigned long)m_Imu.a.x * (unsigned long)m_Imu.a.x;
        unsigned long ulYAcceleration = (unsigned long)m_Imu.a.y * (unsigned long)m_Imu.a.y;
        unsigned long ulNetAcceleration = ulXAcceleration + ulYAcceleration;
        if (ulNetAcceleration >= XY_ACCELERATION_THRESHOLD)
        {
            // int nOption = random(2);
            // if (nOption == 0)
            //     SetCurrentState(FIGHT);
            // else
            //     SetCurrentState(FLIGHT);
            SetCurrentState(Fight);
        }
    }

private:
    ZumoMotors m_Motors;
    ZumoBuzzer m_Buzzer;
    ZumoIMU m_Imu;
    ZumoReflectanceSensorArray m_ReflectanceSensors;
    unsigned int m_rgucReflectanceSensorReadings[6];
    Pushbutton m_Button;
    EState eCurrentState;
    float fWaitDuration;

    void ReadMeasurements()
    {
        m_Imu.read();
        m_Imu.g.x = abs(m_Imu.g.x * 0.00875f);
        m_Imu.g.y = abs(m_Imu.g.y * 0.00875f);
        m_Imu.g.z = abs(m_Imu.g.z * 0.00875f);
    }

    void Turn(float fTargetAngle, bool bIsLeft)
    {
        float fCurrentAngle = 0;
        float fTime = 0;
        float fOldTime = 0;

        SetSpeed(0, 0);
        delay(100);

        if (bIsLeft)
            SetSpeed(TURN_SPEED, -TURN_SPEED);
        else
            SetSpeed(-TURN_SPEED, TURN_SPEED);

        while (fCurrentAngle <= fTargetAngle)
        {
            fTime = millis();
            ReadMeasurements();
            fCurrentAngle += m_Imu.g.z * ((fTime - fOldTime) / MILISECONDS);
            fOldTime = fTime;
        }

        SetSpeed(0, 0);
        SetWaitDuration(100);
        SetCurrentState(Search);
    }

    void PlaySound(char *cNotes = "f4")
    {
        m_Buzzer.play(cNotes);
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
    Serial.begin(115200);
    Wire.begin();
    pZumoRobot = new CZumoRobot;

    pZumoRobot->WaitForPress();
    Serial.println("STARTING!");
    pZumoRobot->SetCurrentState(pZumoRobot->Search);
}

void loop()
{
    pZumoRobot->SetWaitDuration(100);
    pZumoRobot->IsHitBorder();
    pZumoRobot->IsCollided();
    switch (pZumoRobot->GetCurrentState())
    {
    case pZumoRobot->Search:
        pZumoRobot->SetSpeed(SPEED, SPEED);
        break;
    case pZumoRobot->LeftTurn:
        pZumoRobot->TurnLeft(random(180));
        break;
    case pZumoRobot->RightTurn:
        pZumoRobot->TurnRight(random(180));
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
        break;
    default:
        break;
    }
    delay(pZumoRobot->GetWaitDuration());
}