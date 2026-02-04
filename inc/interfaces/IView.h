#ifndef IVIEW_H
#define IVIEW_H

#include "IObserver.h"

/**
 * @file IView.h
 * @brief Interface for UI views (Dependency Inversion Principle)
 * 
 * Defines the contract for all view components using ImGui.
 * Views observe model changes and update themselves automatically.
 */

/**
 * @brief View interface
 * 
 * All ImGui-based views should implement this interface.
 * Extends IObserver to receive updates from observed models.
 */
class IView : public IObserver {
public:
    virtual ~IView() = default;
    
    /**
     * @brief Render the view using ImGui
     * Called every frame in the main render loop.
     */
    virtual void render() = 0;
    
    /**
     * @brief Handle user input
     * Process keyboard, mouse, and other input events.
     */
    virtual void handleInput() = 0;
    
    /**
     * @brief Update view state (called from Observer pattern)
     * @param subject The subject that triggered the update
     */
    virtual void update(void* subject) override = 0;
    
    /**
     * @brief Show the view
     */
    virtual void show() = 0;
    
    /**
     * @brief Hide the view
     */
    virtual void hide() = 0;
    
    /**
     * @brief Check if view is visible
     * @return true if view is visible
     */
    virtual bool isVisible() const = 0;
};

#endif // IVIEW_H
