#ifdef _WIN32

#include "internal.h"
#include <stdio.h>
#include <stdarg.h>

typedef enum os_ioevents_iocp_key_type_e
{
	OS_IOEVENTS_IOCP_KEY_TYPE_INVALID=0,
	OS_IOEVENTS_IOCP_KEY_TYPE_FILE,
	OS_IOEVENTS_IOCP_KEY_TYPE_FSMONITOR,
} os_ioevents_iocp_key_type_t;

typedef struct os_ioevents_async_transmission_buffer_s
{
	OVERLAPPED overlapped;
	int isPending;
	int isWrite;
	DWORD readyDataCount;
	DWORD readOffset;
	uint8_t* currentTransmissionBuffer;
	os_ioevents_process_t* process;
	uint32_t pipeIndex;
	uint8_t inlineAllocation[4096];
} os_ioevents_async_transmission_buffer_t;

typedef struct os_ioevents_process_pipe_state_s
{
	os_ioevents_iocp_key_type_t type;
	HANDLE handle;
	char* namedEndpoint;
	os_ioevents_async_transmission_buffer_t transmissionBuffer;
} os_ioevents_process_pipe_state_t;

typedef struct os_ioevents_file_notify_information_s
{
	FILE_NOTIFY_INFORMATION header;
	WCHAR extraPathBuffer[MAX_PATH + 1];
} os_ioevents_file_notify_information_t;

struct os_ioevents_fsmonitor_handle_s
{
	os_ioevents_iocp_key_type_t type;
	HANDLE handle;
	OVERLAPPED overlapped;
	os_ioevents_file_notify_information_t notifyInformation;
};

typedef union os_ioevents_iocp_key_u
{
	os_ioevents_iocp_key_type_t type;
	os_ioevents_process_pipe_state_t pipe;
	os_ioevents_fsmonitor_handle_t fsmonitor;
} os_ioevents_iocp_key_t;

typedef struct os_ioevents_context_io_s
{
	HANDLE ioCompletionPort;

	os_ioevents_mutex_t processListMutex;
	os_ioevents_list_t processList;

	os_ioevents_mutex_t fsmonitorMutex;

} os_ioevents_context_io_t;

static int os_ioevents_fsmonitor_requestNextChange(os_ioevents_context_t* context, os_ioevents_fsmonitor_handle_t* fsmonitor);

static WCHAR*
os_ioevents_wstr_format(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	// Find the required buffer size.
	int bufferSize = vsnprintf(NULL, 0, format, args);
	if (bufferSize < 0)
	{
		va_end(args);
		return NULL;
	}

	// Now do the actual printing.
	char* buffer = malloc(bufferSize + 1);
	if (!buffer)
		return NULL;
	vsnprintf(buffer, bufferSize + 1, format, args);

	va_end(args);

	// Convert the result into a wide string.
	int wideBufferSize = MultiByteToWideChar(CP_UTF8, 0, buffer, bufferSize, NULL, 0);
	if (wideBufferSize < 0)
	{
		free(buffer);
		return NULL;
	}

	WCHAR* wideBuffer = malloc(sizeof(WCHAR) * (wideBufferSize + 1));
	if (!wideBuffer)
	{
		free(buffer);
		return NULL;
	}

	MultiByteToWideChar(CP_UTF8, 0, buffer, bufferSize, wideBuffer, wideBufferSize);
	wideBuffer[wideBufferSize] = 0;
	free(buffer);

	return wideBuffer;
}

static WCHAR*
os_ioevents_convertToWString(const char* string)
{
	// Convert the result into a wide string.
	int wideBufferSize = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	if (wideBufferSize < 0)
		return NULL;

	WCHAR* wideBuffer = malloc(sizeof(WCHAR) * wideBufferSize);
	if (!wideBuffer)
		return NULL;

	MultiByteToWideChar(CP_UTF8, 0, string, -1, wideBuffer, wideBufferSize);
	return wideBuffer;
}

#include "os_ioevents.c"

OS_IOEVENTS_CORE_EXPORT int
os_ioevents_isCapabilitySupported(os_ioevents_context_t* context, os_ioevents_capability_t capability)
{
	if (!context)
		return 0;

	switch (capability)
	{
	case OS_IOEVENTS_CAPABILITY_EXTERNAL_SEMAPHORE_SIGNALING:
		return context->signalSemaphoreWithIndex != NULL;
	case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORIES:
	case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_DIRECTORY_FILE_MODIFICATIONS:
		return 1;

	case OS_IOEVENTS_CAPABILITY_NUMBERED_EXTRA_PIPES:
	case OS_IOEVENTS_CAPABILITY_NAMED_EXTRA_PIPES:
	case OS_IOEVENTS_CAPABILITY_FSMONITOR_COOKIE:
	case OS_IOEVENTS_CAPABILITY_FSMONITOR_WATCH_FILES:
	default:
		return 0;
	}
}

static void
os_ioevents_iocp_handle_file(os_ioevents_context_t* context, os_ioevents_process_pipe_state_t* pipeState, DWORD transferredBytes, LPOVERLAPPED overlapped)
{
	(void)overlapped;
	os_ioevents_async_transmission_buffer_t* transmissionBuffer = &pipeState->transmissionBuffer;
	os_ioevents_mutex_lock(&context->io.processListMutex);
	if (transmissionBuffer->isPending)
	{
		if (transmissionBuffer->isWrite)
		{
			// Clear the transmission buffer in the write case.
			// TODO: Maybe we should ensure that we managed to write everything we wanted?
			if (transmissionBuffer->currentTransmissionBuffer != transmissionBuffer->inlineAllocation)
				free(transmissionBuffer->currentTransmissionBuffer);
			transmissionBuffer->currentTransmissionBuffer = NULL;
		}
		else
		{
			transmissionBuffer->readOffset = 0;
			transmissionBuffer->readyDataCount = transferredBytes;
		}

		transmissionBuffer->isPending = 0;

		os_ioevents_event_t event = {
			.processPipe = {
				.type = OS_IOEVENTS_EVENT_TYPE_PROCESS_PIPE_READY,
				.process = transmissionBuffer->process,
				.pipeIndex = transmissionBuffer->pipeIndex
			}
		};

		os_ioevents_pushEvent(context, &event);
	}
	os_ioevents_mutex_unlock(&context->io.processListMutex);
}

static void
os_ioevents_iocp_handle_fsmonitorNotification(os_ioevents_context_t* context, os_ioevents_fsmonitor_handle_t* fsmonitor, FILE_NOTIFY_INFORMATION *notification)
{
	if (notification->FileNameLength == 0)
		return;

	// Convert the file name.
	DWORD fileNameLength = notification->FileNameLength / 2;
	int requiredBufferSize = WideCharToMultiByte(CP_UTF8, 0, notification->FileName, fileNameLength, NULL, 0, NULL, NULL);
	if (requiredBufferSize < 0)
		return;

	char* fileNameBuffer = malloc(requiredBufferSize + 1);
	if (!fileNameBuffer)
		return;

	WideCharToMultiByte(CP_UTF8, 0, notification->FileName, fileNameLength, fileNameBuffer, requiredBufferSize, NULL, NULL);
	fileNameBuffer[requiredBufferSize] = 0;

	uint32_t convertedEventMask = 0;
	switch (notification->Action)
	{
	case FILE_ACTION_ADDED:
		convertedEventMask = OS_IOEVENTS_FSMONITOR_EVENT_CREATE;
		break;
	case FILE_ACTION_REMOVED:
		convertedEventMask = OS_IOEVENTS_FSMONITOR_EVENT_DELETE;
		break;
	case FILE_ACTION_MODIFIED:
		convertedEventMask = OS_IOEVENTS_FSMONITOR_EVENT_MODIFY;
		break;
	case FILE_ACTION_RENAMED_OLD_NAME:
		convertedEventMask = OS_IOEVENTS_FSMONITOR_EVENT_MOVED_FROM;
		break;
	case FILE_ACTION_RENAMED_NEW_NAME:
		convertedEventMask = OS_IOEVENTS_FSMONITOR_EVENT_MOVED_TO;
		break;
	default:
		break;
	}

	if (convertedEventMask != 0)
	{
		os_ioevents_event_t phevent = {
			.fsmonitor = {
				.type = OS_IOEVENTS_EVENT_TYPE_FSMONITOR,
				.handle = fsmonitor,
				.mask = convertedEventMask,
				.cookie = 0,
				.nameLength = (uint32_t)requiredBufferSize,
				.name = os_ioevents_strdup(fileNameBuffer)
			}
		};
		os_ioevents_pushEvent(context, &phevent);
	}
	// Free the temporary buffer.
	free(fileNameBuffer);
}

static void
os_ioevents_iocp_handle_fsmonitor(os_ioevents_context_t* context, os_ioevents_fsmonitor_handle_t *fsmonitor, DWORD transferredBytes, LPOVERLAPPED overlapped)
{
	(void)overlapped;
	os_ioevents_mutex_lock(&context->io.fsmonitorMutex);

	// Only handle the notification if we have received some bytes.
	if (transferredBytes > 0)
	{
		FILE_NOTIFY_INFORMATION* currentNotification = &fsmonitor->notifyInformation.header;
		for(;;)
		{
			os_ioevents_iocp_handle_fsmonitorNotification(context, fsmonitor, currentNotification);

			if (currentNotification->NextEntryOffset == 0)
				break;

			currentNotification = (FILE_NOTIFY_INFORMATION*)(((uint8_t*)currentNotification) + currentNotification->NextEntryOffset);
		}
	}

	if (!os_ioevents_fsmonitor_requestNextChange(context, fsmonitor))
	{
		CloseHandle(fsmonitor->handle);
		fsmonitor->handle = NULL;
	}

	os_ioevents_mutex_unlock(&context->io.fsmonitorMutex);
}

static int
os_ioevents_processThreadEntry(void *arg)
{
	os_ioevents_context_t* context = (os_ioevents_context_t*)arg;

	for (;;)
	{
		DWORD transferredBytes;
		LPOVERLAPPED overlapped;
		ULONG_PTR completionKey;
		if (!GetQueuedCompletionStatus(context->io.ioCompletionPort, &transferredBytes, &completionKey, &overlapped, INFINITE))
			break;

		os_ioevents_iocp_key_t* keyUnion = (os_ioevents_iocp_key_t*)completionKey;

		switch (keyUnion->type)
		{
		case OS_IOEVENTS_IOCP_KEY_TYPE_FILE:
			os_ioevents_iocp_handle_file(context, &keyUnion->pipe, transferredBytes, overlapped);
			break;
		case OS_IOEVENTS_IOCP_KEY_TYPE_FSMONITOR:
			os_ioevents_iocp_handle_fsmonitor(context, &keyUnion->fsmonitor, transferredBytes, overlapped);
			break;
		case OS_IOEVENTS_IOCP_KEY_TYPE_INVALID:
		default:
			break;
		}
	}
	return 0;
}

static void
os_ioevents_wakeUpSelectForShutdown(os_ioevents_context_t *context)
{
	CloseHandle(context->io.ioCompletionPort);
}

static int
os_ioevents_createContextIOPrimitives(os_ioevents_context_t *context)
{
	memset(context, 0, sizeof(os_ioevents_context_t));

	context->signalSemaphoreWithIndex = (signalSemaphoreWithIndex_t)GetProcAddress(0, "signalSemaphoreWithIndex");
	if (!context->signalSemaphoreWithIndex)
		context->signalSemaphoreWithIndex = (signalSemaphoreWithIndex_t)GetProcAddress(0, "_signalSemaphoreWithIndex");
	context->io.ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Initialize the synchronization primitives.
	os_ioevents_mutex_init(&context->io.processListMutex);
	os_ioevents_mutex_init(&context->io.fsmonitorMutex);

	return 1;
}

static void
os_ioevents_context_destroyIOData(os_ioevents_context_t *context)
{
	os_ioevents_mutex_destroy(&context->io.processListMutex);
	os_ioevents_mutex_destroy(&context->io.fsmonitorMutex);
}

/* Process spawning */
struct os_ioevents_process_s
{
	os_ioevents_context_t* context;
	int used;
	size_t index;
	os_ioevents_process_spawn_flags_t flags;

	int remainingPipes;
	int exitCode;

	HANDLE processHandle;
	DWORD pid;
	HANDLE waitHandle;

	union
	{
		struct
		{
			os_ioevents_process_pipe_state_t stdinPipe;
			os_ioevents_process_pipe_state_t stdoutPipe;
			os_ioevents_process_pipe_state_t stderrPipe;
			os_ioevents_process_pipe_state_t extraStdinPipe;
			os_ioevents_process_pipe_state_t extraStdoutPipe;
			os_ioevents_process_pipe_state_t extraStderrPipe;
		};
		os_ioevents_process_pipe_state_t pipes[6];
	};
};

static void printLastError(void)
{
	char *messageBuffer;
	DWORD error = GetLastError();
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		&messageBuffer,
		0, NULL);
	fprintf(stderr, "Win32 API error %08X: %s\n", error, messageBuffer);
	LocalFree(messageBuffer);
}

static volatile LONG os_ioevents_anonPipeSerialNumber;

static BOOL
os_ioevents_create_overlapped_pipe(HANDLE* readPipe, BOOL overlappedRead, HANDLE* writePipe, BOOL overlappedWrite)
{
	SECURITY_ATTRIBUTES securityAttributes;
	memset(&securityAttributes, 0, sizeof(securityAttributes));
	securityAttributes.nLength = sizeof(securityAttributes);
	securityAttributes.bInheritHandle = TRUE;

	// This technique comes from: https://stackoverflow.com/questions/60645/overlapped-i-o-on-anonymous-pipe
	// Create the name of the pipe.
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "\\\\.\\pipe\\LibOSIOEventsAnonPipe.%08x.%08x", GetCurrentProcessId(), InterlockedIncrement(&os_ioevents_anonPipeSerialNumber));

	DWORD bufferSize = 4096;
	HANDLE readHandle = CreateNamedPipeA(buffer, PIPE_ACCESS_INBOUND | (overlappedRead ? FILE_FLAG_OVERLAPPED : 0), PIPE_TYPE_BYTE | PIPE_WAIT, 1, bufferSize, bufferSize, 120 * 1000, &securityAttributes);
	if (readHandle == INVALID_HANDLE_VALUE)
	{
		printLastError();
		return FALSE;
	}

	HANDLE writeHandle = CreateFileA(buffer, GENERIC_WRITE, 0, &securityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | (overlappedWrite ? FILE_FLAG_OVERLAPPED : 0), NULL);
	if (writeHandle == INVALID_HANDLE_VALUE)
	{
		printLastError();
		CloseHandle(readHandle);
		return FALSE;
	}

	*readPipe = readHandle;
	*writePipe = writeHandle;
	return TRUE;
}

static os_ioevents_process_t*
os_ioevents_process_allocate(os_ioevents_context_t* context)
{
	os_ioevents_mutex_lock(&context->io.processListMutex);
	/* Find a free process. */
	os_ioevents_process_t* resultProcess = NULL;
	for (size_t i = 0; i < context->io.processList.size; ++i)
	{
		os_ioevents_process_t* process = context->io.processList.data[i];
		if (!process->used)
		{
			resultProcess = process;
			memset(resultProcess, 0, sizeof(os_ioevents_process_t));
			resultProcess->index = i;
		}
	}

	/* Allocate a new result process. */
	if (!resultProcess)
	{
		resultProcess = malloc(sizeof(os_ioevents_process_t));
		if (!resultProcess) abort();
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
os_ioevents_process_getFromIndex(os_ioevents_context_t* context, size_t index)
{
	if (index >= context->io.processList.size)
		return NULL;

	os_ioevents_process_t* process = context->io.processList.data[index];
	if (!process->used)
		return NULL;

	return process;
}

static void
os_ioevents_process_freePipe(os_ioevents_process_pipe_state_t* pipe)
{
	if (pipe->handle)
		CloseHandle(pipe->handle);

	if (pipe->transmissionBuffer.currentTransmissionBuffer)
	{
		if (pipe->transmissionBuffer.currentTransmissionBuffer != pipe->transmissionBuffer.inlineAllocation)
			free(pipe->transmissionBuffer.currentTransmissionBuffer);
	}

	memset(pipe, 0, sizeof(os_ioevents_process_pipe_state_t));
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_process_free(os_ioevents_process_t* process)
{
	printf("Process free: %p\n", process);
	if (!process)
		return;

	os_ioevents_context_t* context = process->context;
	os_ioevents_mutex_lock(&context->io.processListMutex);
	if (process->used)
	{
		os_ioevents_process_freePipe(&process->stdinPipe);
		os_ioevents_process_freePipe(&process->stdoutPipe);
		os_ioevents_process_freePipe(&process->stderrPipe);
		if (process->processHandle)
			CloseHandle(process->processHandle);
	}
	memset(process, 0, sizeof(os_ioevents_process_t));
	os_ioevents_mutex_unlock(&context->io.processListMutex);
}

static VOID CALLBACK
os_ioevents_process_waitCallback(_In_ PVOID lpParameter, _In_ BOOLEAN isTimeOut)
{
	// Ignore the time outs.
	if (isTimeOut)
		return;

	os_ioevents_process_t *process = (os_ioevents_process_t*)lpParameter;
	os_ioevents_context_t *context = process->context;
	os_ioevents_mutex_lock(&context->io.processListMutex);
	if (process->processHandle)
	{
		DWORD exitCode = 0;
		if (GetExitCodeProcess(process->processHandle, &exitCode))
			process->exitCode = exitCode;
	}

	// Push the process finished event.
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

	os_ioevents_mutex_unlock(&context->io.processListMutex);
}

static void
os_ioevents_process_allPipesAreClosed(os_ioevents_process_t* process)
{
	(void)process;
}

static os_ioevents_process_t*
os_ioevents_process_createNewHandle(os_ioevents_context_t* context, LPCWSTR applicationName, LPWSTR commandLine, os_ioevents_process_spawn_flags_t flags)
{
	(void)flags;
	// Create the pipes.
	HANDLE parentStdin = NULL, childStdin = NULL;
	HANDLE parentStdout = NULL, childStdout = NULL;
	HANDLE parentStderr = NULL, childStderr = NULL;

	if (!os_ioevents_create_overlapped_pipe(&childStdin, FALSE, &parentStdin, TRUE))
	{
		return NULL;
	}
	if (!os_ioevents_create_overlapped_pipe(&parentStdout, TRUE, &childStdout, FALSE))
	{
		CloseHandle(parentStdin); CloseHandle(childStdin);
		return NULL;
	}
	if (!os_ioevents_create_overlapped_pipe(&parentStderr, TRUE, &childStderr, FALSE))
	{
		CloseHandle(parentStdin); CloseHandle(childStdin);
		CloseHandle(parentStdout); CloseHandle(childStdout);
		return NULL;
	}

	os_ioevents_process_t* processHandle = os_ioevents_process_allocate(context);
	if (!processHandle ||
		!SetHandleInformation(parentStdin, HANDLE_FLAG_INHERIT, 0) ||
		!SetHandleInformation(parentStdout, HANDLE_FLAG_INHERIT, 0) ||
		!SetHandleInformation(parentStderr, HANDLE_FLAG_INHERIT, 0))
	{
		CloseHandle(parentStdin); CloseHandle(childStdin);
		CloseHandle(parentStdout); CloseHandle(childStdout);
		CloseHandle(parentStderr); CloseHandle(childStderr);
		return NULL;
	}

	// Create the actual process.
	STARTUPINFOW startupInfo;
	memset(&startupInfo, 0, sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.hStdInput = childStdin;
	startupInfo.hStdOutput = childStdout;
	startupInfo.hStdError = childStderr;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION processInformation;
	memset(&processInformation, 0, sizeof(processInformation));
	if (!CreateProcessW(applicationName, commandLine, NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInformation))
	{
		printLastError();
		CloseHandle(parentStdin); CloseHandle(childStdin);
		CloseHandle(parentStdout); CloseHandle(childStdout);
		CloseHandle(parentStderr); CloseHandle(childStderr);
		return NULL;
	}

	// We do not care about the main thread.
	CloseHandle(processInformation.hThread);

	// Close the child handles, we do not need them anymore.
	CloseHandle(childStdin);
	CloseHandle(childStdout);
	CloseHandle(childStderr);

	processHandle->processHandle = processInformation.hProcess;
	processHandle->pid = processInformation.dwProcessId;
	processHandle->remainingPipes = 3;

	processHandle->stdinPipe.type = OS_IOEVENTS_IOCP_KEY_TYPE_FILE;
	processHandle->stdinPipe.handle = parentStdin;
	processHandle->stdoutPipe.type = OS_IOEVENTS_IOCP_KEY_TYPE_FILE;
	processHandle->stdoutPipe.handle = parentStdout;
	processHandle->stderrPipe.type = OS_IOEVENTS_IOCP_KEY_TYPE_FILE;
	processHandle->stderrPipe.handle = parentStderr;
	if (!CreateIoCompletionPort(parentStdin, context->io.ioCompletionPort, (ULONG_PTR)&processHandle->stdinPipe, 0) ||
		!CreateIoCompletionPort(parentStdout, context->io.ioCompletionPort, (ULONG_PTR)&processHandle->stdoutPipe, 0) ||
		!CreateIoCompletionPort(parentStderr, context->io.ioCompletionPort, (ULONG_PTR)&processHandle->stderrPipe, 0) ||
		!RegisterWaitForSingleObject(&processHandle->waitHandle, processHandle->processHandle, os_ioevents_process_waitCallback, processHandle, INFINITE, WT_EXECUTEONLYONCE))
	{
		printLastError();
		os_ioevents_process_free(processHandle);
		return NULL;
	}

	return processHandle;
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t*
os_ioevents_process_spawn(os_ioevents_context_t *context, const char *path, const char **argv, os_ioevents_process_spawn_flags_t flags)
{
	(void)context;
	(void)path;
	(void)argv;
	(void)flags;
	printf("TODO: Spawn\n");
	return NULL;
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t*
os_ioevents_process_spawnInPath(os_ioevents_context_t *context, const char *file, const char **argv, os_ioevents_process_spawn_flags_t flags)
{
	(void)context;
	(void)file;
	(void)argv;
	(void)flags;
	printf("TODO: Spawn in path\n");
	return NULL;
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_process_t*
os_ioevents_process_spawnShell(os_ioevents_context_t *context, const char *command, os_ioevents_process_spawn_flags_t flags)
{
	WCHAR *convertedCommandLine = os_ioevents_wstr_format("cmd.exe /c %s", command);
	if (!convertedCommandLine)
		return NULL;

	os_ioevents_process_t* process = os_ioevents_process_createNewHandle(context, NULL, convertedCommandLine, flags);
	free(convertedCommandLine);
	return process;
}

/* Process termination */
OS_IOEVENTS_CORE_EXPORT void
os_ioevents_process_terminate(os_ioevents_process_t *process)
{
	(void)process;
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_process_kill(os_ioevents_process_t *process)
{
	(void)process;
}

/* Process pipes */
OS_IOEVENTS_CORE_EXPORT intptr_t
os_ioevents_process_pipe_read(os_ioevents_process_t *process, os_ioevents_pipe_index_t pipe, void *buffer, size_t offset, size_t count)
{
	if (!process)
		return OS_IOEVENTS_PIPE_ERROR;

	if (pipe > OS_IOEVENTS_PIPE_INDEX_EXTRA_STDERR)
		return OS_IOEVENTS_PIPE_ERROR_CLOSED;

	os_ioevents_mutex_lock(&process->context->io.processListMutex);

	// Get the pipe file descriptor.
	os_ioevents_process_pipe_state_t *pipeState = &process->pipes[pipe];
	if(!pipeState->handle)
	{
		os_ioevents_mutex_unlock(&process->context->io.processListMutex);
		return OS_IOEVENTS_PIPE_ERROR_CLOSED;
	}

	// Only allow a single pending transmission.
	if (pipeState->transmissionBuffer.isPending)
	{
		os_ioevents_mutex_unlock(&process->context->io.processListMutex);
		return OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK;
	}

	if (!pipeState->transmissionBuffer.isPending && pipeState->transmissionBuffer.readyDataCount > 0)
	{
		intptr_t dataToRead = min(pipeState->transmissionBuffer.readyDataCount, count);
		memcpy((uint8_t*)buffer + offset, pipeState->transmissionBuffer.currentTransmissionBuffer + pipeState->transmissionBuffer.readOffset, dataToRead);
		pipeState->transmissionBuffer.readOffset += (DWORD)dataToRead;
		pipeState->transmissionBuffer.readyDataCount -= (DWORD)dataToRead;
		if(pipeState->transmissionBuffer.readyDataCount == 0)
		{
			if (pipeState->transmissionBuffer.currentTransmissionBuffer != pipeState->transmissionBuffer.inlineAllocation)
				free(pipeState->transmissionBuffer.currentTransmissionBuffer);
		}

		os_ioevents_mutex_unlock(&process->context->io.processListMutex);
		return dataToRead;
	}
	else
	{
		// Enqueue a new read operation.
		pipeState->transmissionBuffer.isPending = 1;
		pipeState->transmissionBuffer.readyDataCount = 0;
		pipeState->transmissionBuffer.readOffset = 0;
		pipeState->transmissionBuffer.isWrite = 0;
		pipeState->transmissionBuffer.process = process;
		pipeState->transmissionBuffer.pipeIndex = pipe;

		if (count <= sizeof(pipeState->transmissionBuffer.inlineAllocation))
		{
			pipeState->transmissionBuffer.currentTransmissionBuffer = pipeState->transmissionBuffer.inlineAllocation;
		}
		else
		{
			pipeState->transmissionBuffer.currentTransmissionBuffer = malloc(count);
			if (!pipeState->transmissionBuffer.currentTransmissionBuffer)
			{
				pipeState->transmissionBuffer.currentTransmissionBuffer = pipeState->transmissionBuffer.inlineAllocation;
				count = sizeof(pipeState->transmissionBuffer.inlineAllocation);
			}
		}

		memset(&pipeState->transmissionBuffer.overlapped, 0, sizeof(pipeState->transmissionBuffer.overlapped));
		BOOL result = ReadFile(pipeState->handle, pipeState->transmissionBuffer.currentTransmissionBuffer, (DWORD)count, NULL, &pipeState->transmissionBuffer.overlapped);
		int returnValue = 0;
		if (!result)
		{
			switch (GetLastError())
			{
			case ERROR_IO_PENDING:
				returnValue = OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK;
				break;
			case ERROR_BROKEN_PIPE:
				returnValue = OS_IOEVENTS_PIPE_ERROR_CLOSED;
				CloseHandle(pipeState->handle);
				if (pipeState->transmissionBuffer.currentTransmissionBuffer != pipeState->transmissionBuffer.inlineAllocation)
				{
					free(pipeState->transmissionBuffer.currentTransmissionBuffer);
					pipeState->transmissionBuffer.currentTransmissionBuffer = NULL;
				}
				pipeState->handle = NULL;
				--process->remainingPipes;
				if (process->remainingPipes == 0)
					os_ioevents_process_allPipesAreClosed(process);
				break;
			default:
				printLastError();
				returnValue = OS_IOEVENTS_PIPE_ERROR;
				break;
			}
		}
		else
		{
			// TODO: Try to handle this case immediately to reduce latency.
			returnValue = OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK;
		}
		os_ioevents_mutex_unlock(&process->context->io.processListMutex);

		return returnValue;
	}
}

OS_IOEVENTS_CORE_EXPORT intptr_t
os_ioevents_process_pipe_write(os_ioevents_process_t *process, os_ioevents_pipe_index_t pipe, const void *buffer, size_t offset, size_t count)
{
	if (!process)
		return OS_IOEVENTS_PIPE_ERROR;

	if (pipe > OS_IOEVENTS_PIPE_INDEX_EXTRA_STDERR)
		return OS_IOEVENTS_PIPE_ERROR_CLOSED;

	os_ioevents_mutex_lock(&process->context->io.processListMutex);

	// Get the pipe file descriptor.
	os_ioevents_process_pipe_state_t* pipeState = &process->pipes[pipe];
	if (!pipeState->handle)
	{
		os_ioevents_mutex_unlock(&process->context->io.processListMutex);
		return OS_IOEVENTS_PIPE_ERROR_CLOSED;
	}

	// Only allow a single pending transmission.
	if (pipeState->transmissionBuffer.isPending)
	{
		os_ioevents_mutex_unlock(&process->context->io.processListMutex);
		return OS_IOEVENTS_PIPE_ERROR_WOULD_BLOCK;
	}

	// Enqueue a new write operation.
	pipeState->transmissionBuffer.isPending = 1;
	pipeState->transmissionBuffer.readyDataCount = 0;
	pipeState->transmissionBuffer.readOffset = 0;
	pipeState->transmissionBuffer.isWrite = 1;
	pipeState->transmissionBuffer.process = process;
	pipeState->transmissionBuffer.pipeIndex = pipe;

	if (count <= sizeof(pipeState->transmissionBuffer.inlineAllocation))
	{
		pipeState->transmissionBuffer.currentTransmissionBuffer = pipeState->transmissionBuffer.inlineAllocation;
	}
	else
	{
		pipeState->transmissionBuffer.currentTransmissionBuffer = malloc(count);
		if (!pipeState->transmissionBuffer.currentTransmissionBuffer)
		{
			pipeState->transmissionBuffer.currentTransmissionBuffer = pipeState->transmissionBuffer.inlineAllocation;
			count = sizeof(pipeState->transmissionBuffer.inlineAllocation);
		}
	}

	memcpy(pipeState->transmissionBuffer.currentTransmissionBuffer, (uint8_t*)buffer + offset, count);
	memset(&pipeState->transmissionBuffer.overlapped, 0, sizeof(pipeState->transmissionBuffer.overlapped));

	BOOL result = WriteFile(pipeState->handle, pipeState->transmissionBuffer.currentTransmissionBuffer, (DWORD)count, NULL, &pipeState->transmissionBuffer.overlapped);
	int returnValue = 0;
	if (!result)
	{
		switch (GetLastError())
		{
		case ERROR_IO_PENDING:
			returnValue = (int)count;
			break;
		case ERROR_BROKEN_PIPE:
			returnValue = OS_IOEVENTS_PIPE_ERROR_CLOSED;
			CloseHandle(pipeState->handle);
			if (pipeState->transmissionBuffer.currentTransmissionBuffer != pipeState->transmissionBuffer.inlineAllocation)
			{
				free(pipeState->transmissionBuffer.currentTransmissionBuffer);
				pipeState->transmissionBuffer.currentTransmissionBuffer = NULL;
			}
			pipeState->handle = NULL;
			--process->remainingPipes;
			if (process->remainingPipes == 0)
				os_ioevents_process_allPipesAreClosed(process);
			break;
		default:
			printLastError();
			returnValue = OS_IOEVENTS_PIPE_ERROR;
			break;
		}
	}
	else
	{
		returnValue = (int)count;
	}
	os_ioevents_mutex_unlock(&process->context->io.processListMutex);

	return returnValue;
}

OS_IOEVENTS_CORE_EXPORT const char*
os_ioevents_process_pipe_getNamedEndpoint(os_ioevents_process_t* process, os_ioevents_pipe_index_t pipe)
{
	// We only create publicly accessible named pipes for the extra standard inputs/outputs.
	if (!process || pipe < OS_IOEVENTS_PIPE_INDEX_EXTRA_STDIN || pipe > OS_IOEVENTS_PIPE_INDEX_EXTRA_STDERR)
		return NULL;

	return process->pipes[pipe].namedEndpoint;
}

/* File system monitors */
OS_IOEVENTS_CORE_EXPORT os_ioevents_fsmonitor_handle_t*
os_ioevents_fsmonitor_watchFile(os_ioevents_context_t *context, const char *path)
{
	(void)context;
	(void)path;
	// We can't watch a single file with the Win32 API, so always return NULL in this case.
	return NULL;
}

static int
os_ioevents_fsmonitor_requestNextChange(os_ioevents_context_t* context, os_ioevents_fsmonitor_handle_t* fsmonitor)
{
	(void)context;

	DWORD notifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME
		| FILE_NOTIFY_CHANGE_ATTRIBUTES /*| FILE_NOTIFY_CHANGE_SIZE*/
		| FILE_NOTIFY_CHANGE_LAST_WRITE /*| FILE_NOTIFY_CHANGE_LAST_ACCESS*/
		| FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;

	memset(&fsmonitor->overlapped, 0, sizeof(fsmonitor->overlapped));
	if (ReadDirectoryChangesW(fsmonitor->handle, &fsmonitor->notifyInformation, sizeof(fsmonitor->notifyInformation),
		FALSE, notifyFilter, NULL, &fsmonitor->overlapped, NULL))
	{
		// The request has succeded, nothing special is required.
	}
	else
	{
		DWORD errorCode = GetLastError();
		switch (errorCode)
		{
		case ERROR_IO_PENDING:
			// This case is fine.
		default:
			printLastError();
			break;
		}
	}

	return 1;
}

OS_IOEVENTS_CORE_EXPORT os_ioevents_fsmonitor_handle_t*
os_ioevents_fsmonitor_watchDirectory(os_ioevents_context_t *context, const char *path)
{
	if (!context)
		return NULL;

	// Convert the path into wide string.
	WCHAR* wpath = os_ioevents_convertToWString(path);
	if (!wpath)
		return NULL;

	// Create the directory handle.
	HANDLE handle = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	free(wpath);
	if (handle == INVALID_HANDLE_VALUE)
	{
		printLastError();
		return NULL;
	}

	// Create the monitor handle.
	os_ioevents_fsmonitor_handle_t* result = malloc(sizeof(os_ioevents_fsmonitor_handle_t));
	if (!result)
	{
		CloseHandle(handle);
		return NULL;
	}
	memset(result, 0, sizeof(os_ioevents_fsmonitor_handle_t));

	result->type = OS_IOEVENTS_IOCP_KEY_TYPE_FSMONITOR;
	result->handle = handle;

	if (!CreateIoCompletionPort(handle, context->io.ioCompletionPort, (ULONG_PTR)result, 0) ||
		!os_ioevents_fsmonitor_requestNextChange(context, result))
	{
		CloseHandle(handle);
		free(result);
	}

	return result;
}

OS_IOEVENTS_CORE_EXPORT uint32_t
os_ioevents_fsmonitor_getSupportedEventMask(os_ioevents_fsmonitor_handle_t* handle)
{
	return
		OS_IOEVENTS_FSMONITOR_EVENT_CREATE |
		OS_IOEVENTS_FSMONITOR_EVENT_DELETE |
		OS_IOEVENTS_FSMONITOR_EVENT_MODIFY |
		OS_IOEVENTS_FSMONITOR_EVENT_MOVED_FROM |
		OS_IOEVENTS_FSMONITOR_EVENT_MOVED_TO;
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_fsmonitor_destroy(os_ioevents_context_t *context, os_ioevents_fsmonitor_handle_t *handle)
{
	if (!context || !handle)
		return;

	os_ioevents_mutex_lock(&context->io.fsmonitorMutex);
	if (handle->handle)
		CloseHandle(handle->handle);
	free(handle);
	os_ioevents_mutex_unlock(&context->io.fsmonitorMutex);
}

#endif //_WIN32
