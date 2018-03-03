{
  'targets': [
    {
      'target_name': 'gphoto2',
      'include_dirs': [
        "<!(node -e \"require('nan')\")"
      ],
      'sources': [
        'src/autodetect.cc',
        'src/binding.cc',
        'src/camera.cc',
        'src/camera_helpers.cc',
        'src/gphoto.cc'
      ],
      'link_settings': {
        'libraries': [
          '-lgphoto2',
          '-lgphoto2_port'
        ]
      },
      'cflags': [
        '--std=c++11'
      ],
      'cflags!': [
        '-fno-exceptions'
      ],
      'target_arch': 'x64',
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'OTHER_CPLUSPLUSFLAGS' : [
              '-std=c++11',
              '-stdlib=libc++'
            ]
          }
        }]
      ]
    }
  ]
}
