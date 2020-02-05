#include "internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef intptr_t (*signalSemaphoreWithIndex_t) (intptr_t index);

struct os_ioevents_context_s
{
    os_ioevents_thread_t thread;

    /* Control mutex */
    os_ioevents_mutex_t controlMutex;
    int shuttingDown;

    intptr_t pendingEventsSemaphoreIndex;
    signalSemaphoreWithIndex_t signalSemaphoreWithIndex;

    /* Event queue */
    os_ioevents_mutex_t eventQueueMutex;
    os_ioevents_condition_t pendingEventCondition;
    os_ioevents_linked_list_t eventQueue;

    /* IO*/
    os_ioevents_context_io_t io;
};


static int os_ioevents_processThreadEntry(void *arg);
static int os_ioevents_createContextIOPrimitives(os_ioevents_context_t *context);
static void os_ioevents_wakeUpSelectForShutdown(os_ioevents_context_t *context);
static void os_ioevents_context_destroyIOData(os_ioevents_context_t *context);

OS_IOEVENTS_CORE_EXPORT os_ioevents_context_t *
os_ioevents_createContext(intptr_t pendingEventsSemaphoreIndex)
{
    /* Allocate the context*/
    os_ioevents_context_t *context = (os_ioevents_context_t*)malloc(sizeof(os_ioevents_context_t));
    memset(context, 0, sizeof(os_ioevents_context_t));
    context->pendingEventsSemaphoreIndex = pendingEventsSemaphoreIndex;

    if(!os_ioevents_createContextIOPrimitives(context))
    {
        free(context);
        return NULL;
    }

    /* Initialize the synchronization primitives. */
    os_ioevents_mutex_init(&context->controlMutex);

    os_ioevents_mutex_init(&context->eventQueueMutex);
    os_ioevents_condition_init(&context->pendingEventCondition);

    /* Start the thread*/
    os_ioevents_thread_create(&context->thread, os_ioevents_processThreadEntry, context);

    return context;
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_destroyContext(os_ioevents_context_t *context)
{
    if(!context)
        return;

    /* Set the shutting down condition. */
    {
        os_ioevents_mutex_lock(&context->controlMutex);
        context->shuttingDown = 1;
        os_ioevents_condition_broadcast(&context->pendingEventCondition);
        os_ioevents_wakeUpSelectForShutdown(context);
        os_ioevents_mutex_unlock(&context->controlMutex);
    }

    /* Wait for the working thread to finish. */
    os_ioevents_thread_join(context->thread);

    os_ioevents_mutex_destroy(&context->controlMutex);

    os_ioevents_mutex_destroy(&context->eventQueueMutex);
    os_ioevents_condition_destroy(&context->pendingEventCondition);

    os_ioevents_context_destroyIOData(context);
    os_ioevents_linked_list_freeData(&context->eventQueue);
    free(context);
}

OS_IOEVENTS_CORE_EXPORT void *
os_ioevents_malloc(size_t size)
{
    return malloc(size);
}

OS_IOEVENTS_CORE_EXPORT void
os_ioevents_free(void *pointer)
{
    free(pointer);
}

OS_IOEVENTS_CORE_EXPORT int
os_ioevents_pollEvent(os_ioevents_context_t *context, os_ioevents_event_t *event)
{
    int result = 0;
    os_ioevents_mutex_lock(&context->eventQueueMutex);
    if(context->eventQueue.first)
    {
        os_ioevents_event_node_t *eventNode = (os_ioevents_event_node_t*)context->eventQueue.first;
        *event = eventNode->event;

        os_ioevents_linked_list_removeNode(&context->eventQueue, (os_ioevents_linked_list_node_t *)eventNode);
        free(eventNode);
        result = 1;
    }

    os_ioevents_mutex_unlock(&context->eventQueueMutex);
    return result;
}

OS_IOEVENTS_CORE_EXPORT int
os_ioevents_waitEvent(os_ioevents_context_t *context, os_ioevents_event_t *event)
{
    int result = 0;
    os_ioevents_mutex_lock(&context->eventQueueMutex);

    /* Wait for an event, or shutting down. */
    while (!context->eventQueue.first && !context->shuttingDown)
        os_ioevents_condition_wait(&context->pendingEventCondition, &context->eventQueueMutex);

    /* Extract the event from the queue. */
    if(context->eventQueue.first)
    {
        os_ioevents_event_node_t *eventNode = (os_ioevents_event_node_t*)context->eventQueue.first;
        *event = eventNode->event;

        os_ioevents_linked_list_removeNode(&context->eventQueue, (os_ioevents_linked_list_node_t *)eventNode);
        free(eventNode);
        result = 1;
    }

    os_ioevents_mutex_unlock(&context->eventQueueMutex);
    return result;
}

OS_IOEVENTS_CORE_EXPORT int
os_ioevents_pushEvent(os_ioevents_context_t *context, os_ioevents_event_t *event)
{
    os_ioevents_event_node_t *node = malloc(sizeof(os_ioevents_event_node_t));
    memset(node, 0, sizeof(os_ioevents_event_node_t));
    node->event = *event;

    os_ioevents_mutex_lock(&context->eventQueueMutex);

    os_ioevents_linked_list_pushBack(&context->eventQueue, (os_ioevents_linked_list_node_t *)node);

    os_ioevents_condition_signal(&context->pendingEventCondition);
    os_ioevents_mutex_unlock(&context->eventQueueMutex);

    /* Notify the VM about the event. */
    /*printf("context->signalSemaphoreWithIndex %p\n", context->signalSemaphoreWithIndex);*/
    if(context->signalSemaphoreWithIndex)
        context->signalSemaphoreWithIndex(context->pendingEventsSemaphoreIndex);
    return 0;
}
