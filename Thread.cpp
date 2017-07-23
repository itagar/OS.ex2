/**
 * @file Thread.cpp
 * @author Itai Tagar <itagar>
 *
 * @brief A Class Implementation for a Thread in the User-Thread Library.
 */


/*-----=  Includes  =-----*/


#include "Thread.h"


/*-----=  Definitions  =-----*/


/**
* @def THREAD_INITIAL_QUANTUM 0
* @brief A Macro that sets the initial number of running quantums.
*/
#define THREAD_INITIAL_QUANTUM 0

/**
* @def NO_SYNCED_ID -1
* @brief A Macro that sets the ID when the Thread is not synced to it.
*/
#define NO_SYNCED_ID -1


/*-----=  Thread Constructors & Destructors  =-----*/


/**
 * @brief A Constructor for the Thread.
 */
Thread::Thread() : _state(READY), _runningQuantums(THREAD_INITIAL_QUANTUM),
                   _synced(false), _syncedTo(NO_SYNCED_ID)
{

}

/**
 * @brief A Constructor for this Thread which receives the running quantums.
 * @param runningQuantums The runningQuantums of this Thread.
 */
Thread::Thread(const int runningQuantums) : _state(RUNNING),
                                           _runningQuantums(runningQuantums),
                                           _synced(false),
                                           _syncedTo(NO_SYNCED_ID)
{
    // The State is already set to RUNNING.
}

/**
 * @brief The Thread Destructor.
 */
Thread::~Thread()
{

}


/*-----=  Thread Class Implementation  =-----*/


/**
 * @brief Set this Thread to be synced to the given 'tid' Thread.
 * @param tid The Thread ID to sync to.
 */
void Thread::setSync(ThreadID const tid)
{
    _syncedTo = tid;
    _synced = true;
}

/**
 * @brief Remove the sync status from this Thread.
 */
void Thread::clearSync()
{
    _synced = false;
    _syncedTo = NO_SYNCED_ID;
}

/**
 * @brief Insert a new Thread that synced to this Thread.
 * @param tid The ID of the Thread to insert.
 */
void Thread::insertSyncedThread(ThreadID const tid)
{
    _syncedThreads.push_back(tid);
}

/**
 * @brief Remove a given Thread that synced to this Thread.
 * @param tid The ID of the Thread to remove.
 */
void Thread::removeSyncedThread(ThreadID const tid)
{
    _syncedThreads.remove(tid);
}

/**
 * @brief Clears the entire list Threads that synced to this Thread.
 */
void Thread::clearSyncedThreads()
{
    _syncedThreads.clear();
}
