#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)

typedef unsigned long long	u64;
typedef unsigned int		u32;
typedef unsigned short		u16;
typedef unsigned char		u8;

struct nd_cmd_pkg {
	u64   nd_family;		/* family of commands */
	u64   nd_command;
	u32   nd_size_in;		/* INPUT: size of input args */
	u32   nd_size_out;		/* INPUT: size of payload */
	u32   nd_reserved2[9];		/* reserved must be zero */
	u32   nd_fw_size;		/* OUTPUT: size fw wants to return */
	unsigned char nd_payload[];	/* Contents of call      */
};

#define NVDIMM_FAMILY_HYPERV 4

#define ND_IOCTL 'N'

enum { ND_CMD_CALL = 10 };
#define ND_IOCTL_CALL	_IOWR(ND_IOCTL, ND_CMD_CALL, struct nd_cmd_pkg)

/* See http://www.uefi.org/RFIC_LIST ("Virtual NVDIMM 0x1901") */
enum {
        ND_HYPERV_CMD_QUERY = 0,

        /* non-root commands */
        ND_HYPERV_CMD_GET_HEALTH_INFO = 1,
        ND_HYPERV_CMD_GET_SHUTDOWN_INFO = 2,
        ND_HYPERV_CMD_INJECT_ERRORS = 3,
        ND_HYPERV_CMD_QUERY_INJECTED_ERRORS = 4,
};

struct nd_hv_health {
        u32   status;
        u32   health;
} __attribute__((packed));

struct nd_hv_shutdown_info {
         u32   status;
         u32   count;
} __attribute__((packed));

struct nd_hv_inject_errors {
	/* input */
         u32   injected_errors;
         u32   injected_shutdown_count;

	/* output */
         u32   status;
} __attribute__((packed));

struct nd_hv_query_injected_errors {
	/* No input */

	/* output */
         u32   status;
         u8    error_injection_enabled;
         u32   injected_errors;
         u32   injected_shutdown_count;
} __attribute__((packed));

union nd_hv_cmd {
        u32                  status;
        struct nd_hv_health  health;
        struct nd_hv_shutdown_info shutdown_info;
        struct nd_hv_inject_errors inject_errors_info;
        struct nd_hv_query_injected_errors injected_error_info;
} __attribute__((packed));

struct nd_pkg_hv {
        struct nd_cmd_pkg   gen;
        union nd_hv_cmd     u;
} __attribute__((packed));


static int inject_errors(int fd, u32 errors, u32 unsafe_shutdown_count)
{
	struct nd_hv_inject_errors *e;
	struct nd_pkg_hv hv_pkg;

	int injecting_usc;
	int rc;


	printf("Trying to inject error=%08x, unsafe_shutdown_count=%08x\n",
		errors, unsafe_shutdown_count);

	injecting_usc = !!(errors & (1U << 6));

	if (injecting_usc && unsafe_shutdown_count == 0)
		printf("Clearing the current Unsafe Shutdown Count\n");

	if (!injecting_usc && unsafe_shutdown_count != 0) {
		printf("Error: non-zero unsafe_shutdown_count was specified\n");
		return -EINVAL;
	}

	memset(&hv_pkg, 0, sizeof(hv_pkg));
	memset(&hv_pkg.gen.nd_reserved2, 0, sizeof(hv_pkg.gen.nd_reserved2));
	hv_pkg.gen.nd_family = NVDIMM_FAMILY_HYPERV;
	hv_pkg.gen.nd_command = ND_HYPERV_CMD_INJECT_ERRORS;
	hv_pkg.gen.nd_fw_size = 0;
	hv_pkg.gen.nd_size_in = offsetof(struct nd_hv_inject_errors, status);
	hv_pkg.gen.nd_size_out = sizeof(hv_pkg.u.inject_errors_info.status);

	e = &hv_pkg.u.inject_errors_info;
	e->injected_errors = errors;
	e->injected_shutdown_count = unsafe_shutdown_count;

	rc = ioctl(fd, ND_IOCTL_CALL, &hv_pkg);
	if (rc) {
		printf("%s Failed! rc=%d\n", __func__, rc);
		return rc;
	}

	if (e->status != 0) {
		printf("%s Failed: status = 0x%x\n", __func__, e->status);
		return -EIO;
	}

	return 0;
}

#ifndef INJECTED_ERROR
/* This is the default injected errors: Bit0~6 are all set. */
#define INJECTED_ERROR	0x7F
#endif

#ifndef INJECTED_UNSAFE_SHUTDOWN_COUNT
/* This is the default */
#define INJECTED_UNSAFE_SHUTDOWN_COUNT	0x2019
#endif

int main()
{
	u32 errors;
	u32 unsafe_shutdown_count;
	int rc;

	int fd = open("/dev/nmem0", O_RDWR);
	if (fd < 0) {
		printf("Failed to open /dev/nmem0!\n");
		exit(-1);
	}

	errors = INJECTED_ERROR;
	unsafe_shutdown_count = INJECTED_UNSAFE_SHUTDOWN_COUNT;

	rc = inject_errors(fd, errors, unsafe_shutdown_count);

	close(fd);
	return rc;
}
