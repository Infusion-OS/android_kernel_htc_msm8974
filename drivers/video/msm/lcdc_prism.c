/* Copyright (c) 2009-2010, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "msm_fb.h"

#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
#include "mddihosti.h"
#endif

static int __init lcdc_prism_init(void)
{
	int ret;
	struct msm_panel_info pinfo;

#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
	ret = msm_fb_detect_client("lcdc_prism_wvga");
	if (ret == -ENODEV)
		return 0;

	if (ret && (mddi_get_client_id() != 0))
		return 0;
#endif

	pinfo.xres = 800;
	pinfo.yres = 480;
	MSM_FB_SINGLE_MODE_PANEL(&pinfo);
	pinfo.type = LCDC_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.fb_num = 2;
	pinfo.clk_rate = 30720000;

	pinfo.lcdc.h_back_porch = 21;
	pinfo.lcdc.h_front_porch = 81;
	pinfo.lcdc.h_pulse_width = 60;
	pinfo.lcdc.v_back_porch = 18;
	pinfo.lcdc.v_front_porch = 27;
	pinfo.lcdc.v_pulse_width = 2;
	pinfo.lcdc.border_clr = 0;	
	pinfo.lcdc.underflow_clr = 0xff;	
	pinfo.lcdc.hsync_skew = 0;

	ret = lcdc_device_register(&pinfo);
	if (ret)
		printk(KERN_ERR "%s: failed to register device!\n", __func__);

	return ret;
}

module_init(lcdc_prism_init);
