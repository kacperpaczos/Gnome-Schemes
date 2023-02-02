schemes_sources = [
  'main.c',
  'schemes-color.c',
  'schemes-color-row.c',
  'schemes-scheme.c',
  'schemes-style.c',
  'schemes-style-row.c',
  'schemes-window.c',
  'schemes-application.c',
]

schemes_deps = [
  cc.find_library('m', required: false),
  dependency('gtk4'),
  dependency('gtksourceview-5'),
  dependency('libadwaita-1', version: '>= 1.2'),
  dependency('libpanel-1'),
]

build_ident_h = vcs_tag(
   command: ['git', 'describe'],
  fallback: meson.project_version(),
     input: 'build-ident.h.in',
    output: 'build-ident.h',
)

gnome = import('gnome')

schemes_sources += gnome.compile_resources('schemes-resources',
  'schemes.gresource.xml',
  c_name: 'schemes'
)

executable('schemes', schemes_sources,
  dependencies: schemes_deps,
  install: true,
)
