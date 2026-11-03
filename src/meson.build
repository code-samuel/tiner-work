rh_pim_sources = [
  'main.c',
  'rh_pim-application.c',
  'rh_pim-db.c',
  'rh_pim-ui.c',
]

# Tenta carregar do sistema local; se não encontrar, usa os wraps em subprojects/
gtk_dep = dependency('gtk4', fallback: ['gtk4', 'gtk_dep'])

# Busca por libmariadb ou mysqlclient no sistema ou compila via wrap
mysql_dep = dependency('libmariadb', 
  fallback: ['libmariadb', 'mariadb_dep'],
  required: false
)

if not mysql_dep.found()
  mysql_dep = dependency('mysqlclient', required: true)
endif

rh_pim_deps = [
  gtk_dep,
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
