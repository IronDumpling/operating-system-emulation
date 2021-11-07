#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stdbool.h>
#include "thread.h"
#include "interrupt.h"

/* Section 1. Data Structure */
/* Wait queue structure */
typedef struct wait_queue {
    int size;
    struct thread *head;
    struct thread *tail;
}threadQueue;

/* Thread State List */
typedef enum {
    EXIT = 0,
    READY = 1,
    RUNNING = 2,
    SLEEP = 3
} THREAD_STATUS;

/* Thread control block */
typedef struct thread {
	Tid id;
	ucontext_t context;
	void *stackPtr;
	struct thread *next;
}threadNode;

/* Section 2. Global Variables */
struct wait_queue readyQueue, exitQueue;
struct wait_queue* waitQueue[THREAD_MAX_THREADS] = {NULL};
THREAD_STATUS threads[THREAD_MAX_THREADS] = {EXIT};

/* Section 3. Queue Helper Functions */
/**
 * Function 3.1 Appends given node to queue
 * @param q
 * @param node
 */
void enqueueNode(threadQueue *q, threadNode *node) {
    if (q->head == NULL) {
        q->head = node;
        q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = q->tail->next;
    }
    q->size++;
}

/**
 * Function 3.2 Moves the target node to the head of the queue
 * @param q
 * @param target
 * @param prev
 */
void enqueueHead(threadQueue *q, threadNode *target, threadNode *prev) {
    if (q->size <= 1 || target == q->head)
        return;

    threadNode *prevHead = q->head;
    q->head = target;
    prev->next = target->next;

    target->next = prevHead;

    if (q->tail == target){
        q->tail = prev;
    }
}

/**
 * Function 3.3 Moves the head of the queue to the end
 * @param q
 */
void enqueueEnd(threadQueue *q) {
	if (q->size <= 1)
		return;
    threadNode *prevHead = q->head;
	q->head = q->head->next;
	q->tail->next = prevHead;
	q->tail = prevHead;
    prevHead->next = NULL;
}

/**
 * Function 3.4 Removes and returns and the head of the queue
 * @param q
 * @return
 */
threadNode* dequeue(threadQueue *q) {
    threadNode* prevHead = q->head;
	q->head = q->head->next;
    prevHead->next = NULL;
	q->size--;
	return prevHead;
}

/**
 * Function 3.5 Frees all nodes in the given queue
 * @param q
 */
void freeQueue(threadQueue *q) {
	if (!q) return;

	while (q->head) {
        threadNode *next = q->head->next;
        free(q->head->stackPtr);
        free(q->head);
		q->head = next;
	}
	q->tail = NULL;
	q->size = 0;
}

/**
 * Function 3.6 Assign ID to a new Thread
 * @return
 */
Tid minAvailableID(){
    int i = 0;
    while(i < THREAD_MAX_THREADS && threads[i]) i++;
    return (i < THREAD_MAX_THREADS) ? i : THREAD_MAX_THREADS;
}

/* Section 4. Thread Library Functions */
/*
 * Function 4.0 Stub
 * The First Thread Running Function
 * */
void thread_stub(void (*thread_main)(void *), void *arg)
{
	interrupts_on();
	thread_main(arg);
	thread_exit();
}

/*
 * Function 4.1 Init
 * Get First Running Thread
 * */
void thread_init(void)
{
	// Create the first thread
    threadNode *curr = (threadNode*) malloc(sizeof(threadNode));
    curr->id = 0;
    curr->next = NULL;
    curr->stackPtr = NULL;
    threads[0] = RUNNING;
    getcontext(&curr->context);

	// Initialize the queue sizes
	readyQueue.size = 0;
    exitQueue.size = 0;

	// Add thread to the ready queue
    enqueueNode(&readyQueue, curr);
}

/*
 * Function 4.2 Get Thread ID
 * */
Tid thread_id()
{
	return readyQueue.head->id;
}

/*
 * Function 4.3 Create
 * None->Ready
 * */
Tid thread_create(void (*fn) (void *), void *parg)
{
	int enabled = interrupts_off();

	// Find an available thread id
	Tid id = minAvailableID();
	if (id == THREAD_MAX_THREADS) {
		interrupts_set(enabled);
		return THREAD_NOMORE;
	}

    // Step 1. Create new threads at tail
    threadNode *curr = (threadNode*) malloc(sizeof(threadNode));
	if (curr == NULL) {
		interrupts_set(enabled);
		return THREAD_NOMEMORY;
	}

    // Step 2. Assign Members
	curr->id = id;
	curr->stackPtr = malloc(THREAD_MIN_STACK);

	// Corner Case 2. No memory available for thread stackPtr
    if (curr->stackPtr == NULL) {
        free(curr);
        interrupts_set(enabled);
        return THREAD_NOMEMORY;
    }

	getcontext(&curr->context);

    curr->context.uc_mcontext.gregs[REG_RDI] = (long long int)fn;
    curr->context.uc_mcontext.gregs[REG_RSI] = (long long int)parg;
    curr->context.uc_mcontext.gregs[REG_RSP] = (long long int)curr->stackPtr + THREAD_MIN_STACK - 8;
    curr->context.uc_mcontext.gregs[REG_RIP] = (long long int)&thread_stub;

	// Add newly created thread to the end of the ready queue
	enqueueNode(&readyQueue, curr);

	// Thread id of newly created thread is now taken
	threads[id] = READY;

	interrupts_set(enabled);
	return id;
}

/*
 * Function 4.5 Yield
 * Running->Ready & Ready->Running
 * */
Tid thread_yield(Tid want_tid) {
    int enable = interrupts_off();

    freeQueue(&exitQueue);

    threadNode *wantThread, *currentThread = readyQueue.head;

    // Case 1. Want Current Running Thread
    // Let running thread continue running
    if (want_tid == THREAD_SELF || want_tid == readyQueue.head->id) {
        wantThread = readyQueue.head;
        want_tid = readyQueue.head->id;
    } else if (want_tid == THREAD_ANY) {
        // Case 2. Want Any Ready Thread
        // Get Ready Queue Head and let it run
        if (readyQueue.size == 1) {
            interrupts_set(enable);
            return THREAD_NONE;
        }

        wantThread = readyQueue.head->next;
        want_tid = wantThread->id;

        // Move current thread to end of ready queue
        enqueueEnd(&readyQueue);
    } else {
        // Case 3. Want Specific Ready Thread
        // Get the want ID thread and let it run
        threadNode *prev;

        if (readyQueue.head->id == want_tid) {
            prev = NULL;
            wantThread = readyQueue.head;
        } else{
            threadNode* current = readyQueue.head;

            while (current->next != NULL) {
                if (current->next->id == want_tid)
                    break;
                current = current->next;
            }

            prev = current;
            wantThread = current->next;
        }

        if (!wantThread) {
            interrupts_set(enable);
            return THREAD_INVALID;
        }

        // Move current thread to end of ready queue
        enqueueEnd(&readyQueue);
        enqueueHead(&readyQueue, wantThread, prev);
    }

    // Flag to check if returning from a different thread
    volatile bool isCalled = false;

    // Set current thread to ready state
    if (threads[currentThread->id] != EXIT) {
        threads[currentThread->id] = READY;
    }

    // Save context for current thread
    getcontext(&currentThread->context);

    // Returning to this thread from a different thread
    if (!isCalled) {
        isCalled = true;
        setcontext(&wantThread->context);
    }

    if (threads[readyQueue.head->id] == EXIT){
        thread_exit();
    }else {
        threads[readyQueue.head->id] = RUNNING;
    }

	interrupts_set(enable);
	return want_tid;
}

/*
 * Function 4.6 Exit
 * Running->Exit
 * */
void thread_exit()
{
    int enable = interrupts_off();
	threads[thread_id()] = EXIT;

	// Wakeup all threads waiting on this thread's exit
	thread_wakeup(waitQueue[thread_id()], 1);

	// Delete the wait queue for this thread
	wait_queue_destroy(waitQueue[thread_id()]);
    waitQueue[thread_id()] = NULL;

	// Pop head of ready queue
    threadNode* exitThread = dequeue(&readyQueue);

	if (!readyQueue.head) {
		// This is the last running thread.
        freeQueue(&exitQueue);

		for (int i = 0; i < THREAD_MAX_THREADS; ++i){
            wait_queue_destroy(waitQueue[i]);
		}

		// Deallocate memory for this thread
        free(exitThread->stackPtr);
		free(exitThread);

        interrupts_set(enable);
		exit(0);
	}

	// Append exited thread to exit queue
    enqueueNode(&exitQueue, exitThread);
	setcontext(&readyQueue.head->context);
    interrupts_set(enable);
}

/*
 * Function 4.7 Kill
 * Ready->Exit
 * */
Tid thread_kill(Tid tid)
{
	int enable = interrupts_off();

    // Corner Case: Invalid ID
	if (tid < 0 || tid >= THREAD_MAX_THREADS ||
	    tid == thread_id() || !threads[tid]){
		interrupts_set(enable);
		return THREAD_INVALID;
	}

    // Common Case: Insert Wanted Thread to Exit Queue
    threads[tid] = EXIT;
	interrupts_set(enable);
	return tid;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/
/* make sure to fill the wait_queue structure defined above */
struct wait_queue * wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	wq->size = 0;
	wq->head = NULL;
	wq->tail = NULL;

	return wq;
}

void wait_queue_destroy(struct wait_queue *wq)
{
    freeQueue(wq);
	free(wq);
}

Tid thread_sleep(struct wait_queue *queue)
{
	int enabled = interrupts_off();

	// Corner Case 1. Invalid Queue
	if (!queue) {
		interrupts_set(enabled);
		return THREAD_INVALID;
	}
    // Corner Case 2. No ready Thread
	if (readyQueue.size <= 1) {
		interrupts_set(enabled);
		return THREAD_NONE;
	}

	threadNode *currentThread = dequeue(&readyQueue);
    enqueueNode(queue, currentThread);

    threadNode *newThread = readyQueue.head;
	int ret = newThread->id;

    // Check if returns from another thread
	volatile bool isCalled = false;

    threads[currentThread->id] = SLEEP;
	getcontext(&currentThread->context);

	if (!isCalled) {
        isCalled = true;
        setcontext(&newThread->context);
	}

    if (threads[readyQueue.head->id] == EXIT){
        thread_exit();
    }else {
        threads[readyQueue.head->id] = RUNNING;
    }
	
	interrupts_set(enabled);
	return ret;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int thread_wakeup(threadQueue *queue, int all)
{
	int enabled = interrupts_off();

    // Corner Case: Invalid or empty queue
	if (!queue || !queue->size) {
		interrupts_set(enabled);
		return 0;
	}

	int count = 0;
	// Case 1. wake up one
	if(!all){
        threadNode *node = dequeue(queue);

        if(threads[node->id] != EXIT){
            threads[node->id] = READY;
        }

        enqueueNode(&readyQueue, node);
		count++;
	}else{
	    // Case 2. Wake up all
        threadNode *node;
        while(queue->head){
            node = dequeue(queue);

            if(threads[node->id] != EXIT){
                threads[node->id] = READY;
            }

            enqueueNode(&readyQueue, node);
            count++;
        }
	}

	interrupts_set(enabled);
	return count;
}

/* suspend current thread until Thread tid exits */
Tid thread_wait(Tid tid)
{
	int enabled = interrupts_off();

    // Corner Cases
	if (tid < 0 || tid == thread_id() || !threads[tid]) {
		interrupts_set(enabled);
		return THREAD_INVALID;
	}

	// Common Cases
	int ret_id = tid;
	threadQueue *wq = waitQueue[tid];
	if (!wq) {
        waitQueue[tid] = wait_queue_create();
		wq = waitQueue[tid];
	}

	thread_sleep(wq);
	interrupts_set(enabled);
	return ret_id;
}

typedef struct lock {
	bool isLocked;
	Tid thread;
	threadQueue *wq;
}Lock;

Lock * lock_create()
{
	int enable = interrupts_off();
	Lock *lock;

	lock = (Lock*)malloc(sizeof(Lock));
	assert(lock);

	lock->isLocked = false;
	lock->wq = wait_queue_create();

	interrupts_set(enable);
	return lock;
}

void lock_destroy(Lock *lock)
{
	int enable = interrupts_off();
	assert(lock);
	assert(!lock->isLocked);

	wait_queue_destroy(lock->wq);

	free(lock);
	interrupts_set(enable);
}

void lock_acquire(Lock *lock)
{
	int enable = interrupts_off();
	assert(lock);

	// Wait until lock is released
	while (lock->isLocked) {
		thread_sleep(lock->wq);
	}

	// Acquire the lock
	lock->thread = thread_id();
    lock->isLocked = true;

	interrupts_set(enable);
}

void lock_release(Lock *lock)
{
	int enable = interrupts_off();
	assert(lock);
	assert(lock->isLocked && lock->thread == thread_id());

	// Release the lock
	lock->isLocked = false;

	// Wake up all threads waiting on the lock
	thread_wakeup(lock->wq, 1);	
	
	interrupts_set(enable);
}

struct cv {
	threadQueue *wq;
};

struct cv * cv_create()
{
	int enable = interrupts_off();

	struct cv *cv = (struct cv*)malloc(sizeof(struct cv));
	assert(cv);
	cv->wq = wait_queue_create();

	interrupts_set(enable);
	return cv;
}

void cv_destroy(struct cv *cv)
{
	int enable = interrupts_off();
	assert(cv);
	assert(!cv->wq->size);

	wait_queue_destroy(cv->wq);
	free(cv);

	interrupts_set(enable);
}

void cv_wait(struct cv *cv, Lock *lock)
{
	int enable = interrupts_off();
	assert(cv);
	assert(lock);
	//assert(lock->isLocked && lock->thread == thread_id());

	lock_release(lock);
	thread_sleep(cv->wq);
	lock_acquire(lock);

	interrupts_set(enable);
}

void cv_signal(struct cv *cv, Lock *lock)
{
	int enabled = interrupts_off();
	assert(cv);
	assert(lock);

	thread_wakeup(cv->wq, 0);
	interrupts_set(enabled);
}

void cv_broadcast(struct cv *cv, Lock *lock)
{
	int enabled = interrupts_off();
	assert(cv);
	assert(lock);

	thread_wakeup(cv->wq, 1);
	interrupts_set(enabled);
}
