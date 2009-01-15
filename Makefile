TOP=../..

include $(TOP)/configure/CONFIG
CROSS_COMPILER_TARGET_ARCHS = $(CROSS_COMPILER_TARGET_VME)

#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

GENINC = O.Common

USR_CFLAGS +=  -DDIRSHELL -DDEBUG -g -fno-strict-aliasing
USR_INCLUDES += -I../$(GENINC)/
#USR_DBDFLAGS += -I$(ECDR_HOME)
#USR_LDFLAGS += -Wl,-M

INC += ecOsDep.h ecErrCodes.h drvrEcdr814.h ecFastKeys.h ecFastKeyDefs.h ecMenuDefs.h

TESTPROD_HOST += genMenuHdr
genMenuHdr_SRCS += genMenuHdr.c bitMenu.c

TESTPROD_HOST += genHeaders
genHeaders_SRCS += genHeaders.c ecDb.c bitMenu.c

TESTPROD_HOST += genDbd
genDbd_SRCS += genDbd.c ecDb.c bitMenu.c

#=============================

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += ecdr814Record ecdr814RXRecord ecdr814ChannelRecord ecdr814BoardRecord
#DBD += ecdr814RecCommon.dbd ecdr814RXFields.dbd ecdr814BoardFields.dbd  ecdr814ChannelFields.dbd  
DBD += ecdr814Menus.dbd


# <name>.dbd will be created from <name>Include.dbd
#DBD += ecdr814.dbd

#=============================

#PROD_RTEMS = ecdr814App
#LIBRARY_vxWorks += drvEcdr814
LIBRARY_RTEMS   += drvEcdr814
LIBRARY_Linux   += drvEcdr814

drvEcdr814_SRCS += drvrEcdr814.c regNodeOps.c dirOps.c bitMenu.c ecDb.c  ecdr814Lowlevel.c ecLookup.c
drvEcdr814_SRCS_DEFAULT += mygetopt_r.c
drvEcdr814_SRCS_RTEMS += -nil-
PROD_HOST_Linux += drvEcdr814Tst

drvEcdr814Tst_SRCS += drvrTst.c
drvEcdr814Tst_LIBS += drvEcdr814
#drvEcdr814Tst_LDFLAGS += -L$(TOP)/lib/$(T_A)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

../$(GENINC):
	mkdir $@

inc: ../$(GENINC) ../$(GENINC)/ecMenuDefs.h ../$(GENINC)/ecFastKeyDefs.h

../$(GENINC)/ecMenuDefs.h ../O.Common/ecdr814Menus.dbd: ../O.$(EPICS_HOST_ARCH)/genMenuHdr ../bitMenu.c
	echo '#ifndef BIT_MENU_HEADER_DEFS_H' > $@
	echo '#define BIT_MENU_HEADER_DEFS_H' >> $@
	echo '/* DONT EDIT THIS FILE, IT WAS AUTOMATICALLY GENERATED */' >>$@
	if  $< ../O.Common/ecdr814Menus.dbd >> $@ && echo '#endif' >>$@  ; then true ; else $(RM) $@; fi

../$(GENINC)/ecFastKeyDefs.h:../O.$(EPICS_HOST_ARCH)/genHeaders 
	if  $< -k > $@ ; then true ; else $(RM) $@; fi

../O.$(EPICS_HOST_ARCH)/genMenuHdr:
	$(MAKE) -C ../O.$(EPICS_HOST_ARCH)/ genMenuHdr

../O.$(EPICS_HOST_ARCH)/genHeaders: ../$(GENINC)/ecMenuDefs.h
	$(MAKE) -C ../O.$(EPICS_HOST_ARCH)/ genHeaders

clean::
	$(RMDIR) $(GENINC)
