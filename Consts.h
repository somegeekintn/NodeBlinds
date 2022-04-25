// Node Blinds
// Consts.h

#define BLIND_1_SLEEP           D1
#define BLIND_1_DIR             D2
#define BLIND_1_STEP            D4
#define BLIND_2_SLEEP           D5
#define BLIND_2_DIR             D6
#define BLIND_2_STEP            D7

#define STEPPER_STEPS_PER_REV   2048
#define STEPPER_SECS_PER_REV    8
#define STEPPER_RPM             (60 / STEPPER_SECS_PER_REV)

#define PUBLISH_DELAY           200
#define MSG_BUF_LEN             96

enum motor_target { motor_left, motor_right, motor_both };
