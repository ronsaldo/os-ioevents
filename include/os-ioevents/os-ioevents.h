#ifndef _OS_IOEVENTS_OS_IOEVENTS_H_
#define _OS_IOEVENTS_OS_IOEVENTS_H_

#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#define OS_IOEVENTS_DLL_EXPORT __declspec(dllexport)
#define OS_IOEVENTS_DLL_IMPORT __declspec(dllimport)
#else
#define OS_IOEVENTS_DLL_EXPORT
#define OS_IOEVENTS_DLL_IMPORT
#endif

#ifdef __cplusplus
#define OS_IOEVENTS_C_LINKAGE extern "C"
#else
#define OS_IOEVENTS_C_LINKAGE
#endif

#ifdef BUILD_OS_IOEVENTS_CORE
#define OS_IOEVENTS_CORE_EXPORT OS_IOEVENTS_C_LINKAGE OS_IOEVENTS_DLL_EXPORT
#else
#define OS_IOEVENTS_CORE_EXPORT OS_IOEVENTS_C_LINKAGE OS_IOEVENTS_DLL_IMPORT
#endif

#define OS_IOEVENTS_BIT(x) (1<<(x))

struct os_ioevents_context_s;
typedef struct os_ioevents_context_s os_ioevents_context_t;

struct os_ioevents_process_s;
typedef struct os_ioevents_process_s os_ioevents_process_t;

struct os_ioevents_fsmonitor_handle_s;
typedef struct os_ioevents_fsmonitor_handle_s os_ioevents_fsmonitor_handle_t;

typedef enum os_ioevents_capability_e
{
	OS_IOEVENTS_CAPABILITY_EXTERNAL_SEMAPHORE_SIGNALING = 0,
	OS_IOEVENTS_CAPABILITY_NUMBERED_EXTRA_PIPES,
	OS_IOEVENTS_CAPABILITY_NAMED_EXTRA_PIPES,

	OS_IOEVENTS_CAPABILITY_FSMONITOR_COOKIE,
	OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_FILES,
	OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORIES,
	OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORY_FILE_MODIFICATIONS,
} os_ioevents_capability_t;

typedef enum os_ioevents_fsmonitor_event_bits_e
{
    OS_IOEVENTS_FSMONITOR_EVENT_ACCESS = OS_IOEVENTS_BIT(0),
    OS_IOEVENTS_FSMONITOR_EVENT_ATTRIB = OS_IOEVENTS_BIT(1),
    OS_IOEVENTS_FSMONITOR_EVENT_CLOSE_WRITE = OS_IOEVENTS_BIT(2),
    OS_IOEVENTS_FSMONITOR_EVENT_CLOSE_NOWRITE = OS_IOEVENTS_BIT(3),
    OS_IOEVENTS_FSMONITOR_EVENT_CREATE = OS_IOEVENTS_BIT(4),
    OS_IOEVENTS_FSMONITOR_EVENT_DELETE = OS_IOEVENTS_BIT(5),
    OS_IOEVENTS_FSMONITOR_EVENT_DELETE_SELF = OS_IOEVENTS_BIT(6),
    OS_IOEVENTS_FSMONITOR_EVENT_MODIFY = OS_IOEVENTS_BIT(7),
    OS_IOEVENTS_FSMONITOR_EVENT_MOVE_SELF = OS_IOEVENTS_BIT(8),
    OS_IOEVENTS_FSMONITOR_EVENT_MOVED_FROM = OS_IOEVENTS_BIT(9),
    OS_IOEVENTS_FSMONITOR_EVENT_MOVED_TO = OS_IOEVENTS_BIT(10),
    OS_IOEVENTS_FSMONITOR_EVENT_OPEN = OS_IOEVENTS_BIT(11),
} os_ioevents_fsmonitor_event_bits_t;

typedef enum os_ioevents_event_type_e
{
    OS_IOEVENTS_EVENT_TYPE_NONE = 0,

    /* Process events*/
    OS_IOEVENTS_EVENT_TYPE_PROCESS_FINISHED = 1,
    OS_IOEVENTS_EVENT_TYPE_PROCESS_PIPE_READY,

    /* File events*/
    OS_IOEVENTS_EVENT_TYPE_FSMONITOR = 100,
}os_ioevents_event_type_t;

typedef enum os_ioevents_pipe_index_e
{
    OS_IOEVENTS_PIPE_INDEX_STDIN = 0,
    OS_IOEVENTS_PIPE_INDEX_STDOUT = 1,
    OS_IOEVENTS_PIPE_INDEX_STDERR = 2,

    /* These extra pipes are for facilitating the GDB I/O redirection*/
    OS_IOEVENTS_PIPE_INDEX_EXTRA_STDIN = 3,
    OS_IOEVENTS_PIPE_INDEX_EXTRA_STDOUT = 4,
    OS_IOEVENTS_PIPE_INDEX_EXTRA_STDERR = 5,
} os_ioevents_pipe_index_t;

typedef enum os_ioevents_process_spawn_flags_e
{
    OS_IOEVENTS_SPAWN_FLAGS_NONE = 0,
    OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES = (1<<0),
	OS_IOEVENTS_SPAWN_FLAGS_OPEN_IN_PSEUDO_TERMINAL = (1<<1),
	OS_IOEVENTS_SPAWN_FLAGS_OPEN_EXTRA_PIPES_IN_PSEUDO_TERMINAL = (1<<2),
} os_ioevents_process_spawn_flags_t;

typedef enum os_ioevents_pipe_error_e
{
    OS_IOEVENTS_PIPE_ERROR = -1,
    OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK = -2,
    OS_IOEVENTS_PIPE_ERROR_CLOSED = -3,
} os_ioevents_pipe_error_t;

typedef struct os_ioevents_event_process_pipe_s
{
    uint32_t type;
    os_ioevents_process_t *process;
    uint32_t pipeIndex;
} os_ioevents_event_process_pipe_t;

typedef struct os_ioevents_event_process_finished_s
{
    uint32_t type;
    os_ioevents_process_t *process;
    int32_t exitCode;
} os_ioevents_event_process_finished_t;

typedef struct os_ioevents_event_fsmonitor_s {
    uint32_t type;
    os_ioevents_fsmonitor_handle_t *handle;
    uint32_t mask;
    uint32_t cookie;
    uint32_t nameLength;
    const char *name;
} os_ioevents_event_fsmonitor_t;

typedef union os_ioevents_event_s
{
    uint32_t type;
    os_ioevents_event_process_pipe_t processPipe;
    os_ioevents_event_process_finished_t processFinished;
    os_ioevents_event_fsmonitor_t fsmonitor;
    uintptr_t padding[16];/* Maximum number of fields*/
} os_ioevents_event_t;

/* Context creation */
OS_IOEVENTS_CORE_EXPORT os_ioevents_context_t *os_ioevents_createContext(intptr_t pendingEventsSemaphoreIndex);
OS_IOEVENTS_CORE_EXPORT void os_ioevents_destroyContext(os_ioevents_context_t *context);

/* Feature querying. */
OS_IOEVENTS_CORE_EXPORT int os_ioevents_isCapabilitySupported(os_ioevents_context_t* context, os_ioevents_capability_t capability);

/* Memory allocation */
OS_IOEVENTS_CORE_EXPORT void *os_ioevents_malloc(size_t size);
OS_IOEVENTS_CORE_EXPORT void os_ioevents_free(void *pointer);

/* Process spawning */
OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t *os_ioevents_process_spawn(os_ioevents_context_t *context, const char *path, const char **argv, os_ioevents_process_spawn_flags_t flags);
OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t *os_ioevents_process_spawnInPath(os_ioevents_context_t *context, const char *file, const char **argv, os_ioevents_process_spawn_flags_t flags);
OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t *os_ioevents_process_spawnShell(os_ioevents_context_t *context, const char *command, os_ioevents_process_spawn_flags_t flags);

/* Process termination */
OS_IOEVENTS_CORE_EXPORT void os_ioevents_process_free(os_ioevents_process_t *process);
OS_IOEVENTS_CORE_EXPORT void os_ioevents_process_terminate(os_ioevents_process_t *process);
OS_IOEVENTS_CORE_EXPORT void os_ioevents_process_kill(os_ioevents_process_t *process);

/* Process pipes */
OS_IOEVENTS_CORE_EXPORT intptr_t os_ioevents_process_pipe_read(os_ioevents_process_t *process, os_ioevents_pipe_index_t pipe, void *buffer, size_t offset, size_t count);
OS_IOEVENTS_CORE_EXPORT intptr_t os_ioevents_process_pipe_write(os_ioevents_process_t *process, os_ioevents_pipe_index_t pipe, const void *buffer, size_t offset, size_t count);
OS_IOEVENTS_CORE_EXPORT const char *os_ioevents_process_pipe_getNamedEndpoint(os_ioevents_process_t* process, os_ioevents_pipe_index_t pipe);
OS_IOEVENTS_CORE_EXPORT int os_ioevents_process_pipe_isATTY(os_ioevents_process_t* process, os_ioevents_pipe_index_t pipe);
OS_IOEVENTS_CORE_EXPORT int os_ioevents_process_pipe_setTTYWindowSize(os_ioevents_process_t* process, os_ioevents_pipe_index_t pipe, int rows, int columns);

/* File system monitors */
OS_IOEVENTS_CORE_EXPORT os_ioevents_fsmonitor_handle_t *os_ioevents_fsmonitor_watchFile(os_ioevents_context_t *context, const char *path);
OS_IOEVENTS_CORE_EXPORT os_ioevents_fsmonitor_handle_t *os_ioevents_fsmonitor_watchFirectory(os_ioevents_context_t *context, const char *path);
OS_IOEVENTS_CORE_EXPORT void os_ioevents_fsmonitor_destroy(os_ioevents_context_t *context, os_ioevents_fsmonitor_handle_t *handle);
OS_IOEVENTS_CORE_EXPORT uint32_t os_ioevents_fsmonitor_getSupportedEventMask(os_ioevents_fsmonitor_handle_t* handle);

/* Event queue */
OS_IOEVENTS_CORE_EXPORT int os_ioevents_pollEvent(os_ioevents_context_t *context, os_ioevents_event_t *event);
OS_IOEVENTS_CORE_EXPORT int os_ioevents_waitEvent(os_ioevents_context_t *context, os_ioevents_event_t *event);
OS_IOEVENTS_CORE_EXPORT int os_ioevents_pushEvent(os_ioevents_context_t *context, os_ioevents_event_t *event);

#endif /* _OS_IOEVENTS_OS_IOEVENTS_H_ */
