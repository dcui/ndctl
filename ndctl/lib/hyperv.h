/*
 * Copyright (c) 2019, Microsoft Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 */
#ifndef __NDCTL_HYPERV_H__
#define __NDCTL_HYPERV_H__

/* See http://www.uefi.org/RFIC_LIST ("Virtual NVDIMM 0x1901") */
enum {
	ND_HYPERV_CMD_QUERY = 0,

	/* non-root commands */
	ND_HYPERV_CMD_GET_HEALTH_INFO = 1,
	ND_HYPERV_CMD_GET_SHUTDOWN_INFO = 2,
};

/*
 * This is actually Function 1's data,
 * This is the closest I can find to match the "smart".
 * Hyper-V _DSM methods don't have a smart function.
 */
struct nd_hyperv_smart_data {
	__u32	health;
} __attribute__((packed));

struct nd_hyperv_smart {
	__u32	status;
	union {
		__u8 buf[4];
		struct nd_hyperv_smart_data data[0];
	};
} __attribute__((packed));

struct nd_hyperv_shutdown_info {
	 __u32   status;
	 __u32   count;
} __attribute__((packed));

union nd_hyperv_cmd {
	__u32			status;
	struct nd_hyperv_smart	smart;
	struct nd_hyperv_shutdown_info shutdown_info;
} __attribute__((packed));

struct nd_pkg_hyperv {
	struct nd_cmd_pkg	gen;
	union nd_hyperv_cmd	u;
} __attribute__((packed));

#endif /* __NDCTL_HYPERV_H__ */
