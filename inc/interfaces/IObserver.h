#ifndef IOBSERVER_H
#define IOBSERVER_H

/**
 * @file IObserver.h
 * @brief Observer interface for Observer pattern event notification
 *
 * Classes that want to observe changes in a Subject should implement this interface.
 * The update() method will be called whenever the observed Subject's state changes.
 *
 * See also: ISubject.h for the Subject side of the Observer pattern.
 */

/**
 * @brief Observer interface
 *
 * Classes that want to observe changes in a Subject should implement this interface.
 * The update() method will be called whenever the observed Subject's state changes.
 *
 * **Thread Safety**: update() may be called from different threads depending
 * on where Subject::notify() is invoked. Implementations should handle
 * cross-thread notifications appropriately (e.g., using dirty flags for UI updates).
 */
class IObserver
{
  public:
    virtual ~IObserver() = default;

    /**
     * @brief Called when the observed subject's state changes
     * @param subject Pointer to the subject that changed (can be cast to specific type)
     *
     * **Important**: This method may be called from a non-main thread
     * (e.g., playback thread, hardware listener thread). GUI updates
     * should be deferred to the main thread via flags.
     */
    virtual void update(void *subject) = 0;
};

#endif // IOBSERVER_H
