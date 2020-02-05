#ifndef OS_IOEVENTS_THREADS_H
#define OS_IOEVENTS_THREADS_H

typedef int (*os_ioevents_thread_entry_point_t)(void*);
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

typedef HANDLE os_ioevents_thread_t;
typedef CRITICAL_SECTION os_ioevents_mutex_t;
typedef CONDITION_VARIABLE os_ioevents_condition_t;

typedef struct os_ioevents_thread_entry_arg_s
{
	os_ioevents_thread_entry_point_t entryPoint;
	void *argument;
} os_ioevents_thread_entry_arg_t;

static inline DWORD WINAPI
os_ioevents_thread_entry(LPVOID param)
{
	os_ioevents_thread_entry_arg_t arg = *((os_ioevents_thread_entry_arg_t*)param);
	free(param);

	return arg.entryPoint(arg.argument);
}

static inline int
os_ioevents_thread_create(os_ioevents_thread_t *thread, os_ioevents_thread_entry_point_t entryPoint, void *argument)
{
	os_ioevents_thread_entry_arg_t *entryArgument = (os_ioevents_thread_entry_arg_t*)malloc(sizeof(os_ioevents_thread_entry_arg_t));
	entryArgument->entryPoint = entryPoint;
	entryArgument->argument = argument;

	/* 1 MB of stack seems overkill. */
	SIZE_T stackSize = 128 * 1024;
	*thread = CreateThread(NULL, stackSize, os_ioevents_thread_entry, entryArgument, 0, NULL);
	if (*thread == NULL)
	{
		free(entryArgument);
		return -1;
	}

	return 0;
}

static inline int
os_ioevents_thread_join(os_ioevents_thread_t thread)
{
	DWORD result = WaitForMultipleObjects(1, thread, TRUE, INFINITE);
	if (result == WAIT_FAILED)
		return 1;

	CloseHandle(thread);
	return 0;
}

static inline int
os_ioevents_mutex_init(os_ioevents_mutex_t *mutex)
{
	InitializeCriticalSection(mutex);
	return 0;
}

static inline int os_ioevents_mutex_destroy(os_ioevents_mutex_t *mutex)
{
	DeleteCriticalSection(mutex);
	return 0;
}

static inline int os_ioevents_mutex_lock(os_ioevents_mutex_t *mutex)
{
	EnterCriticalSection(mutex);
	return 0;
}

static inline int os_ioevents_mutex_unlock(os_ioevents_mutex_t *mutex)
{
	LeaveCriticalSection(mutex);
	return 0;
}

static inline int os_ioevents_condition_init(os_ioevents_condition_t *cond)
{
	InitializeConditionVariable(cond);
	return 0;
}

static inline int os_ioevents_condition_destroy(os_ioevents_condition_t *cond)
{
	/* There is no delete on windows: See https://stackoverflow.com/questions/28975958/why-does-windows-have-no-deleteconditionvariable-function-to-go-together-with */
	(void)cond;
	return 0;
}

static inline int os_ioevents_condition_wait(os_ioevents_condition_t *cond, os_ioevents_mutex_t *mutex)
{
	return SleepConditionVariableCS(cond, mutex, INFINITE) == ERROR_TIMEOUT;
}

static inline int os_ioevents_condition_signal(os_ioevents_condition_t *cond)
{
	WakeConditionVariable(cond);
	return 0;
}

static inline int os_ioevents_condition_broadcast(os_ioevents_condition_t *cond)
{
	WakeAllConditionVariable(cond);
	return 0;
}

#else
/* Assume pthreads. */
#include <pthread.h>
#include <stdint.h>

typedef pthread_t os_ioevents_thread_t;
typedef pthread_mutex_t os_ioevents_mutex_t;
typedef pthread_cond_t os_ioevents_condition_t;

typedef struct os_ioevents_thread_entry_arg_s
{
	os_ioevents_thread_entry_point_t entryPoint;
	void *argument;
} os_ioevents_thread_entry_arg_t;

static inline void* os_ioevents_thread_entry(void *param)
{
	os_ioevents_thread_entry_arg_t arg = *((os_ioevents_thread_entry_arg_t*)param);
	free(param);

	return (void*)((intptr_t)arg.entryPoint(arg.argument));
}

static inline int os_ioevents_thread_create(os_ioevents_thread_t *thread, os_ioevents_thread_entry_point_t entryPoint, void *argument)
{
	os_ioevents_thread_entry_arg_t *entryArgument = (os_ioevents_thread_entry_arg_t*)malloc(sizeof(os_ioevents_thread_entry_arg_t));
	entryArgument->entryPoint = entryPoint;
	entryArgument->argument = argument;

	int error = pthread_create(thread, NULL, os_ioevents_thread_entry, entryArgument);
	if (error)
		free(entryArgument);

	return error;
}

static inline int os_ioevents_thread_join(os_ioevents_thread_t thread)
{
    void *returnValue = NULL;
    return pthread_join(thread, &returnValue);
}

static inline int os_ioevents_mutex_init(os_ioevents_mutex_t *mutex)
{
    return pthread_mutex_init(mutex, NULL);
}

static inline int os_ioevents_mutex_destroy(os_ioevents_mutex_t *mutex)
{
    return pthread_mutex_destroy(mutex);
}

static inline int os_ioevents_mutex_lock(os_ioevents_mutex_t *mutex)
{
    return pthread_mutex_lock(mutex);
}

static inline int os_ioevents_mutex_unlock(os_ioevents_mutex_t *mutex)
{
    return pthread_mutex_unlock(mutex);
}

static inline int os_ioevents_condition_init(os_ioevents_condition_t *cond)
{
    return pthread_cond_init(cond, NULL);
}

static inline int os_ioevents_condition_destroy(os_ioevents_condition_t *cond)
{
    return pthread_cond_destroy(cond);
}

static inline int os_ioevents_condition_wait(os_ioevents_condition_t *cond, os_ioevents_mutex_t *mutex)
{
    return pthread_cond_wait(cond, mutex);
}

static inline int os_ioevents_condition_signal(os_ioevents_condition_t *cond)
{
    return pthread_cond_signal(cond);
}

static inline int os_ioevents_condition_broadcast(os_ioevents_condition_t *cond)
{
    return pthread_cond_broadcast(cond);
}

#endif

#endif /* OS_IOEVENTS_THREADS_H*/
