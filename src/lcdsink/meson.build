glfw3_dep = dependency('glfw3', static: true)

lib_path = '../lib'
lib_include = include_directories('../lib')

glad_path = lib_path + '/glad'
glad_include = include_directories(glad_path + '/include')
glad_lib = static_library(
    'glad',
    sources : glad_path + '/src/glad.c',
    include_directories : glad_include,
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

gloo_path = lib_path + '/gloo'
gloo_inc = include_directories(gloo_path + '/include')
glm_inc = include_directories(lib_path)
gloo_sources = files(
    gloo_path + '/src/addon.cpp',
    gloo_path + '/src/Camera.cpp',
    gloo_path + '/src/ElementBufferObject.cpp',
    gloo_path + '/src/Framebuffer.cpp',
    gloo_path + '/src/Mesh.cpp',
    gloo_path + '/src/Program.cpp',
    gloo_path + '/src/Renderbuffer.cpp',
    gloo_path + '/src/Shader.cpp',
    gloo_path + '/src/Texture.cpp',
    gloo_path + '/src/VertexArrayObject.cpp',
    gloo_path + '/src/VertexBufferObject.cpp'
)
gloo_lib = static_library(
    'gloo',
    sources : gloo_sources,
    include_directories : [
        gloo_inc,
        glad_include,
        glm_inc
    ],
    cpp_args : [
        '-DGLM_ENABLE_EXPERIMENTAL'
    ]
)

testbed_inc = include_directories('../lib')
executable(
    'lcdsink',
    'lcdsink_main.cpp',
    include_directories : [
        glad_include,
        gloo_inc,
        glfw_cxx_inc,
        testbed_inc
    ],
    dependencies : [glfw3_dep],
    link_with : [
        glad_lib,
        glfw_cxx_lib,
        gloo_lib
    ],
    cpp_args : [
        '-Wall',
        '-Wextra',
        '-Wunused',
        '-Wformat',
        '-O3'
    ],
    link_args : ['-lpthread', '-lzmq'],
    install : false
)
