executable('simple',
           'simple.c',
           link_with: lib,
           dependencies: [
             dependency('x11'),
             cc.find_library('pthread')
           ],
           include_directories: include_directories('../../'),
           c_args: '-DBUILD_X11',
           install : true)

executable('simple_af_unix',
           'simple.c',
           link_with: lib,
           include_directories: include_directories('../../'),
           dependencies: [
             dependency('x11'),
             cc.find_library('pthread')
           ],
           c_args: '-DBUILD_AF_UNIX',
           install : true)
