# !/bin/bash
#

echo "Make SPI NAND u-boot Block Size 65536 Bytes....\n"

dd if=/dev/zero of=u-boot-mtk.bin bs=65536 count=8 seek=0

dd if=spl/u-boot-spl-mtk.bin of=u-boot-mtk.bin bs=65536 seek=0 conv=notrunc

dd if=u-boot.img of=u-boot-mtk.bin bs=65536 seek=2 conv=notrunc

echo "Make SPI NAND u-boot-mtk.bin completed.\n"