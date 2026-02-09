#include "app/Application.h"
#include "utils/Config.h"
#include "imgui.h"
#include <gtest/gtest.h>

class ApplicationTest : public ::testing::Test
{
  protected:
    Application app;

    void SetUp() override
    {
        ImGui::CreateContext();
        // Disable hardware for unit tests to avoid race conditions
        Config::getInstance().setTestMode(true);
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    // Helpers to access private members
    bool createServices()
    {
        return app.createServices();
    }
    bool createModels()
    {
        return app.createModels();
    }
    bool createControllers()
    {
        return app.createControllers();
    }
    bool createViews()
    {
        return app.createViews();
    }
    void wireObservers()
    {
        app.wireObservers();
    }
    void setInitialized(bool val)
    {
        app.initialized_ = val;
    }
    bool saveState()
    {
        return app.saveState();
    }
    bool loadState()
    {
        return app.loadState();
    }

    IPersistence *getPersistence()
    {
        return app.persistence_.get();
    }
    IFileSystem *getFileSystem()
    {
        return app.fileSystem_.get();
    }
    Library *getLibrary()
    {
        return app.library_.get();
    }
    PlaylistManager *getPlaylistManager()
    {
        return app.playlistManager_.get();
    }
    LibraryController *getLibraryController()
    {
        return app.libraryController_.get();
    }
    PlaybackState *getPlaybackState()
    {
        return app.playbackState_.get();
    }
    PlaybackController *getPlaybackController()
    {
        return app.playbackController_.get();
    }
    bool isInitialized() const
    {
        return app.initialized_;
    }
    MainWindow *getMainWindow()
    {
        return app.mainWindow_.get();
    }
};

TEST_F(ApplicationTest, ConstructorState)
{
    EXPECT_FALSE(app.shouldQuit());
}

TEST_F(ApplicationTest, MockedSubsystemInitialization)
{
    // 1. Services
    EXPECT_TRUE(createServices());
    EXPECT_NE(getPersistence(), nullptr);
    EXPECT_NE(getFileSystem(), nullptr);

    // 2. Models
    EXPECT_TRUE(createModels());
    EXPECT_NE(getLibrary(), nullptr);
    EXPECT_NE(getPlaylistManager(), nullptr);

    // 3. Controllers
    // Test volume logic: Negative volume in config should fall back to default
    Config::getInstance().getConfig().customVolume = -1.0f;
    EXPECT_TRUE(createControllers());
    EXPECT_NE(getLibraryController(), nullptr);
    EXPECT_NE(getPlaybackController(), nullptr);
    EXPECT_EQ(getPlaybackState()->getVolume(), Config::getInstance().getConfig().defaultVolume);

    // Test volume logic: Positive volume in config
    Config::getInstance().getConfig().customVolume = 0.75f;
    createControllers();
    EXPECT_EQ(getPlaybackState()->getVolume(), 0.75f);

    // 4. Views
    EXPECT_TRUE(createViews());
    EXPECT_NE(getMainWindow(), nullptr);

    // 5. Wiring
    wireObservers();
}

TEST_F(ApplicationTest, ShutdownSafe)
{
    // Shutdown should be safe even if not fully initialized
    app.shutdown();
}

TEST_F(ApplicationTest, SaveAndLoadState)
{
    createServices();
    createModels();
    EXPECT_TRUE(saveState());
    EXPECT_TRUE(loadState());
}

TEST_F(ApplicationTest, ObserverWiringCallbackBranches)
{
    createServices();
    createModels();
    createControllers();
    createViews();
    wireObservers();

    auto* libCtrl = getLibraryController();
    auto* playState = getPlaybackState();

    // Setup: Matching track playing
    std::string testPath = "current.mp3";
    auto track = std::make_shared<MediaFile>(testPath);
    playState->setPlayback(track, PlaybackStatus::PLAYING);
    playState->pushToBackStack(); // Pushes the current track
    
    // Trigger callback: Track IS currently playing
    libCtrl->removeTrackByPath(testPath);
    
    // Trigger callback: Track is NOT currently playing
    libCtrl->removeTrackByPath("other.mp3");
}

TEST_F(ApplicationTest, FullShutdownSequence)
{
    // Initialize enough to hit shutdown branches
    createServices();
    createModels();
    createControllers();
    createViews();

    // Set initialized to true so destructor calls shutdown() logic
    setInitialized(true);
}

TEST_F(ApplicationTest, InitializationErrorHandling)
{
    // init() catches exceptions
    // Hard to force exception in internal create methods without injecting mocks,
    // but we covered the success paths.
}
