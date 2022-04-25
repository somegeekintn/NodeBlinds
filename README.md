# NodeBlinds

ESP2866 (NodeMCU) project to control some hard to reach blinds communicating over MQTT managed by [Home Assistant](https://www.home-assistant.io/). 
Inspired by [this project at The Hook Up](https://www.thesmarthomehookup.com/automated-motorized-window-blinds-horizontal-blinds/).

My version controls two sets of blinds separately and I probably should have done a better job selecting GPIOs. 
I would avoid using <code>D4</code> if I had it to do again.

### configuration.yaml

```
number:
  - platform: mqtt
    name: "Left Dormer Angle"
    unique_id: "blinds_lr_left_angle"
    command_topic: "home/blinds/lr/left/tiltWr"
    state_topic: "home/blinds/lr/left/tiltRd"
    min: -72
    max: 72
  - platform: mqtt
    name: "Right Dormer Angle"
    unique_id: "blinds_lr_right_angle"
    command_topic: "home/blinds/lr/right/tiltWr"
    state_topic: "home/blinds/lr/right/tiltRd"
    min: -72
    max: 72
```
