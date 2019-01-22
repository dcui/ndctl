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

enum {
	ND_HYPERV_CMD_QUERY = 0,

	/* non-root commands */
	ND_HYPERV_CMD_SMART = 1,	/* Get Health Information */
};

/* ND_HYPERV_CMD_SMART */
#define ND_HYPERV_SMART_HEALTH_VALID	ND_SMART_HEALTH_VALID

/*
 * This is actually function 1 data,
 * This is the closest I can find to match smart.
 * Hyper-V _DSM does not have smart function.
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

union nd_hyperv_cmd {
	__u32			query;
	struct nd_hyperv_smart	smart;
} __attribute__((packed));

struct nd_pkg_hyperv {
	struct nd_cmd_pkg	gen;
	union nd_hyperv_cmd	u;
} __attribute__((packed));

#endif /* __NDCTL_HYPERV_H__ */
