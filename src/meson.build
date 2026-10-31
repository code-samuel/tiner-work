rh_pim_sources = [
  'main.c',
  'rh_pim-application.c',

]

rh_pim_deps = [
  dependency('gtk4'),
]

rh_pim_sources += gnome.compile_resources('rh_pim-resources',
  'rh_pim.gresource.xml',
  c_name: 'rh_pim'
)

executable('rh_pim', rh_pim_sources,
  dependencies: rh_pim_deps,
       install: true,
)
