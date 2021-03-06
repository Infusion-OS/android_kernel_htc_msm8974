#ifndef __LINUX_MFD_MSM_ADIE_CODEC_H
#define __LINUX_MFD_MSM_ADIE_CODEC_H

#include <linux/types.h>

#define ADIE_CODEC_ACTION_ENTRY       0x1
#define ADIE_CODEC_ACTION_DELAY_WAIT      0x2
#define ADIE_CODEC_ACTION_STAGE_REACHED   0x3

#define ADIE_CODEC_PATH_OFF                                        0x0050

#define ADIE_CODEC_DIGITAL_READY                                   0x0100

#define ADIE_CODEC_DIGITAL_ANALOG_READY                            0x1000


#define ADIE_CODEC_ANALOG_OFF                                      0x0750


#define ADIE_CODEC_DIGITAL_OFF                                     0x0600

#define ADIE_CODEC_FLASH_IMAGE 					   0x0001

#define ADIE_CODEC_RX 0
#define ADIE_CODEC_TX 1
#define ADIE_CODEC_LB 3
#define ADIE_CODEC_MAX 4

#define ADIE_CODEC_PACK_ENTRY(reg, mask, val) ((val)|(mask << 8)|(reg << 16))

#define ADIE_CODEC_UNPACK_ENTRY(packed, reg, mask, val) \
	do { \
		((reg) = ((packed >> 16) & (0xff))); \
		((mask) = ((packed >> 8) & (0xff))); \
		((val) = ((packed) & (0xff))); \
	} while (0);

struct adie_codec_action_unit {
	u32 type;
	u32 action;
};

struct adie_codec_hwsetting_entry{
	struct adie_codec_action_unit *actions;
	u32 action_sz;
	u32 freq_plan;
	u32 osr;
};

struct adie_codec_dev_profile {
	u32 path_type; 
	u32 setting_sz;
	struct adie_codec_hwsetting_entry *settings;
};

struct adie_codec_register {
	u8 reg;
	u8 mask;
	u8 val;
};

struct adie_codec_register_image {
	struct adie_codec_register *regs;
	u32 img_sz;
};

struct adie_codec_path;

struct adie_codec_anc_data {
	u32 size;
	u32 writes[];
};

struct adie_codec_operations {
	int	 codec_id;
	int (*codec_open) (struct adie_codec_dev_profile *profile,
				struct adie_codec_path **path_pptr);
	int (*codec_close) (struct adie_codec_path *path_ptr);
	int (*codec_setpath) (struct adie_codec_path *path_ptr,
				u32 freq_plan, u32 osr);
	int (*codec_proceed_stage) (struct adie_codec_path *path_ptr,
					u32 state);
	u32 (*codec_freq_supported) (struct adie_codec_dev_profile *profile,
					u32 requested_freq);
	int (*codec_enable_sidetone) (struct adie_codec_path *rx_path_ptr,
					u32 enable);
	int (*codec_enable_anc) (struct adie_codec_path *rx_path_ptr,
		u32 enable, struct adie_codec_anc_data *calibration_writes);
	int (*codec_set_device_digital_volume) (
					struct adie_codec_path *path_ptr,
					u32 num_channels,
					u32 vol_percentage);

	int (*codec_set_device_analog_volume) (struct adie_codec_path *path_ptr,
						u32 num_channels,
						u32 volume);
	int (*codec_set_master_mode) (struct adie_codec_path *path_ptr,
					u8 master);
};

int adie_codec_register_codec_operations(
				const struct adie_codec_operations *codec_ops);
int adie_codec_open(struct adie_codec_dev_profile *profile,
	struct adie_codec_path **path_pptr);
int adie_codec_setpath(struct adie_codec_path *path_ptr,
	u32 freq_plan, u32 osr);
int adie_codec_proceed_stage(struct adie_codec_path *path_ptr, u32 state);
int adie_codec_close(struct adie_codec_path *path_ptr);
u32 adie_codec_freq_supported(struct adie_codec_dev_profile *profile,
							u32 requested_freq);
int adie_codec_enable_sidetone(struct adie_codec_path *rx_path_ptr, u32 enable);
int adie_codec_enable_anc(struct adie_codec_path *rx_path_ptr, u32 enable,
	struct adie_codec_anc_data *calibration_writes);
int adie_codec_set_device_digital_volume(struct adie_codec_path *path_ptr,
		u32 num_channels, u32 vol_percentage );

int adie_codec_set_device_analog_volume(struct adie_codec_path *path_ptr,
		u32 num_channels, u32 volume );

int adie_codec_set_master_mode(struct adie_codec_path *path_ptr, u8 master);
#endif
