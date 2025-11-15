#include "stbmodulator.h"

#include <csignal>
#include <thread>

void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        busy.store(false);
        kato::log::cout << KATO_RED << "stbsink_main.cpp::sigint_handler() Terminating stream ..." << KATO_RESET << std::endl;
    }
}

int main()
{
    kato::log::cout << KATO_GREEN << "stbsink_main.cpp::main() Starting " STBSINK_STR " (" STBSINK_VERSION_STR ")" << KATO_RESET << std::endl;

    std::signal(SIGINT, sigint_handler);

    std::thread listen_thread, sink_thread;
    long port = 8101;

    testbed::Point<float> center = {200, 200};
    float radius = 100;
    StbModulator modulator("STB Modulator", "stb001", port, center, radius);

    ZMQLink link(modulator.port);
    link.setupLink();
    link.isListening.store(true);

    listen_thread = std::thread(ListenWorker, std::ref(modulator), std::ref(link));
    sink_thread = std::thread(SinkWorker, std::ref(modulator));

    listen_thread.join();
    sink_thread.join();

    kato::log::cout << KATO_GREEN << "stbsink_main.cpp::main() Stopping " STBSINK_STR " (" STBSINK_VERSION_STR ")" << KATO_RESET << std::endl;

    return 0;
}
