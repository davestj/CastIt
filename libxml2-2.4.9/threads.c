/**
 * threads.c: set of generic threading related routines 
 *
 * See Copyright for the status of this software.
 *
 * Gary Pennington <Gary.Pennington@uk.sun.com>
 * daniel@veillard.com
 */

#include "libxml.h"

#include <string.h>

#include <libxml/threads.h>
#include <libxml/globals.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if defined(SOLARIS)
#include <note.h>
#endif

/* #define DEBUG_THREADS */

/*
 * TODO: this module still uses malloc/free and not xmlMalloc/xmlFree
 *       to avoid some crazyness since xmlMalloc/xmlFree may actually
 *       be hosted on allocated blocks needing them for the allocation ...
 */

/*
 * xmlMutex are a simple mutual exception locks
 */
struct _xmlMutex {
#ifdef HAVE_PTHREAD_H
    pthread_mutex_t lock;
#else
    int empty;
#endif
};

/*
 * xmlRMutex are reentrant mutual exception locks
 */
struct _xmlRMutex {
#ifdef HAVE_PTHREAD_H
    pthread_mutex_t lock;
    unsigned int    held;
    unsigned int    waiters;
    pthread_t       tid;
    pthread_cond_t  cv;
#else
    int empty;
#endif
};
/*
 * This module still has some internal static data.
 *   - xmlLibraryLock a global lock
 *   - globalkey used for per-thread data
 *   - keylock protecting globalkey
 *   - keyonce to mark initialization of globalkey
 */

static int initialized = 0;
#ifdef HAVE_PTHREAD_H
static pthread_mutex_t	keylock = PTHREAD_MUTEX_INITIALIZER;
static pthread_key_t	globalkey;
static int		keyonce = 0;
static pthread_t	mainthread;
#endif
static xmlRMutexPtr	xmlLibraryLock = NULL;

#if defined(SOLARIS)
NOTE(DATA_READABLE_WITHOUT_LOCK(keyonce))
#endif

/**
 * xmlMutexPtr:
 *
 * xmlNewMutex() is used to allocate a libxml2 token struct for use in
 * synchronizing access to data.
 *
 * Returns a new simple mutex pointer or NULL in case of error
 */
xmlMutexPtr
xmlNewMutex(void)
{
    xmlMutexPtr tok;

    if ((tok = malloc(sizeof(xmlMutex))) == NULL)
        return (NULL);
#ifdef HAVE_PTHREAD_H
    pthread_mutex_init(&tok->lock, NULL);
#endif
    return (tok);
}

/**
 * xmlFreeMutex:
 * @tok:  the simple mutex
 *
 * xmlFreeMutex() is used to reclaim resources associated with a libxml2 token
 * struct.
 */
void
xmlFreeMutex(xmlMutexPtr tok)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&tok->lock);
#endif
    free(tok);
}

/**
 * xmlMutexLock:
 * @tok:  the simple mutex
 *
 * xmlMutexLock() is used to lock a libxml2 token.
 */
void
xmlMutexLock(xmlMutexPtr tok)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&tok->lock);
#endif

}

/**
 * xmlMutexUnlock:
 * @tok:  the simple mutex
 *
 * xmlMutexUnlock() is used to unlock a libxml2 token.
 */
void
xmlMutexUnlock(xmlMutexPtr tok)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_unlock(&tok->lock);
#endif
}

/**
 * xmlRNewMutex:
 *
 * xmlRNewMutex() is used to allocate a reentrant mutex for use in
 * synchronizing access to data. token_r is a re-entrant lock and thus useful
 * for synchronizing access to data structures that may be manipulated in a
 * recursive fashion.
 *
 * Returns the new reentrant mutex pointer or NULL in case of error
 */
xmlRMutexPtr
xmlNewRMutex(void)
{
    xmlRMutexPtr tok;

    if ((tok = malloc(sizeof(xmlRMutex))) == NULL)
        return (NULL);
#ifdef HAVE_PTHREAD_H
    pthread_mutex_init(&tok->lock, NULL);
    tok->held = 0;
    tok->waiters = 0;
#endif
    return (tok);
}

/**
 * xmlRFreeMutex:
 * @tok:  the reentrant mutex
 *
 * xmlRFreeMutex() is used to reclaim resources associated with a
 * reentrant mutex.
 */
void
xmlFreeRMutex(xmlRMutexPtr tok)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&tok->lock);
#endif
    free(tok);
}

/**
 * xmlRMutexLock:
 * @tok:  the reentrant mutex
 *
 * xmlRMutexLock() is used to lock a libxml2 token_r.
 */
void
xmlRMutexLock(xmlRMutexPtr tok)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&tok->lock);
    if (tok->held) {
        if (pthread_equal(tok->tid, pthread_self())) {
            tok->held++;
            pthread_mutex_unlock(&tok->lock);
            return;
        } else {
            tok->waiters++;
            while (tok->held)
                pthread_cond_wait(&tok->cv, &tok->lock);
            tok->waiters--;
        }
    }
    tok->tid = pthread_self();
    tok->held = 1;
    pthread_mutex_unlock(&tok->lock);
#endif
}

/**
 * xmlRMutexUnlock:
 * @tok:  the reentrant mutex
 *
 * xmlRMutexUnlock() is used to unlock a libxml2 token_r.
 */
void
xmlRMutexUnlock(xmlRMutexPtr tok)
{
#ifdef HAVE_PTHREAD_H
    pthread_mutex_lock(&tok->lock);
    tok->held--;
    if (tok->held == 0) {
        if (tok->waiters)
            pthread_cond_signal(&tok->cv);
        tok->tid = 0;
    }
    pthread_mutex_unlock(&tok->lock);
#endif
}

/************************************************************************
 *									*
 *			Per thread global state handling		*
 *									*
 ************************************************************************/

#ifdef LIBXML_THREAD_ENABLED
/**
 * xmlFreeGlobalState:
 * @state:  a thread global state
 *
 * xmlFreeGlobalState() is called when a thread terminates with a non-NULL
 * global state. It is is used here to reclaim memory resources.
 */
static void
xmlFreeGlobalState(void *state)
{
    free(state);
}

/**
 * xmlNewGlobalState:
 *
 * xmlNewGlobalState() allocates a global state. This structure is used to
 * hold all data for use by a thread when supporting backwards compatibility
 * of libmxml2 to pre-thread-safe behaviour.
 *
 * Returns the newly allocated xmlGlobalStatePtr or NULL in case of error
 */
static xmlGlobalStatePtr
xmlNewGlobalState(void)
{
    xmlGlobalState *gs;
    
    gs = malloc(sizeof(xmlGlobalState));
    if (gs == NULL)
	return(NULL);

    memset(gs, 0, sizeof(gs));
    xmlInitializeGlobalState(gs);
    return (gs);
}
#endif /* LIBXML_THREAD_ENABLED */


/**
 * xmlGetGlobalState:
 *
 * xmlGetGlobalState() is called to retrieve the global state for a thread.
 * keyonce will only be set once during a library invocation and is used
 * to create globalkey, the key used to store each thread's TSD.
 *
 * Note: it should not be called for the "main" thread as this thread uses
 *       the existing global variables defined in the library.
 *
 * Returns the thread global state or NULL in case of error
 */
xmlGlobalStatePtr
xmlGetGlobalState(void)
{
#ifdef HAVE_PTHREAD_H
    xmlGlobalState *globalval;

    if (keyonce == 0) {
        (void) pthread_mutex_lock(&keylock);
        if (keyonce == 0) {
            keyonce++;
            (void) pthread_key_create(&globalkey, xmlFreeGlobalState);
        }
        (void) pthread_mutex_unlock(&keylock);
    }
    if ((globalval = (xmlGlobalState *)
         pthread_getspecific(globalkey)) == NULL) {
        xmlGlobalState *tsd = xmlNewGlobalState();

        pthread_setspecific(globalkey, tsd);
        return (tsd);
    }
    return (globalval);
#else
    return(NULL);
#endif
}


/************************************************************************
 *									*
 *			Library wide thread interfaces			*
 *									*
 ************************************************************************/

/**
 * xmlGetThreadId:
 *
 * xmlGetThreadId() find the current thread ID number
 *
 * Returns the current thread ID number
 */
int
xmlGetThreadId(void)
{
#ifdef HAVE_PTHREAD_H
    return((int) pthread_self());
#else
    return((int) 0);
#endif
}

/**
 * xmlIsMainThread:
 *
 * xmlIsMainThread() check wether the current thread is the main thread.
 *
 * Returns 1 if the current thread is the main thread, 0 otherwise
 */
int
xmlIsMainThread(void)
{
    if (!initialized)
        xmlInitThreads();
        
#ifdef DEBUG_THREADS
    xmlGenericError(xmlGenericErrorContext, "xmlIsMainThread()\n");
#endif
#ifdef HAVE_PTHREAD_H
    return(mainthread == pthread_self());
#else
    return(1);
#endif
}

/**
 * xmlLockLibrary:
 *
 * xmlLockLibrary() is used to take out a re-entrant lock on the libxml2
 * library.
 */
void
xmlLockLibrary(void)
{
#ifdef DEBUG_THREADS
    xmlGenericError(xmlGenericErrorContext, "xmlLockLibrary()\n");
#endif
    xmlRMutexLock(xmlLibraryLock);
}

/**
 * xmlUnlockLibrary:
 *
 * xmlUnlockLibrary() is used to release a re-entrant lock on the libxml2
 * library.
 */
void
xmlUnlockLibrary(void)
{
#ifdef DEBUG_THREADS
    xmlGenericError(xmlGenericErrorContext, "xmlUnlockLibrary()\n");
#endif
    xmlRMutexUnlock(xmlLibraryLock);
}

/**
 * xmlInitThreads:
 *
 * xmlInitThreads() is used to to initialize all the thread related
 * data of the libxml2 library.
 */
void
xmlInitThreads(void)
{
    if (initialized != 0)
        return;

#ifdef DEBUG_THREADS
    xmlGenericError(xmlGenericErrorContext, "xmlInitThreads()\n");
#endif

#ifdef HAVE_PTHREAD_H
    mainthread = pthread_self();
#endif

    initialized = 1;
}

/**
 * xmlCleanupThreads:
 *
 * xmlCleanupThreads() is used to to cleanup all the thread related
 * data of the libxml2 library once processing has ended.
 */
void
xmlCleanupThreads(void)
{
    if (initialized == 0)
        return;

#ifdef DEBUG_THREADS
    xmlGenericError(xmlGenericErrorContext, "xmlCleanupThreads()\n");
#endif

    initialized = 0;
}
