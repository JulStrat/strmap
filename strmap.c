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
#define LOAD_FACTOR 0.7
#define POSITION(x, r) ((x) % (r))

struct STRMAP {
    size_t capacity;            // number of allocated entries
    size_t size;                // number of keys in map
    size_t msize;               // max size
    SM_ENTRY *ht;
};

static const SM_ENTRY EMPTY = { 0, 0, 0 };

static SM_ENTRY *find(const STRMAP * sm, const char *key, size_t hash);
static void compress(STRMAP * sm, SM_ENTRY * entry);
static size_t distance(const SM_ENTRY * from, const SM_ENTRY * to, size_t range);
static size_t adjust(size_t x);

STRMAP *
sm_create(size_t size)
{
    SM_ENTRY *ht;
    STRMAP *sm;
    size_t capacity, msize;

    msize = (size >= MIN_SIZE ? size : MIN_SIZE);
    capacity = (size_t)(msize / LOAD_FACTOR);
    capacity = adjust(capacity);

    // check capcity > msize

    if (!(ht = (SM_ENTRY *) calloc(capacity, sizeof (SM_ENTRY)))) {
        errno = ENOMEM;
        return 0;
    }
    if ((sm = (STRMAP *) calloc(1, sizeof (STRMAP)))) {
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

STRMAP *
sm_create_from(const STRMAP * sm, size_t size)
{
    SM_ENTRY *item, *entry, *stop;
    STRMAP *map;

    assert(sm);

    map = sm_create(size);
    if (!map) {
        return 0;
    }

    stop = sm->ht + sm->capacity;
    for (item = sm->ht; item != stop; ++item) {
        if (item->key) {
            entry = find(map, item->key, item->hash);
            *entry = *item;
            ++(map->size);
        }
    }

    return map;
}

SM_RESULT
sm_insert(STRMAP * sm, const char *key, const void *data, SM_ENTRY * item)
{
    SM_ENTRY *entry;
    size_t hash;

    assert(sm);
    assert(key);

    if (sm->size == sm->msize) {
        return SM_MAP_FULL;
    }

    hash = poly_hashs(key);
    entry = find(sm, key, hash);
    if (!(entry->key)) {
        entry->key = key;
        entry->data = data;
        entry->hash = hash;
        if (item) {
            *item = *entry;            
        }
        ++(sm->size);

        return SM_INSERTED;
    }

    return SM_DUPLICATE;
}

SM_RESULT
sm_update(STRMAP * sm, const char *key, const void *data, SM_ENTRY * item)
{
    SM_ENTRY *entry;
    size_t hash;

    assert(sm);
    assert(key);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);
    if (entry->key) {
        if (item) {
            *item = *entry;            
        }
        entry->data = data;
        return SM_UPDATED;
    }

    return SM_NOT_FOUND;
}

SM_RESULT
sm_upsert(STRMAP * sm, const char *key, const void *data, SM_ENTRY * item)
{
    SM_ENTRY *entry;
    size_t hash;

    assert(sm);
    assert(key);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);
    if (entry->key) {
        if (item) {
            *item = *entry;            
        }
        entry->data = data;
        return SM_UPDATED;
    }
    if (sm->size == sm->msize) {
        return SM_MAP_FULL;
    }
    entry->key = key;
    entry->data = data;
    entry->hash = hash;
    if (item) {
        *item = *entry;            
    }
    ++(sm->size);

    return SM_UPDATED;
}

SM_RESULT
sm_lookup(const STRMAP * sm, const char *key, SM_ENTRY * item)
{
    SM_ENTRY *entry;
    size_t hash;

    assert(sm);
    assert(key);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);

    if (entry->key) {
        if (item) {
            *item = *entry;            
        }
        return SM_FOUND;
    }

    return SM_NOT_FOUND;
}

SM_RESULT
sm_remove(STRMAP * sm, const char *key, SM_ENTRY * item)
{
    SM_ENTRY *entry;
    size_t hash;

    assert(sm);
    assert(key);

    hash = poly_hashs(key);
    entry = find(sm, key, hash);

    if (entry->key) {
        if (item) {
            *item = *entry;            
        }
        *entry = EMPTY;
        --(sm->size);
        compress(sm, entry);
        return SM_REMOVED;
    }

    return SM_NOT_FOUND;
}

void
sm_foreach(const STRMAP * sm, void (*action) (SM_ENTRY item, void *ctx), void *ctx)
{
    SM_ENTRY *entry, *stop;
    assert(sm);

    stop = sm->ht + sm->capacity;
    for (entry = sm->ht; entry != stop; ++entry) {
        if (entry->key) {
            action(*entry, ctx);
        }
    }
}

double
sm_probes_mean(const STRMAP * sm)
{
    SM_ENTRY *entry, *root, *stop;
    double mean;

    assert(sm);

    if (!sm->size) {
        return 0.0;
    }

    stop = sm->ht + sm->capacity;
    for (mean = 0.0, entry = sm->ht; entry != stop; ++entry) {
        if (entry->key) {
            root = sm->ht + POSITION(entry->hash, sm->capacity);
            mean += (double)distance(root, entry, sm->capacity);
        }
    }

    return mean / (double)sm->size;
}

double
sm_probes_var(const STRMAP * sm)
{
    SM_ENTRY *entry, *root, *stop;
    double var, diff, mean;

    assert(sm);

    if (!sm->size) {
        return 0.0;
    }

    mean = sm_probes_mean(sm);
    stop = sm->ht + sm->capacity;    
    for (var = 0.0, entry = sm->ht; entry != stop; ++entry) {
        if (entry->key) {
            root = sm->ht + POSITION(entry->hash, sm->capacity);
            diff = mean - (double)distance(root, entry, sm->capacity);
            var += diff * diff;
        }
    }

    return var / (double)sm->size;
}

void
sm_clear(STRMAP * sm)
{
    SM_ENTRY *entry, *stop;
    assert(sm);

    stop = sm->ht + sm->capacity;
    for (entry = sm->ht; entry != stop; ++entry) {
        *entry = EMPTY;
    }
    sm->size = 0;
}

size_t
sm_size(const STRMAP * sm)
{
    assert(sm);

    return sm->size;
}

void
sm_free(STRMAP * sm)
{
    assert(sm);

    free(sm->ht);
    free(sm);
}

/*
 * private static functions
 */

static size_t
distance(const SM_ENTRY * from, const SM_ENTRY * to, size_t range)
{
    return to >= from ? to - from : range - (from - to);
}

static size_t
adjust(size_t x) 
{
    static const int WHEEL[30] = {
        1, 0, 5, 4, 3, 2,
        1, 0, 3, 2, 1, 0,
        1, 0, 3, 2, 1, 0,
        1, 0, 3, 2, 1, 0,
        5, 4, 3, 2, 1, 0
    };
    x += WHEEL[x % 30];
    assert((x % 2) && (x % 3) && (x % 5));

    return x;
}

/*
 * find entry with given key and hash in collision chain or return first
 * empty
 */
SM_ENTRY *
find(const STRMAP * sm, const char *key, size_t hash)
{
    SM_ENTRY *entry, *stop;

    entry = sm->ht + POSITION(hash, sm->capacity);
    stop = sm->ht + sm->capacity;

    while (entry->key) {
        if (hash == entry->hash) {
            if (!strcmp(key, entry->key)) {
                return entry;
            }
        }

        if (++entry == stop) {
            entry = sm->ht;
        }
    }

    return entry;
}

void
compress(STRMAP * sm, SM_ENTRY * entry)
{
    SM_ENTRY *empty, *root, *stop;

    empty = entry;
    stop = sm->ht + sm->capacity;
    if (++entry == stop) {
        entry = sm->ht;
    }

    while (entry->key) {
        root = sm->ht + POSITION(entry->hash, sm->capacity);
        if (distance(root, entry, sm->capacity) >=
            distance(empty, entry, sm->capacity)) {
            // swap current entry with empty
            *empty = *entry;
            *entry = EMPTY;
            empty = entry;
        }
        if (++entry == stop) {
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

#undef MIN_SIZE
#undef LOAD_FACTOR
#undef POSITION
