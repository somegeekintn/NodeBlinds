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
#define STEP_DEGREES        (1.0 / (2048.0 / 360.0))
#define CLOSED_TARGET_ANGLE 72.0
#define OPEN_TARGET_ANGLE   0.0

// The blinds are controlled by a string wound around a cylinder (in my case a cylinder with
// flat bits but we just pretend it's a cyclinder) so we can determine the amount that the
// blinds raise / lower by examining the % of circumference travels and using that as one leg
// of a right triangle to translate it to degrees traveled at the blinds.

#define BARREL_CIRC         (28.0 * M_PI)
#define BLINDS_RAD          26.0

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
    float rads = ((float)degrees / 180.0) * M_PI;
    float travel = sin(rads) * BLINDS_RAD;
    float position = (travel / BARREL_CIRC * 360.0) / STEP_DEGREES;

    return round(position);
}

int MotorDriver::degreesFromPosition(int position) {
    float angleB = (float)position * STEP_DEGREES;
    float travel = angleB / 360.0 * BARREL_CIRC;
    float degrees = asin(travel / BLINDS_RAD) * 180.0 / M_PI;

    return round(degrees);
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
    return degreesFromPosition(mCurPosition);
}

int MotorDriver::targetPosition() {
    return mTargetPosition;
}

int MotorDriver::targetAngle() {
    return degreesFromPosition(mTargetPosition);
}

motor_state MotorDriver::state() {
    motor_state state;

    if (mTargetPosition != mCurPosition) {
        state = abs(mTargetPosition) > abs(mCurPosition) ? closing : opening;
    }
    else {
        state = abs(mCurPosition) > (CLOSED_TARGET_ANGLE / 2.0) ? closed : open;
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
