# GSS daemons & tools Makefile

AM_CFLAGS := -fPIC \
	     -D_LARGEFILE64_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DLUSTRE_UTILS=1 \
	     -D_GNU_SOURCE

sbin_PROGRAMS = l_idmap

if GSS_KEYRING
sbin_PROGRAMS += lsvcgssd lgss_keyring
if GSS_SSK
sbin_PROGRAMS += lgss_sk
endif
endif

COMMON_SRCS = \
        context.c \
        context_lucid.c \
        context_mit.c \
        context_heimdal.c \
        context_spkm3.c \
        gss_util.c \
        gss_oids.c \
        err_util.c \
        lsupport.c \
        \
        context.h \
        err_util.h \
        gss_oids.h \
        gss_util.h \
        lsupport.h \
        write_bytes.h

if GSS_SSK
COMMON_SRCS += sk_utils.h sk_utils.c
endif

lsvcgssd_SOURCES = \
        $(COMMON_SRCS) \
        cacheio.c \
        svcgssd.c \
        svcgssd_main_loop.c \
        svcgssd_mech2file.c \
        svcgssd_proc.c \
        cacheio.h \
        svcgssd.h

lsvcgssd_CFLAGS = $(AM_CFLAGS) $(CFLAGS) $(KRBCFLAGS)
lsvcgssd_LDADD = $(top_builddir)/lustre/utils/liblustreapi.la $(GSSAPI_LIBS) $(KRBLIBS) -lkeyutils -lm
lsvcgssd_LDFLAGS = $(KRBLDFLAGS)
if GSS_SSK
lsvcgssd_LDADD += -lcrypto -lssl
endif

l_idmap_SOURCES = \
	l_idmap.c \
	lsupport.c \
	err_util.c \
	\
	lsupport.h

l_idmap_LDADD = $(top_builddir)/lustre/utils/liblustreapi.la $(KRBLIBS)

lgss_keyring_SOURCES = \
	lgss_keyring.c \
	context.c \
	context_lucid.c \
	context_mit.c \
	context_heimdal.c \
	lgss_krb5_utils.c \
	lgss_null_utils.c \
	lgss_utils.c \
	lsupport.c \
	err_util.c \
	lgss_krb5_utils.h \
	lgss_utils.h \
	err_util.h \
	lsupport.h

lgss_keyring_CFLAGS = $(AM_CFLAGS) $(CFLAGS) $(KRBCFLAGS) -D _NEW_BUILD_
lgss_keyring_LDADD = $(top_builddir)/lustre/utils/liblustreapi.la $(GSSAPI_LIBS) $(KRBLIBS) -lm -lkeyutils
lgss_keyring_LDFLAGS = $(KRBLDFLAGS)

if GSS_SSK
lgss_keyring_SOURCES += sk_utils.c sk_utils.h lgss_sk_utils.c
lgss_keyring_LDADD += -lcrypto -lssl

lgss_sk_SOURCES = \
	lgss_sk.c \
	err_util.c \
	sk_utils.c \
	sk_utils.h

lgss_sk_CFLAGS = $(AM_CFLAGS) $(CFLAGS) $(KRBCFLAGS)
lgss_sk_LDADD = $(top_builddir)/lustre/utils/liblustreapi.la $(GSSAPI_LIBS) $(KRBLIBS) -lcrypto -lssl -lm -lkeyutils
lgss_sk_LDFLAGS = $(KRBLDFLAGS)
endif

EXTRA_DIST =
