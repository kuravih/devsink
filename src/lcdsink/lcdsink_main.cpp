#include "lcdmodulator.h"

#include <csignal>
#include <thread>

void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        busy.store(false);
        kato::log::cout << KATO_RED << "lcdsink_main.cpp::sigint_handler() Terminating stream ..." << KATO_RESET << std::endl;
    }
}

static void glfw_error_callback(int error, const char *description)
{
    std::cerr << "GLFW Error" << error << ":" << description << "\n";
}

int main()
{

    kato::log::cout << KATO_GREEN << "lcdsink_main.cpp::main() Starting " LCDSINK_STR " (" LCDSINK_VERSION_STR ")" << KATO_RESET << std::endl;

    glfw::Error::SetErrorCallback(glfw_error_callback);
    std::signal(SIGINT, sigint_handler);

    std::thread listen_thread, sink_thread;
    long port = 8102;

    testbed::Point<float> center = {818, 892};
    float radius = 180;
    LcdModulator modulator("LCD Modulator", "lcd001", port, center, radius);

    ZMQLink link(modulator.port);
    link.setupLink();
    link.isListening.store(true);

    listen_thread = std::thread(ListenWorker, std::ref(modulator), std::ref(link));
    sink_thread = std::thread(SinkWorker, std::ref(modulator));

    listen_thread.join();
    sink_thread.join();

    kato::log::cout << KATO_GREEN << "lcdsink_main.cpp::main() Stopping " LCDSINK_STR " (" LCDSINK_VERSION_STR ")" << KATO_RESET << std::endl;

    return 0;
}