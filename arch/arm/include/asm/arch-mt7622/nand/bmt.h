#ifndef __BMT_H__
#define __BMT_H__

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <asm/arch-mt7622/nand/mtk_nand.h>

#define MAX_BMT_SIZE        (0x80)
#define BMT_VERSION         (1) // initial version

#define MAIN_SIGNATURE_OFFSET   (0)
#define OOB_SIGNATURE_OFFSET    (1)
#define OOB_INDEX_OFFSET        (29)
#define OOB_INDEX_SIZE          (2)
#define FAKE_INDEX              (0xAAAA)

typedef struct _bmt_entry_
{
    u16 bad_index;              // bad block index
    u16 mapped_index;           // mapping block index in the replace pool
} bmt_entry;

typedef enum
{
    UPDATE_ERASE_FAIL,
    UPDATE_WRITE_FAIL,
    UPDATE_UNMAPPED_BLOCK,
    UPDATE_REASON_COUNT,
} update_reason_t;

typedef struct
{
    bmt_entry table[MAX_BMT_SIZE];
    u8 version;
    u8 mapped_count;            // mapped block count in pool
    u8 bad_count;               // bad block count in pool. Not used in V1
} bmt_struct;

/***************************************************************
*                                                              *
* Interface BMT need to use                                    *
*                                                              *
***************************************************************/
extern int mtk_nand_exec_read_page(struct mtd_info *mtd, u32 row, u32 page_size, u8 * dat, u8 * oob);
extern int mtk_nand_block_bad_hw(struct mtd_info *mtd, loff_t ofs);
extern int mtk_nand_erase_hw(struct mtd_info *mtd, int page);
extern int mtk_nand_block_markbad_hw(struct mtd_info *mtd, loff_t ofs);
extern int mtk_nand_exec_write_page(struct mtd_info *mtd, u32 row, u32 page_size, u8 * dat, u8 * oob);


/***************************************************************
*                                                              *
* Different function interface for preloader/uboot/kernel      *
*                                                              *
***************************************************************/
#if defined (CONFIG_MTK_SPI_NAND_SUPPORT)
    int init_bmt(struct nand_chip *nand, int size);
#else
    bmt_struct *init_bmt(struct nand_chip *nand, int size);
#endif
void set_bad_index_to_oob(u8 * oob, u16 index);
unsigned short get_mapping_block_index(int index);
bool update_bmt(u64 offset, update_reason_t reason, u8 * dat, u8 * oob);

#endif                          // #ifndef __BMT_H__
