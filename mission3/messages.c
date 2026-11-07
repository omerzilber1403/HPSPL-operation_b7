#include "messages.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int store_init(MessageStore *store, size_t initial_capacity)
{
    Message *messages;
    if (store == NULL) {
        return 0;
    }

    /* Start from the valid empty state, so every failure path leaves it. */
    store->messages = NULL;
    store->count = 0;
    store->capacity = 0;

    if (initial_capacity == 0) {
        return 1;
    }

    /* initial_capacity * sizeof(Message) must fit in size_t. */
    if (initial_capacity > SIZE_MAX / sizeof(Message)) {
        return 0;
    }

    /* calloc zeroes every slot: id == 0, text == NULL (empty messages). */
    messages = calloc(initial_capacity, sizeof(Message));
    if (messages == NULL) {
        return 0;
    }

    store->messages = messages;
    store->capacity = initial_capacity;
    return 1;
}

int message_init(Message *msg, int id, const char *text)
{
    size_t len;
    char *copy;

    if (msg == NULL) {
        return 0;
    }

    /* Empty state first, so every failure path leaves id == 0, text == NULL. */
    msg->id = 0;
    msg->text = NULL;

    if (id <= 0 || text == NULL) {
        return 0;
    }

    len = strlen(text);
    if (len == SIZE_MAX) { /* len + 1 would wrap to 0 */
        return 0;
    }

    copy = malloc(len + 1);
    if (copy == NULL) {
        return 0;
    }
    strcpy(copy, text); /* copies the '\0' too; buffer is exactly len + 1 */

    msg->text = copy;
    msg->id = id;
    return 1;
}

int store_reserve(Message **messages, size_t *capacity, size_t required)
{
    size_t new_capacity;
    size_t i;
    Message *tmp;

    if (messages == NULL || capacity == NULL) {
        return 0;
    }

    /* Enough room already: change nothing. */
    if (*capacity >= required) {
        return 1;
    }

    /* Geometric growth: start small, then double until it fits. */
    new_capacity = (*capacity == 0) ? 4 : *capacity;
    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2) { /* doubling would overflow */
            new_capacity = required;
            break;
        }
        new_capacity *= 2;
    }

    /* new_capacity * sizeof(Message) must fit in size_t. */
    if (new_capacity > SIZE_MAX / sizeof(Message)) {
        return 0;
    }

    /* Temporary pointer: if realloc fails, the caller's array is untouched. */
    tmp = realloc(*messages, new_capacity * sizeof(Message));
    if (tmp == NULL) {
        return 0;
    }

    /* realloc leaves the new slots uninitialized: make them empty messages. */
    for (i = *capacity; i < new_capacity; i++) {
        tmp[i].id = 0;
        tmp[i].text = NULL;
    }

    *messages = tmp;
    *capacity = new_capacity;
    return 1;
}

int store_append(MessageStore *store, int id, const char *text)
{
    if (store == NULL || id <= 0 || text == NULL) {
        return 0;
    }

    if (store->count == SIZE_MAX) { /* count + 1 would wrap to 0 */
        return 0;
    }

    if (!store_reserve(&store->messages, &store->capacity, store->count + 1)) {
        return 0;
    }

    /* On failure message_init leaves the slot empty, and count stays the same. */
    if (!message_init(&store->messages[store->count], id, text)) {
        return 0;
    }

    store->count++;
    return 1;
}

void message_free(Message *msg)
{
    if (msg == NULL) {
        return;
    }

    free(msg->text); /* free(NULL) does nothing, so an empty message is safe */
    msg->text = NULL; /* no dangling pointer, and a second call is safe */
    msg->id = 0;
}

void store_free(MessageStore *store)
{
    size_t i;

    if (store == NULL) {
        return;
    }

    /* Strings first: after free(messages) we could no longer reach them. */
    if (store->messages != NULL) {
        for (i = 0; i < store->count; i++) {
            message_free(&store->messages[i]);
        }
    }

    free(store->messages);
    store->messages = NULL;
    store->count = 0;
    store->capacity = 0;
}

int store_clone(MessageStore *dest, const MessageStore *source)
{
    size_t i;

    if (dest == NULL || source == NULL) {
        return 0;
    }

    /* New array of empty slots; an empty source gives an empty dest. */
    if (!store_init(dest, source->count)) {
        return 0;
    }

    /* A new string for every message, never a copy of the source's pointer. */
    for (i = 0; i < source->count; i++) {
        if (!message_init(&dest->messages[i], source->messages[i].id,
                          source->messages[i].text)) {
            store_free(dest); /* frees the dest->count copies made so far */
            return 0;
        }
        dest->count++;
    }

    return 1;
}