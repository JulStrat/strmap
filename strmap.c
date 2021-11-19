/*
 * This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * For more information, please refer to <https://unlicense.org> 
 */

/**
  @file strmap.c
  @brief STRMAP - simple alternative to hcreate_r, hdestroy_r, hsearch_r GNU extensions
  @author I. Kakoulidis    
  @date 2021
  @license Public Domain
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "strmap.h"

#define MIN_SIZE 6
#define POSITION(x, r) ((x) % (r))

struct STRMAP {
    size_t capacity;            // number of allocated entries
    size_t size;                // number of keys in map
    size_t msize;               // max size
    SM_ENTRY *ht;
};

static SM_ENTRY EMPTY = { 0, 0, 0 };

static size_t
DISTANCE(SM_ENTRY * from, SM_ENTRY * to, size_t range)
{
    return to >= from ? to - from : range - (from - to);
}

static void compress(strmap * sm, SM_ENTRY * SM_ENTRY);

static SM_ENTRY *find(const strmap * sm, const char *key, size_t hash);

strmap *
sm_create(size_t size)
{
    SM_ENTRY *ht;

    struct STRMAP *sm;

    size_t capacity, msize;

    msize = (size >= MIN_SIZE ? size : MIN_SIZE);
    capacity = msize + (msize / 3);
    // check capcity > msize

    if (!(ht = (SM_ENTRY *) calloc(capacity, sizeof (SM_ENTRY)))) {
        errno = ENOMEM;
        return 0;
    }
    if ((sm = (struct STRMAP *) calloc(1, sizeof (struct STRMAP)))) {
        sm->size = 0;
        sm->msize = msize;
        sm->capacity = capacity;
        sm->ht = ht;
    } else {
        free(ht);
        errno = ENOMEM;
    }

    return sm;
}

strmap *
sm_create_from(const strmap * sm, size_t size)
{
    SM_ENTRY *item, *entry;

    strmap *map;

    assert(sm);

    map = sm_create(size);
    if (!map) {
        return 0;
    }

    for (item = sm->ht; item != sm->ht + sm->capacity; ++item) {
        if (item->key) {
            entry = find(map, item->key, item->hash);
            *entry = *item;
            ++(map->size);
        }
    }

    return map;
}

SM_RESULT
sm_insert(strmap * sm, const char *key, const void *data, SM_ENTRY * item)
{
    SM_ENTRY *entry;

    size_t hash;

    assert(sm);
    assert(key);
    assert(item);

    if (sm->size == sm->msize) {
        return SM_MAP_FULL;
    }

    hash = poly_hashs(key);
    entry = find(sm, key, hash);
    if (!(entry->key)) {
        entry->key = key;
        entry->data = data;
        entry->hash = hash;
        *item = *entry;
        ++(sm->size);

        return SM_INSERTED;
    }

    return SM_DUPLICATE;
}

SM_RESULT
sm_update(strmap * sm, const char *key, const void *data, SM_ENTRY * item)
{
    SM_ENTRY *entry;

    size_t hash;

    assert(sm);
    assert(key);
    assert(item);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);
    if (entry->key) {
        *item = *entry;
        entry->data = data;
        return SM_UPDATED;
    }

    return SM_NOT_FOUND;
}

SM_RESULT
sm_upsert(strmap * sm, const char *key, const void *data, SM_ENTRY * item)
{
    SM_ENTRY *entry;

    size_t hash;

    assert(sm);
    assert(key);
    assert(item);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);
    if (entry->key) {
        *item = *entry;
        entry->data = data;
        return SM_UPDATED;
    }
    if (sm->size == sm->msize) {
        return SM_MAP_FULL;
    }
    entry->key = key;
    entry->data = data;
    entry->hash = hash;
    *item = *entry;
    ++(sm->size);

    return SM_UPDATED;
}

SM_RESULT
sm_lookup(const strmap * sm, const char *key, SM_ENTRY * item)
{
    SM_ENTRY *entry;

    size_t hash;

    assert(sm);
    assert(key);
    assert(item);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);

    if (entry->key) {
        *item = *entry;
        return SM_FOUND;
    }

    return SM_NOT_FOUND;
}

SM_RESULT
sm_remove(strmap * sm, const char *key, SM_ENTRY * item)
{
    SM_ENTRY *entry;

    size_t hash;

    assert(sm);
    assert(key);
    assert(item);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);

    if (entry->key) {
        *item = *entry;
        *entry = EMPTY;
        --(sm->size);
        compress(sm, entry);
        return SM_REMOVED;
    }

    return SM_NOT_FOUND;
}

void
sm_foreach(const strmap * sm, void (*action) (SM_ENTRY item))
{
    SM_ENTRY *item;

    assert(sm);

    for (item = sm->ht; item != sm->ht + sm->capacity; ++item) {
        if (item->key) {
            action(*item);
        }
    }
}

double
sm_probes_mean(const strmap * sm)
{
    SM_ENTRY *item, *root;

    double mean;

    assert(sm);

    if (!sm->size) {
        return 0.0;
    }

    for (mean = 0.0, item = sm->ht; item != sm->ht + sm->capacity; ++item) {
        if (item->key) {
            root = sm->ht + POSITION(item->hash, sm->capacity);
            mean += DISTANCE(root, item, sm->capacity);
        }
    }

    return mean / sm->size;
}

double
sm_probes_var(const strmap * sm)
{
    SM_ENTRY *item, *root;

    double var, diff, mean;

    assert(sm);

    if (!sm->size) {
        return 0.0;
    }

    mean = sm_probes_mean(sm);
    for (var = 0.0, item = sm->ht; item != sm->ht + sm->capacity; ++item) {
        if (item->key) {
            root = sm->ht + POSITION(item->hash, sm->capacity);
            diff = mean - (double) DISTANCE(root, item, sm->capacity);
            var += diff * diff;
        }
    }

    return var / sm->size;
}

void
sm_clear(strmap * sm)
{
    SM_ENTRY *item;

    assert(sm);

    for (item = sm->ht; item != sm->ht + sm->capacity; ++item) {
        *item = EMPTY;
    }
    sm->size = 0;
}

size_t
sm_size(const strmap * sm)
{
    assert(sm);

    return sm->size;
}

void
sm_free(strmap * sm)
{
    assert(sm);

    free(sm->ht);
    free(sm);
}

/*
 * private static functions 
 */

/*
 * find entry with given key and hash in collision chain or return first
 * empty 
 */
SM_ENTRY *
find(const strmap * sm, const char *key, size_t hash)
{
    SM_ENTRY *entry, *htEnd;

    entry = sm->ht + POSITION(hash, sm->capacity);
    htEnd = sm->ht + sm->capacity;

    while (entry->key) {
        if (hash == entry->hash) {
            if (!strcmp(key, entry->key)) {
                return entry;
            }
        }

        if (++entry == htEnd) {
            entry = sm->ht;
        }
    }

    return entry;
}

void
compress(strmap * sm, SM_ENTRY * entry)
{
    SM_ENTRY *empty = entry, *htEnd;

    SM_ENTRY *root;

    htEnd = sm->ht + sm->capacity;
    if (++entry == htEnd) {
        entry = sm->ht;
    }

    while (entry->key) {
        root = sm->ht + POSITION(entry->hash, sm->capacity);
        if (DISTANCE(root, entry, sm->capacity) >=
            DISTANCE(empty, entry, sm->capacity)) {
            // swap current entry with empty
            *empty = *entry;
            *entry = EMPTY;
            empty = entry;
        }
        if (++entry == htEnd) {
            entry = sm->ht;
        }
    }
}

size_t
poly_hashs(const char *key)
{
    size_t hash = 0;

    while (*key) {
        hash += (hash << 8) + (unsigned char) (*key);
        ++key;
    }

    return hash;
}
