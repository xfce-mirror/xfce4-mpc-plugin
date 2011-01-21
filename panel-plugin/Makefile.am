plugindir = $(libdir)/xfce4/panel/plugins
plugin_PROGRAMS = xfce4-mpc-plugin

xfce4_mpc_plugin_CFLAGS =						\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	@EXO_CFLAGS@							\
	@LIBXFCE4UI_CFLAGS@						\
	@LIBXFCE4PANEL_CFLAGS@

xfce4_mpc_plugin_LDADD =						\
	@EXO_LIBS@							\
	@LIBXFCE4UI_LIBS@						\
	@LIBXFCE4PANEL_LIBS@


xfce4_mpc_plugin_SOURCES = xfce4-mpc-plugin.c xfce4-mpc-plugin.h
if USE_LIBMPD
xfce4_mpc_plugin_CFLAGS += @LIBMPD_CFLAGS@
xfce4_mpc_plugin_LDADD += @LIBMPD_LIBS@
else
xfce4_mpc_plugin_SOURCES += simple-libmpd.c simple-libmpd.h
endif

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
# 
desktop_in_in_files = xfce4-mpc-plugin.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
	
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@PLUGIN_PATH@^$(libdir)/xfce4/panel/plugins^" 	\
		$< > $@
	
EXTRA_DIST =								\
	$(desktop_in_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA) $(desktop_in_files)

