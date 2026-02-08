#include "app/Application.h"
#include "imgui.h"
#include <gtest/gtest.h>

class ApplicationTest : public ::testing::Test
{
  protected:
    Application app;

    void SetUp() override
    {
        ImGui::CreateContext();
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
    PlaybackController *getPlaybackController()
    {
        return app.playbackController_.get();
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
    // We can't call app.init() because of SDL_Init,
    // but we can test private methods thanks to friend class.

    // 1. Services
    EXPECT_TRUE(createServices());
    EXPECT_NE(getPersistence(), nullptr);
    EXPECT_NE(getFileSystem(), nullptr);

    // 2. Models
    EXPECT_TRUE(createModels());
    EXPECT_NE(getLibrary(), nullptr);
    EXPECT_NE(getPlaylistManager(), nullptr);

    // 3. Controllers
    EXPECT_TRUE(createControllers());
    EXPECT_NE(getLibraryController(), nullptr);
    EXPECT_NE(getPlaybackController(), nullptr);

    // 4. Views
    EXPECT_TRUE(createViews());
    EXPECT_NE(getMainWindow(), nullptr);

    // 5. Wiring
    wireObservers();
    // Verify wiring (e.g. check if Library has observers, but Subject doesn't expose list)
    // We just ensure it doesn't crash.
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

TEST_F(ApplicationTest, ObserverWiringCallback)
{
    createServices();
    createModels();
    createControllers();
    createViews();
    wireObservers();

    // Trigger the callback in libraryController
    auto *libCtrl = getLibraryController();
    if (libCtrl)
    {
        // We need a path that's in playbackState or similar to hit branches
        std::string testPath = "test.mp3";
        // removeTrackByPath triggers the callback
        libCtrl->removeTrackByPath(testPath);
    }
}

/*
TEST_F(ApplicationTest, FullShutdown)
{
    // Initialize enough to hit shutdown branches
    createServices();
    createModels();
    createControllers();
    createViews();

    // Set initialized to true so shutdown() doesn't return early
    setInitialized(true);

    app.shutdown();
}
*/
