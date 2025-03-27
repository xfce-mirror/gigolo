gigolo_generated_sources = gnome.compile_resources(
  'gigolo_ui',
  'gigolo.gresource.xml',
  export: false,
)

gigolo_sources = [
  gigolo_generated_sources,
  'backendgvfs.c',
  'backendgvfs.h',
  'bookmark.c',
  'bookmark.h',
  'bookmarkdialog.c',
  'bookmarkdialog.h',
  'bookmarkeditdialog.c',
  'bookmarkeditdialog.h',
  'bookmarkpanel.c',
  'bookmarkpanel.h',
  'browsenetworkpanel.c',
  'browsenetworkpanel.h',
  'common.c',
  'common.h',
  'main.c',
  'main.h',
  'menubuttonaction.c',
  'menubuttonaction.h',
  'mountdialog.c',
  'mountdialog.h',
  'preferencesdialog.c',
  'preferencesdialog.h',
  'settings.c',
  'settings.h',
  'window.c',
  'window.h',
]

gigolo = executable(
  'gigolo',
  gigolo_sources,
  sources: xfce_revision_h,
  dependencies: [
    glib,
    gtk,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
