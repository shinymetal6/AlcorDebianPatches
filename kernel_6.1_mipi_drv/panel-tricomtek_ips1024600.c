// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024, Stonehex
 *
 * This file based on panel-ronbo-rb070d30.c
 */

#include <linux/backlight.h>

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/media-bus-format.h>
#include <linux/module.h>
#include <linux/of.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_connector.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

//#define	TRICOMTEK_DEBUG_PRINTK	1

struct tt_ips1024600panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator *supply;
        struct backlight_device *backlight;

	struct {
		struct gpio_desc *reset;
#ifdef	INCLUDE_STATUS_PIN
		struct gpio_desc *power;
		struct gpio_desc *updn;
		struct gpio_desc *shlr;
#endif
	} gpios;
        bool prepared;
        bool enabled;

};

static inline struct tt_ips1024600panel *panel_to_tt_ips1024600panel(struct drm_panel *panel)
{
	return container_of(panel, struct tt_ips1024600panel, panel);
}

static int tt_ips1024600panel_prepare(struct drm_panel *mipi_panel)
{
	struct tt_ips1024600panel *panel = panel_to_tt_ips1024600panel(mipi_panel);
	int ret;

	ret = regulator_enable(panel->supply);
	if (ret < 0) {
		printk(KERN_INFO "**** FAILED POWER SUPPLY **** %s\n", __FUNCTION__);
		dev_err(&panel->dsi->dev, "Failed to enable supply: %d\n", ret);
		return ret;
	}

	msleep(20);
#ifdef	INCLUDE_STATUS_PIN
	gpiod_set_value(panel->gpios.power, 1);
	msleep(20);
#endif
	gpiod_set_value(panel->gpios.reset, 1);
	msleep(20);
        panel->prepared = true;

	return 0;
}

static int tt_ips1024600panel_unprepare(struct drm_panel *mipi_panel)
{
	struct tt_ips1024600panel *panel = panel_to_tt_ips1024600panel(mipi_panel);

	gpiod_set_value(panel->gpios.reset, 0);
#ifdef	INCLUDE_STATUS_PIN
	gpiod_set_value(panel->gpios.power, 0);
#endif
	regulator_disable(panel->supply);

	return 0;
}

static int tt_ips1024600panel_enable(struct drm_panel *mipi_panel)
{
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** called **** %s\n", __FUNCTION__);
#endif
	struct tt_ips1024600panel *panel = panel_to_tt_ips1024600panel(mipi_panel);
        backlight_enable(panel->backlight);

#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** backlight_enable called **** %s\n", __FUNCTION__);
#endif
	return mipi_dsi_dcs_exit_sleep_mode(panel->dsi);
}

static int tt_ips1024600panel_disable(struct drm_panel *mipi_panel)
{
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** called **** %s\n", __FUNCTION__);
#endif
	struct tt_ips1024600panel *panel = panel_to_tt_ips1024600panel(mipi_panel);
        backlight_disable(panel->backlight);
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** backlight_disable called **** %s\n", __FUNCTION__);
#endif

	return mipi_dsi_dcs_enter_sleep_mode(panel->dsi);
}

/* Default timings */
static const struct drm_display_mode default_mode = {
	.clock		= 51206,
	.hdisplay	= 1024,
	.hsync_start	= 1024 + 160,
	.hsync_end	= 1024 + 160 + 80,
	.htotal		= 1024 + 160 + 80 + 80,
	.vdisplay	= 600,
	.vsync_start	= 600 + 12,
	.vsync_end	= 600 + 12 + 10,
	.vtotal		= 600 + 12 + 10 + 13,

	.width_mm	= 154,
	.height_mm	= 85,
};

static int tt_ips1024600panel_get_modes(struct drm_panel *mipi_panel, struct drm_connector *connector)
{

	struct tt_ips1024600panel *panel = panel_to_tt_ips1024600panel(mipi_panel);
	struct drm_display_mode *mode;
	static const u32 bus_format = MEDIA_BUS_FMT_RGB888_1X24;


	mode = drm_mode_duplicate(connector->dev, &default_mode);
	if (!mode) {
		dev_err(&panel->dsi->dev, "Failed to add mode " DRM_MODE_FMT "\n",
			DRM_MODE_ARG(&default_mode));
		return -EINVAL;
	}

	drm_mode_set_name(mode);
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** drm_mode_set_name called **** %s\n", __FUNCTION__);
#endif
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** drm_mode_probed_add called **** %s\n", __FUNCTION__);
#endif

	connector->display_info.bpc = 8;
	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	drm_display_info_set_bus_formats(&connector->display_info,
					 &bus_format, 1);
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** tt_ips1024600panel_get_modes finished **** %s\n", __FUNCTION__);
#endif
	return 1;
}

static int tt_ips1024600panel_bl_update_status(struct backlight_device *bl)
{       
struct mipi_dsi_device *dsi = bl_get_data(bl);
struct tt_ips1024600panel *ips1024600panel = mipi_dsi_get_drvdata(dsi);
int ret = 0;
        
	if (!ips1024600panel->prepared)
		return 0;
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** prepared **** %s\n", __FUNCTION__);
#endif
	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, bl->props.brightness);
	if (ret < 0)
		return ret;

#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** success **** %s\n", __FUNCTION__);
#endif
	return 0;
}

static const struct backlight_ops tt_ips1024600panel_bl_ops = {
        .update_status = tt_ips1024600panel_bl_update_status,
};

static const struct drm_panel_funcs tt_ips1024600panel_funcs = {
	.get_modes	= tt_ips1024600panel_get_modes,
	.prepare	= tt_ips1024600panel_prepare,
	.enable		= tt_ips1024600panel_enable,
	.disable	= tt_ips1024600panel_disable,
	.unprepare	= tt_ips1024600panel_unprepare,
};

static int tt_ips1024600panel_dsi_probe(struct mipi_dsi_device *dsi)
{
struct device *dev = &dsi->dev;
struct backlight_properties bl_props;

struct tt_ips1024600panel *panel;
int ret;

#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** tt_ips1024600panel_dsi_probe called **** %s\n", __FUNCTION__);
#endif
	panel = devm_kzalloc(&dsi->dev, sizeof(*panel), GFP_KERNEL);
	if (!panel)
	return -ENOMEM;

	memset(&bl_props, 0, sizeof(bl_props));
	bl_props.type = BACKLIGHT_RAW;
	bl_props.brightness = 224;
	bl_props.max_brightness = 255;

	panel->backlight = devm_of_find_backlight(dev);
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** devm_backlight_device_register called **** %s\n", __FUNCTION__);
#endif
	if (IS_ERR(panel->backlight))
	{
#ifdef	TRICOMTEK_DEBUG_PRINTK
		printk(KERN_INFO "**** devm_backlight_device_register failed **** %s\n", __FUNCTION__);
#endif
		return PTR_ERR(panel->backlight);
	}
	printk(KERN_INFO "**** devm_backlight_device_register success **** %s\n", __FUNCTION__);

	panel->supply = devm_regulator_get(&dsi->dev, "vcc-lcd");
	if (IS_ERR(panel->supply))
		return PTR_ERR(panel->supply);
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** regulator is ok **** @ %s\n", __FUNCTION__);
#endif
	mipi_dsi_set_drvdata(dsi, panel);
	panel->dsi = dsi;

	drm_panel_init(&panel->panel, &dsi->dev, &tt_ips1024600panel_funcs,
		       DRM_MODE_CONNECTOR_DSI);

	panel->gpios.reset = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(panel->gpios.reset)) {
		dev_err(&dsi->dev, "Couldn't get our reset GPIO\n");
		return PTR_ERR(panel->gpios.reset);
	}
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** reset pin is ok **** @ %s\n", __FUNCTION__);
#endif
#ifdef	INCLUDE_STATUS_PIN
	panel->gpios.power = devm_gpiod_get(&dsi->dev, "power", GPIOD_OUT_LOW);
	if (IS_ERR(panel->gpios.power)) {
		dev_err(&dsi->dev, "Couldn't get our power GPIO\n");
		return PTR_ERR(panel->gpios.power);
	}

	/*
	 * We don't change the state of that GPIO later on but we need
	 * to force it into a low state.
	 */
	panel->gpios.updn = devm_gpiod_get(&dsi->dev, "updn", GPIOD_OUT_LOW);
	if (IS_ERR(panel->gpios.updn)) {
		dev_err(&dsi->dev, "Couldn't get our updn GPIO\n");
		return PTR_ERR(panel->gpios.updn);
	}

	/*
	 * We don't change the state of that GPIO later on but we need
	 * to force it into a low state.
	 */
	panel->gpios.shlr = devm_gpiod_get(&dsi->dev, "shlr", GPIOD_OUT_LOW);
	if (IS_ERR(panel->gpios.shlr)) {
		dev_err(&dsi->dev, "Couldn't get our shlr GPIO\n");
		return PTR_ERR(panel->gpios.shlr);
	}
#endif

	//ret = drm_panel_of_backlight(&panel->panel);
	//if (ret)
		//return ret;

	drm_panel_add(&panel->panel);

	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST | MIPI_DSI_MODE_LPM;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->lanes = 4;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&panel->panel);
		return ret;
	}
#ifdef	TRICOMTEK_DEBUG_PRINTK
	printk(KERN_INFO "**** success **** @ %s\n", __FUNCTION__);
#endif
	return 0;
}

static void tt_ips1024600panel_dsi_remove(struct mipi_dsi_device *dsi)
{
	struct tt_ips1024600panel *panel = mipi_dsi_get_drvdata(dsi);

	mipi_dsi_detach(dsi);
	drm_panel_remove(&panel->panel);
}

static const struct of_device_id tt_ips1024600panel_of_match[] = {
	{ .compatible = "tricomtek,ips1024600" },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, tt_ips1024600panel_of_match);

static struct mipi_dsi_driver tt_ips1024600panel_driver = {
	.probe = tt_ips1024600panel_dsi_probe,
	.remove = tt_ips1024600panel_dsi_remove,
	.driver = {
		.name = "panel-tricomtek-ips1024600",
		.of_match_table	= tt_ips1024600panel_of_match,
	},
};
module_mipi_dsi_driver(tt_ips1024600panel_driver);

MODULE_AUTHOR("Filippo Visocchi <f.visocchi#stonehex.com>");
MODULE_DESCRIPTION("TricomTek ips1024600 Panel Driver");
MODULE_LICENSE("GPL");
