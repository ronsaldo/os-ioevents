#ifndef OS_IOEVENTS_INTERNAL_H
#define OS_IOEVENTS_INTERNAL_H

#include <os-ioevents/os-ioevents.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "threads.h"

typedef void (*os_ioevents_destructor_t) (void *pointer);

static inline char *
os_ioevents_strdup(const char *string)
{
    size_t stringLength = strlen(string);
    char *result = (char*)malloc(stringLength + 1);
    memcpy(result, string, stringLength);
    result[stringLength] = 0;
    return result;
}
typedef struct os_ioevents_linked_list_node_s
{
    struct os_ioevents_linked_list_node_s *previous;
    struct os_ioevents_linked_list_node_s *next;
} os_ioevents_linked_list_node_t;

typedef struct os_ioevents_event_node_s
{
    os_ioevents_linked_list_node_t header;
    os_ioevents_event_t event;
} os_ioevents_event_node_t;

typedef struct os_ioevents_linked_list_s
{
    os_ioevents_linked_list_node_t *first;
    os_ioevents_linked_list_node_t *last;
} os_ioevents_linked_list_t;

static inline void
os_ioevents_linked_list_removeNode(os_ioevents_linked_list_t *list, os_ioevents_linked_list_node_t *node)
{
    assert(list);
    assert(node);

    /* Remove the previous link*/
    if(node->previous)
        node->previous->next = node->next;
    else
        list->first = node->next;

    /* Remove the next link*/
    if(node->next)
        node->next->previous = node->previous;
    else
        list->last = node->previous;
    node->previous = NULL;
    node->next = NULL;
}

static inline void
os_ioevents_linked_list_pushBack(os_ioevents_linked_list_t *list, os_ioevents_linked_list_node_t *node)
{
    assert(!node->previous);
    assert(!node->next);

    node->previous = list->last;
    if(list->last)
        list->last->next = node;
    list->last = node;

    if(!list->first)
        list->first = node;
}

static inline void
os_ioevents_linked_list_destroyData(os_ioevents_linked_list_t *list, os_ioevents_destructor_t destructor)
{
    os_ioevents_linked_list_node_t *nextNode = list->first;
    while(nextNode)
    {
        os_ioevents_linked_list_node_t *node = nextNode;
        nextNode = nextNode->next;
        destructor(node);
    }
}

static inline void
os_ioevents_linked_list_freeData(os_ioevents_linked_list_t *list)
{
    os_ioevents_linked_list_destroyData(list, free);
}

typedef struct os_ioevents_list_s
{
    size_t capacity;
    size_t size;
    void **data;
} os_ioevents_list_t;

static inline void
os_ioevents_list_increaseCapacity(os_ioevents_list_t *list)
{
    size_t newCapacity = list->capacity*2;
    if(newCapacity <= 16)
        newCapacity = 16;

    size_t newDataSize = newCapacity*sizeof(void*);
    void **newData = (void**)malloc(newDataSize);
    memset(newData, 0, newDataSize);

    for(size_t i = 0; i < list->size; ++i)
        newData[i] = list->data[i];
    free(list->data);
    list->data = newData;
    list->capacity = newCapacity;
}

static inline void
os_ioevents_list_pushBack(os_ioevents_list_t *list, void *value)
{
    if(list->size >= list->capacity)
        os_ioevents_list_increaseCapacity(list);
    list->data[list->size++] = value;
}

static inline void
os_ioevents_list_destroyData(os_ioevents_list_t *list, os_ioevents_destructor_t destructor)
{
    for(size_t i = 0; i < list->size; ++i)
        destructor(list->data[i]);
}

static inline void
os_ioevents_list_freeData(os_ioevents_list_t *list)
{
    os_ioevents_list_destroyData(list, free);
}

#endif /*OS_IOEVENTS_INTERNAL_H*/
