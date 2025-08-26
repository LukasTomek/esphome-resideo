/**
  @AUTHOR (c) 2025 Pluimvee (Erik Veer)

  Specifications CM1106 CO2-Sensor
  https://en.gassensor.com.cn/Product_files/Specifications/CM1106-C%20Single%20Beam%20NDIR%20CO2%20Sensor%20Module%20Specification.pdf

 */ 
#include "cm1106_sniffer_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cm1106_sniffer {

static const char *TAG = "cm1106_sniffer";

///////////////////////////////////////////////////////////////////////////////////////////////////
// We use the standard (default) Serial (UART) to receive the data from the CM1106 sensor.
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::setup() {
  //Serial.begin(9600);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// We capture the value in the loop() method, so we are
// 1. fast to not miss any values
// 2. independend on the update_interval
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::loop() 
{
  while (this->available() > 0) {
    uint8_t data;
    this->read_byte(&data);

    ESP_LOGV(TAG, "Read byte from sensor: %x", data);

    if (this->buffer_.empty() && data != 0xFF)
      continue;

    this->buffer_.push_back(data);
    if (this->buffer_.size() >= 8)
      this->check_buffer_();
  }
}

void CM1106SnifferSensor::check_buffer_() 
{
  // All read responses start with 0x16
  // The payload length for the Co2 message is 0x05
  // The command for the Co2 message is 0x01
  uint8_t expected_header[3] = {0x16, 0x05, 0x01};

   // find the start of the message
  int matched = 0;
  while (matched < this->buffer_.size()) {
    if (this->buffer_[matched] == expected_header[0] && this->buffer_[matched + 1] == expected_header[1] && this->buffer_[matched + 2] == expected_header[2]) {
        ESP_LOGV(TAG, "we got header in : %x", matched);
        break;
    } else {
      matched++;
    }
  }
  // if we end up here we have found the header
  if (this->buffer_.size() < matched + 5) {
    ESP_LOGW(TAG, "Not enough bytes available after header match: %d", this->available());
    return; // not enough bytes available to read the rest of the message
  }

  // Checksum: 256-(HEAD+LEN+CMD+DATA)%256
  uint8_t crc = 0;
  for (int i = matched; i < this->buffer_.size(); ++i)
    crc -= this->buffer_[i];
    ESP_LOGV(TAG, "CRC i: %x", i);

  // We have included the CRC checksum (last byte of the response) so the crc should equal to 0x00
  if (crc != 0x00) {
    ESP_LOGW(TAG, "Bad checksum: got 0x%02X, expected 0x00", crc);
    return; // checksum does not match
  }
  // everything is okay, store the PPM in cached_ppm_ so it can be published
  cached_ppm_ = (this->buffer_[i] << 8) |this->buffer_[i];
  this->buffer_.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Publish the cached PPM value to the frontend. 
///////////////////////////////////////////////////////////////////////////////////////////////////
void CM1106SnifferSensor::update() {
  this->publish_state(cached_ppm_);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

}  // namespace cm1106_sniffer
}  // namespace esphome
