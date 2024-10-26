#ifndef ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MCP9808_H_
#define ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MCP9808_H_

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
// #include <zephyr/sys/util.h>

// MCP9808 registers
#define MCP9808_REG_CONFIG     0x01
#define MCP9808_REG_TEMP_AMB   0x05
#define MCP9808_REG_RESOLUTION 0x08

// Ambient temperature register information
#define MCP9808_TEMP_SCALE_CEL 16
#define MCP9808_TEMP_SIGN_BIT  BIT(12)
#define MCP9808_TEMP_ABS_MASK  0x0FFF

// Sensor data
struct mcp9808_data {
	uint16_t reg_val;
};

// Configuration data
struct mcp9808_config {
	struct i2c_dt_spec i2c;
	uint8_t resolution;
};

// Function declarations
int mcp9808_reg_read(const struct device *dev, uint8_t reg, uint16_t *val);
int mcp9808_reg_write_16bit(const struct device *dev, uint8_t reg, uint16_t val);
int mcp9808_reg_write_8bit(const struct device *dev, uint8_t reg, uint8_t val);

/* Encode a signed temperature in scaled Celsius to the format used in
 * register values.
 */
static inline uint16_t mcp9808_temp_reg_from_signed(int temp)
{
	/* Get the 12-bit 2s complement value */
	uint16_t rv = temp & MCP9808_TEMP_ABS_MASK;

	if (temp < 0) {
		rv |= MCP9808_TEMP_SIGN_BIT;
	}
	return rv;
}

/* Decode a register temperature value to a signed temperature in
 * scaled Celsius.
 */
static inline int mcp9808_temp_signed_from_reg(uint16_t reg)
{
	int rv = reg & MCP9808_TEMP_ABS_MASK;

	if (reg & MCP9808_TEMP_SIGN_BIT) {
		/* Convert 12-bit 2s complement to signed negative
		 * value.
		 */
		rv = -(1U + (rv ^ MCP9808_TEMP_ABS_MASK));
	}
	return rv;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_MICROCHIP_MCP9808_H_ */