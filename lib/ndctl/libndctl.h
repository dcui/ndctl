/*
 * libndctl: helper library for the nd (nvdimm, nfit-defined, persistent
 *           memory, ...) sub-system.
 *
 * Copyright (c) 2014, Intel Corporation.
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
#ifndef _LIBNDCTL_H_
#define _LIBNDCTL_H_

#include <stdarg.h>

/*
 *          "nd/ndctl" device/object hierarchy and kernel modules
 *
 * +-----------+-----------+-----------+------------------+-----------+
 * | DEVICE    |    BUS    |  REGION   |    NAMESPACE     |   BLOCK   |
 * | CLASSES:  | PROVIDERS |  DEVICES  |     DEVICES      |  DEVICES  |
 * +-----------+-----------+-----------+------------------+-----------+
 * | MODULES:  |  nd_core  |  nd_core  |    nd_region     |  nd_pmem  |
 * |           |  nd_acpi  | nd_region |                  |  nd_blk   |
 * |           | nfit_test |           |                  |    btt    |
 * +-----------v-----------v-----------v------------------v-----------v
 *               +-----+
 *               | CTX |
 *               +--+--+    +---------+   +--------------+   +-------+
 *                  |     +-> REGION0 +---> NAMESPACE0.0 +---> PMEM3 |
 * +-------+     +--+---+ | +---------+   +--------------+   +-------+
 * | DIMM0 <-----+ BUS0 +---> REGION1 +---> NAMESPACE1.0 +---> PMEM2 |
 * +-------+     +--+---+ | +---------+   +--------------+   +-------+
 *                  |     +-> REGION2 +---> NAMESPACE2.0 +---> PMEM1 |
 *                  |       +---------+   + ------------ +   +-------+
 *                  |
 * +-------+        |       +---------+   +--------------+   +-------+
 * | DIMM1 <---+ +--+---+ +-> REGION3 +---> NAMESPACE3.0 +---> PMEM0 |
 * +-------+   +-+ BUS1 +-+ +---------+   +--------------+   +-------+
 * | DIMM2 <---+ +--+---+ +-> REGION4 +---> NAMESPACE4.0 +--->  ND0  |
 * +-------+        |       + ------- +   +--------------+   +-------+
 *                  |
 * +-------+        |                     +--------------+   +-------+
 * | DIMM3 <---+    |                   +-> NAMESPACE5.0 +--->  ND2  |
 * +-------+   | +--+---+   +---------+ | +--------------+   +---------------+
 * | DIMM4 <-----+ BUS2 +---> REGION5 +---> NAMESPACE5.1 +--->  BTT1 |  ND1  |
 * +-------+   | +------+   +---------+ | +--------------+   +---------------+
 * | DIMM5 <---+                        +-> NAMESPACE5.2 +--->  BTT0 |  ND0  |
 * +-------+                              +--------------+   +-------+-------+
 *
 * Notes:
 * 1/ The object ids are not guaranteed to be stable from boot to boot
 * 2/ While regions and busses are numbered in sequential/bus-discovery
 *    order, the resulting block devices may appear to have random ids.
 *    Use static attributes of the devices/device-path to generate a
 *    persistent name.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct ndctl_ctx;
struct ndctl_ctx *ndctl_ref(struct ndctl_ctx *ctx);
struct ndctl_ctx *ndctl_unref(struct ndctl_ctx *ctx);
int ndctl_new(struct ndctl_ctx **ctx);
void ndctl_set_log_fn(struct ndctl_ctx *ctx,
                  void (*log_fn)(struct ndctl_ctx *ctx,
                                 int priority, const char *file, int line, const char *fn,
                                 const char *format, va_list args));
int ndctl_get_log_priority(struct ndctl_ctx *ctx);
void ndctl_set_log_priority(struct ndctl_ctx *ctx, int priority);
void ndctl_set_userdata(struct ndctl_ctx *ctx, void *userdata);
void *ndctl_get_userdata(struct ndctl_ctx *ctx);

struct ndctl_bus;
struct ndctl_bus *ndctl_bus_get_first(struct ndctl_ctx *ctx);
struct ndctl_bus *ndctl_bus_get_next(struct ndctl_bus *bus);
#define ndctl_bus_foreach(ctx, bus) \
        for (bus = ndctl_bus_get_first(ctx); \
             bus != NULL; \
             bus = ndctl_bus_get_next(bus))
struct ndctl_ctx *ndctl_bus_get_ctx(struct ndctl_bus *bus);
unsigned int ndctl_bus_get_major(struct ndctl_bus *bus);
unsigned int ndctl_bus_get_minor(struct ndctl_bus *bus);
unsigned int ndctl_bus_get_format(struct ndctl_bus *bus);
unsigned int ndctl_bus_get_revision(struct ndctl_bus *bus);
unsigned int ndctl_bus_get_id(struct ndctl_bus *bus);
const char *ndctl_bus_get_provider(struct ndctl_bus *bus);

struct ndctl_dimm;
struct ndctl_dimm *ndctl_dimm_get_first(struct ndctl_bus *bus);
struct ndctl_dimm *ndctl_dimm_get_next(struct ndctl_dimm *dimm);
#define ndctl_dimm_foreach(bus, dimm) \
        for (dimm = ndctl_dimm_get_first(bus); \
             dimm != NULL; \
             dimm = ndctl_dimm_get_next(dimm))
unsigned int ndctl_dimm_get_handle(struct ndctl_dimm *dimm);
unsigned int ndctl_dimm_get_node(struct ndctl_dimm *dimm);
unsigned int ndctl_dimm_get_socket(struct ndctl_dimm *dimm);
unsigned int ndctl_dimm_get_imc(struct ndctl_dimm *dimm);
unsigned int ndctl_dimm_get_channel(struct ndctl_dimm *dimm);
unsigned int ndctl_dimm_get_dimm(struct ndctl_dimm *dimm);
struct ndctl_bus *ndctl_dimm_get_bus(struct ndctl_dimm *dimm);
struct ndctl_ctx *ndctl_dimm_get_ctx(struct ndctl_dimm *dimm);
struct ndctl_dimm *ndctl_dimm_get_by_handle(struct ndctl_bus *bus,
		unsigned int handle);

struct ndctl_region;
struct ndctl_region *ndctl_region_get_first(struct ndctl_bus *bus);
struct ndctl_region *ndctl_region_get_next(struct ndctl_region *region);
#define ndctl_region_foreach(bus, region) \
        for (region = ndctl_region_get_first(bus); \
             region != NULL; \
             region = ndctl_region_get_next(region))
unsigned int ndctl_region_get_id(struct ndctl_region *region);
unsigned int ndctl_region_get_interleave_ways(struct ndctl_region *region);
unsigned int ndctl_region_get_mappings(struct ndctl_region *region);
unsigned long long ndctl_region_get_size(struct ndctl_region *region);
unsigned int ndctl_region_get_type(struct ndctl_region *region);
const char *ndctl_region_get_type_name(struct ndctl_region *region);
int ndctl_region_wait_probe(struct ndctl_region *region);
struct ndctl_bus *ndctl_region_get_bus(struct ndctl_region *region);
struct ndctl_ctx *ndctl_region_get_ctx(struct ndctl_region *region);
struct ndctl_dimm *ndctl_region_get_first_dimm(struct ndctl_region *region);
struct ndctl_dimm *ndctl_region_get_next_dimm(struct ndctl_region *region,
		struct ndctl_dimm *dimm);
#define ndctl_dimm_foreach_in_region(region, dimm) \
        for (dimm = ndctl_region_get_first_dimm(region); \
             dimm != NULL; \
             dimm = ndctl_region_get_next_dimm(region, dimm))

struct ndctl_mapping;
struct ndctl_mapping *ndctl_mapping_get_first(struct ndctl_region *region);
struct ndctl_mapping *ndctl_mapping_get_next(struct ndctl_mapping *mapping);
#define ndctl_mapping_foreach(region, mapping) \
        for (mapping = ndctl_mapping_get_first(region); \
             mapping != NULL; \
             mapping = ndctl_mapping_get_next(mapping))
struct ndctl_dimm *ndctl_mapping_get_dimm(struct ndctl_mapping *mapping);
struct ndctl_ctx *ndctl_mapping_get_ctx(struct ndctl_mapping *mapping);
struct ndctl_bus *ndctl_mapping_get_bus(struct ndctl_mapping *mapping);
struct ndctl_region *ndctl_mapping_get_region(struct ndctl_mapping *mapping);
unsigned long long ndctl_mapping_get_offset(struct ndctl_mapping *mapping);
unsigned long long ndctl_mapping_get_length(struct ndctl_mapping *mapping);

struct ndctl_namespace;
struct ndctl_namespace *ndctl_namespace_get_first(struct ndctl_region *region);
struct ndctl_namespace *ndctl_namespace_get_next(struct ndctl_namespace *ndns);
#define ndctl_namespace_foreach(region, ndns) \
        for (ndns = ndctl_namespace_get_first(region); \
             ndns != NULL; \
             ndns = ndctl_namespace_get_next(ndns))
struct ndctl_ctx *ndctl_namespace_get_ctx(struct ndctl_namespace *ndns);
struct ndctl_bus *ndctl_namespace_get_bus(struct ndctl_namespace *ndns);
struct ndctl_region *ndctl_namespace_get_region(struct ndctl_namespace *ndns);
unsigned int ndctl_namespace_get_id(struct ndctl_namespace *ndns);
unsigned int ndctl_namespace_get_type(struct ndctl_namespace *ndns);
const char *ndctl_namespace_get_type_name(struct ndctl_namespace *ndns);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif