# Remote control

This project consists of a publisher and subscriber that communicate using Zenoh. The publisher reads joystick input and sends it to the subscriber, which controls the robot.

The message are 12-byte payloads containing 3 floats:

- x-axis speed;
- y-axis speed;
- rotation speed.

The differential drive robot only uses the y-axis speed and rotation speed. The x-axis speed is included for future use, e.g. to control a robot with omni wheels.

## Publisher

Dependencies:

```sh
sudo apt install joystick
```

Building:

```sh
cd publisher
mkdir build && cd build
cmake ..
make
```

Running:

```sh
$ ./joystick -h

Allowed options:
  -h, --help                      produce help message
  -d, --device arg (=0)           device index
  --move-x-axis arg (=3)          movement x-axis number
  --move-y-axis arg (=4)          movement y-axis number
  --turn-axis arg (=0)            turn axis number
  --move-speed arg (=0.2)         max moving speed (m/s)
  --turn-speed arg (=0.5)         max turning speed (rad/s)
  --key arg (=rc/{device index})  zenoh key
  --inc-speed-button arg (=3)     increase speed button number
  --dec-speed-button arg (=2)     decrease speed button number
  --reset-speed-button arg (=0)   reset speed button number
```

## Subscriber

Building:

```sh
cd subscriber
mkdir build && cd build
cmake ..
make
```

Running:

```sh
$ ./differential_drive -h

Allowed options:
  -h, --help                          produce help message
  --key arg (=rc/0)                   zenoh key
  -r, --wheel-radius arg (=0.08)      wheel radius (m) for differential drive calculation
  -b, --vehicle-width arg (=0.31)     distance between wheels (m) for differential drive calculation
  -d, --device arg (=/dev/ttyACM0)    device path
  --max-move-speed arg (=1)           max moving speed (m/s)
  --max-turn-speed arg (=2)           max turning speed (rad/s)
  --left-motor-id arg (=1)            left motor ID
  --right-motor-id arg (=2)           right motor ID
  --kill-timeout arg (=250)           stop motors if no commands received for this time (ms)
  --stop-threshold arg (=0.025)       stop motors if move and turn speeds below this value (m/s or rad/s)
  --max-torque arg (=1)               Moteus max_torque
  --feedforward-torque arg (=0)       Moteus feedforward_torque
  --kp-scale arg (=4)                 Moteus kp_scale
  --kd-scale arg (=4)                 Moteus kd_scale
  -m, ----motor-speed-multiplier arg (=0.67)
                                      Multipler to convert wheel rotation speed to motor speed value
```

To find Moteus device, run:

```sh
sudo dmesg | grep "USB ACM device"
```

Based on the example output

```sh
[   11.902840] cdc_acm 1-2.4:1.0: ttyACM0: USB ACM device
```

the device path is `/dev/ttyACM0`.
