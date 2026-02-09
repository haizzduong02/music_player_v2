#include "app/Application.h"
#include "app/view/MainWindow.h"
#include "app/view/ViewFactory.h"
#include "service/JsonPersistence.h"
#include "service/LocalFileSystem.h"
#include "service/MpvPlaybackEngine.h"
#include "service/MpvPlaybackEngine.h"
#include "service/MpvMetadataReader.h"
#include "service/TagLibMetadataReader.h"
#include "service/HybridMetadataReader.h"
#include "utils/Config.h"
#include "utils/Logger.h"
#include "hal/S32K144Interface.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <chrono>
#include <fstream>
#include <imgui.h>
#include <thread>

Application::Application()
    : window_(nullptr), renderer_(nullptr), glContext_(nullptr), shouldQuit_(false), initialized_(false), headless_(false)
{
    Logger::info("Application instance created");
}

Application::~Application()
{
    shutdown();
    Logger::info("Application destroyed");
}

bool Application::init(bool headless)
{
    headless_ = headless;
    Logger::info("Initializing application" + std::string(headless_ ? " in headless mode" : "") + "...");

    try
    {
        if (!headless_)
        {
            if (!initSDL())
                return false;
            if (!initImGui())
                return false;
        }

        if (!createServices())
            return false;
        if (!createModels())
            return false;
        if (!createControllers())
            return false;
        if (!createViews())
            return false;

        wireObservers();

        if (!loadState())
        {
            Logger::warn("Failed to load application state");
        }

        // Verify and refresh library to ensure data integrity and fix metadata issues
        if (libraryController_) 
        {
            Logger::info("Verifying library integrity...");
            libraryController_->verifyLibrary();
            
            Logger::info("Refreshing library metadata...");
            libraryController_->refreshLibrary();
        }

        initialized_ = true;
        Logger::info("Application initialized successfully");
        return true;
    }
    catch (const std::exception &e)
    {
        Logger::error("Application initialization failed: " + std::string(e.what()));
        return false;
    }
}

bool Application::initSDL()
{
    Logger::info("Initializing SDL...");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        Logger::error("Error: " + std::string(SDL_GetError()));
        return false;
    }
    Logger::info("SDL Initialized");

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

    window_ = SDL_CreateWindow("Music Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               Config::getInstance().getConfig().windowWidth,
                               Config::getInstance().getConfig().windowHeight, window_flags);
    if (window_ == nullptr)
    {
        Logger::error("Error: SDL_CreateWindow(): " + std::string(SDL_GetError()));
        return false;
    }

    glContext_ = SDL_GL_CreateContext(window_);
    SDL_GL_MakeCurrent(window_, glContext_);
    SDL_GL_SetSwapInterval(1);
    Logger::info("Window and GL Context Created");
    return true;
}

bool Application::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    const char *fontPath1 = "../assets/fonts/Inter-Variable.ttf";
    const char *fontPath2 = "assets/fonts/Inter-Variable.ttf";

    ImFont *mainFont = nullptr;
    auto fileExists = [](const char *path)
    {
        std::ifstream f(path);
        return f.good();
    };

    ImFontConfig boldConfig;
    boldConfig.RasterizerMultiply = 1.6f;

    if (fileExists(fontPath1))
    {
        mainFont = io.Fonts->AddFontFromFileTTF(fontPath1, 16.0f);
        io.Fonts->AddFontFromFileTTF(fontPath1, 22.0f);
        io.Fonts->AddFontFromFileTTF(fontPath1, 22.0f, &boldConfig);
    }
    else if (fileExists(fontPath2))
    {
        mainFont = io.Fonts->AddFontFromFileTTF(fontPath2, 16.0f);
        io.Fonts->AddFontFromFileTTF(fontPath2, 22.0f);
        io.Fonts->AddFontFromFileTTF(fontPath2, 22.0f, &boldConfig);
    }
    if (!mainFont)
    {
        Logger::warn("Could not load Inter font, using default");
        io.Fonts->AddFontDefault();
        io.Fonts->AddFontDefault();
        io.Fonts->AddFontDefault();
    }

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.20f, 0.18f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.15f, 0.14f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.20f, 0.18f, 0.95f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.35f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.55f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.35f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.50f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.35f, 0.30f, 0.6f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.60f, 0.50f, 0.8f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.70f, 0.60f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.80f, 0.70f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.60f, 0.50f, 0.5f);

    style.WindowRounding = 0.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);

    ImGui_ImplSDL2_InitForOpenGL(window_, glContext_);
    ImGui_ImplOpenGL3_Init("#version 130");
    return true;
}

bool Application::createServices()
{
    persistence_ = std::make_unique<JsonPersistence>();
    Config::getInstance().init(persistence_.get());
    if (!Config::getInstance().load())
    {
        Logger::warn("Failed to load configuration, using defaults");
    }
    auto tagLibReader = std::make_unique<TagLibMetadataReader>();
    auto mpvReader = std::make_unique<MpvMetadataReader>();
    metadataReader_ = std::make_unique<HybridMetadataReader>(std::move(tagLibReader), std::move(mpvReader));
    fileSystem_ = std::make_unique<LocalFileSystem>();
    playbackEngine_ = std::make_unique<MpvPlaybackEngine>();

    auto s32k = std::make_unique<S32K144Interface>();
    
    const auto& config = Config::getInstance().getConfig();
    if (config.hardwareEnabled && !Config::getInstance().isTestMode()) {
        if (s32k->initialize(config.hardwareIp, config.hardwarePort)) { 
            s32k->startListening();
        } else {
            Logger::warn("Failed to connect to S32K144 hardware");
        }
    } else {
        Logger::info("Hardware interface disabled in config");
    }
    hardwareInterface_ = std::move(s32k);

    return true;
}

bool Application::createModels()
{
    library_ = std::make_unique<Library>(persistence_.get());
    playlistManager_ = std::make_unique<PlaylistManager>(persistence_.get());
    history_ = std::make_unique<History>(100);
    playbackState_ = std::make_unique<PlaybackState>();
    return true;
}

bool Application::createControllers()
{
    playbackController_ = std::make_unique<PlaybackController>(playbackEngine_.get(), playbackState_.get(),
                                                               history_.get(), hardwareInterface_.get(), nullptr);
    float initialVolume = Config::getInstance().getConfig().customVolume;
    if (initialVolume < 0.0f)
    {
        initialVolume = Config::getInstance().getConfig().defaultVolume;
    }
    playbackController_->setVolume(initialVolume);
    libraryController_ = std::make_unique<LibraryController>(library_.get(), fileSystem_.get(), metadataReader_.get(),
                                                             playbackController_.get());
    playlistController_ =
        std::make_unique<PlaylistController>(playlistManager_.get(), library_.get(), metadataReader_.get());
    historyController_ = std::make_unique<HistoryController>(history_.get(), playbackController_.get());
    usbController_ = std::make_unique<USBController>(fileSystem_.get());

    return true;
}

bool Application::createViews()
{
    viewFactory_ = std::make_unique<ViewFactory>();
    IView *mainView = viewFactory_->createMainWindow();
    mainWindow_.reset(dynamic_cast<MainWindow *>(mainView));
    if (!mainWindow_)
        mainWindow_ = std::make_unique<MainWindow>();

    mainWindow_->setLibraryView(dynamic_cast<LibraryView *>(viewFactory_->createLibraryView(
        libraryController_.get(), library_.get(), playbackController_.get(), playlistManager_.get())));
    mainWindow_->setPlaylistView(dynamic_cast<PlaylistView *>(viewFactory_->createPlaylistView(
        playlistController_.get(), playlistManager_.get(), playbackController_.get())));
    mainWindow_->setNowPlayingView(dynamic_cast<NowPlayingView *>(
        viewFactory_->createNowPlayingView(playbackController_.get(), playbackState_.get())));
    if (auto *npView = dynamic_cast<NowPlayingView *>(mainWindow_->getNowPlayingView()))
    {
        npView->setPlaylistManager(playlistManager_.get());
    }
    mainWindow_->setHistoryView(dynamic_cast<HistoryView *>(viewFactory_->createHistoryView(
        historyController_.get(), history_.get(), playbackController_.get(), playlistManager_.get())));
    mainWindow_->setFileBrowserView(dynamic_cast<FileBrowserView *>(
        viewFactory_->createFileBrowserView(fileSystem_.get(), libraryController_.get())));
    return true;
}

void Application::wireObservers()
{
    if (libraryController_)
    {
        libraryController_->setOnTrackRemovedCallback(
            [this](const std::string &path)
            {
                if (playbackController_ && playbackState_)
                {
                    auto currentTrack = playbackState_->getCurrentTrack();
                    if (currentTrack && currentTrack->getPath() == path)
                    {
                        Logger::info("Removed track is currently playing, skipping to next...");
                        playbackController_->next();
                    }
                }
                if (playlistController_)
                {
                    playlistController_->removeTrackFromAllPlaylists(path);
                }
                if (history_)
                {
                    history_->removeTrackByPath(path);
                }
                if (playbackState_)
                {
                    playbackState_->removeTrackFromBackStack(path);
                }
            });
    }

    if (mainWindow_->getLibraryView() && mainWindow_->getFileBrowserView())
    {
        mainWindow_->getLibraryView()->setFileBrowserView(mainWindow_->getFileBrowserView());
    }
    if (mainWindow_->getPlaylistView() && mainWindow_->getFileBrowserView())
    {
        mainWindow_->getPlaylistView()->setFileBrowserView(mainWindow_->getFileBrowserView());
    }
    if (mainWindow_->getFileBrowserView() && playlistController_)
    {
        mainWindow_->getFileBrowserView()->setPlaylistController(playlistController_.get());
    }
    mainWindow_->setPlaybackController(playbackController_.get());
    mainWindow_->setPlaybackState(playbackState_.get());

    if (playbackController_)
    {
        playbackController_->setOnTrackLoadFailedCallback(
            [this](const std::string &path)
            {
                Logger::error("Track load failed for: " + path);

                // If library controller available, try to remove from library
                // This will trigger onTrackRemovedCallback if successful
                if (libraryController_)
                {
                    Logger::info("Removing missing track from library: " + path);
                    // Use force removal and ensure callback is triggered
                    if (!libraryController_->removeMedia(path))
                    {
                        // File was not in library, so callback wasn't triggered.
                        // We must manually advance to next track.
                        Logger::info("Track not found in library, skipping to next...");
                        if (playbackController_)
                        {
                            playbackController_->next();
                        }
                    }
                }
            });
    }
}

bool Application::loadState()
{
    if (library_)
        library_->load();
    if (playlistManager_)
        playlistManager_->loadAll();
    return true;
}

bool Application::saveState()
{
    Config::getInstance().save();
    if (library_)
        library_->save();
    if (playlistManager_)
        playlistManager_->saveAll();
    return true;
}

void Application::run()
{
    if (!initialized_)
    {
        Logger::error("Cannot run - application not initialized");
        return;
    }

    Logger::info("Application running...");

    if (mainWindow_)
    {
        mainWindow_->show();
    }

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!shouldQuit_)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        runOneFrame(deltaTime);

        if (headless_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    Logger::info("Application main loop ended");
}

void Application::runOneFrame(float deltaTime)
{
    if (!initialized_)
        return;

    // Update playback controller
    if (playbackController_)
    {
        playbackController_->updateTime(deltaTime);
    }

    if (!headless_)
    {
        // Update video frame for rendering
        if (playbackEngine_)
        {
            playbackEngine_->updateVideo();
        }

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                shouldQuit_ = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window_))
                shouldQuit_ = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Render main window logic
        if (mainWindow_)
        {
            mainWindow_->handleInput();
            mainWindow_->render();
        }

        ImGui::Render();

        ImGuiIO &io = ImGui::GetIO();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window_);
    }
}

void Application::shutdown()
{
    if (!initialized_)
    {
        // Already shut down or never initialized
        return;
    }

    Logger::info("Shutting down application...");

    saveState();

    if (playbackController_)
    {
        playbackController_->stop();
    }

    playbackController_.reset();
    playbackEngine_.reset();

    if (glContext_ || window_)
    {
        if (glContext_)
        {
            ImGui_ImplOpenGL3_Shutdown();
        }
        if (window_)
        {
            ImGui_ImplSDL2_Shutdown();
        }
        ImGui::DestroyContext();
    }

    if (glContext_)
    {
        SDL_GL_DeleteContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();

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

    Logger::info("Application shutdown complete");
}
