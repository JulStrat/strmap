[![GitHub license](https://img.shields.io/github/license/JulStrat/strmap)](https://github.com/JulStrat/strmap/blob/strmap/LICENSE)

# strmap

`strmap` - simple alternative to `hcreate_r`, `hdestroy_r`, `hsearch_r` GNU extensions.

Benchmark data:
- [English words](https://github.com/dwyl/english-words/blob/master/words.txt) - 466550 keys 
- ASCII letters permutations - 3700000 keys

## API

``` C
    STRMAP *sm_create(size_t size);
```
Create a string map which can contain at least `size` elements.
___
   
``` C
    STRMAP *sm_create_from(const STRMAP * sm, size_t size);
```
Create `strmap` from existing.
___
``` C
    SM_RESULT sm_lookup(const STRMAP * sm, const char *key,
                        SM_ENTRY * item);
```
Retrieves user associated data for given key.
___
``` C
    SM_RESULT sm_insert(STRMAP * sm, const char *key, const void *data,
                        SM_ENTRY * item);
```
Insert key and user data.
___
``` C
    SM_RESULT sm_update(STRMAP * sm, const char *key, const void *data,
                        SM_ENTRY * item);
```
Update user data for given key.
___
``` C
    SM_RESULT sm_upsert(STRMAP * sm, const char *key, const void *data,
                        SM_ENTRY * item);
```
Update user data for given key or insert if key not exists.
___
``` C
    SM_RESULT sm_remove(STRMAP * sm, const char *key, SM_ENTRY * item);
```
Remove key

Based on 
M. A. Kolosovskiy, ["Simple implementation of deletion from open-address hash table"](https://arxiv.org/ftp/arxiv/papers/0909/0909.2547.pdf).
___
``` C
    void sm_foreach(const STRMAP * sm, void (*action) (SM_ENTRY item, void *ctx), void *ctx);
```    
For each callback.
___
``` C
    void sm_clear(STRMAP * sm);
```
Remove all keys.
___
``` C
    size_t sm_size(const STRMAP * sm);
```    
Return number of keys.
___
``` C
    double sm_probes_mean(const STRMAP * sm);
    double sm_probes_var(const STRMAP * sm);
```
Probes mean, variance.
___
``` C
    void sm_free(STRMAP * sm);
```    
Remove all keys and free memory allocated for the map structure.
___
``` C
    size_t poly_hashs(const char *key);
```    
String hash.
