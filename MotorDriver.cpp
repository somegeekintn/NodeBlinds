// NodeBlinds
// MotorDriver.cpp

// 8825 Timing info:
// ƒSTEP Step max frequency 250 kHz
// tWH(STEP) Pulse duration min , STEP high 1.9 μs
// tWL(STEP) Pulse duration min, STEP low 1.9 μs
// tSU(STEP) Setup time min, command before STEP rising 650 ns
// tH(STEP) Hold time min, command after STEP rising 650 ns
// tENBL Enable time min, nENBL active to STEP 650 ns
// tWAKE Wakeup time min, nSLEEP inactive high to STEP input accepted 1.7 ms

// 28BYJ-48 – 5V Stepper Motor
// Speed Variation Ratio 1/64
// Stride Angle 5.625°/64
// Frequency 100Hz
// Yields: 4,096 per revolution. This is in half steps so 2,048 full steps

#include "MotorDriver.h"
#include <Arduino.h>

#define SLEEP               LOW
#define WAKE                HIGH
#define STEP_DEGREES        (360.0 / 2048.0)
#define CLOSED_TARGET_ANGLE 100.0
#define CLOSED_TARGET_POS   (CLOSED_TARGET_ANGLE / STEP_DEGREES)
#define CLOSED_POS          (45.0 / STEP_DEGREES)
#define OPEN_TARGET_ANGLE   0.0
#define OPEN_TARGET_POS     (OPEN_TARGET_ANGLE / STEP_DEGREES)
#define OPEN_POS            (10.0 / STEP_DEGREES)

MotorDriver::MotorDriver(int resolution, int dir_pin, int step_pin, int sleep_pin) { 
    mResolution = resolution;
    mDirPin = dir_pin;
    mStepPin = step_pin;
    mSleepPin = sleep_pin;

    pinMode(mDirPin, OUTPUT);
    pinMode(mStepPin, OUTPUT);
    pinMode(mSleepPin, OUTPUT);
    digitalWrite(mSleepPin, SLEEP);

    setRPM(5);
}

int MotorDriver::openTargetAngle() {
    return OPEN_TARGET_ANGLE;
}

int MotorDriver::closedTargetAngle() {
    return CLOSED_TARGET_ANGLE;
}

long MotorDriver::setRPM(int rpm) {
    long stepsPerMinute = rpm * mResolution;
    
    mStepDur_uS = 60L * 1000000L / stepsPerMinute;
    if (mStepDur_uS < 0) {
        mStepDur_uS = 0;
    }
    
    return mStepDur_uS;
}

void MotorDriver::resetPosition() {
    mCurPosition = 0;
    mTargetPosition = 0;
    mPositionRestored = true;
}

void MotorDriver::restoreAngle(int degrees) {
    mCurPosition = positionFromDegrees(degrees);
    mTargetPosition = mCurPosition;
    mPositionRestored = true;
}

void MotorDriver::setTargetAngle(int degrees) {
    setTargetPosition(positionFromDegrees(degrees));
}

void MotorDriver::setTargetPosition(int position) {
    if (mTargetPosition != position) {
        mTargetPosition = position;
        mStateDirty = true;
    }
}

int MotorDriver::positionFromDegrees(int degrees) {
    float position = (float)degrees / STEP_DEGREES;

    return position < 0.0 ? floor(position) : ceil(position);
}

void MotorDriver::service() {
    if (mTargetPosition != mCurPosition) {
        int posInc = mTargetPosition < mCurPosition ? -1 : 1;
        int dirPin = posInc > 0 ? LOW : HIGH;

        if (digitalRead(mSleepPin) == SLEEP) {
            // Use sleep to indicate if we're in the midst of a transition
            digitalWrite(mSleepPin, WAKE);
            digitalWrite(mDirPin, dirPin);
            delay(2);       // from datasheet: 1.7mS minimum

            mNextStepTime = micros();
        }
        if (digitalRead(mDirPin) != dirPin) {
            digitalWrite(mDirPin, dirPin);
            delayMicroseconds(2);       // Hold time min x 2 = 1.3uS
        }

        if (mNextStepTime <= micros()) {
            digitalWrite(mStepPin, HIGH);
            delayMicroseconds(4);       // Pulse duration min x 2 = 3.9uS
            digitalWrite(mStepPin, LOW);
            
            mCurPosition += posInc;
            mNextStepTime += mStepDur_uS;

            if (mTargetPosition == mCurPosition) {
                // we are done here. go back to sleep
                digitalWrite(mSleepPin, SLEEP);
            }
        }
    }
}

bool MotorDriver::positionRestored() {
    return mPositionRestored;
}

int MotorDriver::currentPosition() {
    return mCurPosition;
}

int MotorDriver::currentAngle() {
    return (int)((float)mCurPosition * STEP_DEGREES);
}

int MotorDriver::targetPosition() {
    return mTargetPosition;
}

int MotorDriver::targetAngle() {
    return (int)((float)mTargetPosition * STEP_DEGREES);
}

motor_state MotorDriver::state() {
    motor_state state;

    if (mTargetPosition != mCurPosition) {
        state = abs(mTargetPosition) > abs(mCurPosition) ? closing : opening;
    }
    else {
        state = abs(mCurPosition) > CLOSED_POS ? closed : open;
    }

    return state;
}

const char * MotorDriver::stateString() {
    const char    *stateString;

    switch (state()) {
        case open:      stateString = "open";       break;
        case opening:   stateString = "opening";    break;
        case closed:    stateString = "closed";     break;
        case closing:   stateString = "closing";    break;
    }

    return stateString;
}    
