// SPDX-License-Identifier:	GPL-2.0+
/*
* Generate MediaTek BootROM header for SPL/U-Boot images
*
* Copyright (C) 2018  MediaTek, Inc.
* Author: Weijie Gao <weijie.gao@mediatek.com>
*
*/
#include "imagetool.h"
#include <image.h>
#include <sha256.h>
#include "mtkimage.h"

/* Image type passed from imagename */
static enum brom_img_type {
	BROM_IMG_INVALID = 0,
	BROM_IMG_NOR,
	BROM_IMG_EMMC,
	BROM_IMG_SD,
	BROM_IMG_NAND,
	BROM_IMG_SNAND
} hdr_type;

/* GFH header + maximum 2 * 4KB pages of NAND */
static char hdr_tmp[sizeof(struct gfh_header) + 0x2000];

/* NAND header passed from imagename */
static union nand_boot_header *hdr_nand;

/* NAND header for SPI-NAND with 2KB page + 64B spare */
static union nand_boot_header snand_hdr_2k_64_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x03, 0x00, 0x40, 0x00,
		0x40, 0x00, 0x00, 0x08, 0x10, 0x00, 0x16, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x7B, 0xC4, 0x17, 0x9D,
		0xCA, 0x42, 0x90, 0xD0, 0x98, 0xD0, 0xE0, 0xF7,
		0xDB, 0xCD, 0x16, 0xF6, 0x03, 0x73, 0xD2, 0xB8,
		0x93, 0xB2, 0x56, 0x5A, 0x84, 0x6E, 0x00, 0x00
	}
};

/* NAND header for SPI-NAND with 2KB page + 120B/128B spare */
static union nand_boot_header snand_hdr_2k_128_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x05, 0x00, 0x70, 0x00,
		0x40, 0x00, 0x00, 0x08, 0x10, 0x00, 0x16, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x90, 0x28, 0xED, 0x13,
		0x7F, 0x12, 0x22, 0xCD, 0x3D, 0x06, 0xF1, 0xB3,
		0x6F, 0x2E, 0xD9, 0xA0, 0x9D, 0x7A, 0xBD, 0xD7,
		0xB3, 0x28, 0x3C, 0x13, 0xDB, 0x4E, 0x00, 0x00
	}
};

/* NAND header for SPI-NAND with 4KB page + 256B spare */
static union nand_boot_header snand_hdr_4k_256_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x10, 0x05, 0x00, 0xE0, 0x00,
		0x40, 0x00, 0x00, 0x08, 0x10, 0x00, 0x16, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x47, 0xED, 0x0E, 0xC3,
		0x83, 0xBF, 0x41, 0xD2, 0x85, 0x21, 0x97, 0x57,
		0xC4, 0x2E, 0x6B, 0x7A, 0x40, 0xE0, 0xCF, 0x8F,
		0x37, 0xBD, 0x17, 0xB6, 0xC7, 0xFE, 0x00, 0x00
	}
};

/* NAND header for Parallel NAND 1Gb with 2KB page + 64B spare */
static union nand_boot_header nand_hdr_1gb_2k_64_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x05, 0x00, 0x40, 0x00,
		0x40, 0x00, 0x00, 0x04, 0x0B, 0x00, 0x11, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x12, 0x28, 0x1C, 0x12,
		0x8F, 0xFD, 0xF8, 0x32, 0x6F, 0x6D, 0xCF, 0x6C,
		0xDA, 0x21, 0x70, 0x8C, 0xDA, 0x0A, 0x22, 0x82,
		0xAA, 0x59, 0xFA, 0x7C, 0x42, 0x2D, 0x00, 0x00
	}
};

/* NAND header for Parallel NAND 2Gb with 2KB page + 64B spare */
static union nand_boot_header nand_hdr_2gb_2k_64_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x05, 0x00, 0x40, 0x00,
		0x40, 0x00, 0x00, 0x08, 0x0B, 0x00, 0x11, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x20, 0x9C, 0x3D, 0x2D,
		0x7B, 0x68, 0x63, 0x52, 0x2E, 0x04, 0x63, 0xF1,
		0x35, 0x4E, 0x44, 0x3E, 0xF8, 0xAC, 0x9B, 0x95,
		0xAB, 0xFE, 0xE4, 0xE1, 0xD5, 0xF9, 0x00, 0x00
	}
};

/* NAND header for Parallel NAND 4Gb with 2KB page + 64B spare */
static union nand_boot_header nand_hdr_4gb_2k_64_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x05, 0x00, 0x40, 0x00,
		0x40, 0x00, 0x00, 0x10, 0x0B, 0x00, 0x11, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xE3, 0x0F, 0x86, 0x32,
		0x68, 0x05, 0xD9, 0xC8, 0x13, 0xDF, 0xC5, 0x0B,
		0x35, 0x3A, 0x68, 0xA5, 0x3C, 0x0C, 0x73, 0x87,
		0x63, 0xB0, 0xBE, 0xCC, 0x84, 0x47, 0x00, 0x00
	}
};

/* NAND header for Parallel NAND 2Gb with 2KB page + 128B spare */
static union nand_boot_header nand_hdr_2gb_2k_128_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x05, 0x00, 0x70, 0x00,
		0x40, 0x00, 0x00, 0x08, 0x0B, 0x00, 0x11, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x01, 0xA5, 0xE9, 0x5A,
		0xDF, 0x58, 0x62, 0x41, 0xD6, 0x26, 0x77, 0xBC,
		0x76, 0x1F, 0x27, 0x4E, 0x4F, 0x6C, 0xC3, 0xF0,
		0x36, 0xDE, 0xD9, 0xB3, 0xFF, 0x93, 0x00, 0x00
	}
};

/* NAND header for Parallel NAND 4Gb with 2KB page + 128B spare */
static union nand_boot_header nand_hdr_4gb_2k_128_data = {
	.data = {
		0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44,
		0x45, 0x52, 0x21, 0x00, 0x56, 0x30, 0x30, 0x36,
		0x4E, 0x46, 0x49, 0x49, 0x4E, 0x46, 0x4F, 0x00,
		0x00, 0x00, 0x00, 0x08, 0x05, 0x00, 0x70, 0x00,
		0x40, 0x00, 0x00, 0x10, 0x0B, 0x00, 0x11, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0xC2, 0x36, 0x52, 0x45,
		0xCC, 0x35, 0xD8, 0xDB, 0xEB, 0xFD, 0xD1, 0x46,
		0x76, 0x6B, 0x0B, 0xD5, 0x8B, 0xCC, 0x2B, 0xE2,
		0xFE, 0x90, 0x83, 0x9E, 0xAE, 0x2D, 0x00, 0x00
	}
};

static int mtk_image_check_image_types(uint8_t type)
{
	if (type == IH_TYPE_MTKIMAGE)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}

static int mtk_brom_parse_imagename(const char *imagename)
{
#define is_blank_char(c) \
	((c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == ' ')

	char *buf = strdup(imagename), *key, *val, *end, *next;
	const char *media = NULL;

	key = buf;
	while (key) {
		next = strchr(key, ';');
		if (next)
			*next = 0;

		val = strchr(key, '=');
		if (val) {
			*val++ = 0;

			/* Trim key */
			while (is_blank_char(*key))
				key++;

			end = key + strlen(key) - 1;
			while ((end >= key) && is_blank_char(*end))
				end--;
			end++;

			if (is_blank_char(*end))
				*end = 0;

			/* Trim value */
			while (is_blank_char(*val))
				val++;

			end = val + strlen(val) - 1;
			while ((end >= val) && is_blank_char(*end))
				end--;
			end++;

			if (is_blank_char(*end))
				*end = 0;

			/* Check key and value */
			if (!strcmp(key, "media")) {
				if (!strcmp(val, "nor"))
					hdr_type = BROM_IMG_NOR;
				else if (!strcmp(val, "emmc"))
					hdr_type = BROM_IMG_EMMC;
				else if (!strcmp(val, "sd"))
					hdr_type = BROM_IMG_SD;
				else if (!strcmp(val, "nand"))
					hdr_type = BROM_IMG_NAND;
				else if (!strcmp(val, "snand"))
					hdr_type = BROM_IMG_SNAND;

				media = val;
			}

			if (!strcmp(key, "nandinfo")) {
				if (!strcmp(val, "2k+64"))
					hdr_nand = &snand_hdr_2k_64_data;
				else if (!strcmp(val, "2k+120"))
					hdr_nand = &snand_hdr_2k_128_data;
				else if (!strcmp(val, "2k+128"))
					hdr_nand = &snand_hdr_2k_128_data;
				else if (!strcmp(val, "4k+256"))
					hdr_nand = &snand_hdr_4k_256_data;
				else if (!strcmp(val, "1g:2k+64"))
					hdr_nand = &nand_hdr_1gb_2k_64_data;
				else if (!strcmp(val, "2g:2k+64"))
					hdr_nand = &nand_hdr_2gb_2k_64_data;
				else if (!strcmp(val, "4g:2k+64"))
					hdr_nand = &nand_hdr_4gb_2k_64_data;
				else if (!strcmp(val, "2g:2k+128"))
					hdr_nand = &nand_hdr_2gb_2k_128_data;
				else if (!strcmp(val, "4g:2k+128"))
					hdr_nand = &nand_hdr_4gb_2k_128_data;
			}
		}

		if (next)
			key = next + 1;
		else
			break;
	}

	free(buf);

	if (hdr_type == BROM_IMG_INVALID) {
		fprintf(stderr, "Error: media type is invalid or missing.\n");
		fprintf(stderr, "       Please specify -n \"media=<Type>\"\n");
		return -EINVAL;
	}

	if ((hdr_type == BROM_IMG_NAND || hdr_type == BROM_IMG_SNAND) &&
	    !hdr_nand) {
		fprintf(stderr, "Error: nand info is invalid or missing.\n");
		fprintf(stderr, "       Please specify -n \"media=%s;nandinfo=<Info>\"\n",
			media);
		return -EINVAL;
	}

	return 0;
}

static int mtk_image_check_params(struct image_tool_params *params)
{
	if (!params->addr) {
		fprintf(stderr, "Error: Load Address must be set.\n");
		return -EINVAL;
	}

	if (!params->imagename) {
		fprintf(stderr, "Error: Image Name (used as configuration) must be set.\n");
		return -EINVAL;
	}

	return mtk_brom_parse_imagename(params->imagename);
}

static int mtk_image_vrec_header(struct image_tool_params *params,
	struct image_type_params *tparams)
{
	if (hdr_type == BROM_IMG_NAND || hdr_type == BROM_IMG_SNAND)
		tparams->header_size = 2 * le16_to_cpu(hdr_nand->pagesize);
	else
		tparams->header_size = sizeof(struct gen_device_header);

	tparams->header_size += sizeof(struct gfh_header);
	tparams->hdr = &hdr_tmp;

	memset(&hdr_tmp, 0xff, tparams->header_size);

	return SHA256_SUM_LEN;
}

static int mtk_image_verify_gen_header(const uint8_t *ptr, int print)
{
	union gen_boot_header *sfb = (union gen_boot_header *) ptr;
	struct brom_layout_header *bh;
	struct gfh_header *gfh;
	const char *boot_media;

	if (!strcmp(sfb->name, SF_BOOT_NAME))
		boot_media = "Serial NOR";
	else if (!strcmp(sfb->name, EMMC_BOOT_NAME))
		boot_media = "eMMC";
	else if (!strcmp(sfb->name, SDMMC_BOOT_NAME))
		boot_media = "SD/MMC";
	else
		return -1;

	if (print)
		printf("Boot Media:   %s\n", boot_media);

	if (le32_to_cpu(sfb->version) != 1 ||
	    le32_to_cpu(sfb->size) != sizeof(union gen_boot_header))
		return -1;

	bh = (struct brom_layout_header *) (ptr + le32_to_cpu(sfb->size));

	if (strcmp(bh->name, BRLYT_NAME))
		return -1;

	if (le32_to_cpu(bh->magic) != BRLYT_MAGIC ||
	    (le32_to_cpu(bh->type) != BRLYT_TYPE_NOR &&
	    le32_to_cpu(bh->type) != BRLYT_TYPE_EMMC &&
	    le32_to_cpu(bh->type) != BRLYT_TYPE_SD))
		return -1;

	gfh = (struct gfh_header *) (ptr + le32_to_cpu(bh->header_size));

	if (strcmp(gfh->file_info.name, GFH_FILE_INFO_NAME))
		return -1;

	if (le32_to_cpu(gfh->file_info.flash_type) != GFH_FLASH_TYPE_GEN)
		return -1;

	if (print)
		printf("Load Address: %08x\n",
			le32_to_cpu(gfh->file_info.load_addr) +
			le32_to_cpu(gfh->file_info.jump_offset));

	return 0;
}

static int mtk_image_verify_nand_header(const uint8_t *ptr, int print)
{
	union nand_boot_header *nh = (union nand_boot_header *) ptr;
	struct brom_layout_header *bh;
	struct gfh_header *gfh;
	const char *boot_media;

	if (strncmp(nh->version, NAND_BOOT_VERSION, sizeof(nh->version)) ||
	    strcmp(nh->id, NAND_BOOT_ID))
		return -1;

	bh = (struct brom_layout_header *) (ptr + le16_to_cpu(nh->pagesize));

	if (strcmp(bh->name, BRLYT_NAME))
		return -1;

	if (le32_to_cpu(bh->magic) != BRLYT_MAGIC) {
		return -1;
	} else {
		if (le32_to_cpu(bh->type) == BRLYT_TYPE_NAND)
			boot_media = "NAND";
		else if (le32_to_cpu(bh->type) == BRLYT_TYPE_SNAND)
			boot_media = "Serial NAND";
		else
			return -1;
	}

	if (print) {
		printf("Boot Media:   %s\n", boot_media);

		if (le32_to_cpu(bh->type) == BRLYT_TYPE_NAND) {
			uint64_t capacity = le16_to_cpu(nh->numblocks)
				* le16_to_cpu(nh->pages_of_block) *
				le16_to_cpu(nh->pagesize);
			printf("Capacity:     %dGb\n",
				(uint32_t) (capacity >> 20));
		}

		if (le16_to_cpu(nh->pagesize) > 512)
			printf("Page Size:    %dKB\n",
				le16_to_cpu(nh->pagesize) >> 10);
		else
			printf("Page Size:    %dB\n",
				le16_to_cpu(nh->pagesize));
		printf("Spare Size:   %dB\n", le16_to_cpu(nh->oobsize));
	}

	gfh = (struct gfh_header *) (ptr + 2 * le16_to_cpu(nh->pagesize));

	if (strcmp(gfh->file_info.name, GFH_FILE_INFO_NAME))
		return -1;

	if (le32_to_cpu(gfh->file_info.flash_type) != GFH_FLASH_TYPE_NAND)
		return -1;

	if (print)
		printf("Load Address: %08x\n",
			le32_to_cpu(gfh->file_info.load_addr) +
			le32_to_cpu(gfh->file_info.jump_offset));

	return 0;
}

static int mtk_image_verify_header(unsigned char *ptr, int image_size,
	struct image_tool_params *params)
{
	if (!strcmp((char *) ptr, NAND_BOOT_NAME))
		return mtk_image_verify_nand_header(ptr, 0);
	else
		return mtk_image_verify_gen_header(ptr, 0);

	return -1;
}

static void mtk_image_print_header(const void *ptr)
{
	printf("Image Type:   MediaTek BootROM Loadable Image\n");

	if (!strcmp((char *) ptr, NAND_BOOT_NAME))
		mtk_image_verify_nand_header(ptr, 1);
	else
		mtk_image_verify_gen_header(ptr, 1);
}

static void put_brom_layout_header(struct brom_layout_header *hdr, int type)
{
	strncpy(hdr->name, BRLYT_NAME, sizeof(hdr->name));
	hdr->version = cpu_to_le32(1);
	hdr->magic = cpu_to_le32(BRLYT_MAGIC);
	hdr->type = cpu_to_le32(type);
}

static void put_ghf_common_header(struct gfh_common_header *gfh, int size,
	int type, int ver)
{
	memcpy(gfh->magic, GFH_HEADER_MAGIC, sizeof(gfh->magic));
	gfh->version = ver;
	gfh->size = cpu_to_le16(size);
	gfh->type = cpu_to_le16(type);
}

static void put_ghf_header(struct gfh_header *gfh, int file_size,
	int dev_hdr_size, int load_addr, int flash_type)
{
	memset(gfh, 0, sizeof(struct gfh_header));

	/* GFH_FILE_INFO header */
	put_ghf_common_header(&gfh->file_info.gfh, sizeof(gfh->file_info),
		GFH_TYPE_FILE_INFO, 1);
	strncpy(gfh->file_info.name, GFH_FILE_INFO_NAME,
		sizeof(gfh->file_info.name));
	gfh->file_info.unused = cpu_to_le32(1);
	gfh->file_info.file_type = cpu_to_le16(1);
	gfh->file_info.flash_type = flash_type;
	gfh->file_info.sig_type = GFH_SIG_TYPE_SHA256;
	gfh->file_info.load_addr = cpu_to_le32(load_addr - sizeof(*gfh));
	gfh->file_info.total_size = cpu_to_le32(file_size - dev_hdr_size);
	gfh->file_info.max_size = cpu_to_le32(file_size);
	gfh->file_info.gfh_total_size = sizeof(*gfh);
	gfh->file_info.sig_size = SHA256_SUM_LEN;
	gfh->file_info.jump_offset = sizeof(*gfh);
	gfh->file_info.processed = 1;

	/* GFH_BL_INFO header */
	put_ghf_common_header(&gfh->bl_info.gfh, sizeof(gfh->bl_info),
		GFH_TYPE_BL_INFO, 1);
	gfh->bl_info.attr = cpu_to_le16(1);

	/* GFH_BROM_CFG header */
	put_ghf_common_header(&gfh->brom_cfg.gfh, sizeof(gfh->brom_cfg),
		GFH_TYPE_BROM_CFG, 3);
	gfh->brom_cfg.cfg_bits = cpu_to_le32(
		GFH_BROM_CFG_USBDL_AUTO_DETECT_DIS |
		GFH_BROM_CFG_USBDL_BY_KCOL0_TIMEOUT_EN |
		GFH_BROM_CFG_USBDL_BY_FLAG_TIMEOUT_EN);
	gfh->brom_cfg.usbdl_by_kcol0_timeout_ms = cpu_to_le32(0x1388);

	/* GFH_BL_SEC_KEY header */
	put_ghf_common_header(&gfh->bl_sec_key.gfh, sizeof(gfh->bl_sec_key),
		GFH_TYPE_BL_SEC_KEY, 1);

	/* GFH_ANTI_CLONE header */
	put_ghf_common_header(&gfh->anti_clone.gfh, sizeof(gfh->anti_clone),
		GFH_TYPE_ANTI_CLONE, 1);
	gfh->anti_clone.ac_offset = cpu_to_le32(0x10);
	gfh->anti_clone.ac_len = cpu_to_le32(0x80);

	/* GFH_BROM_SEC_CFG header */
	put_ghf_common_header(&gfh->brom_sec_cfg.gfh,
		sizeof(gfh->brom_sec_cfg), GFH_TYPE_BROM_SEC_CFG, 1);
	gfh->brom_sec_cfg.cfg_bits =
		cpu_to_le32(BROM_SEC_CFG_JTAG_EN | BROM_SEC_CFG_UART_EN);
}

static void put_hash(uint8_t *buff, int size)
{
	sha256_context ctx;

	sha256_starts(&ctx);
	sha256_update(&ctx, buff, size);
	sha256_finish(&ctx, buff + size);
}

static void mtk_image_set_gen_header(void *ptr, off_t filesize,
	uint32_t loadaddr)
{
	struct gen_device_header *hdr = (struct gen_device_header *) ptr;
	struct gfh_header *gfh;
	const char *boot_name = NULL;
	uint32_t brlyt_type = 0;

	if (hdr_type == BROM_IMG_NOR) {
		boot_name = SF_BOOT_NAME;
		brlyt_type = BRLYT_TYPE_NOR;
	} else if (hdr_type == BROM_IMG_EMMC) {
		boot_name = EMMC_BOOT_NAME;
		brlyt_type = BRLYT_TYPE_EMMC;
	} else if (hdr_type == BROM_IMG_SD) {
		boot_name = SDMMC_BOOT_NAME;
		brlyt_type = BRLYT_TYPE_SD;
	}

	/* Generic device header */
	strncpy(hdr->boot.name, boot_name, sizeof(hdr->boot.name));
	hdr->boot.version = cpu_to_le32(1);
	hdr->boot.size = cpu_to_le32(sizeof(hdr->boot));

	/* BRLYT header */
	put_brom_layout_header(&hdr->brlyt, brlyt_type);
	hdr->brlyt.header_size = cpu_to_le32(sizeof(struct gen_device_header));
	hdr->brlyt.total_size = cpu_to_le32(filesize);
	hdr->brlyt.header_size_2 = hdr->brlyt.header_size;
	hdr->brlyt.total_size_2 = hdr->brlyt.total_size;

	/* GFH header */
	gfh = (struct gfh_header *) (ptr + sizeof(struct gen_device_header));
	put_ghf_header(gfh, filesize,
		sizeof(struct gen_device_header), loadaddr,
		GFH_FLASH_TYPE_GEN);

	/* Generate SHA256 hash */
	put_hash((uint8_t *) gfh,
		filesize - sizeof(struct gen_device_header) - SHA256_SUM_LEN);
}

static void mtk_image_set_nand_header(void *ptr, off_t filesize,
	uint32_t loadaddr)
{
	union nand_boot_header *nh = (union nand_boot_header *) ptr;
	struct brom_layout_header *brlyt;
	struct gfh_header *gfh;
	uint32_t brlyt_type = 0;
	uint32_t payload_pages;
	int i;

	if (hdr_type == BROM_IMG_NAND)
		brlyt_type = BRLYT_TYPE_NAND;
	else if (hdr_type == BROM_IMG_SNAND)
		brlyt_type = BRLYT_TYPE_SNAND;

	/* NAND device header, repeat 4 times */
	for (i = 0; i < 4; i++)
		memcpy(nh + i, hdr_nand, sizeof(union nand_boot_header));

	/* BRLYT header */
	payload_pages = (filesize + le16_to_cpu(hdr_nand->pagesize) - 1) /
		le16_to_cpu(hdr_nand->pagesize);
	brlyt = (struct brom_layout_header *) (ptr + le16_to_cpu(hdr_nand->pagesize));
	put_brom_layout_header(brlyt, brlyt_type);
	brlyt->header_size = cpu_to_le32(2);
	brlyt->total_size = cpu_to_le32(payload_pages);
	brlyt->header_size_2 = brlyt->header_size;
	brlyt->total_size_2 = brlyt->total_size;
	brlyt->unused = cpu_to_le32(1);

	/* GFH header */
	gfh = (struct gfh_header *) (ptr + 2 * le16_to_cpu(hdr_nand->pagesize));
	put_ghf_header(gfh,
		filesize, 2 * le16_to_cpu(hdr_nand->pagesize), loadaddr,
		GFH_FLASH_TYPE_NAND);

	/* Generate SHA256 hash */
	put_hash((uint8_t *) gfh,
		filesize - 2 * le16_to_cpu(hdr_nand->pagesize) - SHA256_SUM_LEN);
}

static void mtk_image_set_header(void *ptr, struct stat *sbuf,
	int ifd, struct image_tool_params *params)
{
	if (hdr_type == BROM_IMG_NAND || hdr_type == BROM_IMG_SNAND)
		mtk_image_set_nand_header(ptr, sbuf->st_size, params->addr);
	else
		mtk_image_set_gen_header(ptr, sbuf->st_size, params->addr);
}

static struct image_type_params mtk_image_params = {
	.name		= "MediaTek BootROM Loadable Image",
	.header_size	= 0,
	.hdr		= NULL,
	.check_image_type = mtk_image_check_image_types,
	.verify_header	= mtk_image_verify_header,
	.print_header	= mtk_image_print_header,
	.set_header	= mtk_image_set_header,
	.check_params = mtk_image_check_params,
	.vrec_header = mtk_image_vrec_header,
};

void init_mtk_image_type(void)
{
	register_image_type(&mtk_image_params);
}
