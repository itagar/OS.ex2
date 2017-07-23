/**
 * @file Thread.h
 * @author Itai Tagar <itagar>
 *
 * @brief A Class Declaration for a Thread in the User-Thread Library.
 */


#ifndef EX2_THREAD_H
#define EX2_THREAD_H


/*-----=  Includes  =-----*/


#include <list>


/*-----=  Type Definitions  =-----*/


/**
 * @brief Type Definition for Thread ID.
 */
typedef int ThreadID;

/**
 * @brief Type Definition for Threads List.
 */
typedef std::list<ThreadID> ThreadList;


/*-----=  Enums  =-----*/


/**
 * @brief The possible states of a Thread.
 */
enum ThreadState {READY, RUNNING, BLOCKED};


/*-----=  Class Declaration  =-----*/


/**
 * @brief The Thread Class. A Thread has a State which can be changed during
 *        running the User-Thread Library. In addition the Thread holds the
 *        number of running quantum it has done. Finally, In order to manage
 *        sync operation, each Thread holds the following:
 *          1. If the Thread itself is in sync, it holds the ID of the Thread
 *             it is synced to.
 *          2. If some Threads synced to this Thread, the Thread holds a list
 *             of all of these Threads.
 */
class Thread
{
public:

    /**
     * @brief A Constructor for the Thread.
     */
    Thread();

    /**
     * @brief A Constructor for this Thread which receives the running quantums.
     * @param runningQuantums The runningQuantums of this Thread.
     */
    Thread(const int runningQuantums);

    /**
     * @brief The Thread Destructor.
     */
    ~Thread();

    /**
     * @brief Set the Thread State.
     * @param state The new State of the Thread.
     */
    void setState(ThreadState const state) { _state = state; };

    /**
     * @brief Gets the Thread State.
     * @return The State of the Thread.
     */
    ThreadState getState() const { return _state; };

    /**
     * @brief Increment the running quantums.
     */
    void incRunningQuantums() { _runningQuantums++; };

    /**
     * @brief Get the running quantums.
     * @return The running quantums.
     */
    int getRunningQuantums() const { return _runningQuantums; };

    /**
     * @brief Indicates whether this Thread is sync to another Thread.
     * @return true if synced, false otherwise.
     */
    bool isSync() const { return _synced; };

    /**
     * @brief Set this Thread to be synced to the given 'tid' Thread.
     * @param tid The Thread ID to sync to.
     */
    void setSync(ThreadID const tid);

    /**
     * @brief Gets the Thread ID of the Thread this Thread is synced to.
     * @return ID of the Thread this Thread is synced to, or -1 if not synced.
     */
    ThreadID getSyncedID() { return _syncedTo; };

    /**
     * @brief Remove the sync status from this Thread.
     */
    void clearSync();

    /**
     * @brief Gets access to the list of all the Threads that are synced to this
     *        Thread.
     * @return List of the Threads synced to this Thread.
     */
    ThreadList syncedThreads() { return _syncedThreads; };

    /**
     * @brief Insert a new Thread that synced to this Thread.
     * @param tid The ID of the Thread to insert.
     */
    void insertSyncedThread(ThreadID const tid);

    /**
     * @brief Remove a given Thread that synced to this Thread.
     * @param tid The ID of the Thread to remove.
     */
    void removeSyncedThread(ThreadID const tid);

    /**
     * @brief Clears the entire list Threads that synced to this Thread.
     */
    void clearSyncedThreads();

private:
    /**
     * @brief The current state of this Thread.
     */
    ThreadState _state;

    /**
     * @brief The quantums that this Thread was running.
     */
    int _runningQuantums;

    /**
     * @brief a Flag indicates if this Thread is sync or not.
     */
    bool _synced;

    /**
     * @brief The ID of the Thread that this Thread is synced to.
     */
    ThreadID _syncedTo;

    /**
     * @brief The list of all Threads that synced to this Thread.
     */
    ThreadList _syncedThreads;
};


#endif