SRCS = drvrEcdr814.c regNodeOps.c dirOps.c

OBJS = $(SRCS:%.c=%.o)

CFLAGS=-g -DDIRSHELL -DDEBUG

tst: $(OBJS)
	$(CC) -o $@ $^

clean:
	$(RM) $(OBJS) tst

depend:
	gccmakedep $(SRCS)
# DO NOT DELETE
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
  drvrEcdr814.h regNodeOps.h ecdr814RegTable.c
regNodeOps.o: regNodeOps.c regNodeOps.h drvrEcdr814.h ecdrRegdefs.h
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
  /usr/include/gconv.h /usr/include/bits/stdio_lim.h drvrEcdr814.h
