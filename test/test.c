#include <stdio.h>
#include <stdlib.h>

#include "greatest.h"
#include "strmap.h"

void fisher_yates_shuffle(char *s) {
  size_t i, j, n = strlen(s);
  char tmp;

  for (i = n - 1; i > 0; --i) {
    j = rand() % (i + 1);
    tmp = s[j];
    s[j] = s[i];
    s[i] = tmp;
  }
}

/* sm_foreach callback */
void check_hash(SM_ENTRY item, void *ctx) {
  if (poly_hashs(item.key) != item.hash) {
    printf("Hash error: %s\n", item.key);
  }
}

char *str_dup(const char *src) {
    size_t len = strlen(src) + 1;
    
    char *dst = malloc(len);
    if (!dst) {
        return 0;
    }
    memcpy (dst, src, len);
    
    return dst;
}

unsigned long MAP_SIZE = 1024;

/* keys to insert */
char **keys;

/* not existing keys */
char **xkeys;

TEST
INSERT_1() {
  STRMAP *ht;
  unsigned long i;  

#ifdef RESERVE
  ht = sm_create(MAP_SIZE);
#else  
  ht = sm_create(0);
#endif    
  if (!ht) {
      FAIL();
  }

  for (i = 0; i < MAP_SIZE; i++) {
    ASSERT(sm_insert(ht, keys[i], 0, 0) == SM_INSERTED);
    ASSERT(sm_size(ht) == i + 1);
  }

  sm_free(ht);
  PASS();
}    

TEST
INSERT_2() {
  STRMAP *ht;
  unsigned long i;  

#ifdef RESERVE
  ht = sm_create(MAP_SIZE);
#else  
  ht = sm_create(0);
#endif    
  if (!ht) {
      FAIL();
  }

  for (i = 0; i < MAP_SIZE; i++) {
    ASSERT(sm_insert(ht, keys[i], 0, 0) == SM_INSERTED);
    ASSERT(sm_size(ht) == i + 1);
    ASSERT(sm_lookup(ht, keys[i], 0) == SM_FOUND);
  }

  sm_free(ht);
  PASS();
}    

GREATEST_MAIN_DEFS();
int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();  
  
  char str[]  = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  char xstr[] = "ZbcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  
  unsigned long i;  
  char *ptr;  

  if (argc > 1) {
    MAP_SIZE = strtoul(argv[1], &ptr, 10);
  }

  keys = calloc(MAP_SIZE, sizeof (char *));
  xkeys = calloc(MAP_SIZE, sizeof (char *));
  
  for (i = 0; i < MAP_SIZE; i++) {
    fisher_yates_shuffle(str);
    keys[i] = str_dup(str);
  }
  
  for (i = 0; i < MAP_SIZE; i++) {
    fisher_yates_shuffle(xstr);
    xkeys[i] = str_dup(xstr);
  }

  RUN_TEST(INSERT_1);
  RUN_TEST(INSERT_2);
  
  free(keys);
  free(xkeys);
  
  GREATEST_MAIN_END();
}
