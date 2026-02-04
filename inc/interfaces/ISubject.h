#ifndef ISUBJECT_H
#define ISUBJECT_H

#include "IObserver.h"

/**
 * @file ISubject.h
 * @brief Subject interface for Observer pattern
 * 
 * Classes that want to be observed should implement this interface.
 * Maintains a list of observers and notifies them of state changes.
 */

/**
 * @brief Subject interface
 * 
 * Classes that want to be observable should implement this interface.
 * Observers can attach/detach from subjects to receive notifications.
 * 
 * **Thread Safety**: Implementations should ensure thread-safe
 * attach/detach/notify operations if used in multi-threaded contexts.
 * See Subject class for thread-safe implementation.
 */
class ISubject {
public:
    virtual ~ISubject() = default;
    
    /**
     * @brief Attach an observer to this subject
     * @param observer Observer to attach (non-owning pointer)
     * 
     * The subject does NOT take ownership. Observer must outlive
     * the subject or detach before destruction.
     */
    virtual void attach(IObserver* observer) = 0;
    
    /**
     * @brief Detach an observer from this subject
     * @param observer Observer to detach
     * 
     * Safe to call even if observer was not attached.
     */
    virtual void detach(IObserver* observer) = 0;
    
    /**
     * @brief Notify all attached observers of a state change
     * 
     * Calls update() on each observer. May be called from
     * any thread depending on implementation.
     */
    virtual void notify() = 0;
};

#endif // ISUBJECT_H
