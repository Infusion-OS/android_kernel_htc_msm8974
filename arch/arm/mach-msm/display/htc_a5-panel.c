#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <asm/mach-types.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/debug_display.h>
#include "../../../../drivers/video/msm/mdss/mdss_dsi.h"

#define PANEL_ID_A5_SHARP_HX      0
#define PANEL_ID_A5_JDI_NT35521_C3      1
struct dsi_power_data {
	uint32_t sysrev;
	struct regulator *vddio;
	struct regulator *vddpll;
	struct regulator *vdda;

	int lcmp5v;
	int lcmn5v;
	int lcm_bl_en;
};

#ifdef MODULE
extern struct module __this_module;
#define THIS_MODULE (&__this_module)
#else
#define THIS_MODULE ((struct module *)0)
#endif

static struct i2c_adapter	*i2c_bus_adapter = NULL;

struct i2c_dev_info {
	uint8_t				dev_addr;
	struct i2c_client	*client;
};

#define I2C_DEV_INFO(addr) \
	{.dev_addr = addr >> 1, .client = NULL}

static struct i2c_dev_info device_addresses[] = {
	I2C_DEV_INFO(0x7C)
};

static inline int platform_write_i2c_block(struct i2c_adapter *i2c_bus
								, u8 page
								, u8 offset
								, u16 count
								, u8 *values
								)
{
	struct i2c_msg msg;
	u8 *buffer;
	int ret;

	buffer = kmalloc(count + 1, GFP_KERNEL);
	if (!buffer) {
		printk("%s:%d buffer allocation failed\n",__FUNCTION__,__LINE__);
		return -ENOMEM;
	}

	buffer[0] = offset;
	memmove(&buffer[1], values, count);

	msg.flags = 0;
	msg.addr = page >> 1;
	msg.buf = buffer;
	msg.len = count + 1;

	ret = i2c_transfer(i2c_bus, &msg, 1);

	kfree(buffer);

	if (ret != 1) {
		printk("%s:%d I2c write failed 0x%02x:0x%02x\n"
				,__FUNCTION__,__LINE__, page, offset);
		ret = -EIO;
	} else {
		ret = 0;
	}

	return ret;
}


static int tps_65132_add_i2c(struct i2c_client *client)
{
	struct i2c_adapter *adapter = client->adapter;
	int idx;

	i2c_bus_adapter = adapter;
	if (i2c_bus_adapter == NULL) {
		PR_DISP_ERR("%s() failed to get i2c adapter\n", __func__);
		return ENODEV;
	}

	for (idx = 0; idx < ARRAY_SIZE(device_addresses); idx++) {
		if(idx == 0)
			device_addresses[idx].client = client;
		else {
			device_addresses[idx].client = i2c_new_dummy(i2c_bus_adapter,
											device_addresses[idx].dev_addr);
			if (device_addresses[idx].client == NULL){
				return ENODEV;
			}
		}
	}

	return 0;
}


static int __devinit tps_65132_tx_i2c_probe(struct i2c_client *client,
					      const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		PR_DISP_ERR("%s: Failed to i2c_check_functionality \n", __func__);
		return -EIO;
	}


	if (!client->dev.of_node) {
		PR_DISP_ERR("%s: client->dev.of_node = NULL\n", __func__);
		return -ENOMEM;
	}

	ret = tps_65132_add_i2c(client);

	if(ret < 0) {
		PR_DISP_ERR("%s: Failed to tps_65132_add_i2c, ret=%d\n", __func__,ret);
		return ret;
	}

	return 0;
}


static const struct i2c_device_id tps_65132_tx_id[] = {
	{"tps65132", 0}
};

static struct of_device_id TSP_match_table[] = {
	{.compatible = "disp-tps-65132",}
};

static struct i2c_driver tps_65132_tx_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "tps65132",
		.of_match_table = TSP_match_table,
		},
	.id_table = tps_65132_tx_id,
	.probe = tps_65132_tx_i2c_probe,
	.command = NULL,
};

static int htc_a5_regulator_init(struct platform_device *pdev)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct dsi_power_data *pwrdata = NULL;

	PR_DISP_INFO("%s\n", __func__);
	if (!pdev) {
		PR_DISP_ERR("%s: invalid input\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = platform_get_drvdata(pdev);
	if (!ctrl_pdata) {
		PR_DISP_ERR("%s: invalid driver data\n", __func__);
		return -EINVAL;
	}

	pwrdata = devm_kzalloc(&pdev->dev,
				sizeof(struct dsi_power_data), GFP_KERNEL);
	if (!pwrdata) {
		PR_DISP_ERR("%s: FAILED to alloc pwrdata\n", __func__);
		return -ENOMEM;
	}

	ctrl_pdata->dsi_pwrctrl_data = pwrdata;

	pwrdata->vddio = devm_regulator_get(&pdev->dev, "vddio");
	if (IS_ERR(pwrdata->vddio)) {
		PR_DISP_ERR("%s: could not get vddio reg, rc=%ld\n",
			__func__, PTR_ERR(pwrdata->vddio));
		return PTR_ERR(pwrdata->vddio);
	}

	pwrdata->vddpll = devm_regulator_get(&pdev->dev, "vddpll");
	if (IS_ERR(pwrdata->vddpll)) {
		PR_DISP_ERR("%s: could not get vddpll reg, rc=%ld\n",
			__func__, PTR_ERR(pwrdata->vddpll));
		return PTR_ERR(pwrdata->vddpll);
	}

	pwrdata->vdda = devm_regulator_get(&pdev->dev, "vdda");
	if (IS_ERR(pwrdata->vdda)) {
		PR_DISP_ERR("%s: could not get vdda vreg, rc=%ld\n",
			__func__, PTR_ERR(pwrdata->vdda));
		return PTR_ERR(pwrdata->vdda);
	}

	ret = regulator_set_voltage(pwrdata->vdda, 1200000,
		1200000);
	if (ret) {
		PR_DISP_ERR("%s: set voltage failed on vdda vreg, rc=%d\n",
			__func__, ret);
		return ret;
	}

	pwrdata->lcmp5v = of_get_named_gpio(pdev->dev.of_node,
						"htc,lcm_p5v-gpio", 0);
	pwrdata->lcmn5v = of_get_named_gpio(pdev->dev.of_node,
						"htc,lcm_n5v-gpio", 0);
	pwrdata->lcm_bl_en = of_get_named_gpio(pdev->dev.of_node,
						"htc,lcm_bl_en-gpio", 0);

	ret = i2c_add_driver(&tps_65132_tx_i2c_driver);
	if (ret < 0) {
		PR_DISP_ERR("%s: FAILED to add i2c_add_driver ret=%x\n",
			__func__, ret);
	}
	return 0;
}

static int htc_a5_regulator_deinit(struct platform_device *pdev)
{
	return 0;
}

void htc_a5_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		PR_DISP_ERR("%s: Invalid input data\n", __func__);
		return;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		PR_DISP_DEBUG("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return;
	}

	PR_DISP_DEBUG("%s: enable = %d\n", __func__, enable);

	if (enable) {
		if (pdata->panel_info.first_power_on == 1) {
			PR_DISP_INFO("reset already on in first time\n");
			return;
		}

		if (pdata->panel_info.panel_id == PANEL_ID_A5_SHARP_HX){
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			usleep_range(1000,1500);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			usleep_range(1000,1500);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			msleep(25);
		} else {
			msleep(40);

			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			usleep_range(1000,1500);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			usleep_range(1000,1500);
			gpio_set_value((ctrl_pdata->rst_gpio), 1);
			usleep_range(20000,20500);
		}

	} else {
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
	}

}

static int htc_a5_panel_power_on(struct mdss_panel_data *pdata, int enable)
{
	int ret;

	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct dsi_power_data *pwrdata = NULL;
	u8 avdd_level = 0x00;


	PR_DISP_INFO("%s: en=%d\n", __func__, enable);
	if (pdata == NULL) {
		PR_DISP_ERR("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);
	pwrdata = ctrl_pdata->dsi_pwrctrl_data;

	if (!pwrdata) {
		PR_DISP_ERR("%s: pwrdata not initialized\n", __func__);
		return -EINVAL;
	}

	if (enable) {
		if (pdata->panel_info.panel_id == PANEL_ID_A5_SHARP_HX){
			ret = regulator_set_optimum_mode(pwrdata->vddio, 100000);
			if (ret < 0) {
				PR_DISP_ERR("%s: vddio set opt mode failed.\n",
					__func__);
				return ret;
			}

			ret = regulator_set_optimum_mode(pwrdata->vddpll, 100000);
			if (ret < 0) {
				PR_DISP_ERR("%s: vddpll set opt mode failed.\n",
					__func__);
				return ret;
			}

			ret = regulator_set_optimum_mode(pwrdata->vdda, 100000);
			if (ret < 0) {
				PR_DISP_ERR("%s: vdda set opt mode failed.\n",
					__func__);
				return ret;
			}

			ret = regulator_enable(pwrdata->vddio);
			if (ret) {
				PR_DISP_ERR("%s: Failed to enable regulator.\n",
					__func__);
				return ret;
			}

			usleep_range(1000,1500);

			gpio_set_value(pwrdata->lcmp5v, 1);
			avdd_level = 0x00;
			platform_write_i2c_block(i2c_bus_adapter,0x7C,0x00, 0x01, &avdd_level);
			platform_write_i2c_block(i2c_bus_adapter,0x7C,0x01, 0x01, &avdd_level);
			usleep_range(12000,12500);
			gpio_set_value(pwrdata->lcmn5v, 1);
			usleep_range(12000,12500);

			ret = regulator_enable(pwrdata->vddpll);
			if (ret) {
				PR_DISP_ERR("%s: Failed to enable regulator.\n",
					__func__);
				return ret;
			}

			ret = regulator_enable(pwrdata->vdda);
			if (ret) {
				PR_DISP_ERR("%s: Failed to enable regulator.\n",
					__func__);
				return ret;
			}

		} else {
			ret = regulator_set_optimum_mode(pwrdata->vddio, 100000);
			if (ret < 0) {
				PR_DISP_ERR("%s: vddio set opt mode failed.\n",
					__func__);
				return ret;
			}

			ret = regulator_set_optimum_mode(pwrdata->vddpll, 100000);
			if (ret < 0) {
				PR_DISP_ERR("%s: vddpll set opt mode failed.\n",
					__func__);
				return ret;
			}


			ret = regulator_set_optimum_mode(pwrdata->vdda, 100000);
			if (ret < 0) {
				PR_DISP_ERR("%s: vdda set opt mode failed.\n",
					__func__);
				return ret;
			}

			ret = regulator_enable(pwrdata->vddio);
			if (ret) {
				PR_DISP_ERR("%s: Failed to enable regulator.\n",
					__func__);
				return ret;
			}

			usleep_range(1000,1500);

			gpio_set_value(pwrdata->lcmp5v, 1);
			avdd_level = 0x0F;
			platform_write_i2c_block(i2c_bus_adapter,0x7C,0x00, 0x01, &avdd_level);
			platform_write_i2c_block(i2c_bus_adapter,0x7C,0x01, 0x01, &avdd_level);
			usleep_range(2000,2500);
			gpio_set_value(pwrdata->lcmn5v, 1);
			usleep_range(2000,2500);

			ret = regulator_enable(pwrdata->vddpll);
			if (ret) {
				PR_DISP_ERR("%s: Failed to enable regulator.\n",
					__func__);
				return ret;
			}

			ret = regulator_enable(pwrdata->vdda);
			if (ret) {
				PR_DISP_ERR("%s: Failed to enable regulator.\n",
					__func__);
				return ret;
			}
		}

		gpio_set_value(pwrdata->lcm_bl_en, 1);
	} else {

		gpio_set_value(pwrdata->lcm_bl_en, 0);
		usleep_range(2000,2500);
		htc_a5_panel_reset(pdata, 0);

		ret = regulator_disable(pwrdata->vdda);
		if (ret) {
			PR_DISP_ERR("%s: Failed to disable regulator.\n",
				__func__);
			return ret;
		}

		ret = regulator_disable(pwrdata->vddpll);
		if (ret) {
			PR_DISP_ERR("%s: Failed to disable regulator.\n",
				__func__);
			return ret;
		}

		usleep_range(2000,2500);
		gpio_set_value(pwrdata->lcmn5v, 0);
		usleep_range(2000,2500);
		gpio_set_value(pwrdata->lcmp5v, 0);
		usleep_range(2000,2500);

		ret = regulator_disable(pwrdata->vddio);
		if (ret) {
			PR_DISP_ERR("%s: Failed to disable regulator.\n",
				__func__);
			return ret;
		}

		ret = regulator_set_optimum_mode(pwrdata->vddio, 100);
		if (ret < 0) {
			PR_DISP_ERR("%s: vdd_io_vreg set opt mode failed.\n",
				__func__);
			return ret;
		}

		ret = regulator_set_optimum_mode(pwrdata->vddpll, 100);
		if (ret < 0) {
			PR_DISP_ERR("%s: vdd_io_vreg set opt mode failed.\n",
				__func__);
			return ret;
		}

		ret = regulator_set_optimum_mode(pwrdata->vdda, 100);
		if (ret < 0) {
			PR_DISP_ERR("%s: vdda_vreg set opt mode failed.\n",
				__func__);
			return ret;
		}

	}
	PR_DISP_INFO("%s: en=%d done\n", __func__, enable);

	return 0;
}

static struct mdss_dsi_pwrctrl dsi_pwrctrl = {
	.dsi_regulator_init = htc_a5_regulator_init,
	.dsi_regulator_deinit = htc_a5_regulator_deinit,
	.dsi_power_on = htc_a5_panel_power_on,
	.dsi_panel_reset = htc_a5_panel_reset,
};

static struct platform_device dsi_pwrctrl_device = {
	.name          = "mdss_dsi_pwrctrl",
	.id            = -1,
	.dev.platform_data = &dsi_pwrctrl,
};

int __init htc_8226_dsi_panel_power_register(void)
{
	PR_DISP_INFO("%s#%d\n", __func__, __LINE__);
	platform_device_register(&dsi_pwrctrl_device);
	return 0;
}
