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
// Private functions

int mcp9808_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct mcp9808_config *cfg = dev->config;
	int rc = i2c_write_read_dt(&cfg->i2c, &reg, sizeof(reg), val, sizeof(*val));

	if (rc == 0) {
		*val = sys_be16_to_cpu(*val);
	}

	return rc;
}

int mcp9808_reg_write_16bit(const struct device *dev, uint8_t reg, uint16_t val)
{
	const struct mcp9808_config *cfg = dev->config;

	uint8_t buf[3];

	buf[0] = reg;
	sys_put_be16(val, &buf[1]);

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

int mcp9808_reg_write_8bit(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct mcp9808_config *cfg = dev->config;
	uint8_t buf[2] = {
		reg,
		val,
	};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int mcp9808_set_temperature_resolution(const struct device *dev, uint8_t resolution)
{
	return mcp9808_reg_write_8bit(dev, MCP9808_REG_RESOLUTION, resolution);
}

int mcp9808_init(const struct device *dev)
{
	const struct mcp9808_config *cfg = dev->config;
	int rc = 0;

	LOG_DBG("Initializing");

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	rc = mcp9808_set_temperature_resolution(dev, cfg->resolution);
	LOG_DBG("Setting resolution to index %d", cfg->resolution);
	if (rc) {
		LOG_ERR("Could not set the resolution of mcp9808 module");
		return rc;
	}

	return rc;
}

//------------------------------------------------------------------------------
// Public functions

static int mcp9808_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mcp9808_data *data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	return mcp9808_reg_read(dev, MCP9808_REG_TEMP_AMB, &data->reg_val);
}

static int mcp9808_channel_get(const struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	const struct mcp9808_data *data = dev->data;
	int temp = mcp9808_temp_signed_from_reg(data->reg_val);

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	val->val1 = temp / MCP9808_TEMP_SCALE_CEL;
	temp -= val->val1 * MCP9808_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000) / MCP9808_TEMP_SCALE_CEL;

	return 0;
}

//------------------------------------------------------------------------------
// Devicetree handling

// Define the API functions for the driver
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