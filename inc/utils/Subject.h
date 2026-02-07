#ifndef SUBJECT_H
#define SUBJECT_H

#include "interfaces/ISubject.h"
#include <algorithm>
#include <mutex>
#include <vector>

/**
 * @file Subject.h
 * @brief Base implementation of ISubject (Observer Pattern)
 *
 * Provides a reusable implementation of the Subject side of Observer pattern.
 * Models and Services can inherit from this class instead of reimplementing.
 */

/**
 * @brief Subject base class
 *
 * Concrete implementation of ISubject interface.
 * Manages observer registration and notification.
 */
class Subject : public ISubject
{
  public:
    virtual ~Subject() = default;

    /**
     * @brief Attach an observer
     * @param observer Observer to attach
     * @note Thread-safe
     */
    void attach(IObserver *observer) override
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end())
        {
            observers_.push_back(observer);
        }
    }

    /**
     * @brief Detach an observer
     * @param observer Observer to detach
     * @note Thread-safe
     */
    void detach(IObserver *observer) override
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
    }

    /**
     * @brief Notify all observers
     * Calls update() on each attached observer.
     * @note Thread-safe
     */
    void notify() override
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto *observer : observers_)
        {
            if (observer)
            {
                observer->update(this);
            }
        }
    }

  protected:
    std::vector<IObserver *> observers_; ///< List of attached observers
    mutable std::mutex observerMutex_;   ///< Mutex for thread-safe observer operations
};

#endif // SUBJECT_H
