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
  @date 2022
  @license The Unlicense
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "strmap.h"

static const size_t MIN_SIZE = 6;
static const size_t MAX_SIZE = (~((size_t)0)) >> 1;
static const double LOAD_FACTOR = 0.75;
static const double GROW_FACTOR = 1.5;

struct STRMAP {
    size_t capacity;            /* number of allocated entries */
    size_t size;                /* number of keys in map */
    size_t msize;               /* max size */
    size_t (*hash_func)(const char *key);
    SM_ENTRY *ht;
};

static const SM_ENTRY EMPTY = { 0, 0, 0 };

static size_t DISTANCE(const SM_ENTRY * from, const SM_ENTRY * to, size_t capacity);
static size_t HOME(size_t x, size_t capacity);

STRMAP *grow(STRMAP * sm);
static size_t adjust(size_t x);

/* Robin Hood helpers */
SM_RESULT rh_find(const STRMAP * sm, const char *key,
                  size_t hash, SM_ENTRY **item);

SM_ENTRY *rh_insert(const STRMAP * sm, SM_ENTRY item, SM_ENTRY *entry);

/*
TO DO - check dist 0
*/
static void rh_shift(STRMAP * sm, SM_ENTRY * entry);


STRMAP *
sm_create(size_t size, size_t (*hash_func)(const char *key))
{
    SM_ENTRY *ht;
    STRMAP *sm;
    size_t capacity, msize;

    msize = (size < MIN_SIZE ? MIN_SIZE : size);
    msize = (msize > MAX_SIZE ? MAX_SIZE : msize);
    capacity = (size_t)(msize / LOAD_FACTOR);

    capacity = adjust(capacity);


    assert(msize < capacity);

    if (!(ht = (SM_ENTRY *) calloc(capacity, sizeof (SM_ENTRY)))) {
        errno = ENOMEM;
        return 0;
    }
    if ((sm = (STRMAP *) calloc(1, sizeof (STRMAP)))) {
        sm->size = 0;
        sm->msize = msize;
        sm->capacity = capacity;
        if (hash_func) {
            sm->hash_func = hash_func;
        }
        else {
            sm->hash_func = poly_hashs;
        }
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
    SM_RESULT r;
    STRMAP *map;

    assert(sm);
    
    size = (size < sm->size ? sm->size : size);
    map = sm_create(size, sm->hash_func);
    if (!map) {
        return 0;
    }

    stop = sm->ht + sm->capacity;
    for (item = sm->ht; item != stop; ++item) {
        if (item->key) {
            r = rh_find(map, item->key, item->hash, &entry);
            entry = rh_insert(map, *item, entry);
            ++(map->size);
        }
    }
    assert(map->size == sm->size);
    return map;
}

SM_RESULT
sm_insert(STRMAP * sm, const char *key, const void *data, SM_ENTRY * item)
{
    SM_ENTRY *entry, tmp;
    SM_RESULT r;
    size_t hash;

    assert(sm);
    assert(key);

    hash = (sm->hash_func)(key);
    r = rh_find(sm, key, hash, &entry);

    if (r == SM_NOT_FOUND) {
        if (sm->size == sm->msize) {
            if (grow(sm)) {
                r = rh_find(sm, key, hash, &entry);
            }
            else {
                return SM_MAP_FULL;
            }
        }
        tmp.key = key;
        tmp.data = data;
        tmp.hash = hash;
        entry = rh_insert(sm, tmp, entry);
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
    SM_RESULT r;
    size_t hash;

    assert(sm);
    assert(key);

    hash = (sm->hash_func)(key);
    r = rh_find(sm, key, hash, &entry);

    if (r == SM_FOUND) {
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
    SM_ENTRY *entry, tmp;
    SM_RESULT r;    
    size_t hash;

    assert(sm);
    assert(key);

    hash = (sm->hash_func)(key);
    r = rh_find(sm, key, hash, &entry); 
    
    if (r == SM_FOUND) {
        if (item) {
            *item = *entry;            
        }
        entry->data = data;
        return SM_UPDATED;
    }
    if (sm->size == sm->msize) {
        if (grow(sm)) {
            r = rh_find(sm, key, hash, &entry);
        }
        else {
            return SM_MAP_FULL;
        }
    }

    tmp.key = key;
    tmp.data = data;
    tmp.hash = hash;
    entry = rh_insert(sm, tmp, entry);

    if (item) {
        *item = *entry;            
    }
    ++(sm->size);

    return SM_INSERTED;
}

SM_RESULT
sm_lookup(const STRMAP * sm, const char *key, SM_ENTRY * item)
{
    SM_ENTRY *entry;
    SM_RESULT r;
    size_t hash;

    assert(sm);
    assert(key);

    hash = (sm->hash_func)(key);
    r = rh_find(sm, key, hash, &entry);

    if (r == SM_FOUND) {
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
    SM_RESULT r;
    size_t hash;

    assert(sm);
    assert(key);

    hash = (sm->hash_func)(key);
    r = rh_find(sm, key, hash, &entry);

    if (r == SM_FOUND) {
        if (item) {
            *item = *entry;            
        }
        *entry = EMPTY;
        --(sm->size);
        rh_shift(sm, entry);
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
sm_probes_mean(const STRMAP * sm, size_t *max_probes)
{
    SM_ENTRY *entry, *root, *stop;
    size_t dist, max_dist;
    double mean;

    assert(sm);

    if (max_probes) {
        *max_probes = 0;
    }

    if (!sm->size) {
        return 0.0;
    }
    max_dist = 0;
    stop = sm->ht + sm->capacity;
    for (mean = 0.0, entry = sm->ht; entry != stop; ++entry) {
        if (entry->key) {
            root = sm->ht + HOME(entry->hash, sm->capacity);
            dist = DISTANCE(root, entry, sm->capacity);
            if (dist > max_dist) {
                max_dist = dist;
            }
            mean += (double)dist;
        }
    }
    
    if (max_probes) {
        *max_probes = max_dist;
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

    mean = sm_probes_mean(sm, 0);
    stop = sm->ht + sm->capacity;    
    for (var = 0.0, entry = sm->ht; entry != stop; ++entry) {
        if (entry->key) {
            root = sm->ht + HOME(entry->hash, sm->capacity);
            diff = mean - (double)DISTANCE(root, entry, sm->capacity);
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

double
sm_load_factor(const STRMAP * sm)
{
    assert(sm);

    return (double)sm->size / sm->capacity;
}

void
sm_free(STRMAP * sm)
{
    assert(sm);

    free(sm->ht);
    free(sm);
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

size_t 
djb_hashs(const char *key)
{
    size_t h = 5381;

    while (*key) {
        h = ((h << 5) + h) ^ (unsigned char) (*key);
        ++key;
    }
    
    return h;
}

/*
 * private static functions
 */

static size_t
DISTANCE(const SM_ENTRY * from, const SM_ENTRY * to, size_t capacity)
{
    return to >= from ? to - from : capacity - (from - to);
}

static size_t 
HOME(size_t x, size_t capacity) {
    return x % capacity;
}

static size_t
adjust(size_t x) 
{
    static const size_t WHEEL[64] = {
        1, 11, 13, 17, 19, 23, 29, 31, 
        37, 41, 43, 47, 53, 59, 61, 67,
        71, 73, 79, 83, 89, 97, 101, 103,
        107, 109, 113, 121, 127, 131, 137, 139,
        143, 149, 151, 157, 163, 167, 169, 173,
        179, 181, 187, 191, 193, 197, 199, 209
    };
    size_t m = x % 210;    
    int i = 0;
    
    while ((i < 64) && (m > WHEEL[i])) {
        ++i;
    }
    /*
    209 = 210 - 1
    */
    return x + (WHEEL[i] - m);
}

SM_RESULT
rh_find(const STRMAP * sm, const char *key, size_t hash, SM_ENTRY **item)
{
    SM_ENTRY *entry, *stop, *ht;
    size_t dist, entry_dist, capacity;

    ht = sm->ht;
    capacity = sm->capacity;

    entry = ht + HOME(hash, capacity);
    stop = ht + capacity;

    dist = 0;
    while (entry->key) {
        if ((hash == entry->hash) && (!strcmp(key, entry->key))) {
            *item = entry;
            return SM_FOUND;
        }

        entry_dist = DISTANCE(ht + HOME(entry->hash, capacity),
                              entry, capacity);
        if (dist <= entry_dist) {
            ++dist;
            if (++entry == stop) {
                entry = ht;
            }
        }
        else {
            break;
        }
    }

    *item = entry;
    return SM_NOT_FOUND;
}

SM_ENTRY *
rh_insert(const STRMAP * sm, SM_ENTRY item, SM_ENTRY *entry)
{
    SM_ENTRY *stop, *ht, temp;
    size_t dist, entry_dist, capacity;

    ht = sm->ht;
    capacity = sm->capacity;

    stop = ht + capacity;
    dist = DISTANCE(ht + HOME(item.hash, capacity), entry, capacity);

    while (entry->key) {
        entry_dist = DISTANCE(ht + HOME(entry->hash, capacity),
                              entry, capacity);
        if (dist > entry_dist) {
            /* swap */
            temp = *entry;
            *entry = item;
            item = temp;
            dist = entry_dist;
        }

        ++dist;
        if (++entry == stop) {
            entry = ht;
        }
    }
    
    *entry = item;
    return entry;
}

void
rh_shift(STRMAP * sm, SM_ENTRY * entry)
{
    SM_ENTRY *empty, *root, *stop;

    empty = entry;
    stop = sm->ht + sm->capacity;
    if (++entry == stop) {
        entry = sm->ht;
    }

    while (entry->key) {
        root = sm->ht + HOME(entry->hash, sm->capacity);
        if (DISTANCE(root, entry, sm->capacity) >=
            DISTANCE(empty, entry, sm->capacity)) {
            /* swap current entry with empty */
            *empty = *entry;
            *entry = EMPTY;
            empty = entry;
        }
        if (++entry == stop) {
            entry = sm->ht;
        }
    }
}

STRMAP *
grow(STRMAP * sm) {
    STRMAP *map;
    
    if (sm->size == MAX_SIZE) {
        return 0;
    }
    
    if (!(map = sm_create_from(sm, (sm->size) * GROW_FACTOR))) {
       return 0; 
    }

    free(sm->ht);
    sm->ht = map->ht;
    sm->msize = map->msize;
    sm->capacity = map->capacity;
    free(map);
    
    return sm;
}
