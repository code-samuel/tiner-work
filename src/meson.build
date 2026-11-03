rh_pim_sources = [
  'main.c',
  'rh_pim-application.c',
  'rh_pim-db.c',
  'rh_pim-ui.c',
]

# Cliente MySQL: mysqlclient.pc ou libmariadb.pc
mysql_dep = dependency('mysqlclient', required: false)
if not mysql_dep.found()
  mysql_dep = dependency('libmariadb')
endif

rh_pim_deps = [
  dependency('gtk4'),
  mysql_dep,
]

rh_pim_sources += gnome.compile_resources('rh_pim-resources',
  'rh_pim.gresource.xml',
  c_name: 'rh_pim'
)

executable('rh_pim', rh_pim_sources,
  dependencies: rh_pim_deps,
       install: true,
)
