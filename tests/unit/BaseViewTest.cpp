#include <gtest/gtest.h>
#include "app/view/BaseView.h"

// Concrete implementation of BaseView for testing
class TestView : public BaseView {
public:
    bool onShowCalled = false;
    bool onHideCalled = false;

    void render() override {}
    void onShow() override { onShowCalled = true; }
    void onHide() override { onHideCalled = true; }
};

TEST(BaseViewTest, VisibilityAndHooks) {
    TestView view;
    
    EXPECT_FALSE(view.isVisible());
    
    view.show();
    EXPECT_TRUE(view.isVisible());
    EXPECT_TRUE(view.onShowCalled);
    
    view.hide();
    EXPECT_FALSE(view.isVisible());
    EXPECT_TRUE(view.onHideCalled);
}

TEST(BaseViewTest, DefaultImplementations) {
    TestView view;
    
    // Test default no-op methods
    view.handleInput(); // Should do nothing
    view.update(nullptr); // Should do nothing
    view.renderPopups(); // Should do nothing
}
