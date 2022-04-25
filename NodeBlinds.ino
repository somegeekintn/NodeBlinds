// Node Blinds
// node_blinds.ino

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <LittleFS.h>
#include <deque>

#include "config.h"
#include "Consts.h"
#include "MotorDriver.h"
#include "ServerMgr.h"

WiFiClient      gWifiClient;
ServerMgr       gWebServer;
PubSubClient    gMQTTClient(gWifiClient);
char            gMsgBuffer[MSG_BUF_LEN];
unsigned long   gMQTTConnectTime = 0;

MotorDriver     gStepperL(STEPPER_STEPS_PER_REV, BLIND_1_DIR, BLIND_1_STEP, BLIND_1_SLEEP);
MotorDriver     gStepperR(STEPPER_STEPS_PER_REV, BLIND_2_DIR, BLIND_2_STEP, BLIND_2_SLEEP);

void setup_wifi() {
    Serial.print("Connecting to "); Serial.println(kSSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(kSSID, kWIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("WiFi connected @ : "); Serial.println(WiFi.localIP());

    gWebServer.setup();
}

void setup_mqtt() {
    gMQTTClient.setServer(kMQTT_SERVER, 1883);
    gMQTTClient.setCallback(mqttCallback);
}

void setup_motors() {
    gStepperL.setRPM(STEPPER_RPM);
    gStepperR.setRPM(STEPPER_RPM);
}

std::deque<String> parseTopic(char *topic) {
    std::deque<String>   topicPath;
    char                *topicCopy = strdup(topic);
    char                *token, *string = topicCopy;

    while((token = strsep(&string, "/")) != NULL) {
        topicPath.push_back(String(token));
    }

    free(topicCopy);

    return topicPath;    
}

void handleBlindsCommand(String &node, motor_target target, String &payload) {
    // command_topic: "home/blinds/lr/*/command"
    // state_topic: "home/blinds/lr/*/state"
    // availability_topic: "home/blinds/lr/avail"
    // tilt_command_topic: "home/blinds/lr/*/tiltWr"
    // tilt_status_topic: "home/blinds/lr/*/tiltRd"

    if (node == "command") {
        if (payload == "STOP") {
            if (target == motor_left || target == motor_both) {
                int curAng = gStepperL.currentAngle();
                int tgtAng = gStepperL.targetAngle();
                
                if (curAng != tgtAng) {
                    int newTarget = tgtAng < curAng ? curAng - 1 : curAng + 1;

                    gMQTTClient.publish("home/blinds/lr/left/tiltWr", String(newTarget).c_str());
                }
            }
            if (target == motor_right || target == motor_both) {
                int curAng = gStepperR.currentAngle();
                int tgtAng = gStepperR.targetAngle();
                
                if (curAng != tgtAng) {
                    int newTarget = tgtAng < curAng ? curAng - 1 : curAng + 1;

                    gMQTTClient.publish("home/blinds/lr/right/tiltWr", String(newTarget).c_str());
                }
            }
        }
        else {
            int targetAngle;

            if (payload == "OPEN") {
                targetAngle = MotorDriver::openTargetAngle();
            }
            else if (payload == "CLOSE") {
                targetAngle = MotorDriver::closedTargetAngle();
            }

            if (target == motor_left || target == motor_both) {
                gMQTTClient.publish("home/blinds/lr/left/tiltWr", String(targetAngle).c_str());
            }
            if (target == motor_right || target == motor_both) {
                gMQTTClient.publish("home/blinds/lr/right/tiltWr", String(targetAngle).c_str());
            }
        }
    }
    else if (node == "tiltWr") {
        int newAngle = payload.toInt();

        if (target == motor_left || target == motor_both) {
            gStepperL.setTargetAngle(newAngle);
        }
        if (target == motor_right || target == motor_both) {
            gStepperR.setTargetAngle(newAngle);
        }
    }
    else if (node == "tiltRd") {
        switch (target) {
            case motor_left:
                if (!gStepperL.positionRestored()) {
                    gStepperL.restoreAngle(payload.toInt());
                }
                break;

            case motor_right:
                if (!gStepperR.positionRestored()) {
                    gStepperR.restoreAngle(payload.toInt());
                }
                break;
            
            default:
                break;
        }
    }

    if (gStepperL.mStateDirty) {
        gMQTTClient.publish("home/blinds/lr/left/state", gStepperL.stateString());
        gStepperL.mStateDirty = false;
    }
    if (gStepperR.mStateDirty) {
        gMQTTClient.publish("home/blinds/lr/right/state", gStepperR.stateString());
        gStepperR.mStateDirty = false;
    }
}

void mqttCallback(char* c_topic, byte* rawPayload, unsigned int length) {
    char                *payloadCopy = strndup((char *)rawPayload, length);
    String              topic(c_topic);
    String              payload(payloadCopy);
    String              node;
    std::deque<String>  topicPath = parseTopic(c_topic);

    node = topicPath.front();
    if (node == "home") {
        topicPath.pop_front();
        node = topicPath.front();

        if (node == "blinds") {
            topicPath.pop_front();
            node = topicPath.front();

            if (node == "lr") {
                topicPath.pop_front();
                node = topicPath.front();

                if (node == "all") {
                    handleBlindsCommand(topicPath[1], motor_both, payload);
                }
                else if (node == "left") {
                    handleBlindsCommand(topicPath[1], motor_left, payload);
                }
                else if (node == "right") {
                    handleBlindsCommand(topicPath[1], motor_right, payload);
                }
                else if (node == "home") {
                    gStepperL.resetPosition();
                    gStepperR.resetPosition();
                    gMQTTClient.publish("home/blinds/lr/left/tiltRd", "0", true);
                    gMQTTClient.publish("home/blinds/lr/right/tiltRd", "0", true);
                }
            }
        }
    }
    else if (node == "homeassistant") {
        topicPath.pop_front();
        node = topicPath.front();

        if (node == "status") {
            // One of "online" / "offline"
            // TODO: Relevant messages should be retained, so do we need to resend anything?
            Serial.print("home assistant state change: "); Serial.println(payload.c_str());
        }
    }

    free(payloadCopy);
}

void mqttReconnect() {
    while (!gMQTTClient.connected()) {
        Serial.print("Attempting MQTT connection...");

        if (gMQTTClient.connect(kMQTT_CLIENT, kMQTT_USER, kMQTT_PASS, "home/blinds/lr/avail", 2, false, "offline")) {
            gMQTTConnectTime = millis();
            Serial.print("MQTT connected @ "); Serial.println(gMQTTConnectTime);
            gMQTTClient.publish("home/blinds/lr/avail", "online", true);
            gMQTTClient.subscribe("home/blinds/lr/#");
            gMQTTClient.subscribe("homeassistant/status");
        } else {
            snprintf(gMsgBuffer, MSG_BUF_LEN, "failed, rc=%d try again in 5 seconds", gMQTTClient.state());
            Serial.println(gMsgBuffer);

            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);

    if (!LittleFS.begin()) {
        Serial.print(F("Failed to mount filesystem (LittleFS)"));
    }
    
    setup_motors();
    setup_wifi();
    setup_mqtt();
}

void serviceSteppers() {
    motor_state prevState;

    prevState = gStepperL.state();
    if (prevState == closing || prevState == opening) {
        int prevAngle = gStepperL.currentAngle();

        gStepperL.service();

        if (gStepperL.state() != prevState || (gStepperL.currentAngle() != prevAngle && (millis() - gStepperL.mPublishTime) > PUBLISH_DELAY)) {
            if (prevState != gStepperL.state()) {
                gMQTTClient.publish("home/blinds/lr/left/state", gStepperL.stateString(), true);
            }
            gMQTTClient.publish("home/blinds/lr/left/tiltRd", String(gStepperL.currentAngle()).c_str(), true);
            gStepperL.mPublishTime = millis();
        }
    }

    prevState = gStepperR.state();
    if (prevState == closing || prevState == opening) {
        int prevAngle = gStepperR.currentAngle();

        gStepperR.service();

        if (gStepperR.state() != prevState || (gStepperR.currentAngle() != prevAngle && (millis() - gStepperR.mPublishTime) > PUBLISH_DELAY)) {
            if (prevState != gStepperR.state()) {
                gMQTTClient.publish("home/blinds/lr/right/state", gStepperR.stateString(), true);
            }
            gMQTTClient.publish("home/blinds/lr/right/tiltRd", String(gStepperR.currentAngle()).c_str(), true);
            gStepperR.mPublishTime = millis();
        }
    }
}

void loop() {
    if (!gMQTTClient.connected()) {
        mqttReconnect();
    }
    else {
        unsigned long connectedTime = millis() - gMQTTConnectTime;

        // Allow 2 seconds for retained position to arrive which seems more than ample
        if (!gStepperL.positionRestored() && connectedTime > 2000) {
            gStepperL.restoreAngle(gStepperR.currentAngle());
            gMQTTClient.publish("home/blinds/lr/left/state", gStepperL.stateString(), true);
            gMQTTClient.publish("home/blinds/lr/left/tiltRd", String(gStepperL.currentAngle()).c_str(), true);
        }
        if (!gStepperR.positionRestored() && connectedTime > 2000) {
            gStepperR.restoreAngle(gStepperR.currentAngle());
            gMQTTClient.publish("home/blinds/lr/right/state", gStepperR.stateString(), true);
            gMQTTClient.publish("home/blinds/lr/right/tiltRd", String(gStepperR.currentAngle()).c_str(), true);
        }
    }

    gMQTTClient.loop();
    gWebServer.loop();
    
    serviceSteppers();
}