#include "lcdmodulator.h"

#include <csignal>
#include <thread>
#include <getopt.h>
#include <pthread.h>

void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        busy.store(false);
        if (g_storage)
        {
            g_storage->has_response = true;
            pthread_cond_broadcast(&g_storage->has_response_cond);
        }
        if (g_link)
            g_link->isListening.store(false);
        kato::log::cout << KATO_RED << "lcdsink_main.cpp::sigint_handler() Terminating stream ..." << KATO_RESET << std::endl;
    }
}

static void glfw_error_callback(int error, const char *description)
{
    std::cerr << "GLFW Error" << error << ":" << description << "\n";
}

int main(int argc, char *argv[])
{
    kato::log::cout << KATO_GREEN << "lcdsink_main.cpp::main() Starting " LCDSINK_STR " (" LCDSINK_VER_STR ")" << KATO_RESET << std::endl;

    glfw::Error::SetErrorCallback(glfw_error_callback);
    std::signal(SIGINT, sigint_handler);

    std::thread listen_thread, sink_thread;

    // defaults
    long port = 8102;
    std::string serial = "lcd001";
    testbed::Point<float> center = {817, 898};
    float radius = 160;

    // CLI args
    bool set_port = false, set_center = false, set_radius = false;

    static struct option long_options[] = {
        {"port", required_argument, nullptr, 'p'},
        {"center", required_argument, nullptr, 'c'},
        {"radius", required_argument, nullptr, 'r'},
        {nullptr, 0, nullptr, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "p:c:r:", long_options, nullptr)) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = std::stol(optarg);
            set_port = true;
            break;
        case 'r':
            radius = std::stof(optarg);
            set_radius = true;
            break;
        case 'c':
        {
            float cx, cy;
            if (std::sscanf(optarg, "%f,%f", &cx, &cy) == 2)
            {
                center = {cx, cy};
                set_center = true;
            }
            break;
        }
        default:
            break;
        }
    }

    // shared memory fallback for unset parameters
    if (!set_port || !set_center || !set_radius)
    {
        shmio::SharedMemory prev;
        if (shmio::open_shared_memory(prev, (serial + "_" LCDSINK_STR).c_str()) == 0)
        {
            shmio::Keyword *kw;
            if (!set_port && (kw = shmio::find_keyword(prev, "PORT")))
                port = kw->value.numl;
            if (!set_radius && (kw = shmio::find_keyword(prev, "RADIUS")))
                radius = kw->value.numf;
            if (!set_center)
            {
                shmio::Keyword *cx = shmio::find_keyword(prev, "CENTER.X");
                shmio::Keyword *cy = shmio::find_keyword(prev, "CENTER.Y");
                if (cx && cy)
                    center = {cx->value.numf, cy->value.numf};
            }
            shmio::close_shared_memory(prev);
        }
    }

    LcdModulator modulator("LCD Modulator", serial.c_str(), port, center, radius);

    ZMQLink link(modulator.port);
    g_link = &link;
    link.setupLink();
    link.isListening.store(true);

    listen_thread = std::thread(ListenWorker, std::ref(modulator), std::ref(link));
    sink_thread = std::thread(SinkWorker, std::ref(modulator));

    listen_thread.join();
    sink_thread.join();

    kato::log::cout << KATO_GREEN << "lcdsink_main.cpp::main() Stopping " LCDSINK_STR " (" LCDSINK_VER_STR ")" << KATO_RESET << std::endl;

    return 0;
}
