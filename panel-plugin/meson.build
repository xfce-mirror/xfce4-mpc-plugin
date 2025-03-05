plugin_sources = [
  'xfce4-mpc-plugin.c',
  'xfce4-mpc-plugin.h',
  xfce_revision_h,
]

if not have_libmpd
  plugin_sources += [
    'simple-libmpd.c',
    'simple-libmpd.h',
  ]
endif

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_library(
  'mpc',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-mpc-plugin'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gtk,
    libmpd,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'xfce4-mpc-plugin.desktop.in',
  output: 'xfce4-mpc-plugin.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
