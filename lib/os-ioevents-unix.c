#ifndef _WIN32

#define _GNU_SOURCE
#include "internal.h"
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <dlfcn.h>

#define OS_IOEVENTS_STDIN_PIPE_INDEX 0

#define OS_IOEVENTS_MASK_FOR_BIT_COUNT(bc) ((((uint64_t)1) << bc) - 1)

#define OS_IOEVENTS_EVENT_DESCRIPTOR_TYPE_MASK OS_IOEVENTS_MASK_FOR_BIT_COUNT(3)
#define OS_IOEVENTS_EVENT_DESCRIPTOR_TYPE_SHIFT 0

#define OS_IOEVENTS_EVAL_MACRO1(x) x
#define OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(x, fn) (x >> OS_IOEVENTS_EVAL_MACRO1(OS_IOEVENTS_EVENT_DESCRIPTOR_ ## fn ## _SHIFT)) & OS_IOEVENTS_EVAL_MACRO1(OS_IOEVENTS_EVENT_DESCRIPTOR_ ## fn ## _MASK)
#define OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(x, fn, v) (x | ((v & OS_IOEVENTS_EVAL_MACRO1(OS_IOEVENTS_EVENT_DESCRIPTOR_ ## fn ## _MASK)) << OS_IOEVENTS_EVAL_MACRO1(OS_IOEVENTS_EVENT_DESCRIPTOR_ ## fn ## _SHIFT)))

#define OS_IOEVENTS_EVENT_DESCRIPTOR_SUBPROCESS_PIPE_MASK OS_IOEVENTS_MASK_FOR_BIT_COUNT(3)
#define OS_IOEVENTS_EVENT_DESCRIPTOR_SUBPROCESS_PIPE_SHIFT 3

#define OS_IOEVENTS_EVENT_DESCRIPTOR_SUBPROCESS_INDEX_MASK OS_IOEVENTS_MASK_FOR_BIT_COUNT(58)
#define OS_IOEVENTS_EVENT_DESCRIPTOR_SUBPROCESS_INDEX_SHIFT 6

#define OS_IOEVENTS_EVENT_DESCRIPTOR_FD_MASK OS_IOEVENTS_MASK_FOR_BIT_COUNT(61)
#define OS_IOEVENTS_EVENT_DESCRIPTOR_FD_SHIFT 3

typedef enum os_ioevents_fd_event_descriptor_type_e
{
    OS_IOEVENTS_FD_EVENT_WAKE_UP = 0,
    OS_IOEVENTS_FD_EVENT_SUBPROCESS_PIPE,
    OS_IOEVENTS_FD_EVENT_INOTIFY,
    OS_IOEVENTS_FD_EVENT_FILE,
    OS_IOEVENTS_FD_EVENT_DIRECTORY,

} os_ioevents_fd_event_descriptor_type_t;

#if defined(linux)

#define USE_EPOLL 1
#define USE_EVENT_FD 1
#define USE_INOTIFY 1

#define USE_SELECT_AS_CONDITION 1

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>

typedef union os_ioevents_inotify_event_buffer_s
{
    int wd; /* For alignment purposes */
    uint8_t bytes[sizeof(struct inotify_event) + NAME_MAX + 1];
}os_ioevents_inotify_event_buffer_t;

#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/event.h>

#define USE_KQUEUE 1
#endif

typedef struct os_ioevents_context_io_s
{
#ifdef USE_EPOLL
    int epollFD;
    int eventFD;
#endif

#ifdef USE_KQUEUE
    int kqueueFD;
#endif

    os_ioevents_mutex_t processListMutex;
    os_ioevents_list_t processList;

#ifdef USE_INOTIFY
    int inotifyFD;
    os_ioevents_inotify_event_buffer_t inotifyEventBuffer;
#endif

    os_ioevents_mutex_t fsmonitorMutex;

} os_ioevents_context_io_t;

#include "os-ioevents.c"

static os_ioevents_process_t *os_ioevents_process_getFromIndex(os_ioevents_context_t *context, size_t index);
static void os_ioevents_process_destructor (void *arg);
static void os_ioevents_process_pendingData(os_ioevents_process_t *process, int pipeIndex);
static void os_ioevents_process_pipeHungUpOrError(os_ioevents_process_t *process, int pipeIndex);

#if defined(USE_INOTIFY)
static void os_ioevents_inotify_pendingEvents(os_ioevents_context_t *context);
#endif

static int
os_ioevents_createContextIOPrimitives(os_ioevents_context_t *context)
{
#if defined(USE_EPOLL)
    /* epoll */
    context->io.epollFD = epoll_create1(EPOLL_CLOEXEC);
    if(context->io.epollFD < 0)
        return 0;

    /* event fd */
    context->io.eventFD = eventfd(0, EFD_CLOEXEC);
    if(context->io.eventFD < 0)
    {
        close(context->io.epollFD);
        return 0;
    }

    {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.u64 = 0;
        epoll_ctl(context->io.epollFD, EPOLL_CTL_ADD, context->io.eventFD, &event);
    }

    /* inotify*/
    {
        context->io.inotifyFD = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
        if(context->io.inotifyFD < 0)
        {
            close(context->io.epollFD);
            return 0;
        }

        {
            struct epoll_event event;
            event.events = EPOLLIN;
            event.data.u64 = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(0, TYPE, OS_IOEVENTS_FD_EVENT_INOTIFY);
            epoll_ctl(context->io.epollFD, EPOLL_CTL_ADD, context->io.inotifyFD, &event);
        }
    }
#elif defined(USE_KQUEUE)
    context->io.kqueueFD = kqueue();
    if(context->io.kqueueFD < 0)
        return 0;

    {
        struct kevent ev;
        EV_SET(&ev, 0, EVFILT_USER, EV_ADD, NOTE_FFCOPY, 0, NULL);
        kevent(context->io.kqueueFD, &ev, 1, NULL, 0, NULL);
    }
#endif

    context->signalSemaphoreWithIndex = (signalSemaphoreWithIndex_t)dlsym(RTLD_DEFAULT, "signalSemaphoreWithIndex");
    if(!context->signalSemaphoreWithIndex)
        context->signalSemaphoreWithIndex = (signalSemaphoreWithIndex_t)dlsym(RTLD_DEFAULT, "_signalSemaphoreWithIndex");

    /* Initialize the synchronization primitives. */
    os_ioevents_mutex_init(&context->io.processListMutex);
    os_ioevents_mutex_init(&context->io.fsmonitorMutex);

    return 1;
}

OS_IOEVENTS_CORE_EXPORT int
os_ioevents_isCapabilitySupported(os_ioevents_context_t* context, os_ioevents_capability_t capability)
{
    if (!context)
        return 0;

    switch (capability)
    {
    case OS_IOEVENTS_CAPABILITY_EXTERNAL_SEMAPHORE_SIGNALING:
        return context->signalSemaphoreWithIndex != NULL;
    case OS_IOEVENTS_CAPABILITY_NUMBERED_EXTRA_PIPES: return 1;
    case OS_IOEVENTS_CAPABILITY_NAMED_EXTRA_PIPES: return 0;

#if defined(USE_KQUEUE)
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_FILES:
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORIES:
        return 1;
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_COOKIE:
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORY_FILE_MODIFICATIONS:
        return 0;
#elif defined(USE_INOTIFY)
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_FILES:
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORIES:
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORY_FILE_MODIFICATIONS:
    case OS_IOEVENTS_CAPABILITY_FSMONITOR_COOKIE:
        return 1;
#endif
    default:
        return 0;
    }
}
static void
os_ioevents_context_destroyIOData(os_ioevents_context_t *context)
{
#if USE_EVENT_FD
    close(context->io.inotifyFD);

    close(context->io.epollFD);
    close(context->io.eventFD);
#endif

    os_ioevents_mutex_destroy(&context->io.processListMutex);
    os_ioevents_list_destroyData(&context->io.processList, os_ioevents_process_destructor);

    os_ioevents_mutex_destroy(&context->io.fsmonitorMutex);
}

static
void os_ioevents_wakeUpSelectForShutdown(os_ioevents_context_t *context)
{
#if USE_EVENT_FD
    uint64_t count = 1;
    ssize_t res = write(context->io.eventFD, &count, sizeof(count));
    if(res < 0)
        perror("Failed to wake up process threads");
#elif USE_KQUEUE
    {
        struct kevent ev;
        EV_SET(&ev, 0, EVFILT_USER, EV_ENABLE, NOTE_FFCOPY | NOTE_TRIGGER, 0, NULL);
        kevent(context->io.kqueueFD, &ev, 1, NULL, 0, NULL);
    }

#else
#error Pipe not yet implemented.
#endif
}

#if USE_EPOLL
static void
os_ioevents_processEPollEvent(os_ioevents_context_t *context, struct epoll_event *event)
{
    uint64_t descriptor = event->data.u64;
    uint64_t eventType = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(descriptor, TYPE);
    switch(eventType)
    {
    case OS_IOEVENTS_FD_EVENT_WAKE_UP:
        {
            if(event->events & EPOLLIN)
            {
                uint64_t count;
                ssize_t readedCount = read(context->io.eventFD, &count, sizeof(count));
                if(readedCount < 0)
                    perror("Failed to read event FD.\n");
            }
        }
        break;
    case OS_IOEVENTS_FD_EVENT_SUBPROCESS_PIPE:
        {
            os_ioevents_mutex_lock(&context->io.processListMutex);

            int pipe = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(descriptor, SUBPROCESS_PIPE);
            int subprocess = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(descriptor, SUBPROCESS_INDEX);

            /* Pending data*/
            os_ioevents_process_t *process = os_ioevents_process_getFromIndex(context, subprocess);
            if(process)
            {
                if(event->events & EPOLLIN)
                    os_ioevents_process_pendingData(process, pipe);

                /* Pipe closed */
                if(event->events & EPOLLHUP || event->events & EPOLLERR)
                    os_ioevents_process_pipeHungUpOrError(process, pipe);
            }

            os_ioevents_mutex_unlock(&context->io.processListMutex);
        }
        break;
#if USE_INOTIFY
    case OS_IOEVENTS_FD_EVENT_INOTIFY:
        {
            if(event->events & EPOLLIN)
                os_ioevents_inotify_pendingEvents(context);
        }
        break;
#endif
    default:
        break;
    }
}

static void
os_ioevents_processEPollEvents(os_ioevents_context_t *context, struct epoll_event *events, int eventCount)
{
    for(int i = 0; i < eventCount; ++i)
    {
        os_ioevents_processEPollEvent(context, &events[i]);
    }
}
#endif /* USE_EPOLL */

#ifdef USE_KQUEUE
static void
os_ioevents_processKQueueEvent(os_ioevents_context_t *context, struct kevent *event)
{
    uint64_t descriptor = (uintptr_t)event->udata;
    uint64_t eventType = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(descriptor, TYPE);
    switch(eventType)
    {
    case OS_IOEVENTS_FD_EVENT_WAKE_UP:
        {
            {
                struct kevent ev;
                EV_SET(&ev, 0, EVFILT_USER, EV_DISABLE, NOTE_FFCOPY, 0, NULL);
                kevent(context->io.kqueueFD, &ev, 1, NULL, 0, NULL);
            }
        }
        break;
    case OS_IOEVENTS_FD_EVENT_SUBPROCESS_PIPE:
        {
            os_ioevents_mutex_lock(&context->io.processListMutex);

            int pipe = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(descriptor, SUBPROCESS_PIPE);
            int subprocess = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(descriptor, SUBPROCESS_INDEX);

            /* Pending data*/
            os_ioevents_process_t *process = os_ioevents_process_getFromIndex(context, subprocess);
            if(process)
            {
                if(event->filter == EVFILT_READ)
                    os_ioevents_process_pendingData(process, pipe);

                /* Pipe closed */
                if(event->flags & EV_EOF || event->flags & EV_ERROR)
                    os_ioevents_process_pipeHungUpOrError(process, pipe);
            }

            os_ioevents_mutex_unlock(&context->io.processListMutex);
        }
        break;
    case OS_IOEVENTS_FD_EVENT_INOTIFY:
        // Kqueue does not support inotify.
        break;

    case OS_IOEVENTS_FD_EVENT_DIRECTORY:
    case OS_IOEVENTS_FD_EVENT_FILE:
        {
            int fd = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_GET(descriptor, FD);
            uint32_t fflags = event->fflags;
            uint32_t convertedEventMask = 0;
            if(fflags & NOTE_DELETE)
                convertedEventMask |= OS_IOEVENTS_FSMONITOR_EVENT_DELETE_SELF;
            if(fflags & NOTE_WRITE)
                convertedEventMask |= OS_IOEVENTS_FSMONITOR_EVENT_MODIFY;
            if(fflags & NOTE_ATTRIB)
                convertedEventMask |= OS_IOEVENTS_FSMONITOR_EVENT_ATTRIB;
            if(fflags & NOTE_RENAME)
            {
                fprintf(stderr, "TODO: Convert rename event\n");
            }

            if(convertedEventMask != 0)
            {
                os_ioevents_event_t phevent = {
                    .fsmonitor = {
                        .type = OS_IOEVENTS_EVENT_TYPE_FSMONITOR,
                        .handle = (os_ioevents_fsmonitor_handle_t *)(intptr_t)fd,
                        .mask = convertedEventMask,
                        .cookie = 0,
                        .nameLength = 0,
                        .name = 0
                    }
                };
                os_ioevents_pushEvent(context, &phevent);
            }
        }
        break;
    default:
        break;
    }
}

static void
os_ioevents_processKQueueEvents(os_ioevents_context_t *context, struct kevent *events, int eventCount)
{
    for(int i = 0; i < eventCount; ++i)
    {
        os_ioevents_processKQueueEvent(context, &events[i]);
    }
}

#endif

static int
os_ioevents_processThreadEntry(void *arg)
{
    os_ioevents_context_t *context = (os_ioevents_context_t *)arg;
    for(;;)
    {
#if defined(USE_EPOLL)
        struct epoll_event events[64];
        int eventCount = epoll_wait(context->io.epollFD, events, 64, -1);
        if(eventCount < 0)
        {
            perror("epoll failed");
            return 0;
        }

        os_ioevents_processEPollEvents(context, events, eventCount);
#elif defined(USE_KQUEUE)
        struct kevent events[64];
        int eventCount = kevent(context->io.kqueueFD, NULL, 0, events, 64, NULL);
        if(eventCount < 0)
        {
            perror("kevent failed");
            return 0;
        }

        struct kevent ev;
        EV_SET(&ev, 0, EVFILT_VNODE | EV_CLEAR, EV_ADD, NOTE_FFCOPY, 0, NULL);
        kevent(context->io.kqueueFD, &ev, 1, NULL, 0, NULL);

        os_ioevents_processKQueueEvents(context, events, eventCount);
#else
#error Select not yet implemented
#endif
        os_ioevents_mutex_lock(&context->controlMutex);

        /* Are we shutting down? */
        if(context->shuttingDown)
        {
            os_ioevents_mutex_unlock(&context->controlMutex);
            break;
        }

        os_ioevents_mutex_unlock(&context->controlMutex);
    }

    return 0;
}

/* Process spawning. */
struct os_ioevents_process_s
{
    os_ioevents_context_t *context;
    int used;
    size_t index;
    pid_t childPid;
    os_ioevents_process_spawn_flags_t flags;

    int remainingPipes;
    int exitCode;

    union
    {
        struct
        {
            int stdinPipe;
            int stdoutPipe;
            int stderrPipe;
            int extraStdinPipe;
            int extraStdoutPipe;
            int extraStderrPipe;
        };
        int pipes[6];
    };
};

static os_ioevents_process_t *
os_ioevents_process_allocate(os_ioevents_context_t *context)
{
    os_ioevents_mutex_lock(&context->io.processListMutex);
    /* Find a free process. */
    os_ioevents_process_t *resultProcess = NULL;
    for(size_t i = 0; i < context->io.processList.size; ++i)
    {
        os_ioevents_process_t *process = context->io.processList.data[i];
        if(!process->used)
        {
            resultProcess = process;
            memset(resultProcess, 0, sizeof(os_ioevents_process_t));
            resultProcess->index = i;
        }
    }

    /* Allocate a new result process. */
    if(!resultProcess)
    {
        resultProcess = malloc(sizeof(os_ioevents_process_t));
        memset(resultProcess, 0, sizeof(os_ioevents_process_t));
        resultProcess->index = context->io.processList.size;
        os_ioevents_list_pushBack(&context->io.processList, resultProcess);
    }

    resultProcess->context = context;
    resultProcess->used = 1;
    os_ioevents_mutex_unlock(&context->io.processListMutex);

    return resultProcess;
}

static os_ioevents_process_t*
os_ioevents_process_getFromIndex(os_ioevents_context_t *context, size_t index)
{
    if(index >= context->io.processList.size)
        return NULL;

    os_ioevents_process_t *process = context->io.processList.data[index];
    if(!process->used)
        return NULL;

    return process;
}

static void
os_ioevents_process_closePipeFD(os_ioevents_process_t *process, int fd)
{
#ifdef USE_EPOLL
    epoll_ctl(process->context->io.epollFD, EPOLL_CTL_DEL, fd, NULL);
#endif
    close(fd);
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_process_free(os_ioevents_process_t *process)
{
    if(!process)
        return;

    /* TODO: Perform process clean up*/
    os_ioevents_context_t *context = process->context;
    os_ioevents_mutex_lock(&context->io.processListMutex);
    if(process->used)
    {
        if(process->stdinPipe)
            os_ioevents_process_closePipeFD(process, process->stdinPipe);
        if(process->stdoutPipe)
            os_ioevents_process_closePipeFD(process, process->stdoutPipe);
        if(process->stderrPipe)
            os_ioevents_process_closePipeFD(process, process->stderrPipe);

        if(process->flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
        {
            if(process->extraStdinPipe)
                os_ioevents_process_closePipeFD(process, process->extraStdinPipe);
            if(process->extraStdoutPipe)
                os_ioevents_process_closePipeFD(process, process->extraStdoutPipe);
            if(process->extraStderrPipe)
                os_ioevents_process_closePipeFD(process, process->extraStderrPipe);
        }

        if(process->childPid)
        {
            int status;
            int res = waitpid(process->childPid, &status, WNOHANG);
            (void)status;
            (void)res;
        }
    }
    memset(process, 0, sizeof(os_ioevents_process_t));
    os_ioevents_mutex_unlock(&context->io.processListMutex);
}

static void os_ioevents_process_closeAllOpenFileDescriptors(int inheritedCount)
{
    DIR *dir = opendir("/proc/self/fd/");
    if(!dir)
        dir = opendir("/dev/fd/");

    /* TODO: Support the brute force approach as a fallback. */
    if(!dir)
        return;

    int dirFD = dirfd(dir);
    for(struct dirent *entry = readdir(dir); entry; entry = readdir(dir))
    {
        int entryFDNumber = atoi(entry->d_name);
        if(/* stdin stdout stderr */entryFDNumber >= inheritedCount && entryFDNumber != dirFD)
            close(entryFDNumber);
    }

    closedir(dir);
}

static void
os_ioevents_register_pipeForPolling(os_ioevents_context_t *context, int isReadPipe, int processIndex, int fd, int pipeIndex)
{
    if(isReadPipe)
    {
        uint64_t descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(0, TYPE, OS_IOEVENTS_FD_EVENT_SUBPROCESS_PIPE);
        descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, SUBPROCESS_PIPE, pipeIndex);
        descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, SUBPROCESS_INDEX, processIndex);

#if defined(USE_EPOLL)
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.u64 = descriptor;
        epoll_ctl(context->io.epollFD, EPOLL_CTL_ADD, fd, &event);
#elif defined(USE_KQUEUE)
        struct kevent event;
        EV_SET(&event, fd, EVFILT_READ, EV_ADD|EV_ENABLE, 0, 0, (void*)(uintptr_t)descriptor);
        kevent(context->io.kqueueFD, &event, 1, NULL, 0, NULL);
#else
#error Not yet implemented
#endif
    }
    else
    {
        uint64_t descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(0, TYPE, OS_IOEVENTS_FD_EVENT_SUBPROCESS_PIPE);
        descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, SUBPROCESS_PIPE, pipeIndex);
        descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, SUBPROCESS_INDEX, processIndex);

#if defined(USE_EPOLL)
        struct epoll_event event;
        event.events = 0;
        event.data.u64 = descriptor;
        epoll_ctl(context->io.epollFD, EPOLL_CTL_ADD, fd, &event);
#elif defined(USE_KQUEUE)
        struct kevent event;
        EV_SET(&event, fd, EVFILT_WRITE, EV_ADD|EV_DISABLE, 0, 0, (void*)(uintptr_t)descriptor);
        kevent(context->io.kqueueFD, &event, 1, NULL, 0, NULL);
#else
        #error Not yet implemented
#endif
    }
}
static os_ioevents_process_t *
os_ioevents_process_forkForSpawn(os_ioevents_context_t *context, os_ioevents_process_spawn_flags_t flags, int *error)
{
    int stdinPipe[2];
    int stdoutPipe[2];
    int stderrPipe[2];
    int extraStdinPipe[2];
    int extraStdoutPipe[2];
    int extraStderrPipe[2];

    /* Create the pipes */
    int result = pipe(stdinPipe);
    if(result < 0)
    {
        *error = errno;
        return NULL;
    }

    result = pipe(stdoutPipe);
    if(result < 0)
    {
        *error = errno;
        close(stdinPipe[0]); close(stdinPipe[1]);
        return NULL;
    }

    result = pipe(stderrPipe);
    if(result < 0)
    {
        *error = errno;
        close(stdinPipe[0]); close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        return NULL;
    }

    if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
    {
        int result = pipe(extraStdinPipe);
        if(result < 0)
        {
            *error = errno;
            close(stdinPipe[0]); close(stdinPipe[1]);
            close(stdoutPipe[0]); close(stdoutPipe[1]);
            close(stderrPipe[0]); close(stderrPipe[1]);
            return NULL;
        }

        result = pipe(extraStdoutPipe);
        if(result < 0)
        {
            *error = errno;
            close(stdinPipe[0]); close(stdinPipe[1]);
            close(stdoutPipe[0]); close(stdoutPipe[1]);
            close(stderrPipe[0]); close(stderrPipe[1]);
            close(extraStdinPipe[0]); close(extraStdinPipe[1]);
            return NULL;
        }

        result = pipe(extraStderrPipe);
        if(result < 0)
        {
            *error = errno;
            close(stdinPipe[0]); close(stdinPipe[1]);
            close(stdoutPipe[0]); close(stdoutPipe[1]);
            close(stderrPipe[0]); close(stderrPipe[1]);
            close(extraStdinPipe[0]); close(extraStdinPipe[1]);
            close(extraStdoutPipe[0]); close(extraStdoutPipe[1]);
            return NULL;
        }
    }

    /* Fork the process */
    pid_t forkResult = fork();
    if(forkResult < 0)
    {
        /* This should not happen. */
        perror("Failed to fork\n");
        *error = errno;
        close(stdinPipe[0]); close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        close(stderrPipe[0]); close(stderrPipe[1]);
        if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
        {
            close(extraStdinPipe[0]); close(extraStdinPipe[1]);
            close(extraStdoutPipe[0]); close(extraStdoutPipe[1]);
            close(extraStderrPipe[0]); close(extraStderrPipe[1]);
        }
        return NULL;
    }

    /* Are we the child? */
    if(forkResult == 0)
    {
        /* Redirect the standard file descriptors to the pipes. */
        result = dup2(stdinPipe[0], STDIN_FILENO); (void)result;
        result = dup2(stdoutPipe[1], STDOUT_FILENO); (void)result;
        result = dup2(stderrPipe[1], STDERR_FILENO); (void)result;
        if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
        {
            result = dup2(extraStdinPipe[0], 3); (void)result;
            result = dup2(extraStdoutPipe[1], 4); (void)result;
            result = dup2(extraStderrPipe[1], 5); (void)result;
        }

        /* Close the copies from the pipes. */
        close(stdinPipe[0]); close(stdinPipe[1]);
        close(stdoutPipe[0]); close(stdoutPipe[1]);
        close(stderrPipe[0]); close(stderrPipe[1]);
        if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
        {
            close(extraStdinPipe[0]); close(extraStdinPipe[1]);
            close(extraStdoutPipe[0]); close(extraStdoutPipe[1]);
            close(extraStderrPipe[0]); close(extraStderrPipe[1]);
        }

        /* Close all the open file descriptors. */
        int inheritedPipes = 3;
        if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
            inheritedPipes += 3;
        os_ioevents_process_closeAllOpenFileDescriptors(inheritedPipes);
        return NULL;
    }

    /* Create the process */
    os_ioevents_process_t *process = os_ioevents_process_allocate(context);
    process->flags = flags;

    /* We are the parent. Close the pipe endpoint that are unintesting to us. */
    /* read */ close(stdinPipe[0]); process->stdinPipe = /* write */stdinPipe[1];
    /* read */ process->stdoutPipe = stdoutPipe[0]; /* write */ close(stdoutPipe[1]);
    /* read */ process->stderrPipe = stderrPipe[0]; /* write */ close(stderrPipe[1]);

    if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
    {
        /* read */ close(extraStdinPipe[0]); process->extraStdinPipe = /* write */extraStdinPipe[1];
        /* read */ process->extraStdoutPipe = extraStdoutPipe[0]; /* write */ close(extraStdoutPipe[1]);
        /* read */ process->extraStderrPipe = extraStderrPipe[0]; /* write */ close(extraStderrPipe[1]);
    }

    /* Set non-blocking mode for stdout and stderr. */
    fcntl(process->stdoutPipe, F_SETFL, fcntl(process->stdoutPipe, F_GETFL, 0) | O_NONBLOCK);
    fcntl(process->stderrPipe, F_SETFL, fcntl(process->stderrPipe, F_GETFL, 0) | O_NONBLOCK);
    process->remainingPipes = 3;

    if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
    {
        fcntl(process->extraStdoutPipe, F_SETFL, fcntl(process->extraStdoutPipe, F_GETFL, 0) | O_NONBLOCK);
        fcntl(process->extraStderrPipe, F_SETFL, fcntl(process->extraStderrPipe, F_GETFL, 0) | O_NONBLOCK);
        process->remainingPipes = 6;
    }

    os_ioevents_register_pipeForPolling(context, 0, process->index, process->stdinPipe, OS_IOEVENTS_PIPE_INDEX_STDIN);
    os_ioevents_register_pipeForPolling(context, 1, process->index, process->stdoutPipe, OS_IOEVENTS_PIPE_INDEX_STDOUT);
    os_ioevents_register_pipeForPolling(context, 1, process->index, process->stderrPipe, OS_IOEVENTS_PIPE_INDEX_STDERR);
    if(flags & OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES)
    {
        os_ioevents_register_pipeForPolling(context, 0, process->index, process->extraStdinPipe, OS_IOEVENTS_PIPE_INDEX_EXTRA_STDIN);
        os_ioevents_register_pipeForPolling(context, 1, process->index, process->extraStdoutPipe, OS_IOEVENTS_PIPE_INDEX_EXTRA_STDOUT);
        os_ioevents_register_pipeForPolling(context, 1, process->index, process->extraStderrPipe, OS_IOEVENTS_PIPE_INDEX_EXTRA_STDERR);
    }

    return process;
}

static void
os_ioevents_process_setPipeReadPolling(int enabled, os_ioevents_process_t *process, int pipeIndex)
{
    uint64_t descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(0, TYPE, OS_IOEVENTS_FD_EVENT_SUBPROCESS_PIPE);
    descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, SUBPROCESS_PIPE, pipeIndex);
    descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, SUBPROCESS_INDEX, process->index);

#if defined(USE_EPOLL)
    struct epoll_event event;
    event.events = enabled ? EPOLLIN : 0;
    event.data.u64 = descriptor;
    epoll_ctl(process->context->io.epollFD, EPOLL_CTL_MOD, process->pipes[pipeIndex], &event);

#elif defined(USE_KQUEUE)
    struct kevent event;
    EV_SET(&event, process->pipes[pipeIndex], EVFILT_READ, enabled ? EV_ENABLE : EV_DISABLE, 0, 0, (void*)(uintptr_t)descriptor);
    kevent(process->context->io.kqueueFD, &event, 1, NULL, 0, NULL);
#else
#error Not yet implemented
#endif
}

static void
os_ioevents_process_pendingData(os_ioevents_process_t *process, int pipeIndex)
{
    os_ioevents_process_setPipeReadPolling(0, process, pipeIndex);

    os_ioevents_event_t event = {
        .processPipe = {
            .type = OS_IOEVENTS_EVENT_TYPE_PROCESS_PIPE_READY,
            .process = process,
            .pipeIndex = pipeIndex
        }
    };

    os_ioevents_pushEvent(process->context, &event);
}

static void
os_ioevents_process_pipeHungUpOrError(os_ioevents_process_t *process, int pipeIndex)
{
#if defined (USE_EPOLL)
    epoll_ctl(process->context->io.epollFD, EPOLL_CTL_DEL, process->pipes[pipeIndex], NULL);
#elif defined (USE_KQUEUE)
    struct kevent event;
    if(pipeIndex == OS_IOEVENTS_STDIN_PIPE_INDEX)
    {
        EV_SET(&event, process->pipes[pipeIndex], EV_DELETE, EVFILT_WRITE, 0, 0, 0);
    }
    else
    {
        EV_SET(&event, process->pipes[pipeIndex], EV_DELETE, EVFILT_READ, 0, 0, 0);
    }
    kevent(process->context->io.kqueueFD, &event, 1, NULL, 0, NULL);

#else

#endif
    --process->remainingPipes;
    if(process->remainingPipes != 0)
        return;

    /* Time to bury the child. */
    int status;
    waitpid(process->childPid, &status, 0);
    process->exitCode = WEXITSTATUS(status);
    process->childPid = 0;

    /* There is no need to keep the stdin pipe. */
    close(process->stdinPipe);
    process->stdinPipe = 0;

    /* Push a process finished event. */
    {
        os_ioevents_event_t event = {
            .processFinished = {
                .type = OS_IOEVENTS_EVENT_TYPE_PROCESS_FINISHED,
                .process = process,
                .exitCode = process->exitCode,
            }
        };
        os_ioevents_pushEvent(process->context, &event);
    }
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t *
os_ioevents_process_spawn(os_ioevents_context_t *context, const char *path, const char **argv, os_ioevents_process_spawn_flags_t flags)
{
    if(!context)
        return NULL;

    int error = 0;
    os_ioevents_process_t *result = os_ioevents_process_forkForSpawn(context, flags, &error);
    if(result || error)
        return result;

    int res = execv(path, (char *const*)argv);
    (void)res;

    /* Should never reach here. */
    perror("Failed to perform exec");
    exit(1);
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t *
os_ioevents_process_spawnInPath(os_ioevents_context_t *context, const char *file, const char **argv, os_ioevents_process_spawn_flags_t flags)
{
    if(!context)
        return NULL;

    int error = 0;
    os_ioevents_process_t *result = os_ioevents_process_forkForSpawn(context, flags, &error);
    if(result || error)
        return result;

    int res = execvp(file, (char *const*)argv);
    (void)res;

    /* Should never reach here. */
    perror("Failed to perform exec");
    exit(1);
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t *
os_ioevents_process_spawnShell(os_ioevents_context_t *context, const char *command, os_ioevents_process_spawn_flags_t flags)
{
    if(!context)
        return NULL;

    int error = 0;
    os_ioevents_process_t *result = os_ioevents_process_forkForSpawn(context, flags, &error);
    if(result || error)
        return result;

    execl("/bin/sh", "sh", "-c", command, NULL);

    /* Should never reach here. */
    perror("Failed to perform exec.");
    exit(1);
}


OS_IOEVENTS_CORE_EXPORT void
os_ioevents_process_terminate(os_ioevents_process_t *process)
{
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_process_kill(os_ioevents_process_t *process)
{
}

OS_IOEVENTS_CORE_EXPORT intptr_t
os_ioevents_process_pipe_read(os_ioevents_process_t *process, os_ioevents_pipe_index_t pipe, void *buffer, size_t offset,  size_t count)
{
    if(!process)
        return OS_IOEVENTS_PIPE_ERROR;

    os_ioevents_mutex_lock(&process->context->io.processListMutex);

    /* Get the pipe file descriptor. */
    int fd = process->pipes[pipe];
    if(fd == 0)
    {
        os_ioevents_mutex_unlock(&process->context->io.processListMutex);
        return OS_IOEVENTS_PIPE_ERROR_CLOSED;
    }

    /* Read from the pipe. */
    ssize_t result;
    {
        result = read(fd, ((char*)buffer) + offset, count);
    } while(result < 0 && errno == EINTR);

    if(errno == EWOULDBLOCK)
        os_ioevents_process_setPipeReadPolling(1, process, pipe);
    os_ioevents_mutex_unlock(&process->context->io.processListMutex);

    /* Convert the error code. */
    if(result < 0)
    {
        switch(errno)
        {
        case EWOULDBLOCK:
            return OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK;
        default:
            return OS_IOEVENTS_PIPE_ERROR;
        }
    }

    return result;
}

OS_IOEVENTS_CORE_EXPORT intptr_t
os_ioevents_process_pipe_write(os_ioevents_process_t *process, os_ioevents_pipe_index_t pipe, const void *buffer, size_t offset, size_t count)
{
    if(!process)
        return OS_IOEVENTS_PIPE_ERROR;

    os_ioevents_mutex_lock(&process->context->io.processListMutex);

    /* Get the pipe file descriptor. */
    int fd = process->pipes[pipe];
    if(fd == 0)
    {
        os_ioevents_mutex_unlock(&process->context->io.processListMutex);
        return OS_IOEVENTS_PIPE_ERROR_CLOSED;
    }

    /* Read from the pipe. */
    ssize_t result;
    {
        result = write(fd, ((char*)buffer) + offset, count);
    } while(result < 0 && errno == EINTR);

    os_ioevents_mutex_unlock(&process->context->io.processListMutex);

    /* Convert the error code. */
    if(result < 0)
    {
        switch(errno)
        {
        case EWOULDBLOCK:
            return OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK;
        default:
            return OS_IOEVENTS_PIPE_ERROR;
        }
    }

    return result;
}

OS_IOEVENTS_CORE_EXPORT const char*
os_ioevents_process_pipe_getNamedEndpoint(os_ioevents_process_t* process, os_ioevents_pipe_index_t pipe)
{
	(void)process;
	(void)pipe;
	return NULL;
}

static void
os_ioevents_process_destructor (void *arg)
{
    os_ioevents_process_t *process = (os_ioevents_process_t*)arg;
    if(process->used)
    {
        if(process->stdinPipe)
            close(process->stdinPipe);
        if(process->stdoutPipe)
            close(process->stdoutPipe);
        if(process->stderrPipe)
            close(process->stderrPipe);
        if(process->extraStdinPipe)
            close(process->stdinPipe);
        if(process->extraStdoutPipe)
            close(process->stdoutPipe);
        if(process->extraStderrPipe)
            close(process->stderrPipe);

        if(process->childPid)
        {
            int status;
            int res = waitpid(process->childPid, &status, WNOHANG);
            (void)status;
            (void)res;
        }
    }
    free(process);
}


#if USE_INOTIFY
static int
os_ioevents_inotify_pendingEvent(os_ioevents_context_t *context)
{
    ssize_t readCount;
    do
    {
        readCount = read(context->io.inotifyFD, context->io.inotifyEventBuffer.bytes, sizeof(context->io.inotifyEventBuffer));
    } while (readCount < 0 && errno == EINTR);

    if(readCount < 0)
        return 0;

    uint8_t *bytes = context->io.inotifyEventBuffer.bytes;
    while(readCount > 0)
    {
        struct inotify_event *event = (struct inotify_event *)bytes;
        size_t eventSize = sizeof(struct inotify_event) + event->len;

        /* Map the event mask */
        uint32_t mappedMask = 0;
        uint32_t mask = event->mask;

#define MAP_INOTIFY_EVENT(name) if(mask & IN_##name) mappedMask |= OS_IOEVENTS_FSMONITOR_EVENT_ ##name;
        MAP_INOTIFY_EVENT(ACCESS);
        MAP_INOTIFY_EVENT(ATTRIB);
        MAP_INOTIFY_EVENT(CLOSE_WRITE);
        MAP_INOTIFY_EVENT(CLOSE_NOWRITE);
        MAP_INOTIFY_EVENT(CREATE);
        MAP_INOTIFY_EVENT(DELETE);
        MAP_INOTIFY_EVENT(DELETE_SELF);
        MAP_INOTIFY_EVENT(MODIFY);
        MAP_INOTIFY_EVENT(MOVE_SELF);
        MAP_INOTIFY_EVENT(MOVED_FROM);
        MAP_INOTIFY_EVENT(MOVED_TO);
        MAP_INOTIFY_EVENT(OPEN);
#undef MAP_INOTIFY_EVENT

        os_ioevents_event_t phevent = {
            .fsmonitor = {
                .type = OS_IOEVENTS_EVENT_TYPE_FSMONITOR,
                .handle = (os_ioevents_fsmonitor_handle_t *)(size_t)event->wd,
                .mask = mappedMask,
                .cookie = event->cookie,
                .nameLength = event->len,
                .name = event->len ? os_ioevents_strdup(event->name) : 0
            }
        };
        os_ioevents_pushEvent(context, &phevent);

        readCount -= eventSize;
        bytes += eventSize;
    }

    return 1;
}

static void
os_ioevents_inotify_pendingEvents(os_ioevents_context_t *context)
{
    while(os_ioevents_inotify_pendingEvent(context))
        ;
}
#endif

OS_IOEVENTS_CORE_EXPORT uint32_t
os_ioevents_fsmonitor_getSupportedEventMask(os_ioevents_fsmonitor_handle_t* handle)
{
#if defined(USE_KQUEUE)
    return
        /*NOTE_DELETE */ OS_IOEVENTS_FSMONITOR_EVENT_DELETE |
        OS_IOEVENTS_FSMONITOR_EVENT_DELETE_SELF |
        /* NOTE_WRITE */ OS_IOEVENTS_FSMONITOR_EVENT_MODIFY |
        /* NOTE_ATTRIB */ OS_IOEVENTS_FSMONITOR_EVENT_ATTRIB  |
        /*NOTE_RENAME */ OS_IOEVENTS_FSMONITOR_EVENT_MOVED_FROM |
        OS_IOEVENTS_FSMONITOR_EVENT_MOVED_TO |

        /* Deducted events*/
        OS_IOEVENTS_FSMONITOR_EVENT_CREATE;
#elif defined(USE_INOTIFY)
    return
        OS_IOEVENTS_FSMONITOR_EVENT_ACCESS |
        OS_IOEVENTS_FSMONITOR_EVENT_ATTRIB |
        OS_IOEVENTS_FSMONITOR_EVENT_CLOSE_WRITE |
        OS_IOEVENTS_FSMONITOR_EVENT_CLOSE_NOWRITE |
        OS_IOEVENTS_FSMONITOR_EVENT_CREATE |
        OS_IOEVENTS_FSMONITOR_EVENT_DELETE |
        OS_IOEVENTS_FSMONITOR_EVENT_DELETE_SELF |
        OS_IOEVENTS_FSMONITOR_EVENT_MODIFY |
        OS_IOEVENTS_FSMONITOR_EVENT_MOVE_SELF |
        OS_IOEVENTS_FSMONITOR_EVENT_MOVED_FROM |
        OS_IOEVENTS_FSMONITOR_EVENT_MOVED_TO |
        OS_IOEVENTS_FSMONITOR_EVENT_OPEN;
#else
    return 0;
#endif
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_fsmonitor_handle_t *
os_ioevents_fsmonitor_watchFile(os_ioevents_context_t *context, const char *path)
{
    if(!context)
        return NULL;

os_ioevents_fsmonitor_handle_t *result = NULL;

#if USE_INOTIFY
    os_ioevents_mutex_lock(&context->io.fsmonitorMutex);
    int wd = inotify_add_watch(context->io.inotifyFD, path,
        IN_ATTRIB | IN_CLOSE | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVE | IN_OPEN);
    if(wd >= 0)
        result = (os_ioevents_fsmonitor_handle_t*)(size_t)wd;

    os_ioevents_mutex_unlock(&context->io.fsmonitorMutex);

#elif USE_KQUEUE
    int eventFD = open(path, O_EVTONLY);
    if(eventFD < 0)
        return NULL;

    result = (os_ioevents_fsmonitor_handle_t*)(size_t)eventFD;

    uint64_t descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(0, TYPE, OS_IOEVENTS_FD_EVENT_FILE);
    descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, FD, eventFD);

    os_ioevents_mutex_lock(&context->io.fsmonitorMutex);

    struct kevent event;
    unsigned int fileEvents = NOTE_DELETE | NOTE_WRITE | NOTE_ATTRIB | NOTE_RENAME;
    EV_SET(&event, eventFD, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, fileEvents, 0, (void*)(uintptr_t)descriptor);
    kevent(context->io.kqueueFD, &event, 1, NULL, 0, NULL);

    os_ioevents_mutex_unlock(&context->io.fsmonitorMutex);
#endif
    return result;
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_fsmonitor_handle_t *
os_ioevents_fsmonitor_watchDirectory(os_ioevents_context_t *context, const char *path)
{
    if(!context)
        return NULL;

    os_ioevents_fsmonitor_handle_t *result = NULL;
#if USE_INOTIFY
    os_ioevents_mutex_lock(&context->io.fsmonitorMutex);
    int wd = inotify_add_watch(context->io.inotifyFD, path,
        IN_ATTRIB | IN_CLOSE | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVE | IN_OPEN | IN_EXCL_UNLINK);
    if(wd >= 0)
        result = (os_ioevents_fsmonitor_handle_t*)(size_t)wd;

    os_ioevents_mutex_unlock(&context->io.fsmonitorMutex);
#elif USE_KQUEUE
    int eventFD = open(path, O_RDONLY);
    if(eventFD < 0)
        return NULL;

    result = (os_ioevents_fsmonitor_handle_t*)(size_t)eventFD;

    uint64_t descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(0, TYPE, OS_IOEVENTS_FD_EVENT_DIRECTORY);
    descriptor = OS_IOEVENTS_EVENT_DESCRIPTOR_FIELD_SET(descriptor, FD, eventFD);

    os_ioevents_mutex_lock(&context->io.fsmonitorMutex);

    struct kevent event;
    unsigned int fileEvents = NOTE_DELETE | NOTE_WRITE | NOTE_ATTRIB | NOTE_LINK | NOTE_RENAME;
    EV_SET(&event, eventFD, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, fileEvents, 0, (void*)(uintptr_t)descriptor);
    kevent(context->io.kqueueFD, &event, 1, NULL, 0, NULL);

    os_ioevents_mutex_unlock(&context->io.fsmonitorMutex);
#endif

    return result;
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_fsmonitor_destroy(os_ioevents_context_t *context, os_ioevents_fsmonitor_handle_t *handle)
{
    if(!context)
        return;

#if USE_INOTIFY
    os_ioevents_mutex_lock(&context->io.fsmonitorMutex);
    int wd = (int)(size_t)handle;
    inotify_rm_watch(context->io.inotifyFD, wd);
    os_ioevents_mutex_unlock(&context->io.fsmonitorMutex);
#elif USE_KQUEUE
    if(!handle)
        return;

    int eventFD = (int)(uintptr_t)handle;
    close(eventFD);

#endif
}

#endif //_WIN32
