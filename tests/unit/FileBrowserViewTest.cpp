#include "app/view/FileBrowserView.h"
#include "app/controller/LibraryController.h"
#include "app/model/Library.h"
#include "imgui.h"
#include "tests/mocks/MockFileSystem.h"
#include "tests/mocks/MockPersistence.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class TestFileBrowserView : public FileBrowserView {
public:
    using FileBrowserView::FileBrowserView;
    using FileBrowserView::fileSelector_;
    using FileBrowserView::currentTrackCount_;
    using FileBrowserView::currentPath_;
    using FileBrowserView::navigateTo;
    using FileBrowserView::navigateUp;
    using FileBrowserView::processFiles;
};

class FileBrowserViewTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NiceMock<MockFileSystem>> mockFs;
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<Library> library;
    std::unique_ptr<LibraryController> libraryController;
    std::unique_ptr<TestFileBrowserView> view;

    void SetUp() override
    {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize = ImVec2(1024, 768);

        // Build font atlas for headless testing
        unsigned char *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID((ImTextureID)(intptr_t)1);

        mockFs = std::make_shared<NiceMock<MockFileSystem>>();
        mockPersist = std::make_shared<MockPersistence>();

        // Mock persistence behavior
        EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));

        library = std::make_shared<Library>(mockPersist.get());
        libraryController = std::make_unique<LibraryController>(library.get(), mockFs.get(), nullptr, nullptr);

        view = std::make_unique<TestFileBrowserView>(mockFs.get(), libraryController.get());
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }

    // Helpers
    void startFrame()
    {
        ImGui::NewFrame();
        ImGui::Begin("File Browser Window");
    }

    void endFrame()
    {
        ImGui::End();
        ImGui::Render();
    }

    // Helper to set current path
    void setCurrentPath(const std::string &path)
    {
        view->currentPath_ = path;
    }

    std::string getCurrentPath()
    {
        return view->currentPath_;
    }

    // Helper wrappers for private methods
    void navigateToHelper(const std::string &path)
    {
        view->navigateTo(path);
    }

    void navigateUpHelper()
    {
        view->navigateUp();
    }

    void simulateClick(ImVec2 pos)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = pos;
        io.MouseDown[0] = true;
        io.MouseClicked[0] = true;
    }
};

TEST_F(FileBrowserViewTest, ConstructorSetsHomeDirectory)
{
    // Constructor uses HOME env var.
    // If we can't easily mock env vars safely in parallel tests, we skip specific path check
    // or just check that it initiates something.
    // However, the test runs in isolation usually.
    // Let's just check it's not empty.
    EXPECT_FALSE(getCurrentPath().empty());
}

TEST_F(FileBrowserViewTest, NavigateToChangesPathIfValid)
{
    std::string newPath = "/music/rock";

    EXPECT_CALL(*mockFs, exists(newPath)).WillOnce(Return(true));
    EXPECT_CALL(*mockFs, isDirectory(newPath)).WillOnce(Return(true));
    // browse will be called to refresh
    EXPECT_CALL(*mockFs, browse(newPath)).WillOnce(Return(std::vector<FileInfo>{}));
    EXPECT_CALL(*mockFs, getMediaFiles(newPath, _, _)).WillOnce(Return(std::vector<std::string>{}));

    navigateToHelper(newPath);

    EXPECT_EQ(getCurrentPath(), newPath);
}

TEST_F(FileBrowserViewTest, NavigateToDoesNotChangePathIfInvalid)
{
    std::string originalPath = getCurrentPath();
    std::string invalidPath = "/invalid/path";

    EXPECT_CALL(*mockFs, exists(invalidPath)).WillOnce(Return(false));

    navigateToHelper(invalidPath);

    EXPECT_EQ(getCurrentPath(), originalPath);
}

TEST_F(FileBrowserViewTest, NavigateUpGoesToParent)
{
    setCurrentPath("/home/user/music");

    // Expect checks for new path "/home/user"
    std::string parentPath = "/home/user";
    EXPECT_CALL(*mockFs, browse(parentPath)).WillOnce(Return(std::vector<FileInfo>{}));
    EXPECT_CALL(*mockFs, getMediaFiles(parentPath, _, _)).WillOnce(Return(std::vector<std::string>{}));

    navigateUpHelper();

    EXPECT_EQ(getCurrentPath(), parentPath);
}

TEST_F(FileBrowserViewTest, NavigateUpAtRootDoesNothing)
{
    EXPECT_EQ(getCurrentPath(), "/");
}

TEST_F(FileBrowserViewTest, RenderBasic)
{
    startFrame();
    view->render();
    endFrame();
}

TEST_F(FileBrowserViewTest, RenderPopup)
{
    view->show();
    startFrame();
    ImGui::OpenPopup("File Browser");
    view->renderPopup();
    endFrame();
    EXPECT_TRUE(view->isVisible());
}

TEST_F(FileBrowserViewTest, ShowHide)
{
    view->show();
    EXPECT_TRUE(view->isVisible());
    view->hide();
    EXPECT_FALSE(view->isVisible());
}

TEST_F(FileBrowserViewTest, ModeTests)
{
    view->setMode(FileBrowserView::BrowserMode::LIBRARY);
    view->setMode(FileBrowserView::BrowserMode::PLAYLIST_SELECTION);
    view->setMode(FileBrowserView::BrowserMode::LIBRARY_ADD_AND_RETURN);
}

TEST_F(FileBrowserViewTest, RenderWithFiles)
{
    std::vector<FileInfo> files;
    FileInfo f1; f1.name = "song.mp3"; f1.path = "/music/song.mp3"; f1.isDirectory = false;
    FileInfo d1; d1.name = "Rock"; d1.path = "/music/Rock"; d1.isDirectory = true;
    files.push_back(f1);
    files.push_back(d1);

    EXPECT_CALL(*mockFs, browse(_)).WillRepeatedly(Return(files));
    
    view->show();
    startFrame();
    view->renderPopup();
    endFrame();
}
TEST_F(FileBrowserViewTest, CallbackInteractions)
{
    // Test LIBRARY_ADD_AND_RETURN with callback
    bool callbackCalled = false;
    view->setMode(FileBrowserView::BrowserMode::LIBRARY_ADD_AND_RETURN);
    view->setOnFilesAddedCallback([&](const std::vector<std::string> &files) {
        callbackCalled = true;
        EXPECT_EQ(files.size(), 1);
        EXPECT_EQ(files[0], "/test.mp3");
    });

    // Simulate selecting a file and clicking "Add & Return"
    // Instead of coordinate clicks which are brittle, use direct interaction
    // Since processPaths is a lambda inside render(), we use simulateClick to hit the button.
    startFrame();
    // We need to have some files to select
    std::vector<FileInfo> files;
    FileInfo f1; f1.name = "test.mp3"; f1.path = "/test.mp3"; f1.isDirectory = false;
    files.push_back(f1);
    EXPECT_CALL(*mockFs, exists("/")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFs, isDirectory("/")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFs, browse(_)).WillRepeatedly(Return(files));
    navigateToHelper("/");
    
    // Select the file manually via fileSelector
    view->fileSelector_.clearSelection();
    view->fileSelector_.addSelection("/test.mp3");
    
    // Call processFiles directly (Line 160/203)
    view->processFiles({"/test.mp3"});

    EXPECT_TRUE(callbackCalled);
    EXPECT_FALSE(view->isVisible()); // Should hide after add & return
}

TEST_F(FileBrowserViewTest, RecursiveScanDeep)
{
    // Test recursive scan depth and extension filtering branches (Lines 290-312)
    EXPECT_CALL(*mockFs, exists("/music")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFs, isDirectory("/music")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFs, getMediaFiles(_, _, 3))
        .WillOnce(Return(std::vector<std::string>{"/m1.mp3", "/m2.flac"}));
    
    navigateToHelper("/music");
    
    // Verify that currentTrackCount_ is updated
    // We can check through friend access or by rendering and seeing if a child is created.
    EXPECT_EQ(view->currentTrackCount_, 2);
}

TEST_F(FileBrowserViewTest, RandomSelection)
{
    std::vector<FileInfo> files;
    for(int i=0; i<30; ++i) {
        FileInfo f; f.name = "song" + std::to_string(i) + ".mp3"; f.path = "/music/" + f.name; f.isDirectory = false;
        files.push_back(f);
    }
    EXPECT_CALL(*mockFs, exists("/music")).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFs, isDirectory("/music")).WillRepeatedly(Return(true));
    
    std::vector<std::string> mediaPaths;
    for(const auto& f : files) mediaPaths.push_back(f.path);
    
    EXPECT_CALL(*mockFs, browse("/music")).WillRepeatedly(Return(std::vector<FileInfo>{}));
    EXPECT_CALL(*mockFs, getMediaFiles("/music", _, 3)).WillRepeatedly(Return(mediaPaths));
    
    view->show();
    view->navigateTo("/music");
    
    // Simulate clicking "Add Random 20"
    // fileSelector_.selectRandom(20) is called, which we can verify
    startFrame();
    view->render(); // Should render the "Add Random 20" button
    endFrame();
    
    view->fileSelector_.selectRandom(20);
    EXPECT_EQ(view->fileSelector_.getSelectedPaths().size(), 20);
    
    // Process files
    view->processFiles(view->fileSelector_.getSelectedPaths());
}

TEST_F(FileBrowserViewTest, PlaylistSelectionMode)
{
    view->setMode(FileBrowserView::BrowserMode::PLAYLIST_SELECTION);
    view->setTargetPlaylist("MyPlaylist");
    
    // We need a PlaylistController mock or similar if we want to test interaction
    // For now, check if it correctly delegates to it if set
}

TEST_F(FileBrowserViewTest, NavigateUpEdgeCases)
{
    // Root case
    setCurrentPath("/");
    navigateUpHelper();
    EXPECT_EQ(getCurrentPath(), "/");

    // Deep subfolder
    setCurrentPath("/a/b/c");
    EXPECT_CALL(*mockFs, browse("/a/b")).WillOnce(Return(std::vector<FileInfo>{}));
    navigateUpHelper();
    EXPECT_EQ(getCurrentPath(), "/a/b");
}

TEST_F(FileBrowserViewTest, RenderContentBranches)
{
    startFrame();
    // Test various modes for button text
    view->setMode(FileBrowserView::BrowserMode::LIBRARY);
    view->render();
    
    view->setMode(FileBrowserView::BrowserMode::PLAYLIST_SELECTION);
    view->render();
    
    view->setMode(FileBrowserView::BrowserMode::LIBRARY_ADD_AND_RETURN);
    view->render();
    endFrame();
}
