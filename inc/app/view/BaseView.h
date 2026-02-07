#ifndef BASE_VIEW_H
#define BASE_VIEW_H

#include "interfaces/IView.h"

/**
 * @file BaseView.h
 * @brief Base class for all ImGui views
 * 
 * Provides common functionality for views.
 * Implements IView and IObserver interfaces.
 */

/**
 * @brief Base view class
 * 
 * All ImGui views should inherit from this class.
 * Provides default implementations for common view operations.
 */
class BaseView : public IView {
public:
    BaseView();
    virtual ~BaseView() = default;
    
    virtual void render() override = 0;  // Pure virtual - must be implemented
    virtual void handleInput() override {}  // Default: empty
    virtual void update(void* subject) override {}  // Default: empty
    
    /**
     * @brief Render popups/modals that need to be at root level
     */
    virtual void renderPopups() {}
    
    virtual void show() override { visible_ = true; }
    virtual void hide() override { visible_ = false; }
    virtual bool isVisible() const override { return visible_; }
    
protected:
    bool visible_;
};

#endif // BASE_VIEW_H
