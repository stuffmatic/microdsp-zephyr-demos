#include "codec.h"
#include <zephyr/drivers/i2c.h>

#define WM8758B_ADDRESS 0b11010
#define WM8904_ADDRESS 0b110100
#define I2C_NODE DT_NODELABEL(i2c0)
static const struct device* i2c_dev = DEVICE_DT_GET(I2C_NODE);

void init_codec() {
  if (device_is_ready(i2c_dev)) {
    uint8_t payload[2] = { 0, 0 }; // Reset command
    int write_result = i2c_write(i2c_dev, payload, 2, WM8758B_ADDRESS);
    printk("i2c_write result %d\n", write_result);
  } else {
    // TODO: report error
  }
}