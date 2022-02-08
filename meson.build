project('libspnav', 'c',
  version : run_command(['git', 'describe', '--tags'], capture : true, check : true).stdout().strip(),
  default_options : ['warning_level=3'])

conf_data = configuration_data()
has_x11 = get_option('x11')
conf_data.set('USE_X11', has_x11)
if (!has_x11)
  message('WARNING: you have disabled the X11 interface, the resulting library won't be compatible with the proprietary 3Dconnexion daemon (3dxserv)!'
endif

configure_file(output : 'spnav_config.h',
               configuration : conf_data)

lib = library('spnav',
           'spnav_magellan.c',
           'spnav.c',
           dependencies: [dependency('x11')],
           install : true)

install_headers('spnav.h', subdir: 'spnav')

if get_option('examples')
  subdir('examples')
endif
