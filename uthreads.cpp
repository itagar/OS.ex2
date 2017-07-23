/**
 * @file uthreads.cpp
 * @author Itai Tagar <itagar>
 *
 * @brief A User-Level Threads Library.
 */


/*-----=  Includes  =-----*/


#include <iostream>
#include <deque>
#include <sys/time.h>
#include <signal.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include "uthreads.h"
#include "Thread.h"


/*-----=  Address Black Box  =-----*/


#ifdef __x86_64__
/* code for 64 bit Intel arch */
typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
            "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}
#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}
#endif


/*-----=  Definitions  =-----*/


/**
* @def SUCCESS_STATE 0
* @brief A Macro that sets return state in case of success.
*/
#define SUCCESS_STATE 0

/**
* @def FAILURE_STATE -1
* @brief A Macro that sets return state in case of failure.
*/
#define FAILURE_STATE -1

/**
* @def ERROR_MSG_SUFFIX '\n'
* @brief A Macro that sets the suffix for an error message.
*/
#define ERROR_MSG_SUFFIX '\n'

/**
* @def SYSCALL_ERROR_MSG "system error: "
* @brief A Macro that sets the prefix for System Call error message.
*/
#define SYSCALL_ERROR_MSG "system error: "

/**
* @def UTL_ERROR_MSG "thread library error: "
* @brief A Macro that sets the prefix for Thread Library error message.
*/
#define UTL_ERROR_MSG "thread library error: "

/**
* @def INVALID_QUANTUM_LENGTH "Invalid quantum length"
* @brief A Macro that sets the error message for invalid quantum length.
*/
#define INVALID_QUANTUM_LENGTH "Invalid quantum length."

/**
* @def SIGNAL_HANDLER_FAILURE "Signal Handler failure."
* @brief A Macro that sets the error message for failure of signal handler.
*/
#define SIGNAL_HANDLER_FAILURE "Signal Handler failure."

/**
* @def SIGNAL_BLOCK_ERROR "Block signal failure."
* @brief A Macro that sets the error message for failure of signal block.
*/
#define SIGNAL_BLOCK_ERROR "Block signal failure."

/**
* @def SIGNAL_UNBLOCK_ERROR "Unblock signal failure."
* @brief A Macro that sets the error message for failure of signal unblock.
*/
#define SIGNAL_UNBLOCK_ERROR "Unblock signal failure."

/**
* @def VIRTUAL_TIMER_FAILURE "Virtual Timer failure."
* @brief A Macro that sets the error message for failure of virtual timer.
*/
#define VIRTUAL_TIMER_FAILURE "Virtual Timer failure."

/**
* @def INVALID_THREADS_NUMBER "Number of Threads exceed the limit."
* @brief A Macro that sets the error message for invalid threads number.
*/
#define INVALID_THREADS_NUMBER "Number of Threads exceed the limit."

/**
* @def INVALID_THREAD_TERMINATE "Invalid thread ID to terminate."
* @brief A Macro that sets the error message for invalid thread terminate.
*/
#define INVALID_THREAD_TERMINATE "Invalid Thread ID to terminate."

/**
* @def INVALID_THREAD_BLOCK "Invalid Thread ID to block."
* @brief A Macro that sets the error message for invalid thread block.
*/
#define INVALID_THREAD_BLOCK "Invalid Thread ID to block."

/**
* @def INVALID_THREAD_RESUME "Invalid Thread ID to resume."
* @brief A Macro that sets the error message for invalid thread resume.
*/
#define INVALID_THREAD_RESUME "Invalid Thread ID to resume."

/**
* @def INVALID_THREAD_RESUME "Invalid Thread ID to resume."
* @brief A Macro that sets the error message for invalid thread resume.
*/
#define INVALID_THREAD_SYNC "Invalid Thread ID to sync."

/**
* @def INVALID_THREAD_QUANTUMS "Invalid Thread ID to calculate quantums."
* @brief A Macro that sets the error message for invalid thread to get quantum.
*/
#define INVALID_THREAD_QUANTUMS "Invalid Thread ID to calculate quantums."

/**
* @def ERROR_MAIN_BLOCK "Unable to block the Main Thread."
* @brief A Macro that sets the error message for blocking the Main Thread.
*/
#define ERROR_MAIN_BLOCK "Unable to block the Main Thread."

/**
* @def ERROR_MAIN_SYNC "Unable to sync the Main Thread."
* @brief A Macro that sets the error message for sync the Main Thread.
*/
#define ERROR_MAIN_SYNC "Unable to sync the Main Thread."

/**
* @def ERROR_RUNNING_SYNC "Unable to sync to the running Thread."
* @brief A Macro that sets the error message for sync the running Thread.
*/
#define ERROR_RUNNING_SYNC "Unable to sync to the current running Thread."

/**
* @def MINIMUM_QUANTUM_USECS 1
* @brief A Macro that sets the minimum length of a quantum in micro-seconds.
*/
#define MINIMUM_QUANTUM_USECS 1

/**
* @def MINIMUM_THREAD_ID 0
* @brief A Macro that sets the minimum value for Thread ID.
*/
#define MINIMUM_THREAD_ID 0

/**
* @def MAIN_THREAD_ID 0
* @brief A Macro that sets the ID value for the Main Thread.
*/
#define MAIN_THREAD_ID 0

/**
* @def MINIMUM_THREADS_NUMBER 0
* @brief A Macro that sets the minimum number of Threads.
*/
#define MINIMUM_THREADS_NUMBER 0

/**
* @def SECONDS_TO_MICRO 1000000
* @brief A Macro that sets the ratio between seconds to microseconds.
*/
#define SECONDS_TO_MICRO 1000000

/**
* @def INITIAL_QUANTUM_COUNT 0
* @brief A Macro that sets the initial quantum count.
*/
#define INITIAL_QUANTUM_COUNT 0

/**
* @def INITIAL_TIMER_VALUE 0
* @brief A Macro that sets the initial timer value.
*/
#define INITIAL_TIMER_VALUE 0

/**
* @def INITIAL_RUNNING_QUANTUM 1
* @brief A Macro that sets the initial running quantum for the Main Thread.
*/
#define MAIN_INITIAL_QUANTUM 1


/*-----=  Type Definitions  =-----*/


/**
 * @brief Type Definition for Threads Double Ended Queue.
 */
typedef std::deque<ThreadID> ThreadQueue;


/*-----=  Global Variables  =-----*/


/**
 * @brief An array which helps to find the minimum available ID for a Thread.
 */
bool threadsID[MAX_THREAD_NUM] = {false};

/**
 * @brief The current number of all the Threads.
 */
unsigned int numberOfThreads = MINIMUM_THREADS_NUMBER;

/**
 * @brief The array of the Threads. At cell i will be placed Thread with ID i.
 */
Thread threads[MAX_THREAD_NUM];

/**
 * @brief The Jump Buffer envelope.
 */
sigjmp_buf env[MAX_THREAD_NUM];

/**
 * @brief The entire stack for the Threads.
 */
char stack[MAX_THREAD_NUM][STACK_SIZE];

/**
 * @brief The Threads Queue for the READY state Threads.
 */
ThreadQueue readyQueue;

/**
 * @brief The Threads Queue for the BLOCKED state Threads.
 */
ThreadQueue blockedQueue;

/**
 * @brief The Threads Queue for the synced Threads.
 */
ThreadQueue syncedQueue;

/**
 * @brief The ThreadID of the current running Thread.
 */
ThreadID runningThread = MAIN_THREAD_ID;

/**
 * @brief Total amount of quantums.
 */
int quantumCounter = INITIAL_QUANTUM_COUNT;

/**
 * @brief The Virtual Timer.
 */
struct itimerval timer;

/**
 * @brief Timer interval value in seconds.
 */
int q_sec = INITIAL_TIMER_VALUE;

/**
 * @brief Timer interval value in micro-seconds.
 */
int q_usec = INITIAL_TIMER_VALUE;


/*-----=  Forward Declarations  =-----*/


/**
 * @brief Block Virtual Time signals.
 * @param set Signal Set.
 */
void blockTimeSignal(sigset_t &set);

/**
 * @brief Unblock Virtual Time signals.
 * @param set Signal Set.
 */
void unblockTimeSignal(sigset_t &set);


/*-----=  Error Message Functions  =-----*/


/**
 * @brief A function that prints the error messages associated with the
 *        System Call errors.
 *        The function prints the error message in the required format, with
 *        the given error message text.
 * @param errorMessage The given text description of the error.
 */
static void systemError(const char *errorMessage)
{
    std::cerr << SYSCALL_ERROR_MSG << errorMessage << ERROR_MSG_SUFFIX;
}

/**
 * @brief A function that prints the error messages associated with the
 *        Thread Library errors.
 *        The function prints the error message in the required format, with
 *        the given error message text.
 * @param errorMessage The given text description of the error.
 */
static void threadLibraryError(const char *errorMessage)
{
    std::cerr << UTL_ERROR_MSG << errorMessage << ERROR_MSG_SUFFIX;
}


/*-----=  Scheduling Functions  =-----*/


/**
 * @brief Removes a given Thread from the given Queue.
 * @param tid The Thread to remove.
 * @param threadQueue The Queue to remove from.
 */
void removeFromQueue(ThreadID const tid, ThreadQueue &threadQueue)
{
    auto i = threadQueue.begin();
    for ( ; i != threadQueue.end(); ++i)
    {
        if (*i == tid)
        {
            // Found the Thread to remove.
            break;
        }
    }
    assert(i != threadQueue.end());
    threadQueue.erase(i);
}

/**
 * @brief Change the queue in which the given Thread 'tid' will be placed.
 * @param tid The Thread ID to move.
 * @param oldQueue The old queue to move from.
 * @param newQueue The new queue to move into.
 */
void changeQueue(ThreadID const tid, ThreadQueue &oldQueue,
                 ThreadQueue &newQueue)
{
    removeFromQueue(tid, oldQueue);
    newQueue.push_back(tid);
}

/**
 * @brief Release the given Thread 'tid' from it's sync state and update it's
 *        status and queue placing.
 * @param toRelease The Thread ID to release from sync mode.
 */
void releaseSynced(ThreadID const toRelease)
{
    assert(toRelease >= MINIMUM_THREAD_ID && toRelease < MAX_THREAD_NUM);
    threads[toRelease].clearSync();  // Thread can sync to one Thread at a time.
    ThreadState state = threads[toRelease].getState();
    switch (state)
    {
        case READY:
            changeQueue(toRelease, syncedQueue, readyQueue);
            break;
        case BLOCKED:
            changeQueue(toRelease, syncedQueue, blockedQueue);
            break;
        default:
            break;
    }
}

/**
 * @brief Release all associated Threads that synced to the given Thread ID
 *        from their sync state.
 * @param tid The Thread ID which all the Threads that synced
 *        to it should be released.
 */
void releaseAllSynced(ThreadID const tid)
{
    assert(tid >= MINIMUM_THREAD_ID && tid < MAX_THREAD_NUM);
    // The list of all Threads synced to Thread 'tid'.
    ThreadList syncedThreads = threads[tid].syncedThreads();
    for (auto i = syncedThreads.begin() ; i != syncedThreads.end(); ++i)
    {
        releaseSynced(*i);
    }
    threads[tid].clearSyncedThreads();
}

/**
 * @brief Switch Thread function used by the Signal Handler and the Scheduler.
 * @param toReady A flag indicates if the Thread needs to be in ready or not.
 */
void switchThreads(bool const toReady)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    int ret_val = sigsetjmp(env[runningThread], 1);
    if (ret_val == 1)
    {
        unblockTimeSignal(set);
        return;
    }

    // After the running Thread finished it's run, we release all the
    // Threads that were synced to it and waited for it to run.
    releaseAllSynced(runningThread);

    if (toReady)
    {
        // Move the Running Thread to Ready.
        threads[runningThread].setState(READY);
        readyQueue.push_back(runningThread);
    }

    // Set the first in line ready thread to running.
    runningThread = readyQueue.front();
    readyQueue.pop_front();
    threads[runningThread].setState(RUNNING);

    // Update quantum.
    threads[runningThread].incRunningQuantums();
    quantumCounter++;

    siglongjmp(env[runningThread], 1);
}


/*-----=  Time Signal Functions  =-----*/


/**
 * @brief Block Virtual Time signals.
 * @param set Signal Set.
 */
void blockTimeSignal(sigset_t &set)
{
    if (sigemptyset(&set))
    {
        systemError(SIGNAL_BLOCK_ERROR);
        exit(1);
    }

    if (sigaddset(&set, SIGVTALRM))
    {
        systemError(SIGNAL_BLOCK_ERROR);
        exit(1);
    }

    if (sigprocmask(SIG_BLOCK, &set, NULL))
    {
        systemError(SIGNAL_BLOCK_ERROR);
        exit(1);
    }
}

/**
 * @brief Unblock Virtual Time signals.
 * @param set Signal Set.
 */
void unblockTimeSignal(sigset_t &set)
{
    if (sigaddset(&set, SIGVTALRM))
    {
        systemError(SIGNAL_UNBLOCK_ERROR);
        exit(1);
    }

    if (sigprocmask(SIG_UNBLOCK, &set, NULL))
    {
        systemError(SIGNAL_UNBLOCK_ERROR);
        exit(1);
    }
}

/**
 * @brief Resets the Virtual Timer.
 * @return On success, return 0. On failure, exit with value 1.
 */
int resetTimer()
{
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
    {
        systemError(VIRTUAL_TIMER_FAILURE);
        exit(1);
    }
    return SUCCESS_STATE;
}

/**
 * @brief Signal Handler for the timer.
 * @param sigNum The signal number.
 */
void timerHandler(int sigNum)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    switchThreads(true);

    // Unblock Signals.
    unblockTimeSignal(set);
}

/**
 * @brief Setup the timer and the timer handler.
 * @param quantum_usecs The length of a quantum in micro-seconds.
 * @return On success, return 0. On failure, return -1.
 */
int setupTimer(const int quantum_usecs)
{
    // Setup the data for the timer and signal handlers.
    struct sigaction sa;

    // Install 'timer_handler' as the signal handler for SIGVTALRM.
    sa.sa_handler = &timerHandler;
    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
    {
        systemError(SIGNAL_HANDLER_FAILURE);
        exit(1);
    }

    // Calculate the value of Seconds and the value of Micro-Seconds.
    assert(quantum_usecs >= MINIMUM_QUANTUM_USECS);
    q_sec = quantum_usecs / SECONDS_TO_MICRO;
    q_usec = quantum_usecs % SECONDS_TO_MICRO;

    // Configure the timer to expire after the given 'quantum_usecs'.
    timer.it_value.tv_sec = q_sec;
    timer.it_value.tv_usec = q_usec;
    // Configure the timer to expire every 'quantum_usecs' after that.
    timer.it_interval.tv_sec = q_sec;
    timer.it_interval.tv_usec = q_usec;

    // Start a Virtual Timer.
    resetTimer();

    return SUCCESS_STATE;
}


/*-----=  Thread ID Functions  =-----*/


/**
 * @brief Receives the minimum available ID for a Thread.
 * @return The minimum available ID for a Thread.
 *         If no ID is available returns MAX_THREAD_NUM.
 */
ThreadID getAvailableID()
{
    assert(numberOfThreads < MAX_THREAD_NUM);
    for (ThreadID i = MINIMUM_THREAD_ID; i < MAX_THREAD_NUM; ++i)
    {
        // Iterate through the available ID array and check for a cell which
        // contains the value 'false', this means that this ID is available.
        if (!threadsID[i])
        {
            return i;
        }
    }
    return MAX_THREAD_NUM;
}

/**
 * @brief Sets the given ID to be unavailable.
 * @param id The ID to set.
 */
void setIDUnavailable(ThreadID const id)
{
    assert(id >= MINIMUM_THREAD_ID && id < MAX_THREAD_NUM);
    threadsID[id] = true;
}

/**
 * @brief Sets the given ID to be available.
 * @param id The ID to set.
 */
void setIDAvailable(ThreadID const id)
{
    assert(id >= MINIMUM_THREAD_ID && id < MAX_THREAD_NUM);
    threadsID[id] = false;
}

/**
 * @brief Validates the given Thread ID and checks that it is in the required
 *        ID values and that ID is an ID of an active Thread.
 * @param tid The Thread ID to validate.
 * @param invalidMessage The message in case the validation failed.
 * @return On success, return 0. On failure, return -1.
 */
int threadIDValidation(ThreadID const tid, const char *invalidMessage)
{
    if (tid < MINIMUM_THREAD_ID || tid >= MAX_THREAD_NUM)
    {
        // If the given Thread ID is an invalid ID.
        threadLibraryError(invalidMessage);
        return FAILURE_STATE;
    }

    if (!threadsID[tid])
    {
        // If the ID is not in use, i.e. there is no Thread with this ID.
        threadLibraryError(invalidMessage);
        return FAILURE_STATE;
    }

    return SUCCESS_STATE;
}


/*-----=  Spawn Functions  =-----*/


/**
 * @brief Setup a Thread when Spawned.
 * @param f The entry point of the new Thread.
 * @param threadID The ID of the Thread to setup.
 */
void setupSpawn(void (*f)(void), ThreadID const threadID)
{
    assert(threadID >= MINIMUM_THREAD_ID && threadID < MAX_THREAD_NUM);
    address_t sp, pc;
    sp = (address_t)stack[threadID] + STACK_SIZE - sizeof(address_t);
    pc = (address_t)f;
    sigsetjmp(env[threadID], 1);
    (env[threadID]->__jmpbuf)[JB_SP] = translate_address(sp);
    (env[threadID]->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env[threadID]->__saved_mask);
}

/**
 * @brief Updates the required data when a new Thread is spawned.
 * @param spawnedID The ID of the new Thread that was spawned.
 */
void updateDataSpawned(ThreadID const spawnedID)
{
    setIDUnavailable(spawnedID);
    numberOfThreads++;
}

/**
 * @brief Spawn the Main Thread and set it to be running Thread with
 *        total quantums of 1.
 */
void spawnMain()
{
    // Create the Main Thread.
    Thread mainThread(MAIN_INITIAL_QUANTUM);
    threads[MAIN_THREAD_ID] = mainThread;

    // Update all the required data that changed due to this main Thread spawn.
    updateDataSpawned(MAIN_THREAD_ID);

    // Setup the data of the Main Thread.
    setupSpawn(nullptr, MAIN_THREAD_ID);

    // Set Main Thread to Running.
    runningThread = MAIN_THREAD_ID;
}


/*-----=  Terminate Functions  =-----*/


/**
 * @brief Updates the required data when a Thread is terminated.
 * @param terminatedID The ID of the new Thread that was terminated.
 */
void updateDataTerminated(ThreadID const terminatedID)
{
    setIDAvailable(terminatedID);
    numberOfThreads--;
}


/*-----=  Library Functions  =-----*/


/**
 * @brief This function initializes the thread library.
 *        You may assume that this function is called before any other
 *        thread library function, and that it is called exactly once.
 *        It is an error to call this function with non-positive quantum_usecs.
 * @param quantum_usecs The length of a quantum in micro-seconds.
 * @return On success, return 0. On failure, return -1.
 */
int uthread_init(int quantum_usecs)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    if (quantum_usecs < MINIMUM_QUANTUM_USECS)
    {
        threadLibraryError(INVALID_QUANTUM_LENGTH);
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    // Setup Timer.
    if (setupTimer(quantum_usecs))
    {
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    // Spawn Main Thread.
    spawnMain();

    // Update Quantum Counter.
    quantumCounter++;

    unblockTimeSignal(set);
    return SUCCESS_STATE;
}

/**
 * @brief This function creates a new thread, whose entry point is the
 *        function f with the signature void f(void).
 *        The thread is added to the end of the READY threads list.
 *        The uthread_spawn function should fail if it would cause the
 *        number of concurrent threads to exceed the limit (MAX_THREAD_NUM).
 *        Each thread should be allocated with a stack of size STACK_SIZE bytes.
 * @param f The entry point of the new Thread.
 * @return On success, return the ID of the created thread.
 *         On failure, return -1.
 */
int uthread_spawn(void (*f)(void))
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    if (numberOfThreads == MAX_THREAD_NUM)
    {
        threadLibraryError(INVALID_THREADS_NUMBER);
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    // Create a new Thread with the minimum available ID.
    ThreadID threadID = getAvailableID();
    assert(threadID != MAX_THREAD_NUM);
    threads[threadID] = Thread();

    // Update all the required data that changed due to this new Thread spawn.
    updateDataSpawned(threadID);

    // The new Thread should be in the Ready Queue.
    readyQueue.push_back(threadID);

    // Setup the data of the Thread.
    setupSpawn(f, threadID);

    unblockTimeSignal(set);
    return threadID;
}

/**
 * @brief This function terminates the thread with ID tid and deletes
 *        it from all relevant control structures. All the resources allocated
 *        by the library for this thread should be released. If no thread with
 *        ID tid exists it is considered as an error. Terminating the main
 *        thread (tid == 0) will result in the termination of the entire
 *        process using exit(0) [after releasing the assigned library memory].
 * @param tid The ID of the Thread to terminate.
 * @return The function returns 0 if the thread was successfully
 *         terminated and -1 otherwise. If a thread terminates itself or
 *         the main thread is terminated, the function does not return.
 */
int uthread_terminate(int tid)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    if (threadIDValidation(tid, INVALID_THREAD_TERMINATE))
    {
        // If Thread ID validation failed.
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    // If terminate is on the Main Thread.
    if (tid == MAIN_THREAD_ID)
    {
        unblockTimeSignal(set);
        exit(0);
    }

    assert(tid >= MINIMUM_THREAD_ID && tid < MAX_THREAD_NUM);

    // Release all Threads that synced to this Thread, since it terminates.
    releaseAllSynced(tid);

    // If the Thread to terminate is in sync mode.
    if (threads[tid].isSync())
    {
        // Remove this Thread from syncedQueue.
        removeFromQueue(tid, syncedQueue);
        // Remove this Thread from the synced list of the Thread it synced to.
        ThreadID syncedTo = threads[tid].getSyncedID();
        assert(syncedTo >= MINIMUM_THREAD_ID && tid < MAX_THREAD_NUM);
        threads[syncedTo].removeSyncedThread(tid);
    }
    else
    {
        // If the Thread to terminate is not synced to any other Thread.
        ThreadState state = threads[tid].getState();
        switch (state)
        {
            case READY:
                removeFromQueue(tid, readyQueue);
                break;

            case BLOCKED:
                removeFromQueue(tid, blockedQueue);
                break;

            case RUNNING:
                // Update all the required data that changed due
                // to this Thread termination.
                updateDataTerminated(tid);
                // Reset the Virtual Timer.
                resetTimer();
                switchThreads(false);
                break;

            default:
                break;
        }
    }

    // Update all the required data that changed due to this Thread termination.
    updateDataTerminated(tid);

    unblockTimeSignal(set);
    return SUCCESS_STATE;
}

/**
 * @brief This function blocks the thread with ID tid. The thread may
 *        be resumed later using uthread_resume. If no thread with ID tid exists
 *        it is considered as an error. In addition, it is an error to try
 *        blocking the main thread (tid == 0). If a thread blocks itself, a
 *        scheduling decision should be made. Blocking a thread in BLOCKED state
 *        has no effect and is not considered as an error.
 * @param tid The ID of the Thread to block.
 * @return On success, return 0. On failure, return -1.
 */
int uthread_block(int tid)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    if (threadIDValidation(tid, INVALID_THREAD_BLOCK))
    {
        // If Thread ID validation failed.
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    if (tid == MAIN_THREAD_ID)
    {
        // It is an error to try and block the main Thread.
        threadLibraryError(ERROR_MAIN_BLOCK);
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    assert(tid >= MINIMUM_THREAD_ID && tid < MAX_THREAD_NUM);

    if (threads[tid].isSync())
    {
        // If the Thread to block is in sync mode we only change it's state
        // but keeping it in sync mode.
        threads[tid].setState(BLOCKED);
    }
    else
    {
        // If the Thread 'tid' is not in sync.
        ThreadState state = threads[tid].getState();
        switch (state)
        {
            case READY:
                changeQueue(tid, readyQueue, blockedQueue);
                threads[tid].setState(BLOCKED);
                break;

            case RUNNING:
                blockedQueue.push_back(tid);
                threads[tid].setState(BLOCKED);
                // Reset the Virtual Timer.
                resetTimer();
                switchThreads(false);
                break;

            default:
                break;
        }
    }

    unblockTimeSignal(set);
    return SUCCESS_STATE;
}

/**
 * @brief This function resumes a blocked thread with ID tid and moves it to
 *        the READY state. Resuming a thread in a RUNNING or READY state
 *        has no effect and is not considered as an error. If no thread with
 *        ID tid exists it is considered as an error.
 * @param tid The ID of the Thread to resume.
 * @return On success, return 0. On failure, return -1.
 */
int uthread_resume(int tid)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    if (threadIDValidation(tid, INVALID_THREAD_RESUME))
    {
        // If Thread ID validation failed.
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    assert(tid >= MINIMUM_THREAD_ID && tid < MAX_THREAD_NUM);

    if (threads[tid].isSync())
    {
        // If the Thread to resume is in sync mode we only change it's state
        // but keeping it in sync mode.
        threads[tid].setState(READY);
    }
    else
    {
        // If the Thread 'tid' is not in sync.
        ThreadState state = threads[tid].getState();
        switch (state)
        {
            case BLOCKED:
                changeQueue(tid, blockedQueue, readyQueue);
                threads[tid].setState(READY);
                break;

            default:
                break;
        }
    }

    unblockTimeSignal(set);
    return SUCCESS_STATE;
}

/**
 * @brief This function blocks the RUNNING thread until thread with
 *        ID tid will move to RUNNING state (i.e.right after the next time that
 *        thread tid will stop running, the calling thread will be resumed
 *        automatically). If thread with ID tid will be terminated
 *        before RUNNING again, the calling thread should move to READY
 *        state right after thread tid is terminated
 *        (i.e. it won’t be blocked forever). It is considered as an error if
 *        no thread with ID tid exists or if the main thread (tid==0)
 *        calls this function. Immediately after the RUNNING thread transitions
 *        to the BLOCKED state a scheduling decision should be made.
 * @param tid The ID of the Thread to sync to.
 * @return On success, return 0. On failure, return -1.
 */
int uthread_sync(int tid)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    if (threadIDValidation(tid, INVALID_THREAD_SYNC))
    {
        // If Thread ID validation failed.
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    if (runningThread == tid)
    {
        // It is an error to sync to itself.
        threadLibraryError(ERROR_RUNNING_SYNC);
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    if (runningThread == MAIN_THREAD_ID)
    {
        // It is an error to sync the Main Thread.
        threadLibraryError(ERROR_MAIN_SYNC);
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    assert(tid >= MINIMUM_THREAD_ID && tid < MAX_THREAD_NUM);

    // Set the running thread to be sync.
    threads[runningThread].setSync(tid);
    // Add to Thread 'tid' the running thread as synced to it.
    threads[tid].insertSyncedThread(runningThread);

    // Add the running Thread to syncedQueue and set it's state to READY
    // for later, this state may change before it is unsynced.
    syncedQueue.push_back(runningThread);
    threads[runningThread].setState(READY);

    // Reset the Virtual Timer.
    resetTimer();

    // Switch Threads.
    switchThreads(false);

    unblockTimeSignal(set);
    return SUCCESS_STATE;
}

/**
 * @brief This function returns the thread ID of the calling thread.
 * @return The ID of the calling thread.
 */
int uthread_get_tid()
{
    return runningThread;
}

/**
 * @brief This function returns the total number of quantums that were
 *        started since the library was initialized, including the
 *        current quantum. Right after the call to uthread_init, the
 *        value should be 1. Each time a new quantum starts, regardless of
 *        the reason, this number should be increased by 1.
 * @return The total number of quantums.
 */
int uthread_get_total_quantums()
{
    return quantumCounter;
}

/**
 * @brief This function returns the number of quantums the thread with
 *        ID tid was in RUNNING state. On the first time a thread runs,
 *        the function should return 1. Every additional quantum that the
 *        thread starts should increase this value by 1 (so if the thread with
 *        ID tid is in RUNNING state when this function is called,
 *        include also the current quantum). If no thread with ID tid exists
 *        it is considered as an error.
 * @param tid The ID of the Thread to get it's quantums.
 * @return On success, return the number of quantums of the thread with ID tid.
 *         On failure, return -1.
 */
int uthread_get_quantums(int tid)
{
    // Block Signals.
    sigset_t set;
    blockTimeSignal(set);

    if (threadIDValidation(tid, INVALID_THREAD_QUANTUMS))
    {
        // If Thread ID validation failed.
        unblockTimeSignal(set);
        return FAILURE_STATE;
    }

    assert(tid >= MINIMUM_THREAD_ID && tid < MAX_THREAD_NUM);
    int threadQuantum = threads[tid].getRunningQuantums();

    unblockTimeSignal(set);
    return threadQuantum;
}
