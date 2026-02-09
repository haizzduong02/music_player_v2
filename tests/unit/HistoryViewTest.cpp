#include "app/view/HistoryView.h"
#include "app/model/History.h"
#include "app/controller/HistoryController.h"
#include "app/controller/PlaybackController.h"
#include "imgui.h"
#include "tests/mocks/MockPersistence.h"
#include "tests/mocks/MockPlaybackEngine.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class HistoryViewTest : public ::testing::Test
{
  protected:
    std::shared_ptr<MockPersistence> mockPersist;
    std::shared_ptr<NiceMock<MockPlaybackEngine>> mockEngine;
    std::shared_ptr<History> history;
    std::shared_ptr<PlaybackState> playbackState;
    std::unique_ptr<PlaybackController> playbackController;
    std::unique_ptr<HistoryController> historyController;
    std::unique_ptr<HistoryView> view;

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

        mockPersist = std::make_shared<MockPersistence>();
        mockEngine = std::make_shared<NiceMock<MockPlaybackEngine>>();

        EXPECT_CALL(*mockPersist, loadFromFile(_, _)).WillRepeatedly(Return(false));
        EXPECT_CALL(*mockPersist, saveToFile(_, _)).WillRepeatedly(Return(true));

        history = std::make_shared<History>(10, mockPersist.get());
        playbackState = std::make_shared<PlaybackState>();
        playbackController = std::make_unique<PlaybackController>(mockEngine.get(), playbackState.get(), history.get());

        historyController = std::make_unique<HistoryController>(history.get(), playbackController.get());

        view = std::make_unique<HistoryView>(historyController.get(), history.get(), playbackController.get(), nullptr);
    }

    void TearDown() override
    {
        ImGui::DestroyContext();
    }
};

TEST_F(HistoryViewTest, RenderBasic)
{
    history->addTrack(std::make_shared<MediaFile>("/h1.mp3"));
    
    ImGui::NewFrame();
    view->render();
    ImGui::Render();
}

TEST_F(HistoryViewTest, UpdateResetsSelection)
{
    view->update(history.get());
    SUCCEED();
}

TEST_F(HistoryViewTest, NullHistory)
{
    HistoryView nullView(nullptr, nullptr, nullptr, nullptr);
    ImGui::NewFrame();
    nullView.render();
    ImGui::Render();
}

TEST_F(HistoryViewTest, HandleInput)
{
    view->handleInput();
}
