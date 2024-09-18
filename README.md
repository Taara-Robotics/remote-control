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
  -h, --help               produce help message
  -d, --device arg (=0)    device index
  --move-axis arg (=4)     move axis
  --turn-axis arg (=0)     turn axis
  --move-speed arg (=0.5)  max moving speed (m/s)
  --turn-speed arg (=0.5)  max turning speed (rad/s)
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
