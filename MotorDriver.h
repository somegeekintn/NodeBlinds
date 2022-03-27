// NodeBlinds
// MotorDriver.h

#ifndef MotorDriver_h
#define MotorDriver_h

enum motor_command { cmd_open, cmd_close, cmd_stop, cmd_unknown };
enum motor_state { open, opening, closed, closing };

class MotorDriver {
    public:
        MotorDriver(int resolution, int dir_pin, int step_pin, int sleep_pin);

        static int      openTargetAngle();
        static int      closedTargetAngle();

        long            setRPM(int rpm);

        void            resetPosition();
        void            restoreAngle(int degrees);
        void            setTargetAngle(int degrees);
        void            setTargetPosition(int position);
        int             positionFromDegrees(int degrees);
        int             degreesFromPosition(int position);
        void            service();

        bool            positionRestored();
        int             currentPosition();
        int             currentAngle();
        int             targetPosition();
        int             targetAngle();

        motor_state     state();
        const char *    stateString();

        bool            mStateDirty;
        unsigned long   mPublishTime;

    private:
        // void step(unsigned long pulseTime);

        int             mResolution;
        int             mRPM;
        int             mDirPin;
        int             mStepPin;
        int             mSleepPin;
        int             mStepDur_uS;
        unsigned long   mNextStepTime;

        int             mCurPosition;
        int             mTargetPosition;
        bool            mPositionRestored;
};

#endif
