threads_dep = dependency('threads')
zmq_dep = dependency('libzmq')
glfw3_dep = dependency('glfw3')
epoxy_dep = dependency('epoxy')

conf = configure_file(
    input: 'lcdsink_conf_def.h.in',
    output: 'lcdsink_conf_def.h',
    configuration: {
        'VERSION': meson.project_version(),
        'SRC_ROOT': meson.project_source_root(),
    },
)

lib_path = '../lib'

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
    glfw_cxx_path + '/src/Window.cpp',
)
glfw_cxx_lib = static_library('glfw-cxx', sources: glfw_cxx_sources, include_directories: glfw_cxx_inc)

gloo_inc = include_directories(lib_path)

testbed_inc = include_directories(lib_path)

executable(
    'lcdsink',
    ['lcdsink_main.cpp', conf],
    include_directories: [glfw_cxx_inc, testbed_inc, gloo_inc],
    dependencies: [glfw3_dep, threads_dep, zmq_dep, epoxy_dep],
    link_whole: glfw_cxx_lib,
    cpp_args: ['-DGLFW_INCLUDE_NONE'],
    install: false,
)