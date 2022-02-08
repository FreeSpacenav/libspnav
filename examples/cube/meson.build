cc = meson.get_compiler('c')
executable('cube',
           'cube.c',
           'vmath.c',
           dependencies: [
             dependency('x11'),
             dependency('gl'),
             dependency('glu'),
             cc.find_library('m'),
             cc.find_library('pthread')
           ],
           link_with: lib,
           include_directories: include_directories('../..'),
           install : true)
