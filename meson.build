project('schemes', 'c',
          version: '0.2.0',
    meson_version: '>= 0.59.0',
  default_options: ['warning_level=2', 'c_std=gnu11'],
)

app_id = 'me.hergert.Schemes'
version_split = meson.project_version().split('.')
major_version = version_split[0].to_int()
minor_version = version_split[1].to_int()

i18n = import('i18n')
gnome = import('gnome')
cc = meson.get_compiler('c')

is_development = get_option('development')

if is_development
  app_id = '@0@.Devel'.format(app_id)
endif

config_h = configuration_data()
config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'schemes')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('PACKAGE_WEBSITE', 'https://gitlab.gnome.org/chergert/schemes')
config_h.set_quoted('PACKAGE_ISSUE_URL', 'https://gitlab.gnome.org/chergert/schemes/-/issues/new')
config_h.set_quoted('PACKAGE_ICON_NAME', app_id)
config_h.set10('DEVELOPMENT', is_development)

configure_file(
  output: 'config.h',
  configuration: config_h,
)

global_c_args = []
test_c_args = [
  '-Wcast-align',
  '-Werror=address',
  '-Werror=array-bounds',
  '-Werror=empty-body',
  '-Werror=implicit',
  '-Werror=implicit-function-declaration',
  '-Werror=init-self',
  '-Werror=int-conversion',
  '-Werror=int-to-pointer-cast',
  '-Werror=main',
  '-Werror=misleading-indentation',
  '-Werror=missing-braces',
  '-Werror=missing-include-dirs',
  '-Werror=nonnull',
  '-Werror=overflow',
  '-Werror=parenthesis',
  '-Werror=pointer-arith',
  '-Werror=pointer-to-int-cast',
  '-Werror=return-type',
  '-Werror=sequence-point',
  '-Werror=shadow',
  '-Werror=strict-prototypes',
  '-Werror=trigraphs',
  '-Werror=undef',
  '-Werror=write-strings',
  '-Wformat-nonliteral',
  ['-Werror=format-security', '-Werror=format=2' ],
  '-Wignored-qualifiers',
  '-Wincompatible-pointer-types',
  '-Wimplicit-function-declaration',
  '-Wlogical-op',
  '-Wmissing-format-attribute',
  '-Wmissing-include-dirs',
  '-Wmissing-noreturn',
  '-Wnested-externs',
  '-Wno-cast-function-type',
  '-Wno-missing-field-initializers',
  '-Wno-sign-compare',
  '-Wno-unused-parameter',
  '-Wno-typedef-redefinition',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wundef',
  '-Wuninitialized',
  '-Wunused',
  '-fno-strict-aliasing',
]

foreach arg: test_c_args
  if cc.has_multi_arguments(arg)
    global_c_args += arg
  endif
endforeach

add_project_arguments(global_c_args, language: 'c')

add_project_arguments([
  '-I' + meson.project_build_root(),
], language: 'c')

subdir('data')
subdir('src')
subdir('po')

gnome.post_install(
    gtk_update_icon_cache: true,
  update_desktop_database: true,
     glib_compile_schemas: true,
)
