/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Misc boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>
#include <lzma/LzmaTypes.h>
#include <lzma/LzmaDec.h>
#include <lzma/LzmaTools.h>

#ifdef CONFIG_CMD_GO

/* Allow ports to override the default behavior */
__attribute__((weak))
unsigned long do_go_exec(ulong (*entry)(int, char * const []), int argc,
				 char * const argv[])
{
	return entry (argc, argv);
}

static int do_go(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong	addr, rc;
	int     rcode = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);

	printf ("## Starting application at 0x%08lX ...\n", addr);

	/*
	 * pass address parameter as argv[0] (aka command name),
	 * and all remaining args
	 */
	rc = do_go_exec ((void *)addr, argc - 1, argv + 1);
	if (rc != 0) rcode = 1;

	printf ("## Application terminated, rc = 0x%lX\n", rc);
	return rcode;
}

/* -------------------------------------------------------------------- */

U_BOOT_CMD(
	go, CONFIG_SYS_MAXARGS, 1,	do_go,
	"start application at address 'addr'",
	"addr [arg ...]\n    - start application at address 'addr'\n"
	"      passing 'arg' as arguments"
);

#endif

static int do_jmp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	typedef void __noreturn (*image_entry_noargs_t)(void);
	image_entry_noargs_t image_entry = NULL;
	struct image_header hdr;
	unsigned int addr = 0;
	int loadaddr = 0;
	int rc = 0;
	SizeT len = 0;
	SizeT destLen = 0;

	if (argc < 2)
		return CMD_RET_USAGE;

	addr = simple_strtoul(argv[1], NULL, 16);
	printf ("## Jump to Normal boot at 0x%08X ...\n", addr);

	memcpy(&hdr, (const void *) addr, sizeof (hdr));
	if (image_get_magic(&hdr) != IH_MAGIC)
	{
		printf ("Bad Magic Number at address 0x%08x\n",addr);
		return -1;
	}

	len  = image_get_data_size(&hdr);
	loadaddr =  image_get_load(&hdr);

	rc = lzmaBuffToBuffDecompress((u8 *) loadaddr,
				&destLen,
				(u8 *) (addr + sizeof(struct image_header)),
				len);
	if (rc)
	{
		printf("Error: LZMA decompression failed with %d\n", rc);
		return rc;
	}

	flush_cache(loadaddr, destLen);

	image_entry = (image_entry_noargs_t) loadaddr;
	image_entry();

	return 0;
}

U_BOOT_CMD(
	jmpaddr, CONFIG_SYS_MAXARGS, 1,	do_jmp,
	"Jump to Normal Uboot",
	""
);

U_BOOT_CMD(
	reset, 1, 0,	do_reset,
	"Perform RESET of the CPU",
	""
);
