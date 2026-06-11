threads_dep = dependency('threads')
zmq_dep = dependency('libzmq', required: true)
glfw3_dep = dependency('glfw3')

conf = configure_file(
    input: 'lcdsink_conf_def.h.in',
    output: 'lcdsink_conf_def.h',
    configuration: {
        'VERSION': meson.project_version(),
        'SRC_ROOT': meson.project_source_root()
    }
)

lib_path = '../lib'

glm_inc = include_directories(lib_path)

glad_path = lib_path + '/glad'
glad_inc = include_directories(glad_path + '/include')
glad_lib = static_library(
    'glad',
    sources : glad_path + '/src/glad.c',
    include_directories : glad_inc,
    c_args : [
        '-DIMGUI_IMPL_OPENGL_LOADER_GLAD',
        '-Wno-pedantic'
    ]
)

glfw_cxx_path = lib_path + '/glfw-cxx'
glfw_cxx_inc = include_directories(glfw_cxx_path + '/include')
glfw_cxx_sources = files(
    glfw_cxx_path + '/src/Context.cpp',
    glfw_cxx_path + '/src/Error.cpp',
    glfw_cxx_path + '/src/EventCallbacks.cpp',
    glfw_cxx_path + '/src/GammaRamp.cpp',
    glfw_cxx_path + '/src/Init.cpp',
    glfw_cxx_path + '/src/Monitor.cpp',
    glfw_cxx_path + '/src/Time.cpp',
    glfw_cxx_path + '/src/Version.cpp',
    glfw_cxx_path + '/src/VideoMode.cpp',
    glfw_cxx_path + '/src/WindowCallbacks.cpp',
    glfw_cxx_path + '/src/Window.cpp'
)
glfw_cxx_lib = static_library(
    'glfw-cxx',
    sources : glfw_cxx_sources,
    include_directories : glfw_cxx_inc
)

gloo_inc = include_directories(lib_path)

testbed_inc = include_directories(lib_path)

executable(
    'lcdsink',
    ['lcdsink_main.cpp', conf],
    include_directories : [
        glad_inc,
        gloo_inc,
        glfw_cxx_inc,
        testbed_inc
    ],
    dependencies : [
        glfw3_dep,
        threads_dep,
        zmq_dep
    ],
    link_whole : glfw_cxx_lib,
    link_with : [
        glad_lib
    ],
    install : false
)
