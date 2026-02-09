#include "app/view/LibraryView.h"
#include "app/controller/LibraryController.h"
#include "app/controller/PlaybackController.h"
#include "app/model/Library.h"
#include "app/view/FileBrowserView.h"
#include "app/model/PlaylistManager.h"
#include "imgui.h"
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockFileSystem.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include "utils/Config.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class LibraryViewTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<NiceMock<MockPlaybackEngine>> mockEngine;
    std::shared_ptr<Library> library;
    std::unique_ptr<PlaylistManager> playlistManager;
    std::shared_ptr<PlaybackState> playbackState;
    std::shared_ptr<History> history;
    std::unique_ptr<PlaybackController> playbackController;
    std::unique_ptr<LibraryController> libraryController;
    std::unique_ptr<LibraryView> view;

    void SetUp() override
    {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1920, 1080);

        // Build font atlas for headless testing
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)(intptr_t)1);

        AppConfig dummyConfig;
        dummyConfig.configPath = "/tmp/test_config_view.json";
        Config::getInstance().setAppConfig(dummyConfig);

        mockPersist = std::make_shared<MockPersistence>();
        mockEngine = std::make_shared<NiceMock<MockPlaybackEngine>>();

        EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
        EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

        library = std::make_shared<Library>(mockPersist.get());
        playlistManager = std::make_unique<PlaylistManager>(mockPersist.get());

        history = std::make_shared<History>(10, mockPersist.get());
        playbackState = std::make_shared<PlaybackState>();
        playbackController = std::make_unique<PlaybackController>(mockEngine.get(), playbackState.get(), history.get());

        // Correct constructor: Library, FileSystem(nullptr), MetadataReader(nullptr), PlaybackController
        libraryController =
            std::make_unique<LibraryController>(library.get(), nullptr, nullptr, playbackController.get());

        view = std::make_unique<LibraryView>(libraryController.get(), library.get(), playbackController.get(),
                                             playlistManager.get());
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    // Helpers
    void startFrame()
    {
        ImGui::NewFrame();
        ImGui::Begin("Test Window");
    }

    void endFrame()
    {
        ImGui::End();
        ImGui::Render();
    }

    // Helper methods to access private/protected members
    const std::vector<std::shared_ptr<MediaFile>> &getDisplayedFiles() const
    {
        return view->displayedFiles_;
    }

    void setSearchQuery(const std::string &query)
    {
        view->searchQuery_ = query;
    }

    void performRefresh()
    {
        view->refreshDisplay();
    }

    void simulateClick(ImVec2 pos)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = pos;
        io.MouseDown[0] = true;
        io.MouseClicked[0] = true;
    }

    void setShowAddFilePopup(bool val) { view->showAddFilePopup_ = val; }
    void setShowAddDirPopup(bool val) { view->showAddDirPopup_ = val; }
    void toggleEditMode() { view->toggleEditMode(); }
    bool isEditMode() const { return view->isEditMode_; }
    void selectAll(const std::vector<std::shared_ptr<MediaFile>>& tracks) { view->selectAll(tracks); }
    bool isSelected(const std::string& path) const { return view->isSelected(path); }
};

TEST_F(LibraryViewTest, UpdateRefreshesDisplayedFiles)
{
    auto f1 = std::make_shared<MediaFile>("/active.mp3");
    library->addMedia(f1);

    // Trigger update via Observer pattern
    view->update(library.get());
    performRefresh(); // Manually refresh since we are not in render loop

    ASSERT_EQ(getDisplayedFiles().size(), 1);
    EXPECT_EQ(getDisplayedFiles()[0]->getPath(), "/active.mp3");
}

TEST_F(LibraryViewTest, SearchFiltersFiles)
{
    auto f1 = std::make_shared<MediaFile>("/queen.mp3");
    MediaMetadata meta1;
    meta1.artist = "Queen";
    f1->setMetadata(meta1);

    auto f2 = std::make_shared<MediaFile>("/abba.mp3");
    MediaMetadata meta2;
    meta2.artist = "Abba";
    f2->setMetadata(meta2);

    library->addMedia(f1);
    library->addMedia(f2);

    // Initial update shows all
    view->update(library.get());
    performRefresh();
    ASSERT_EQ(getDisplayedFiles().size(), 2);

    // Set search query directly using helper
    setSearchQuery("Queen");
    performRefresh(); // Force refresh to apply search

    ASSERT_EQ(getDisplayedFiles().size(), 1);
    EXPECT_EQ(getDisplayedFiles()[0]->getPath(), "/queen.mp3");

    // Clear search
    setSearchQuery("");
    performRefresh();
    ASSERT_EQ(getDisplayedFiles().size(), 2);
}

TEST_F(LibraryViewTest, SearchIsCaseInsensitive)
{
    auto f1 = std::make_shared<MediaFile>("/metallica.mp3");
    MediaMetadata meta1;
    meta1.artist = "Metallica";
    f1->setMetadata(meta1);
    library->addMedia(f1);

    view->update(library.get());
    performRefresh(); // Process update

    // Search with lowercase
    setSearchQuery("metallica");
    performRefresh();
    ASSERT_EQ(getDisplayedFiles().size(), 1);

    // Search with uppercase
    setSearchQuery("METALLICA");
    performRefresh();
    ASSERT_EQ(getDisplayedFiles().size(), 1);

    // Search with mixed case
    setSearchQuery("MeTaLlIcA");
    performRefresh();
    ASSERT_EQ(getDisplayedFiles().size(), 1);
}

TEST_F(LibraryViewTest, RenderBasic)
{
    auto f1 = std::make_shared<MediaFile>("/song.mp3");
    library->addMedia(f1);
    view->update(library.get());

    startFrame();
    view->render();
    endFrame();
}

TEST_F(LibraryViewTest, RenderWithPopups)
{
    auto f1 = std::make_shared<MediaFile>("/song.mp3");
    library->addMedia(f1);
    view->update(library.get());

    startFrame();
    // Simulate clicking the "+" button for Add File
    // Height is roughly at top of child.
    simulateClick(ImVec2(100, 50)); 
    
    view->render();
    endFrame();

    // Reset click
    ImGui::GetIO().MouseDown[0] = false;

    // Directly set flags for more robust coverage of popup content
    setShowAddFilePopup(true);
    setShowAddDirPopup(true);

    startFrame();
    view->render();
    endFrame();
}

TEST_F(LibraryViewTest, AddFilesButtonWithBrowser)
{
    // Mock FileSystem for the browser
    auto mockFS = std::make_unique<NiceMock<MockFileSystem>>();
    ON_CALL(*mockFS, exists(_)).WillByDefault(Return(true));
    ON_CALL(*mockFS, isDirectory(_)).WillByDefault(Return(true));
    ON_CALL(*mockFS, browse(_)).WillByDefault(Return(std::vector<FileInfo>{}));
    ON_CALL(*mockFS, getMediaFiles(_, _, _)).WillByDefault(Return(std::vector<std::string>{}));

    FileBrowserView browser(mockFS.get(), libraryController.get());
    view->setFileBrowserView(&browser);
    
    startFrame();
    simulateClick(ImVec2(100, 50)); // Click "Add Files"
    view->render();
    endFrame();
}

TEST_F(LibraryViewTest, RenderWithEditMode)
{
    auto f1 = std::make_shared<MediaFile>("/song.mp3");
    library->addMedia(f1);
    view->update(library.get());

    toggleEditMode();
    EXPECT_TRUE(isEditMode());

    startFrame();
    view->render();
    endFrame();
}

TEST_F(LibraryViewTest, RenderWithMetadataAndAddPopups)
{
    auto f1 = std::make_shared<MediaFile>("/song.mp3");
    library->addMedia(f1);
    view->update(library.get());

    startFrame();
    ImGui::OpenPopup("MetadataPopup");
    ImGui::OpenPopup("AddToPlaylistPopup##0");
    view->render();
    endFrame();
}

TEST_F(LibraryViewTest, RenderEmptyLibrary)
{
    library->clear();
    view->update(library.get());

    startFrame();
    view->render();
    endFrame();
}
TEST_F(LibraryViewTest, NullLibrary)
{
    LibraryView nullView(nullptr, nullptr, nullptr, nullptr);
    startFrame();
    nullView.render();
    endFrame();
    // Destructor hits if(library_)
}

TEST_F(LibraryViewTest, AddFilesButton)
{
    // Mock FileBrowserView
    // LibraryView expects a FileBrowserView pointer
    // We can't easily mock it without a header, but we can hit the button click
    startFrame();
    // Simulate clicking "Add Files"
    // The button is at the top.
    simulateClick(ImVec2(50, 50)); 
    view->render();
    endFrame();
}

TEST_F(LibraryViewTest, SearchInputInteraction)
{
    startFrame();
    // Simulate search input
    ImGui::SetNextItemAllowOverlap();
    static char buf[128] = "test search";
    ImGui::InputText("Search", buf, sizeof(buf)); 
    
    // The view's search bar will also be rendered and might capture input if we are clever,
    // but simply hitting the renderSearchBar logic is enough for coverage.
    view->render();
    endFrame();
}
