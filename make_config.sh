#!/bin/sh
CURRDIR=$(pwd)
# create soft links
cd arch/arm/include/asm && ln -s arch-leopard arch && ln -s proc-armv proc

# decompress pre-compiled dramc objs
cd $CURRDIR
mkdir -p spl_preconfig
tar -xf leopard_evb.tar.bz2 -C spl_preconfig/
rm -rf spl/board/mediatek/leopard_evb
mv spl_preconfig/leopard_evb spl/board/mediatek/