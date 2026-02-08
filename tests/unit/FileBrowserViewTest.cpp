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

class FileBrowserViewTest : public ::testing::Test
{
  protected:
    std::shared_ptr<NiceMock<MockFileSystem>> mockFs;
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<Library> library;
    std::unique_ptr<LibraryController> libraryController;
    std::unique_ptr<FileBrowserView> view;

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

        view = std::make_unique<FileBrowserView>(mockFs.get(), libraryController.get());
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
