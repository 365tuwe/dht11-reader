# DHT11 for Raspberry Pi 4

Yet another tool to use a DHT11 sensor with a Raspberry Pi 4.

I use a Raspberry Pi 4 as inexpensive hardware to run several services in my home network (Pi-Hole, Heimdall, Home Assistant, ...).

The sensor is simply plugged into the GPIO pins. I use the following pins:

* Pin 4 -> VCC (5V)
* Pin 6 -> Ground
* Pin 7 -> Signal

Note: Pin 7 is GPIO pin 4 and must be entered as such in the configuration.

## Build

Dependencies: libyaml-cpp, wiringPi, paho-mqtt

Just run the Makefile like so:

```
make
```

## Run

Create a settings file in `yaml` format:

```yaml
mqtt:
  address: "tcp://192.168.1.10:1883"
  username: "admin"
  password: "topsecret123!"
  client_id: "DHT11_Sensor"
topic:
  humidity: "home/serverroom/humidity"
  temperature: "home/serverroom/temperature"
pin: 4
interval: 60
debug: false
```

Then simply run `./dht11_reader -f config.yaml`.

### Run as systemd service

Create a systemd service, for example `/etc/systemd/system/dht11.service`:

```
[Unit]
Description="Read DHT11 sensor data"

[Service]
ExecStart=/usr/local/bin/dht11_reader -f /home/pi/dht11-config.yaml

[Install]
WantedBy=multi-user.target
```

and activate it with

```
sudo systemctl daemon-reload
sudo systemctl enable dht11.service
sudo systemctl start dht11.service
```

## Useful links

* [DHT11 Sensor Datasheet](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf)
* [WiringPi](https://github.com/WiringPi/WiringPi)
* [Paho MQTT](https://eclipse.dev/paho/)
