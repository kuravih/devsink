threads_dep = dependency('threads')
zmq_dep = dependency('libzmq', required: true)
lib_include = include_directories('../lib')

conf = configure_file(
    input: 'stbsink_conf_def.h.in',
    output: 'stbsink_conf_def.h',
    configuration: {
        'VERSION': meson.project_version(),
        'LIB_KATO_DIR': join_paths(meson.project_source_root(), 'lib', 'kato')
    }
)

executable(
    'stbsink',
    ['stbsink_main.cpp', conf],
    include_directories : lib_include,
    dependencies : [
        threads_dep, 
        zmq_dep
    ],
    install : false
)
