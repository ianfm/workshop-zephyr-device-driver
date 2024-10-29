#define DT_DRV_COMPAT microchip_mcp9808

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "mcp9808.h"

LOG_MODULE_REGISTER(MCP9808, CONFIG_SENSOR_LOG_LEVEL);

//------------------------------------------------------------------------------
// Forward declarations

static int mcp9808_reg_read(const struct device *dev,
							uint8_t reg,
							uint16_t *val);
static int mcp9808_reg_write_8bit(const struct device *dev,
								  uint8_t reg,
								  uint8_t val);
static int mcp9808_init(const struct device *dev);
static int mcp9808_sample_fetch(const struct device *dev,
								enum sensor_channel chan);
static int mcp9808_channel_get(const struct device *dev,
							   enum sensor_channel chan,
							   struct sensor_value *val);

//------------------------------------------------------------------------------
// Private functions

static int mcp9808_reg_read(const struct device *dev, 
							uint8_t reg, 
							uint16_t *val)
{
	const struct mcp9808_config *cfg = dev->config;
	int rc = i2c_write_read_dt(&cfg->i2c, &reg, sizeof(reg), val, sizeof(*val));

	if (rc == 0) {
		*val = sys_be16_to_cpu(*val);
	}

	return rc;
}

static int mcp9808_reg_write_8bit(const struct device *dev, 
								  uint8_t reg, 
								  uint8_t val)
{
	const struct mcp9808_config *cfg = dev->config;
	uint8_t buf[2] = {
		reg,
		val,
	};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int mcp9808_init(const struct device *dev)
{
	const struct mcp9808_config *cfg = dev->config;
	int rc = 0;

	LOG_DBG("Initializing");

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	// Set temperature resolution
	rc = mcp9808_reg_write_8bit(dev, MCP9808_REG_RESOLUTION, cfg->resolution);
	LOG_DBG("Setting resolution to index %d", cfg->resolution);
	if (rc) {
		LOG_ERR("Could not set the resolution of mcp9808 module");
		return rc;
	}

	return rc;
}

//------------------------------------------------------------------------------
// Public functions

static int mcp9808_sample_fetch(const struct device *dev, 
								enum sensor_channel chan)
{
	struct mcp9808_data *data = dev->data;

	// Check if the channel is supported
	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	return mcp9808_reg_read(dev, MCP9808_REG_TEMP_AMB, &data->reg_val);
}

static int mcp9808_channel_get(const struct device *dev, 
							   enum sensor_channel chan,
			    			   struct sensor_value *val)
{
	const struct mcp9808_data *data = dev->data;

	// Convert the 12-bit two's complement to a signed integer value
	int temp = data->reg_val & MCP9808_TEMP_ABS_MASK;
	if (data->reg_val & MCP9808_TEMP_SIGN_BIT) {
		temp = -(1U + (temp ^ MCP9808_TEMP_ABS_MASK));
	}

	// Check if the channel is supported
	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	// Store the value as integer (val1) and millionths (val2)
	val->val1 = temp / MCP9808_TEMP_SCALE_CEL;
	temp -= val->val1 * MCP9808_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000) / MCP9808_TEMP_SCALE_CEL;

	return 0;
}

//------------------------------------------------------------------------------
// Devicetree handling

// Define the public API functions for the driver
static const struct sensor_driver_api mcp9808_api_funcs = {
	.sample_fetch = mcp9808_sample_fetch,
	.channel_get = mcp9808_channel_get,
};

// Expansion macro to define the driver instances
#define MCP9808_DEFINE(inst)                                                                          \
	static struct mcp9808_data mcp9808_data_##inst;                                                  \
                                                                                                   \
	static const struct mcp9808_config mcp9808_config_##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.resolution = DT_INST_PROP(inst, resolution),                                      \
	};        																		\
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, mcp9808_init, NULL, &mcp9808_data_##inst,                     \
				     &mcp9808_config_##inst, POST_KERNEL,                             \
				     CONFIG_SENSOR_INIT_PRIORITY, &mcp9808_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(MCP9808_DEFINE)