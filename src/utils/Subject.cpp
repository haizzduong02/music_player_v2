#include "../../inc/utils/Subject.h"

/**
 * @file Subject.cpp
 * @brief Implementation of Subject class (Observer pattern)
 */

Subject::~Subject() {
    // Virtual destructor - observers are not owned, so no cleanup needed
}

void Subject::attach(IObserver* observer) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    if (observer && std::find(observers_.begin(), observers_.end(), observer) == observers_.end()) {
        observers_.push_back(observer);
    }
}

void Subject::detach(IObserver* observer) {
    std::lock_guard<std::mutex> lock(observerMutex_);
    observers_.erase(
        std::remove(observers_.begin(), observers_.end(), observer),
        observers_.end()
    );
}

void Subject::notify() {
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (auto* observer : observers_) {
        if (observer) {
            observer->update(this);
        }
    }
}
