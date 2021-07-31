#include <ZumoShield.h>
#include <Wire.h>

constexpr float XY_ACCELERATION_THRESHOLD = 200000000;
constexpr float TURN_SPEED = 150;
constexpr float MIN_SPEED = 75;
constexpr float SPEED = 150;
constexpr float ACCELERATED_SPEED = 300;
constexpr float MAX_SPEED = 400;
constexpr float MILISECONDS = 1000;
constexpr float DEAFULT_WAIT_DURATION = 50;
constexpr float REFLECTANCE_SENSOR_THRESHOLD = 500;

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

    template <class T>
    void MoveForwards(T nSpeed, T nDuration)
    {
        m_Motors.setSpeeds(nSpeed, nSpeed);
        delay(nDuration * 1000);
        m_Motors.setSpeeds(0, 0);
    }

    template <class T>
    void SetSpeed(T nLeftSpeed, T nRightSpeed)
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
            Serial.println("SEARCH");
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
            break;
        }
        return eCurrentState;
    }

    template <class T>
    void SetWaitDuration(T fNextWaitDuration)
    {
        fWaitDuration = fNextWaitDuration;
    }

    float GetWaitDuration()
    {
        return fWaitDuration;
    }

    template <class T>
    void TurnLeft(T fDegrees)
    {
        Turn(fDegrees, true);
    }

    template <class T>
    void TurnRight(T fDegrees)
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
        if (m_rgucReflectanceSensorReadings[0] > REFLECTANCE_SENSOR_THRESHOLD)
            SetCurrentState(LeftTurn);
        else if (m_rgucReflectanceSensorReadings[5] > REFLECTANCE_SENSOR_THRESHOLD)
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
            int nOption = random(2);
            if (nOption == 0)
                SetCurrentState(Fight);
            else
                SetCurrentState(Flight);
            PlaySound("!T240 L8 agafaea f4");
        }
    }

    void TurnUntilWhite(bool bIsLeft)
    {
        m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
        unsigned int unReflectanceSensorReading = 1000;

        while (unReflectanceSensorReading > REFLECTANCE_SENSOR_THRESHOLD)
        {
            m_ReflectanceSensors.readCalibrated(m_rgucReflectanceSensorReadings);
            if (bIsLeft)
            {
                SetSpeed(TURN_SPEED, -TURN_SPEED);
                unReflectanceSensorReading = m_rgucReflectanceSensorReadings[0];
            }
            else
            {
                SetSpeed(-TURN_SPEED, TURN_SPEED);
                unReflectanceSensorReading = m_rgucReflectanceSensorReadings[5];
            }
        }

        SetSpeed(0, 0);
        delay(50);
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
        delay(50);

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

    void PlaySound(char *cNotes = "f4")
    {
        m_Buzzer.play(cNotes);
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
    pZumoRobot->SetWaitDuration(0);
    pZumoRobot->IsHitBorder();
    pZumoRobot->IsCollided();
    switch (pZumoRobot->GetCurrentState())
    {
    case pZumoRobot->Search:
        pZumoRobot->SetSpeed(random(MIN_SPEED, SPEED), random(MIN_SPEED, SPEED));
        break;
    case pZumoRobot->LeftTurn:
        pZumoRobot->TurnUntilWhite(true);
        pZumoRobot->TurnLeft(random(90));
        break;
    case pZumoRobot->RightTurn:
        pZumoRobot->TurnUntilWhite(false);
        pZumoRobot->TurnRight(random(90));
        break;
    case pZumoRobot->Fight:
        float fNewSpeed = random(ACCELERATED_SPEED, MAX_SPEED);
        pZumoRobot->SetSpeed(fNewSpeed, fNewSpeed);
        break;
    case pZumoRobot->Flight:
        int nOption = random(2);
        if (nOption == 1)
            pZumoRobot->TurnLeft(random(180));
        else
            pZumoRobot->TurnRight(random(180));
        pZumoRobot->SetCurrentState(pZumoRobot->Search);
        break;
    default:
        break;
    }
    delay(pZumoRobot->GetWaitDuration());
}