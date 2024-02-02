#
# Makefile for http stuff
#

#include $(TOPDIR)/config.mk
#
#LIB   = lib_verify.a
##OBJS += uip.o uip_arp.o httpd.o fs.o clock-arch.o psock.o timer.o uip-fw.o uip-neighbor.o uip-split.o uiplib.o 

obj-$(CONFIG_UIP) += bigNumber.o
obj-$(CONFIG_UIP) += md5.o
obj-$(CONFIG_UIP) += rsaVerify.o
obj-$(CONFIG_UIP) += shaAndSha512.o

ccflags-$(CONFIG_UIP) += -I$(srctree)/lib/lib_verify

#all:    $(LIB)
#
#$(LIB): $(START) $(OBJS)
#	    $(AR) crv $@ $(OBJS)
#
##########################################################################
#
#.depend: Makefile $(OBJS:.o=.c)
#	     $(CC) -M $(CFLAGS) $(OBJS:.o=.c) > $@
#
#sinclude .depend
#
##########################################################################

