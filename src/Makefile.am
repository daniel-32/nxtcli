bin_PROGRAMS = nxtcli
nxtcli_SOURCES = nxtcli.c cli.c cli.h builtincommands.c builtincommands.h nxtcommands.c nxtcommands.h
nxtcli_CFLAGS = $(LIBXML_CFLAGS) -DHERE=\"${pkgdatadir}\"
nxtcli_LDADD = $(LIBXML_LIBS) -lreadline -lnxtbt
