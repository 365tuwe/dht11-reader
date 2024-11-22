#ifndef DHT11_h
#define DHT11_h

#include <vector>

extern bool debug;

class DHT11 {
public:
  DHT11(int);
  void getData();
  float getTemperature();
  float getHumidity();
  std::vector<int> hum_high, hum_low, temp_high, temp_low, checksum;
  bool check();

private:
  int pin;
  float hum, temp;
  std::vector<int> readBits();
  float conv(std::vector<int>);
};
#endif
