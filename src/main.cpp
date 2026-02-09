#include "app/Application.h"
#include "utils/Logger.h"
#include <iostream>

int main(int argc, char *argv[])
{
    setenv("PULSE_SERVER", "unix:/mnt/wslg/PulseServer", 0);
    setenv("SDL_AUDIODRIVER", "pulseaudio", 1);

    Logger::info("=================================");
    Logger::info("Music Player Application Starting");
    Logger::info("=================================");

    try
    {
        Application app;

        Logger::info("Initializing application...");
        if (!app.init())
        {
            Logger::error("Failed to initialize application");
            std::cerr << "Failed to initialize application. Check logs for details." << std::endl;
            return 1;
        }

        Logger::info("Application initialized successfully");

        Logger::info("Starting main loop...");
        app.run();

        Logger::info("Shutting down application...");
        app.shutdown();

        Logger::info("=================================");
        Logger::info("Music Player Application Exited");
        Logger::info("=================================");

        return 0;
    }
    catch (const std::exception &e)
    {
        Logger::error("Fatal error: " + std::string(e.what()));
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        Logger::error("Unknown fatal error occurred");
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}
