#ifndef __STBMODULATOR_H__
#define __STBMODULATOR_H__

#pragma once

#include "testbed/common.hpp"
#include "testbed/shmio_functions.hpp"
#include "kato/truetype.hpp"
#include "kato/log.hpp"
#include "link/zmq_link.hpp"
#include "stbsink_def.h"
#include "stbsink_path_def.h"
#include "toml11/toml.hpp"
#include "glm/glm.hpp"

#include <atomic>

volatile std::atomic<bool> busy{true};

// ====================================================================================================================
struct StbModulator
{
    std::string name, serial;
    testbed::FrameArea<long> full;
    testbed::Point<float> center;
    const long max_radius;
    float radius;
    uint8_t datatype;
    long port;
    shmio::SharedMemory memory; // 2*max_radius*2*max_radius
    shmio::Keyword *shm_radius, *shm_center_x, *shm_center_y;

    StbModulator(const char *_name, const char *_serial, long _port, const testbed::Point<float> _center, const float _radius) : name(_name), serial(_serial), full({{0, 0}, {640, 480}}), center(_center), max_radius(_radius), radius(_radius), datatype(_DATATYPE_UINT16), port(_port) {}
    int openStream()
    {
        if (testbed::create_modulator_memory(memory, (serial + "_" STBSINK_STREAM_STR).c_str(), full.size(), center, radius, shmio::DataType::UINT16, serial.c_str(), (std::pow(2, 16) - 1), port) == 0)
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
    ~StbModulator() = default;
};
// ====================================================================================================================
void ListenWorker(StbModulator &_modulator, ZMQLink &_link)
{
    kato::log::cout << KATO_MAGENTA << "stbmodulator.h::ListenWorker() Listen thread starting..." << KATO_RESET << std::endl;

    static std::string rxMessage;
    while (_link.isListening.load() && busy.load())
    {
        std::this_thread::sleep_for(std::chrono::microseconds(LINK_SHORT_SLEEP_US));
        rxMessage = _link.Receive();
        if (rxMessage.size() > 0)
        {
            // kato::log::cout << KATO_MAGENTA << "stbmodulator.h::ListenWorker() rxMessage = " << rxMessage << KATO_RESET << std::endl;

            toml::value data = toml::parse_str(rxMessage);
            std::string sync = "";
            std::ostringstream txStream;
            std::string txMessage;

            try // [settings] radius = radius_value
            {
                float radius = data.at("settings").at("radius").as_floating();
                kato::log::cout << KATO_GREEN << "stbmodulator.h::ListenWorker() radius = " << radius << KATO_RESET << std::endl;
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

    kato::log::cout << KATO_MAGENTA << "stbmodulator.h::ListenWorker() Listen thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================
void SinkWorker(StbModulator &_modulator)
{
    kato::log::cout << KATO_MAGENTA << "stbmodulator.h::SinkWorker() Source thread starting..." << KATO_RESET << std::endl;
    if (_modulator.openStream() == 0)
    {
        kato::TrueTypeFont ttf(STBSINK_SRC_ROOT "/lib/kato/ProggyClean.ttf", 12);
        std::chrono::system_clock::time_point t0, t1;
        shmio::SharedStorage *storage = shmio::get_storage_ptr(_modulator.memory);
        shmio::Keyword *framerate = shmio::find_keyword(_modulator.memory, "FRMRATE");
        std::span<uint16_t> pixels = shmio::get_pixels_as<uint16_t>(_modulator.memory);

        kato::log::cout << KATO_MAGENTA << "  - name : " << _modulator.memory.name << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - size : " << _modulator.memory.size << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - creationtime : " << kato::function::TimeStampString(0, "%Y%m%d.%H%M%S", ".", kato::function::timespec_to_time_point(storage->creationtime)) << KATO_RESET << std::endl;
        kato::log::cout << KATO_MAGENTA << "  - lastaccesstime : " << kato::function::TimeStampString(0, "%Y%m%d.%H%M%S", ".", kato::function::timespec_to_time_point(storage->lastaccesstime)) << KATO_RESET << std::endl;

        kato::log::cout << KATO_MAGENTA << "stbmodulator.h::SinkWorker() - starting ..." << KATO_RESET << std::endl;
        while (busy.load())
        {
            t0 = std::chrono::system_clock::now();
            // ---- begin critical section ----------------------------------------------------------------------------
            pthread_mutex_lock(&storage->mutex);

            storage->request_flag = true;
            pthread_cond_signal(&storage->request_cond);

            while (!storage->ready_flag && !storage->terminate)
                pthread_cond_wait(&storage->ready_cond, &storage->mutex);

            t1 = std::chrono::system_clock::now();
            framerate->value.numf = kato::function::delta_time_point_to_framerate(t0, t1);
            storage->lastaccesstime = kato::function::time_point_to_timespec(t1);
            kato::log::cout << KATO_MAGENTA << "stbmodulator.h::SinkWorker() - framerate = " << std::scientific << std::setprecision(5) << framerate->value.numf << KATO_RESET << std::flush;

            storage->ready_flag = false;

            pthread_mutex_unlock(&storage->mutex);
            // ---- end critical section ------------------------------------------------------------------------------
            usleep(1);
            std::cout << "\r\33[2K";
        }

        // ---- begin critical section --------------------------------------------------------------------------------
        // Terminate shared state cleanly
        pthread_mutex_lock(&storage->mutex);
        storage->terminate = true;
        pthread_cond_broadcast(&storage->ready_cond);
        pthread_cond_broadcast(&storage->request_cond);
        pthread_mutex_unlock(&storage->mutex);
        // ---- end critical section ----------------------------------------------------------------------------------

        kato::log::cout << KATO_MAGENTA << "stbmodulator.h::SinkWorker() - stop ..." << KATO_RESET << std::endl;

        _modulator.closeStream();
    }
    kato::log::cout << KATO_MAGENTA << "stbmodulator.h::SinkWorker() Source thread stopping..." << KATO_RESET << std::endl;
}
// ====================================================================================================================

#endif //__STBMODULATOR_H__