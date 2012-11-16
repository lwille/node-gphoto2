{
  'targets': [
    {
      'target_name': 'gphoto2',
      'sources': [
        'src/autodetect.cc',
        'src/binding.cc',
        'src/camera.cc',
        'src/camera_helpers.cc',
        'src/gphoto.cc'
      ],
      'libraries': [
        '-lgphoto2',
        '-lgphoto2_port'
      ],
      'cflags!': ['-fno-exceptions'],
      'target_arch': 'x64',
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
          }
        }]
      ]
    }
  ]
}