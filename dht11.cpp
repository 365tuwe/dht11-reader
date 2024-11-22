#include "dht11.h"
#include <chrono>
#include <iostream>
#include <math.h>
#include <unistd.h>
#include <vector>
#include <wiringPi.h>

#define LH_THRESHOLD 28 // Low/High threshold depending on sensor

// Setup pin as input
DHT11::DHT11(int gpio) {
  wiringPiSetupGpio();
  pin = gpio;
  std::cout << "Setting pin: " << pin << std::endl;
}

void DHT11::getData() {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  usleep(18000);
  digitalWrite(pin, HIGH);
  pinMode(pin, INPUT);

  do {
  } while (digitalRead(pin) == HIGH);
  do {
  } while (digitalRead(pin) == LOW);
  do {
  } while (digitalRead(pin) == HIGH);

  hum_high = readBits();
  hum_low = readBits();
  temp_high = readBits();
  temp_low = readBits();
  checksum = readBits();
}

// Returns temperature in float data type
float DHT11::getTemperature() { return conv(temp_high) + 0.1 * conv(temp_low); }

// Returns humidity in float data type
float DHT11::getHumidity() { return conv(hum_high) + 0.1 * conv(hum_low); }

// Reads 8 bit incoming data from DHT11
// Uncomment the following lines to see the time that it takes to read a bit
std::vector<int> DHT11::readBits() {

  std::vector<int> sum;
  for (int i = 0; i < 8; i++) {
    int width = 0;
    auto start1 = std::chrono::high_resolution_clock::now();
    do {
    } while (digitalRead(pin) == LOW);
    auto stop1 = std::chrono::high_resolution_clock::now();
    auto wait =
        std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);
    auto start2 = std::chrono::high_resolution_clock::now();
    while (digitalRead(pin) == HIGH) {
      width++;
      if (width > 1000) {
        // The sensor sometimes times out and keeps the voltage high.
        if (debug)
          std::cout << "TIME OUT " << std::endl;
        // Drop dataset.
        break;
      }
    }
    auto stop2 = std::chrono::high_resolution_clock::now();
    auto signal =
        std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start2);
    if (width > LH_THRESHOLD * 10) {
      sum.push_back(1);
    } else {
      sum.push_back(0);
    }
    if (debug) {
      std::cout << "bit:       " << i << std::endl;
      std::cout << "Wait time: " << wait.count() << std::endl;
      std::cout << "Width:     " << signal.count() << std::endl;
    }
  }
  return sum;
}

// converts vector data type to float datatype
float DHT11::conv(std::vector<int> num) {
  float x = 0;

  for (int i = 0; i < 8; i++) {
    x += num[i] * pow(2, 7 - i);
  }

  return x;
}

// Checks to see if checksum is valid
bool DHT11::check() {
  float total =
      conv(hum_high) + conv(hum_low) + conv(temp_high) + conv(temp_low);
  if (conv(checksum) == total) {
    return true;
  }

  return false;
}
