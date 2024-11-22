#include "dht11.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <mqtt/async_client.h>
#include <thread>
#include <yaml-cpp/yaml.h>

bool debug = false;

// Overload operator<< for debugging
// to output raw data
#include <algorithm>
#include <iterator>
std::ostream &operator<<(std::ostream &out, const std::vector<int> &v) {
  if (!v.empty()) {
    out << '[';
    std::ranges::copy(v, std::ostream_iterator<int>(out, ", "));
    out << "\b\b]"; // backspace to overwrite last ", "
  }
  return out;
}

struct {
  std::string server_address;
  std::string client_id = "DHT11_Sensor";
  std::string username;
  std::string password;
  std::string temperature_topic;
  std::string humidity_topic;
  unsigned int pin = 4;
  unsigned int qos = 1;
  unsigned int interval = 60;
} settings;

// MQTT Callback function, see
// https://github.com/eclipse-paho/paho.mqtt.cpp/blob/3631bce7e0bd74cc11a46410669ccd2e23b6caed/examples/async_publish.cpp#L66
class Callback : public virtual mqtt::callback {
public:
  void connected(const std::string &cause) override {
    if (debug) {
      std::cout << "\n\tConnection success\n";
      if (!cause.empty()) {
        std::cout << "\tcause: " << cause << std::endl;
      }
    }
  }

  void connection_lost(const std::string &cause) override {
    if (debug) {
      std::cout << "\n\tConnection lost\n";
      if (!cause.empty()) {
        std::cout << "\tcause: " << cause << std::endl;
      }
    }
  }

  void delivery_complete(mqtt::delivery_token_ptr tok) override {
    if (debug) {
      std::cout << "Delivery complete for token: "
                << (tok ? tok->get_message_id() : -1) << std::endl;
    }
  }
};

int main(int argc, char *argv[]) {
  int opt;
  std::string settingsFile;
  while ((opt = getopt(argc, argv, "f:d")) != -1) {
    if (opt == 'f') {
      settingsFile = optarg;
    } else if (opt == 'd') {
      debug = true;
    }
  }

  if (settingsFile.empty()) {
    settingsFile = "./config.yaml";
  }
  std::ifstream ifs(settingsFile);
  if (!ifs.good()) {
    std::cerr << "Cannot read file " << settingsFile << std::endl;
    return 1;
  }
  // Read settings from yaml file
  // Beware: no error checking
  YAML::Node config = YAML::LoadFile(settingsFile);
  settings.server_address = config["mqtt"]["address"].as<std::string>();
  settings.client_id =
      config["mqtt"]["client_id"].as<std::string>(settings.client_id);
  settings.username = config["mqtt"]["username"].as<std::string>();
  settings.password = config["mqtt"]["password"].as<std::string>();
  settings.temperature_topic = config["topic"]["temperature"].as<std::string>();
  settings.humidity_topic = config["topic"]["humidity"].as<std::string>();
  settings.pin = config["pin"].as<unsigned int>(settings.pin);
  settings.qos = config["qos"].as<unsigned int>(settings.qos);
  settings.interval = config["interval"].as<unsigned int>(settings.interval);

  // DHT11 sensor connected to GPIO pin 4 (which is pin 7 on rpi 4)
  DHT11 sensor(settings.pin);

  float temperature, humidity;

  // mqtt publisher setup
  mqtt::async_client client(settings.server_address, settings.client_id,
                            nullptr);
  Callback cb;
  client.set_callback(cb);

  mqtt::connect_options connOpts;
  connOpts.set_user_name(settings.username);
  connOpts.set_password(settings.password);
  connOpts.set_keep_alive_interval(20);
  connOpts.set_clean_session(true);

  std::cout << "Connecting to the MQTT server... ";
  mqtt::token_ptr connTok = client.connect(connOpts);
  connTok->wait();
  std::cout << "OK" << std::endl;

  while (true) {
    sensor.getData();
    if (debug) {
      std::cout << sensor.hum_high << sensor.hum_low << std::endl;
      std::cout << sensor.temp_high << sensor.temp_low << std::endl;
    }
    if (sensor.check()) {
      temperature = sensor.getTemperature();
      humidity = sensor.getHumidity();
      std::cout << "Temperature: " << temperature
                << " °C, Humidity: " << humidity << " %" << std::endl;

      mqtt::message_ptr pubmsg = mqtt::make_message(
          settings.temperature_topic, std::to_string(temperature));
      pubmsg->set_qos(settings.qos);
      client.publish(pubmsg)->wait_for(std::chrono::seconds(10));

      pubmsg =
          mqtt::make_message(settings.humidity_topic, std::to_string(humidity));
      pubmsg->set_qos(settings.qos);
      client.publish(pubmsg)->wait_for(std::chrono::seconds(10));
    } else {
      std::cout << "Failed to read sensor data" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(settings.interval));
  }

  client.disconnect()->wait();
  return 0;
}
