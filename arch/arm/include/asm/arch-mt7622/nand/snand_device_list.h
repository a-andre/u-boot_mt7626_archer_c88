
#ifndef __SNAND_DEVICE_LIST_H__
#define __SNAND_DEVICE_LIST_H__

#define SNAND_MAX_ID		3

typedef struct
{
    u8 id[SNAND_MAX_ID];
    u8 id_length;
    u16 totalsize;
    u16 blocksize;
    u16 pagesize;
    u16 sparesize;
    u32 SNF_DLY_CTL1;
    u32 SNF_DLY_CTL2;
    u32 SNF_DLY_CTL3;
    u32 SNF_DLY_CTL4;
    u32 SNF_MISC_CTL;
    u32 SNF_DRIVING;
    u8 devicename[30];
    u32 advancedmode;
}snand_flashdev_info,*psnandflashdev_info;

static const snand_flashdev_info gen_snand_FlashTable[]={
	{{0xEF, 0xAA, 0x20}, 3, 64, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "Winbond 512Mb", 0x00000000},
	{{0xEF, 0xAA, 0x21}, 3, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "Winbond 1Gb", 0x00000000},
	{{0xEF, 0xAB, 0x21}, 3, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "Winbond 2Gb", 0x00000000},
	{{0xC8, 0xD4}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "GD5F4GQ4UBYIG", 0x00000000},
	{{0xC8, 0xF4}, 2, 512, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "GD5F4GQ4UAYIG", 0x00000000},
	{{0xC8, 0xD1}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "GD5F1GQ4UX", 0x00000000},
	{{0xC8, 0xD2}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "GD5F2GQ4UX", 0x00000000},
	{{0xC2, 0x22}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "MX35LF2GE4AB", 0x00000000},
	{{0xC2, 0x20}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "MX35LF2G14AC", 0x00000000},
	{{0xC2, 0x12}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "MX35LF1GE4AB", 0x00000000},
	{{0xC8, 0x21}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "F50L1G41A", 0x00000000},
	{{0xC8, 0x01}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "F50L1G41LB", 0x00000000},
	{{0xC8, 0x0a}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "F50L2G41A", 0x00000000},
	{{0x2C, 0x14}, 2, 128, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "MT29F1G01ABAGD", 0x00000000},
	{{0x2C, 0x24}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "MT29F2G01ABAGD", 0x00000000},
	{{0x2C, 0x36}, 2, 512, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "MT29F4G01ADAGD", 0x00000000},
	{{0x98, 0xC2}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "TC58CVG0S3HRAIG", 0x00000000},
	{{0x98, 0xCB}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "TC58CVG1S3HRAIG", 0x00000000},
	{{0x98, 0xCD}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "TC58CVG2S3HRAIG", 0x00000000},
	{{0xD5, 0x11}, 2, 128, 128, 2048, 120, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73C01G44SNB", 0x00000000},
	{{0xD5, 0x12}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73D02G44SNA", 0x00000000},
	{{0xD5, 0x03}, 2, 512, 256, 4096, 256, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73E04G44SNA", 0x00000000},
	{{0xD5, 0x1D}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73F044SND", 0x00000000},
	{{0xD5, 0x1C}, 2, 128, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73F044VCD", 0x00000000},
	{{0xD5, 0x10}, 2, 256, 128, 2048, 128, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73F044SNF", 0x00000000},
	{{0xD5, 0x1F}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73F044VCG", 0x00000000},
	{{0xD5, 0x1B}, 2, 256, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73F044VCH", 0x00000000},
	{{0xD5, 0x01}, 2, 64, 128, 2048, 64, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73B044VCA", 0x00000000},
	{{0xD5, 0x24}, 2, 1024, 256, 4096, 256, 0x00000000, 0x00000000, 0x1A00001A,
		0x00000000, 0x0552000A, 0x01, "EM73F044SNA", 0x00000000},
};

#endif