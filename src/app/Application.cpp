#include "../../inc/app/Application.h"
#include "../../inc/utils/Logger.h"
#include "../../inc/utils/Config.h"
#include "../../inc/service/TagLibMetadataReader.h"
#include "../../inc/service/LocalFileSystem.h"
#include "../../inc/service/JsonPersistence.h"
#include "../../inc/service/MpvPlaybackEngine.h"
#include "../../inc/app/view/ViewFactory.h"
#include "../../inc/app/view/MainWindow.h"

// SDL and ImGui includes
#include <SDL.h>
#include <SDL_opengl.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <thread>
#include <thread>
#include <chrono>
#include <fstream>

Application::Application() 
    : window_(nullptr),
      renderer_(nullptr),
      glContext_(nullptr),
      shouldQuit_(false),
      initialized_(false) {
    
    Logger::getInstance().info("Application instance created");
}

Application::~Application() {
    shutdown();
    Logger::getInstance().info("Application destroyed");
}

bool Application::init() {
    Logger::getInstance().info("Initializing application...");
    
    try {
        // Step 1: Initialize SDL Video (Audio handled by MPV)
        Logger::getInstance().info("Initializing SDL...");
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
            Logger::getInstance().error("Error: " + std::string(SDL_GetError()));
            return false;
        }
        Logger::getInstance().info("SDL Initialized");

        // Setup window
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
        
        window_ = SDL_CreateWindow("Music Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            Config::getInstance().getConfig().windowWidth, 
            Config::getInstance().getConfig().windowHeight, 
            window_flags);
        if (window_ == nullptr) {
            Logger::getInstance().error("Error: SDL_CreateWindow(): " + std::string(SDL_GetError()));
            return false;
        }

        glContext_ = SDL_GL_CreateContext(window_);
        SDL_GL_MakeCurrent(window_, glContext_);
        SDL_GL_SetSwapInterval(1); // Enable vsync
        Logger::getInstance().info("Window and GL Context Created");

        // Step 2: Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking (Requires Docking Branch)
        
        // Load Inter font (modern, clean UI font)
        // Try relative path from build directory first, then from project root
        // Check file existence first to avoid ImGui assertion crash
        const char* fontPath1 = "../assets/fonts/Inter-Variable.ttf";
        const char* fontPath2 = "assets/fonts/Inter-Variable.ttf";
        
        ImFont* mainFont = nullptr;
        
        // Helper to check file existence
        auto fileExists = [](const char* path) {
            std::ifstream f(path);
            return f.good();
        };

        ImFontConfig boldConfig;
        boldConfig.RasterizerMultiply = 1.6f; // Simulate bold (thicker glyphs)

        if (fileExists(fontPath1)) {
            mainFont = io.Fonts->AddFontFromFileTTF(fontPath1, 16.0f);
            // Index 1: Large Font
            io.Fonts->AddFontFromFileTTF(fontPath1, 22.0f);
            // Index 2: Large Bold Font
            io.Fonts->AddFontFromFileTTF(fontPath1, 22.0f, &boldConfig);
        } else if (fileExists(fontPath2)) {
            mainFont = io.Fonts->AddFontFromFileTTF(fontPath2, 16.0f);
            // Index 1: Large Font
            io.Fonts->AddFontFromFileTTF(fontPath2, 22.0f);
            // Index 2: Large Bold Font
            io.Fonts->AddFontFromFileTTF(fontPath2, 22.0f, &boldConfig);
        }
        if (!mainFont) {
            Logger::getInstance().warn("Could not load Inter font, using default");
            io.Fonts->AddFontDefault();
            io.Fonts->AddFontDefault(); // Index 1
            io.Fonts->AddFontDefault(); // Index 2
        }

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        
        // Apply custom color scheme
        ImGuiStyle& style = ImGui::GetStyle();
        
        // Background colors - Darker Teal/Green
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.20f, 0.18f, 1.0f); // Darker
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.15f, 0.14f, 1.0f);  // Darker for contrast
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.20f, 0.18f, 0.95f);
        
        // Frame colors
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.35f, 0.30f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.55f, 0.45f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f); // Bright Cyan
        
        // Button colors
        style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.35f, 0.30f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.50f, 0.45f, 1.0f); // More visible hover
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f);
        
        // Header colors (Selectables) - High Contrast
        style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.35f, 0.30f, 0.6f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.60f, 0.50f, 0.8f); // Brighter and less transparent
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f); // Bright active state
        
        // Slider colors
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.80f, 0.70f, 1.0f);
        
        // Text
        style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f); // Brighter disabled text
        
        // Separator
        style.Colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.60f, 0.50f, 0.5f);
        
        // Style tweaks
        style.WindowRounding = 0.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(8, 6);

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
        ImGui_ImplOpenGL3_Init("#version 130");

        // Step 3: Initialize Services, Models, Controllers...
        Logger::getInstance().info("Initializing services...");
        persistence_ = std::make_unique<JsonPersistence>();
        
        // Initialize Config singleton
        Config::getInstance().init(persistence_.get());
        if (!Config::getInstance().load()) {
            Logger::getInstance().warn("Failed to load configuration, using defaults");
        }
        // ... (rest of init remains checking Application.cpp content - wait, replace_file_content replaces blocks. I need to be careful with "Services, Models..." lines if they are not in TargetContent)
        metadataReader_ = std::make_unique<TagLibMetadataReader>();
        fileSystem_ = std::make_unique<LocalFileSystem>();
        playbackEngine_ = std::make_unique<MpvPlaybackEngine>();
        
        Logger::getInstance().info("Services initialized");
        
        Logger::getInstance().info("Initializing models...");
        library_ = std::make_unique<Library>(persistence_.get());
        playlistManager_ = std::make_unique<PlaylistManager>(persistence_.get());
        history_ = std::make_unique<History>(100);
        playbackState_ = std::make_unique<PlaybackState>();
        Logger::getInstance().info("Models initialized");
        
        Logger::getInstance().info("Initializing controllers...");
        libraryController_ = std::make_unique<LibraryController>(
            library_.get(),
            fileSystem_.get(),
            metadataReader_.get()
        );
        playlistController_ = std::make_unique<PlaylistController>(
            playlistManager_.get(),
            library_.get(),
            metadataReader_.get()
        );
        playbackController_ = std::make_unique<PlaybackController>(
            playbackEngine_.get(),
            playbackState_.get(),
            history_.get(),
            hardwareInterface_.get(),
            nullptr
        );
        historyController_ = std::make_unique<HistoryController>(
            history_.get()
        );
        usbController_ = std::make_unique<USBController>(
            fileSystem_.get()
        );
        Logger::getInstance().info("Controllers initialized");
        
        Logger::getInstance().info("Initializing views...");
        viewFactory_ = std::make_unique<ViewFactory>();
        
        // Use ViewFactory to create MainWindow via interface (or direct if factory returns pointer)
        IView* mainView = viewFactory_->createMainWindow();
        mainWindow_.reset(dynamic_cast<MainWindow*>(mainView)); // Safe cast as we know ViewFactory returns MainWindow* really
        // OR simply:
        if (!mainWindow_) mainWindow_ = std::make_unique<MainWindow>();

        // Set references in MainWindow
        mainWindow_->setLibraryView(dynamic_cast<LibraryView*>(viewFactory_->createLibraryView(
            libraryController_.get(), 
            library_.get(),
            playbackController_.get(),
            playlistManager_.get()
        )));
        mainWindow_->setPlaylistView(dynamic_cast<PlaylistView*>(viewFactory_->createPlaylistView(
            playlistController_.get(), 
            playlistManager_.get(),
            playbackController_.get()
        )));
        mainWindow_->setNowPlayingView(dynamic_cast<NowPlayingView*>(viewFactory_->createNowPlayingView(playbackController_.get(), playbackState_.get())));
        // Inject PlaylistManager into NowPlayingView
        if (auto* npView = dynamic_cast<NowPlayingView*>(mainWindow_->getNowPlayingView())) {
            npView->setPlaylistManager(playlistManager_.get());
        }

        mainWindow_->setHistoryView(dynamic_cast<HistoryView*>(viewFactory_->createHistoryView(
            historyController_.get(), 
            history_.get(),
            playbackController_.get()
        )));
        mainWindow_->setFileBrowserView(dynamic_cast<FileBrowserView*>(viewFactory_->createFileBrowserView(fileSystem_.get(), libraryController_.get())));
        
        // Connect FileBrowserView to LibraryView
        if (mainWindow_->getLibraryView() && mainWindow_->getFileBrowserView()) {
            mainWindow_->getLibraryView()->setFileBrowserView(mainWindow_->getFileBrowserView());
        }
        
        // Connect FileBrowserView to PlaylistView
        if (mainWindow_->getPlaylistView() && mainWindow_->getFileBrowserView()) {
            mainWindow_->getPlaylistView()->setFileBrowserView(mainWindow_->getFileBrowserView());
        }
        
        // Inject PlaylistController into FileBrowserView
        if (mainWindow_->getFileBrowserView() && playlistController_) {
            mainWindow_->getFileBrowserView()->setPlaylistController(playlistController_.get());
        }
        
        // Inject Playback references into MainWindow for unified controls
        mainWindow_->setPlaybackController(playbackController_.get());
        mainWindow_->setPlaybackState(playbackState_.get());
        
        Logger::getInstance().info("Views initialized");
        
        // Load data
        library_->load();
        
        initialized_ = true;
        Logger::getInstance().info("Application initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::getInstance().error("Application initialization failed: " + std::string(e.what()));
        return false;
    }
}

void Application::run() {
    if (!initialized_) {
        Logger::getInstance().error("Cannot run - application not initialized");
        return;
    }
    
    Logger::getInstance().info("Application running...");
    
    if (mainWindow_) {
        mainWindow_->show();
    }

    // Main application loop
    ImGuiIO& io = ImGui::GetIO();
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (!shouldQuit_) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Update playback controller
        if (playbackController_) {
            playbackController_->updateTime(deltaTime);
        }
        
        // Update video frame for rendering
        if (playbackEngine_) {
            playbackEngine_->updateVideo();
        }
        
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                shouldQuit_ = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window_))
                shouldQuit_ = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        // DockSpace not available in master branch
        // ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        // Render main window logic
        if (mainWindow_) {
            mainWindow_->handleInput(); // Can be empty if handled by ImGui
            mainWindow_->render();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Update and Render additional Platform Windows (for Docking) - Disabled for master branch
        /*
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }
        */

        SDL_GL_SwapWindow(window_);
        
        // Small delay to reduce CPU usage if needed, but VSync handles it mostly
        // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    Logger::getInstance().info("Application main loop ended");
}

void Application::shutdown() {
    if (!initialized_) {
        // Already shut down or never initialized
        return;
    }
    
    Logger::getInstance().info("Shutting down application...");
    
    // Save data
    Config::getInstance().save();
    if (library_) library_->save();
    
    // Release specific controllers and engine BEFORE destroying GL context
    if (playbackController_) {
        playbackController_->stop();
        // Give a tiny moment for the async stop to register if needed, 
        // though strictly not required if mpv handles it well.
    }
    
    playbackController_.reset();
    playbackEngine_.reset();
    
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // Cleanup SDL
    if (glContext_) {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
    
    // Cleanup members
    views_.clear();
    mainWindow_.reset();
    viewFactory_.reset();
    
    usbController_.reset();
    historyController_.reset();
    // playbackController_.reset(); // Already reset
    playlistController_.reset();
    libraryController_.reset();
    
    playbackState_.reset();
    history_.reset();
    playlistManager_.reset();
    library_.reset();
    
    hardwareInterface_.reset();
    // playbackEngine_.reset(); // Already reset
    fileSystem_.reset();
    metadataReader_.reset();
    persistence_.reset();
    
    initialized_ = false; // Mark as shut down
    
    Logger::getInstance().info("Application shutdown complete");
}
