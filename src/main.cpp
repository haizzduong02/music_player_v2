#include "app/Application.h"
#include "utils/Logger.h"
#include <iostream>

/**
 * @file main.cpp
 * @brief Entry point for the Music Player application
 */

int main(int argc, char* argv[]) {
    // WSL Audio Fix: Explicitly set PULSE_SERVER if not set
    // Check if /mnt/wslg/PulseServer exists is implied by typical WSLg setup
    setenv("PULSE_SERVER", "unix:/mnt/wslg/PulseServer", 0); // 0 = do not overwrite if already set
    
    // Force SDL to use PulseAudio as well
    setenv("SDL_AUDIODRIVER", "pulseaudio", 1);
    
    // Initialize logger
    Logger::info("=================================");
    Logger::info("Music Player Application Starting");
    Logger::info("=================================");
    
    try {
        // Create application instance
        Application app;
        
        // Initialize all components
        Logger::info("Initializing application...");
        if (!app.init()) {
            Logger::error("Failed to initialize application");
            std::cerr << "Failed to initialize application. Check logs for details." << std::endl;
            return 1;
        }
        
        Logger::info("Application initialized successfully");
        
        // Run main loop
        Logger::info("Starting main loop...");
        app.run();
        
        // Shutdown gracefully
        Logger::info("Shutting down application...");
        app.shutdown();
        
        Logger::info("=================================");
        Logger::info("Music Player Application Exited");
        Logger::info("=================================");
        
        return 0;
        
    } catch (const std::exception& e) {
        Logger::error("Fatal error: " + std::string(e.what()));
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
        
    } catch (...) {
        Logger::error("Unknown fatal error occurred");
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}
