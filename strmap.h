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
  @file strmap.h
  @brief STRMAP - simple alternative to hcreate_r, hdestroy_r, hsearch_r GNU extensions
  @author I. Kakoulidis  
  @date 2021
  @license The Unlicense  
*/

#ifndef _STRMAP_H
#define _STRMAP_H

#include <stddef.h> /* size_t */

#define STRMAP_VERSION_MAJOR 0
#define STRMAP_VERSION_MINOR 5
#define STRMAP_VERSION_PATCH 1

typedef struct STRMAP STRMAP;

typedef struct SM_ENTRY {
    const char *key;            /* C null terminated string */
    const void *data;           /* user data */
    size_t hash;                /* key hash value */
} SM_ENTRY;

typedef enum SM_RESULT {
    SM_NOT_FOUND = -1,
    SM_DUPLICATE = -2,
    SM_MAP_FULL = -3,

    SM_FOUND = 1,
    SM_INSERTED = 2,
    SM_UPDATED = 3,
    SM_REMOVED = 4
} SM_RESULT;

#ifdef __cplusplus
extern "C" {
#endif

/**
  @brief Create a string map which can contain at least `size` elements
*/
    STRMAP *sm_create(size_t size);

    STRMAP *sm_create_from(const STRMAP * sm, size_t size);

/**
  @brief Retrieves user associated data for given key
  @return SM_FOUND on success, SM_NOT_FOUND otherwise
*/
    SM_RESULT sm_lookup(const STRMAP * sm, const char *key,
                        SM_ENTRY * item);

/**
  @brief Insert key (if not exists) and user data
  @return SM_INSERTED on success, SM_DUPLICATE or SM_MAP_FULL otherwise  
*/
    SM_RESULT sm_insert(STRMAP * sm, const char *key, const void *data,
                        SM_ENTRY * item);

/**
  @brief Update user data for given key
  @return SM_UPDATED on success, SM_NOT_FOUND otherwise    
*/
    SM_RESULT sm_update(STRMAP * sm, const char *key, const void *data,
                        SM_ENTRY * item);

/**
  @brief Update user data for given key or insert if key not exists
  @return SM_UPDATED or SM_INSERTED on success, SM_MAP_FULL otherwise    
*/
    SM_RESULT sm_upsert(STRMAP * sm, const char *key, const void *data,
                        SM_ENTRY * item);

/**
  @brief Remove key

  Based on 
  M. A. Kolosovskiy, "Simple implementation of deletion from open-address hash table"
  https://arxiv.org/ftp/arxiv/papers/0909/0909.2547.pdf
  @return SM_DELETED on success, SM_NOT_FOUND otherwise    
*/
    SM_RESULT sm_remove(STRMAP * sm, const char *key, SM_ENTRY * item);

/**
  @brief For each callback
*/
    void sm_foreach(const STRMAP * sm, void (*action) (SM_ENTRY item, void *ctx), void *ctx);

/**
  @brief Remove all keys
*/
    void sm_clear(STRMAP * sm);

/**
  @brief Return number of keys
*/
    size_t sm_size(const STRMAP * sm);

    double sm_probes_mean(const STRMAP * sm);
    double sm_probes_var(const STRMAP * sm);
    double sm_load_factor(const STRMAP * sm);

/**
  @brief Remove all keys and free memory allocated for the map structure
*/
    void sm_free(STRMAP * sm);

    size_t poly_hashs(const char *key);

#ifdef __cplusplus
}
#endif
#endif
