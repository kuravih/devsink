#ifndef __LCDMODULATOR_H__
#define __LCDMODULATOR_H__

#pragma once

#include "testbed/common.hpp"
#include "testbed/shmio_functions.hpp"
#include "kato/truetype.hpp"
#include "kato/log.hpp"
#include "link/zmq_link.hpp"
#include "lcdsink_def.h"
#include "lcdsink_path_def.h"
#include "toml11/toml.hpp"
#include "glm/glm.hpp"

#include <atomic>

// ---- lcdmodulator specific -----------------------------------------------------------------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "gloo/Texture.h"
#include "gloo/Program.h"
#include "gloo/Mesh.h"
#include "glad/glad.h"
#include "glfw-cxx/Window.hpp"
#include "glfw-cxx/Error.hpp"

// ---- screen vertices -----------------------------------------------------------------------------------------------
#define SCREEN_VERTICES                                                                                       \
    {                                                                                                         \
        gloo::Vertex{glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0), glm::vec3(0), glm::vec2(0.0f, 0.0f)},        \
            gloo::Vertex{glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0), glm::vec3(0), glm::vec2(1.0f, 0.0f)},     \
            gloo::Vertex{glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0), glm::vec3(0), glm::vec2(1.0f, 1.0f)},    \
            gloo::Vertex { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0), glm::vec3(0), glm::vec2(0.0f, 1.0f) } \
    }
// ---- screen indices ------------------------------------------------------------------------------------------------
#define SCREEN_INDICES \
    {                  \
        0, 1, 2,       \
        2, 3, 0}
// --------------------------------------------------------------------------------------------------------------------

volatile std::atomic<bool> busy{true};

// ====================================================================================================================
struct LcdModulator
{
    std::string name, serial;
    testbed::FrameArea<long> full; // 3*540, 2560
    testbed::Point<float> center;
    const long max_radius;
    float radius;
    uint8_t datatype;
    long port;
    std::vector<uint16_t> display_pixels; // 3*540*2560
    shmio::SharedMemory memory;           // 2*max_radius*2*max_radius
    shmio::Keyword *shm_radius, *shm_center_x, *shm_center_y;
    // ---- hardware --------------------------------------------------------------------------------------------------
    glfw::Window window;
    std::shared_ptr<gloo::Program> pProgram;
    std::shared_ptr<gloo::Mesh> pMesh;
    std::shared_ptr<gloo::Texture> pTexture;

    LcdModulator(const char *_name, const char *_serial, long _port, const testbed::Point<float> _center, const float _radius) : name(_name), serial(_serial), full({{0, 0}, {3 * LCDSINK_WIDTH, LCDSINK_HEIGHT}}), center(_center), max_radius(_radius), radius(_radius), datatype(_DATATYPE_UINT16), port(_port), display_pixels(3 * LCDSINK_WIDTH * LCDSINK_HEIGHT) {}
    void setup()
    {
        window.Hint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        window.Hint(GLFW_CONTEXT_VERSION_MINOR, 3);
        window.Hint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        std::list<glfw::Monitor> monitors = glfw::Monitor::GetMonitors();

        kato::log::cout << KATO_GREEN << "lcdmodulator.cpp::LcdModulator::setup_device() " << monitors.size() << " monitors found." << KATO_RESET << std::endl;

        std::list<glfw::Monitor>::iterator it = std::find_if(monitors.begin(), monitors.end(), [](const glfw::Monitor &_monitor)
                                                             { return _monitor.GetVideoMode().GetWidth() == LCDSINK_WIDTH && _monitor.GetVideoMode().GetHeight() == LCDSINK_HEIGHT; });
        if (it != monitors.end())
        {
            glfw::Monitor &monitor = *it;
            kato::log::cout << KATO_GREEN << "lcdmodulator.cpp::LcdModulator::setup_device() SLM monitor found. Running fullscreen [" << monitor.GetVideoMode().GetWidth() << "x" << monitor.GetVideoMode().GetHeight() << "]" << KATO_RESET << std::endl;
            window.Create(monitor.GetVideoMode().GetWidth(), monitor.GetVideoMode().GetHeight(), "SLM", monitor);
            window.SetAttrib(GLFW_DECORATED, false);
        }
        else
        {
            kato::log::cout << KATO_GREEN << "lcdmodulator.cpp::LcdModulator::setup_device() SLM monitor not found. Running windowed [" << LCDSINK_WIDTH << "x" << LCDSINK_HEIGHT << "]" << KATO_RESET << std::endl;
            window.Create(LCDSINK_WIDTH, LCDSINK_HEIGHT, "SLM");
        }

        window.MakeContextCurrent();

        gladLoadGL();

        glViewport(0, 0, LCDSINK_WIDTH, LCDSINK_HEIGHT);

        kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::SinkWorker() - loading vertex shader : " << LCDSINK_SRC_ROOT "/lcdsink/direct.vert" << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::SinkWorker() - loading fragment shader : " << LCDSINK_SRC_ROOT "/lcdsink/direct.frag" << KATO_RESET << std::endl;
        pProgram = std::make_shared<gloo::Program>(gloo::Shader(gloo::getFileContents(LCDSINK_SRC_ROOT "/lcdsink/direct.vert"), gloo::Shader::Type::Vertex), gloo::Shader(gloo::getFileContents(LCDSINK_SRC_ROOT "/lcdsink/direct.frag"), gloo::Shader::Type::Fragment));
        pMesh = std::make_shared<gloo::Mesh>(std::vector<gloo::Vertex>(SCREEN_VERTICES), std::vector<GLuint>(SCREEN_INDICES));
        pMesh->LinkPositionToLocation(0);
        pMesh->LinkTextureUVToLocation(1);

        pTexture = std::make_shared<gloo::Texture>(display_pixels.data(), LCDSINK_WIDTH, LCDSINK_HEIGHT, gloo::Texture::Type::UnsignedShort, gloo::Texture::InternalFormat::RGB, gloo::Texture::Format::RGB, gloo::Texture::Slot::slot00, gloo::Texture::Target::Texture2D);
        pProgram->Uniform("in_texture", *pTexture);
        pTexture->Bind();

        pProgram->Uniform("in_resolution", glm::vec2{full.size().width, full.size().height});
        pProgram->Uniform("in_center", glm::vec2{center.x, center.y});
        pProgram->Uniform("in_radius", radius);
    }
    void render_command(std::span<uint16_t> _command)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        pProgram->Uniform("in_center", glm::vec2{center.x, center.y});
        pProgram->Uniform("in_radius", radius);
        testbed::FrameSize framesize = {2 * max_radius, 2 * max_radius};
        testbed::Point top_left = {(long)(center.x - max_radius), (long)(center.y - max_radius)};
        testbed::inset(_command.data(), framesize, display_pixels.data(), top_left, full.size());
        pTexture->Update(display_pixels.data());
        pProgram->Activate();
        pMesh->Draw();
        window.SwapBuffers();
    }
    int openStream()
    {
        if (testbed::create_modulator_memory(memory, (serial + "_" LCDSINK_STREAM_STR).c_str(), full.size(), center, max_radius, shmio::DataType::UINT16, serial.c_str(), (std::pow(2, 16) - 1), port) == 0)
        {
            shm_radius = shmio::find_keyword(memory, "RADIUS");
            shm_radius->value.numf = radius;
            shm_center_x = shmio::find_keyword(memory, "CENTER.X");
            shm_center_y = shmio::find_keyword(memory, "CENTER.Y");
            shm_center_x->value.numf = center.x;
            shm_center_y->value.numf = center.y;
            return 0;
        }
        return -1;
    }
    int closeStream()
    {
        return shmio::close_shared_memory(memory);
    }
    void setCenter(const testbed::Point<float> &_center)
    {
        shm_center_x->value.numf = center.x = _center.x;
        shm_center_y->value.numf = center.y = _center.y;
    }
    void setRadius(const float _radius)
    {
        shm_radius->value.numf = radius = _radius;
    }
    ~LcdModulator() = default;
};
// ====================================================================================================================
void ListenWorker(LcdModulator &_modulator, ZMQLink &_link)
{
    kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::ListenWorker() Listen thread starting..." << KATO_RESET << std::endl;

    static std::string rxMessage;
    while (_link.isListening.load() && busy.load())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(LINK_SHORT_SLEEP_US));
        rxMessage = _link.Receive();
        if (rxMessage.size() > 0)
        {
            // kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::ListenWorker() rxMessage = " << rxMessage << KATO_RESET << std::endl;

            toml::value data = toml::parse_str(rxMessage);
            std::string sync = "";
            std::ostringstream txStream;
            std::string txMessage;

            try // [settings] radius = radius_value
            {
                float radius = data.at("settings").at("radius").as_floating();
                kato::log::cout << KATO_GREEN << "lcdmodulator.h::ListenWorker() radius = " << kato::function::StringPrintf("%.0f", radius) << KATO_RESET << std::endl;
                _modulator.setRadius(radius);
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"radius", _modulator.radius}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }

            try // [settings.nudge] x = amount, y = amount
            {
                int nudge_x = data.at("settings").at("nudge").at("x").as_integer();
                int nudge_y = data.at("settings").at("nudge").at("y").as_integer();
                kato::log::cout << KATO_GREEN << "lcdmodulator.h::ListenWorker() nudge center : (" << (int)_modulator.center.x << "," << (int)_modulator.center.y << ") by (" << nudge_x << "," << nudge_y << ")" << KATO_RESET << std::endl;
                _modulator.setCenter({_modulator.center.x + nudge_x, _modulator.center.y + nudge_y});
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"center", toml::table{{"x", _modulator.center.x}, {"y", _modulator.center.y}}}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }

            try // Settings = "sync"
            {
                std::string sync = data.at("settings").as_string();
                kato::log::cout << KATO_GREEN << "lcdmodulator.h::ListenWorker() syncing..." << KATO_RESET << std::endl;
                toml::value reply = toml::value{toml::table{{"settings", toml::table{{"radius", _modulator.radius}, {"center", toml::table{{"x", _modulator.center.x}, {"y", _modulator.center.y}}}}}}};
                txStream << reply << "\n";
                txMessage = txStream.str();
                _link.Send(txMessage);
                continue;
            }
            catch (const std::exception &)
            {
            }
        }
    }
    _link.isListening.store(false);

    kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::ListenWorker() Listen thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================
void SinkWorker(LcdModulator &_modulator)
{
    _modulator.setup();
    kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::SinkWorker() Source thread starting..." << KATO_RESET << std::endl;
    if (_modulator.openStream() == 0)
    {
        std::chrono::system_clock::time_point t0, t1;
        shmio::SharedStorage *storage = shmio::get_storage_ptr(_modulator.memory);
        shmio::Keyword *framerate = shmio::find_keyword(_modulator.memory, "FRMRATE");
        std::span<uint16_t> pixels = shmio::get_pixels_as<uint16_t>(_modulator.memory);

        // ------------------------------------------------------------------------------------------------------------------
        int width, height, nCh;
        kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::SinkWorker() - loading background image : " << LCDSINK_SRC_ROOT "/lcdsink/data/grid_gray16_1620_2560.png" << KATO_RESET << std::endl;
        uint16_t *imData = stbi_load_16(LCDSINK_SRC_ROOT "/lcdsink/data/grid_gray16_1620_2560.png", &width, &height, &nCh, 0);
        for (size_t i = 0; i < (size_t)width * height * nCh; ++i)
            _modulator.display_pixels[i] = imData[i];
        stbi_image_free(imData);
        _modulator.pTexture->Update(_modulator.display_pixels.data());
        // ------------------------------------------------------------------------------------------------------------------

        kato::log::cout << KATO_MAGENTA << "  - name : " << _modulator.memory.name << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - size : " << _modulator.memory.size << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - creationtime : " << kato::function::TimeStampString(0, "%Y%m%d.%H%M%S", ".", kato::function::timespec_to_time_point(storage->creationtime)) << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - lastaccesstime : " << kato::function::TimeStampString(0, "%Y%m%d.%H%M%S", ".", kato::function::timespec_to_time_point(storage->lastaccesstime)) << KATO_RESET << std::endl;

        kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::SinkWorker() - starting ..." << KATO_RESET << std::endl;
        while (busy.load() && !_modulator.window.ShouldClose()) // Main while loop
        {
            t0 = std::chrono::system_clock::now();

            // ==== begin critical section ============================================================================
            shmio::consumer_request_start(storage);

            _modulator.render_command(pixels);
            t1 = std::chrono::system_clock::now();
            framerate->value.numf = kato::function::delta_time_point_to_framerate(t0, t1);
            storage->lastaccesstime = kato::function::time_point_to_timespec(t1);
            kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::SinkWorker() - framerate = " << std::scientific << std::setprecision(5) << framerate->value.numf << KATO_RESET << std::flush;

            shmio::consumer_wait_for_ready(storage);
            // ==== end critical section ==============================================================================

            std::cout << "\r\33[2K";
        }

        // ==== begin critical section ================================================================================
        // Terminate shared state cleanly
        pthread_mutex_lock(&storage->mutex);
        pthread_cond_broadcast(&storage->ready_cond);
        pthread_cond_broadcast(&storage->request_cond);
        pthread_mutex_unlock(&storage->mutex);
        // ==== end critical section ==================================================================================

        _modulator.closeStream();
    }
    kato::log::cout << KATO_MAGENTA << "lcdmodulator.h::SinkWorker() Source thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================

#endif //__LCDMODULATOR_H__