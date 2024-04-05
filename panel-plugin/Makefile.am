plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libmpc.la

libmpc_la_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	@LIBXFCE4UI_CFLAGS@						\
	@LIBXFCE4PANEL_CFLAGS@

libmpc_la_LIBADD =							\
	@LIBXFCE4UI_LIBS@						\
	@LIBXFCE4PANEL_LIBS@

libmpc_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libmpc_la_SOURCES = xfce4-mpc-plugin.c xfce4-mpc-plugin.h
if USE_LIBMPD
libmpc_la_CFLAGS += @LIBMPD_CFLAGS@
libmpc_la_LIBADD += @LIBMPD_LIBS@
else
libmpc_la_SOURCES += simple-libmpd.c simple-libmpd.h
endif

# .desktop file
#
# We need to let intltool merge the translated fields, so we add a
# level of indirection: a <name>.desktop.in file.
# 
desktop_in_files = xfce4-mpc-plugin.desktop.in
	
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
$(desktop_DATA): $(desktop_in_files) Makefile
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(desktop_in_files)

DISTCLEANFILES = $(desktop_DATA)
