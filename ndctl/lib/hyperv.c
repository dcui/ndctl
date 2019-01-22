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
#include <stdlib.h>
#include <limits.h>
#include <util/bitmap.h>
#include <util/log.h>
#include <ndctl/libndctl.h>
#include "private.h"
#include "hyperv.h"

#define CMD_HYPERV(_c) ((_c)->hyperv)
#define CMD_HYPERV_SMART(_c) (CMD_HYPERV(_c)->u.smart.data)

static struct ndctl_cmd *hyperv_dimm_cmd_new_smart(struct ndctl_dimm *dimm)
{
	struct ndctl_bus *bus = ndctl_dimm_get_bus(dimm);
	struct ndctl_ctx *ctx = ndctl_bus_get_ctx(bus);
	struct ndctl_cmd *cmd;
	size_t size;
	struct nd_pkg_hyperv *hyperv;

	if (!ndctl_dimm_is_cmd_supported(dimm, ND_CMD_CALL)) {
		dbg(ctx, "unsupported cmd\n");
		return NULL;
	}

	if (test_dimm_dsm(dimm, ND_HYPERV_CMD_SMART) == DIMM_DSM_UNSUPPORTED) {
		dbg(ctx, "unsupported function\n");
		return NULL;
	}

	size = sizeof(*cmd) + sizeof(struct nd_pkg_hyperv);
	cmd = calloc(1, size);
	if (!cmd)
		return NULL;

	cmd->dimm = dimm;
	ndctl_cmd_ref(cmd);
	cmd->type = ND_CMD_CALL;
	cmd->size = size;
	cmd->status = 1;

	hyperv = CMD_HYPERV(cmd);
	hyperv->gen.nd_family = NVDIMM_FAMILY_HYPERV;
	hyperv->gen.nd_command = ND_HYPERV_CMD_SMART;
	hyperv->gen.nd_fw_size = 0;
	hyperv->gen.nd_size_in = offsetof(struct nd_hyperv_smart, status);
	hyperv->gen.nd_size_out = sizeof(hyperv->u.smart);
	hyperv->u.smart.status = 0;

	cmd->firmware_status = &hyperv->u.smart.status;

	return cmd;
}

static int hyperv_smart_valid(struct ndctl_cmd *cmd)
{
	if (cmd->type != ND_CMD_CALL ||
	    cmd->size != sizeof(*cmd) + sizeof(struct nd_pkg_hyperv) ||
	    CMD_HYPERV(cmd)->gen.nd_family != NVDIMM_FAMILY_HYPERV ||
	    CMD_HYPERV(cmd)->gen.nd_command != ND_HYPERV_CMD_SMART ||
	    cmd->status != 0)
		return cmd->status < 0 ? cmd->status : -EINVAL;
	return 0;
}

static unsigned int hyperv_cmd_smart_get_flags(struct ndctl_cmd *cmd)
{
	int rc;

	rc = hyperv_smart_valid(cmd);
	if (rc < 0) {
		errno = -rc;
		return UINT_MAX;
	}

	/* below health data can be retrieved via HYPERV _DSM function 1 */
	return ND_HYPERV_SMART_HEALTH_VALID;
}

static unsigned int hyperv_cmd_smart_get_health(struct ndctl_cmd *cmd)
{
	unsigned int health = 0;
	__u32 num;
	int rc;

	rc = hyperv_smart_valid(cmd);
	if (rc < 0) {
		errno = -rc;
		return UINT_MAX;
	}

	num = CMD_HYPERV_SMART(cmd)->health & 0x3F;

	if (num & (BIT(0) | BIT(1)))
		health = ND_SMART_CRITICAL_HEALTH;

	if (num & BIT(2))
		health = ND_SMART_FATAL_HEALTH;

	if (num & (BIT(3) | BIT(4) | BIT(5)))
		health = ND_SMART_NON_CRITICAL_HEALTH;

	return health;
}

struct ndctl_dimm_ops * const hyperv_dimm_ops = &(struct ndctl_dimm_ops) {
	.new_smart = hyperv_dimm_cmd_new_smart,
	.smart_get_flags = hyperv_cmd_smart_get_flags,
	.smart_get_health = hyperv_cmd_smart_get_health,
};
