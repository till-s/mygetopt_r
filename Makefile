TOP=../../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

GENINC = geninc

USR_CFLAGS +=  -fpermissive -DDIRSHELL -DDEBUG
USR_INCLUDES += -I../$(GENINC)/
#USR_DBDFLAGS += -I$(ECDR_HOME)
#USR_LDFLAGS += -Wl,-M

INC += drvrEcdr814.h

PROD_HOST += genMenuHdr
genMenuHdr_SRCS += genMenuHdr.c bitMenu.c

PROD_HOST += genHeaders
genHeaders_SRCS += genHeaders.c ecDb.c bitMenu.c

PROD_HOST += genDbd
genDbd_SRCS += genDbd.c ecDb.c bitMenu.c


#caExample_LIBS	+= ca
#caExample_LIBS	+= Com

#=============================

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += ecdr814Record ecdr814RXRecord ecdr814ChannelRecord ecdr814BoardRecord
#DBD += ecdr814RecCommon.dbd ecdr814RXFields.dbd ecdr814BoardFields.dbd  ecdr814ChannelFields.dbd  


# <name>.dbd will be created from <name>Include.dbd
#DBD += ecdr814.dbd

#=============================

#PROD_RTEMS = ecdr814App
LIBRARY += drvEcdr814

drvEcdr814_SRCS += drvrEcdr814.c regNodeOps.c dirOps.c bitMenu.c ecDb.c  ecdr814Lowlevel.c ecLookup.c mygetopt_r.c
PROD_HOST_Linux += drvEcdr814Tst

drvEcdr814Tst_SRCS += drvrTst.c
drvEcdr814Tst_LIBS += drvEcdr814
drvEcdr814Tst_LDFLAGS += -L$(TOP)/lib/$(T_A)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

../$(GENINC):
	mkdir $@

inc: ../$(GENINC) ../$(GENINC)/menuDefs.h ../$(GENINC)/fastKeyDefs.h

../$(GENINC)/menuDefs.h: ../O.$(HOST_ARCH)/genMenuHdr ../bitMenu.c
	echo '#ifndef BIT_MENU_HEADER_DEFS_H' > $@
	echo '#define BIT_MENU_HEADER_DEFS_H' >> $@
	echo '/* DONT EDIT THIS FILE, IT WAS AUTOMATICALLY GENERATED */' >>$@
	if  ! $< >> $@ || ! echo '#endif' >>$@  ; then $(RM) $@; fi

../$(GENINC)/fastKeyDefs.h:../O.$(HOST_ARCH)/genHeaders 
	if ! $< -k > $@ ; then $(RM) $@; fi

../O.$(HOST_ARCH)/genMenuHdr:
	$(MAKE) -C ../O.$(HOST_ARCH)/ genMenuHdr

../O.$(HOST_ARCH)/genHeaders:
	$(MAKE) -C ../O.$(HOST_ARCH)/ genHeaders

clean::
	$(RMDIR) $(GENINC)
