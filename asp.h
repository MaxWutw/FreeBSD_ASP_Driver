#pragma once

/* AMD Family 19h */
/* PSP hardware */
#define PSP_REG_INTEN 		0x10690
#define PSP_REG_INTSTS		0x10694
/* ASP Mailbox register */
#define ASP_REG_CMDRESP 	0x10980
#define ASP_REG_ADDR_LO 	0x109e0
#define ASP_REG_ADDR_HI		0x109e4

/* SEV API Version 0.24 */
/* Platform commands */
#define SEV_CMD_INIT				0x01	
#define SEV_CMD_SHUTDOWN			0x02
#define SEV_CMD_PLATFORM_RESET		0x03		
#define SEV_CMD_PLATFORM_STATUS		0x04	
#define SEV_CMD_PEK_GEN				0x05
#define SEV_CMD_PEK_CSR				0x06
#define SEV_CMD_PEK_CERT_IMPORT 	0x07
#define SEV_CMD_PDH_CERT_EXPORT		0x08
#define SEV_CMD_PDH_GEN				0x09
#define SEV_CMD_DF_FLUSH			0x0a
#define SEV_CMD_DOWNLOAD_FIRMWARE	0x0b

/* Guest commands */
#define SEV_CMD_DECOMMISSION 		0x20
#define SEV_CMD_ACTIVATE			0x21
#define SEV_CMD_DEACTIVATE			0x22
#define SEV_CMD_GUEST_STATUS		0x23
#define SEV_CMD_LAUNCH_START		0x30
#define SEV_CMD_LAUNCH_UPDATE_DATA 	0x31
#define SEV_CMD_LAUNCH_UPDATE_VMSA	0x32
#define SEV_CMD_LAUNCH_MEASURE		0x33
#define SEV_CMD_LAUNCH_UPDATE_SECRET 0x34
#define SEV_CMD_LAUNCH_FINISH		0x35
#define SEV_CMD_ATTESTATION			0x36

#define ASP_CMDRESP_RESPONSE (1 << 31)
#define ASP_CMDRESP_COMPLETE (1 << 1)
#define ASP_CMDRESP_IOC		 (1 << 0)

/* Status Code */
#define SEV_STATUS_MASK				0xffff
#define SEV_STATUS_SUCCESS			0x0000

#define SEV_TMR_SIZE (1024 * 1024)


struct sev_init {
	uint32_t enable_es; 	/* In */
	uint32_t reserved;		/* - */
	uint64_t tmr_paddr;		/* In */
	uint32_t tmr_length;	/* In */
} __packed;

struct sev_platform_status {
	uint8_t 	api_major;		/* Out */
	uint8_t 	api_minor;		/* Out */
	uint8_t 	state;			/* Out */
	uint8_t 	owner;			/* Out */
	uint32_t 	cfges_build;	/* Out */
	uint32_t 	guest_count;	/* Out */
} __packed;

struct sev_activate {
	uint32_t handle;	/* In */
	uint32_t asid;		/* In */
} __packed;

struct sev_deactivate {
	uint32_t handle;	/* In */
} __packed;

struct sev_decommission {
	uint32_t handle;	/* In */
} __packed;

struct sev_guest_status {
	uint32_t handle;	/* In  */
	uint32_t policy;	/* Out */
	uint32_t asid;		/* Out */
	uint8_t  state;		/* Out */
} __packed;

struct sev_launch_start {
	uint32_t handle;		/* In/Out */
	uint32_t policy;		/* In */
	uint64_t dh_cert_paddr; /* In */
	uint32_t dh_cert_len;	/* In */
	uint32_t reserved;		/* - */
	uint64_t session_paddr; /* In */
	uint32_t session_len;	/* In */
} __packed;

struct sev_launch_update_data {
	uint32_t handle;	/* In */
	uint32_t reserved;	/* - */
	uint64_t paddr;		/* In */
	uint32_t length;	/* In */
} __packed;

struct sev_launch_update_vmsa {
	uint32_t handle;	/* In */
	uint32_t reserved;	/* - */
	uint64_t paddr;		/* In */
	uint32_t length;	/* In */
} __packed;

struct sev_launch_measure {
	uint32_t handle;			/* In */
	uint32_t reserved;			/* - */
	uint64_t measure_paddr;		/* In */
	uint32_t measure_len;		/* In/Out */
	uint8_t measure[32];		/* Out */
	uint8_t measure_nonce[16];  /* Out */
} __packed;

struct sev_launch_finish {
	uint32_t handle;	/* In */
} __packed;

struct sev_attestation {
	uint32_t handle;		/* In */
	uint32_t reserved;		/* - */
	uint64_t paddr;			/* In */
	uint8_t mnounce[16];	/* In */
	uint32_t length;		/* In/Out */
} __packed;

struct sev_guest_shutdown_args {
	uint32_t handle;
} __packed;
