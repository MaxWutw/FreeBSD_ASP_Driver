#pragma once

/* AMD Family 19h */
#define ASP_REG_CMDRESP 	0x10980
#define ASP_REG_ADDR_LO 	0x109e0
#define ASP_REG_ADDR_HI		0x109e4

/* SEV API Version 0.24 */
#define SEV_CMD_INIT			0x1	
#define SEV_CMD_SHUTDOWN		0x2
#define SEV_CMD_PLATFORM_RESET	0x3		
#define SEV_CMD_PLATFORM_STATUS	0x4	

#define ASP_CMDRESP_RESPONSE (1 << 31)

struct sev_platform_status {
	uint8_t 	api_major;
	uint8_t 	api_minor;
	uint8_t 	state;
	uint8_t 	owner;
	uint32_t 	cfges_build;
	uint32_t 	guest_count;
} __packed;
