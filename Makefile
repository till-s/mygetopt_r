LIBSRCS = drvrEcdr814.c regNodeOps.c dirOps.c bitMenu.c ecDb.c  ecdr814Lowlevel.c
SRCS = drvrTst.c genHeaders.c $(LIBSRCS)

OBJS = $(SRCS:%.c=%.o)
LIBOBJS = $(LIBSRCS:%.c=%.o)

CFLAGS=-g -O2 -DDIRSHELL -DDEBUG

HOSTCC = gcc

drvrTst: drvrTst.o $(LIBOBJS)
	$(CC) $(CFLAGS) -o $@ $^

drvr:	$(LIBOBJS)
	$(CC) $(CFLAGS) -o $@ $^

fastKeyDefs.h:	ecdr814RegTable.c
	if ! ./genHeaders -k > $@ ; then $(RM) $@; fi

menuDefs.h:	bitMenu.c 
	echo '#ifndef BIT_MENU_HEADER_DEFS_H' > $@
	echo '#define BIT_MENU_HEADER_DEFS_H' >> $@
	echo '/* DONT EDIT THIS FILE, IT WAS AUTOMATICALLY GENERATED */' >>$@
	if  ! ./genHeaders -m >> $@ || ! echo '#endif' >>$@  ; then $(RM) $@; fi

genHeaders: genHeaders.o ecDb.o bitMenu.o ecdrRegTable.c
	$(CC) -o $@ $^

includes: fastKeyDefs.h

clean:
	$(RM) $(OBJS) tst fastKeyDefs.h

depend: includes
	gccmakedep $(SRCS)
# DO NOT DELETE
drvrTst.o: drvrTst.c /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/alloca.h /usr/include/string.h /usr/include/assert.h \
  drvrEcdr814.h ecErrCodes.h ecdrRegdefs.h ecFastKeys.h fastKeyDefs.h
genHeaders.o: genHeaders.c /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/alloca.h /usr/include/string.h /usr/include/unistd.h \
  /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
  /usr/include/getopt.h /usr/include/assert.h drvrEcdr814.h ecErrCodes.h \
  bitMenu.h
drvrEcdr814.o: drvrEcdr814.c /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/alloca.h /usr/include/string.h /usr/include/assert.h \
  drvrEcdr814.h ecErrCodes.h bitMenu.h ecdrRegdefs.h ecFastKeys.h \
  fastKeyDefs.h
regNodeOps.o: regNodeOps.c regNodeOps.h drvrEcdr814.h ecErrCodes.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h bitMenu.h \
  ecdrRegdefs.h /usr/include/assert.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
dirOps.o: dirOps.c /usr/include/stdlib.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/sys/types.h /usr/include/bits/types.h \
  /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h \
  /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/alloca.h dirOps.h /usr/include/stdio.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/libio.h /usr/include/_G_config.h /usr/include/wchar.h \
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h drvrEcdr814.h \
  ecErrCodes.h bitMenu.h ecFastKeys.h fastKeyDefs.h
bitMenu.o: bitMenu.c bitMenu.h drvrEcdr814.h ecErrCodes.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/string.h /usr/include/features.h /usr/include/sys/cdefs.h \
  /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/include/assert.h
ecDb.o: ecDb.c /usr/include/stdio.h /usr/include/features.h \
  /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stddef.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h \
  /usr/include/bits/types.h /usr/include/bits/pthreadtypes.h \
  /usr/include/bits/sched.h /usr/include/libio.h /usr/include/_G_config.h \
  /usr/include/wchar.h /usr/include/gconv.h /usr/include/bits/stdio_lim.h \
  /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h \
  /usr/include/endian.h /usr/include/bits/endian.h \
  /usr/include/sys/select.h /usr/include/bits/select.h \
  /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
  /usr/include/alloca.h /usr/include/string.h /usr/include/assert.h \
  drvrEcdr814.h ecErrCodes.h ecdr814RegTable.c
ecdr814Lowlevel.o: ecdr814Lowlevel.c drvrEcdr814.h ecErrCodes.h \
  /usr/lib/gcc-lib/i386-redhat-linux/2.96/include/stdarg.h ecdrRegdefs.h \
  ecFastKeys.h fastKeyDefs.h
