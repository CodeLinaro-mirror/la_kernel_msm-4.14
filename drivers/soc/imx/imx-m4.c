// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2026 NXP.
 */

#include <linux/arm-smccc.h>
#include <linux/module.h>
#include <soc/imx/src.h>
#include <soc/imx/soc.h>

#define FSL_SIP_SRC                    0xc2000005
#define FSL_SIP_SRC_M4_START           0x00
#define FSL_SIP_SRC_M4_STARTED         0x01

static bool m4_is_enabled;

bool imx_src_is_m4_enabled(void)
{
	return m4_is_enabled;
}
EXPORT_SYMBOL_GPL(imx_src_is_m4_enabled);

int check_m4_enabled(void)
{
	struct arm_smccc_res res;

	arm_smccc_smc(FSL_SIP_SRC, FSL_SIP_SRC_M4_STARTED, 0,
			0, 0, 0, 0, 0, &res);
	m4_is_enabled = !!res.a0;

	if (m4_is_enabled)
		pr_info("M4 is started\n");

	return 0;
}
EXPORT_SYMBOL_GPL(check_m4_enabled);

MODULE_DESCRIPTION("i.MX M4 core helper");
MODULE_LICENSE("GPL");
