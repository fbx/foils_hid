project(
  'foils_hid',
  'c',
  version: '0.3',
  license: 'MIT',
  default_options: [
    'warning_level=2',
    'werror=true',
  ],
)

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments([
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Wno-unused-parameter',
]), language: 'c')

math_dep = cc.find_library('m')

ela_dep = dependency('ela', fallback: ['libela', 'ela_dep'])
rudp_dep = dependency('rudp', fallback: ['librudp', 'rudp_dep'])

foils_files = []
foils_deps = [
  ela_dep,
  rudp_dep,
]

subdir('include')
subdir('src')

foils_inc = include_directories('include')

lib_foils = library(
  meson.project_name(), foils_files,
  version: meson.project_version(),
  dependencies: foils_deps,
  include_directories: [foils_inc],
  install: true,
)

foils_dep = declare_dependency(
  link_with: lib_foils,
  dependencies: foils_deps,
  include_directories: foils_inc,
)

if get_option('tests')
  subdir('test')
endif

pkgconfig = import('pkgconfig')
pkgconfig.generate(lib_foils,
  version: meson.project_version(),
  filebase: meson.project_name(),
  name: meson.project_name(),
  description: 'HID device UDP client library',
)
