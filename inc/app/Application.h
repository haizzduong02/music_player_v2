#ifndef APPLICATION_H
#define APPLICATION_H

#include "../interfaces/IPlaybackEngine.h"
#include "../interfaces/IFileSystem.h"
#include "../interfaces/IMetadataReader.h"
#include "../interfaces/IHardwareInterface.h"
#include "../interfaces/IPersistence.h"
#include "../interfaces/IViewFactory.h"

#include "../app/model/Library.h"
#include "../app/model/PlaylistManager.h"
#include "../app/model/History.h"
#include "../app/model/PlaybackState.h"

#include "../app/controller/LibraryController.h"
#include "../app/controller/PlaylistController.h"
#include "../app/controller/PlaybackController.h"
#include "../app/controller/HistoryController.h"
#include "../app/controller/USBController.h"

#include "../app/view/MainWindow.h"

#include <memory>
#include <vector>

// Forward declarations for SDL
struct SDL_Window;
struct SDL_Renderer;

/**
 * @file Application.h
 * @brief Main application class - Dependency Injection Container
 * 
 * Initializes all components, wires dependencies, and manages application lifecycle.
 * Follows Dependency Inversion Principle by injecting interfaces.
 */

/**
 * @brief Main application class
 * 
 * Acts as the Dependency Injection container and application coordinator.
 * Responsibilities:
 * - Initialize all services, models, controllers, and views
 * - Wire up Observer relationships
 * - Manage main event loop
 * - Handle application lifecycle
 */
class Application {
public:
    /**
     * @brief Constructor
     */
    Application();
    
    /**
     * @brief Destructor - cleanup all resources
     */
    ~Application();
    
    /**
     * @brief Initialize the application
     * Loads config, creates all components, wires dependencies
     * @return true if initialized successfully
     */
    bool init();
    
    /**
     * @brief Run the main application loop
     * Handles events, updates, and rendering
     */
    void run();
    
    /**
     * @brief Shutdown the application
     * Saves state and releases resources
     */
    void shutdown();
    
    /**
     * @brief Check if application should quit
     * @return true if should quit
     */
    bool shouldQuit() const {
        return shouldQuit_;
    }
    
private:
    // SDL resources
    SDL_Window* window_;
    SDL_Renderer* renderer_; // Kept for compatibility, though we use OpenGL context
    void* glContext_; // SDL_GLContext is a void* typedef
    
    // Services (concrete implementations - owned)
    std::unique_ptr<IPlaybackEngine> playbackEngine_;
    std::unique_ptr<IFileSystem> fileSystem_;
    std::unique_ptr<IMetadataReader> metadataReader_;
    std::unique_ptr<IHardwareInterface> hardwareInterface_;
    std::unique_ptr<IPersistence> persistence_;
    std::unique_ptr<IViewFactory> viewFactory_;
    
    // Models (owned)
    std::unique_ptr<Library> library_;
    std::unique_ptr<PlaylistManager> playlistManager_;
    std::unique_ptr<History> history_;
    std::unique_ptr<PlaybackState> playbackState_;
    
    // Controllers (owned)
    std::unique_ptr<LibraryController> libraryController_;
    std::unique_ptr<PlaylistController> playlistController_;
    std::unique_ptr<PlaybackController> playbackController_;
    std::unique_ptr<HistoryController> historyController_;
    std::unique_ptr<USBController> usbController_;
    
    // Views (owned)
    std::unique_ptr<MainWindow> mainWindow_;
    std::vector<std::unique_ptr<IView>> views_;
    
    bool shouldQuit_;
    bool initialized_;
    
    /**
     * @brief Initialize SDL and ImGui
     * @return true if successful
     */
    bool initSDL();
    
    /**
     * @brief Initialize ImGui
     * @return true if successful
     */
    bool initImGui();
    
    /**
     * @brief Create all services
     * @return true if successful
     */
    bool createServices();
    
    /**
     * @brief Create all models
     * @return true if successful
     */
    bool createModels();
    
    /**
     * @brief Create all controllers
     * @return true if successful
     */
    bool createControllers();
    
    /**
     * @brief Create all views
     * @return true if successful
     */
    bool createViews();
    
    /**
     * @brief Wire up Observer relationships
     * Connect models to views and controllers
     */
    void wireObservers();
    
    /**
     * @brief Load application state from disk
     * @return true if loaded successfully
     */
    bool loadState();
    
    /**
     * @brief Save application state to disk
     * @return true if saved successfully
     */
    bool saveState();
    
    /**
     * @brief Handle SDL events
     */
    void handleEvents();
    
    /**
     * @brief Update application state
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    /**
     * @brief Render frame
     */
    void render();
    
    /**
     * @brief Cleanup SDL and ImGui
     */
    void cleanupSDL();
    
    /**
     * @brief Cleanup ImGui
     */
    void cleanupImGui();
};

#endif // APPLICATION_H
