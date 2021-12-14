#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

#include "robin_hood.h"
#include "strmap.h"

typedef std::chrono::high_resolution_clock Clock;

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

using namespace std;

// sm_foreach callback
void check_hash(SM_ENTRY item, void *ctx) {
  if (poly_hashs(item.key) != item.hash) {
    cout << "Hash error: " << item.hash << '\n';
  }
}

int main(int argc, char **argv) {
  unsigned long MAP_SIZE = 1024;
  string str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  string xstr =
      "ZbcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::chrono::duration<double> elapsed;
  SM_ENTRY rentry;

  // keys to insert
  vector<string> keys;
  // not existing keys
  vector<string> xkeys;

  // key value
  int val = 1551;
  // key value for update and upsert
  int uval = 7117;
  // pointer for lookup
  // int *data;
  // size_t csize;

  // STRMAP *nht;
  STRMAP *ht;
  char *ptr;
  
  MAP_SIZE = strtoul(argv[1], &ptr, 10);

  auto t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    // random_shuffle(str.begin(), str.end());
    fisher_yates_shuffle((char *)str.c_str());
    keys.push_back(str);
  }
  auto t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Generate keys: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    // random_shuffle(xstr.begin(), xstr.end());
    fisher_yates_shuffle((char *)xstr.c_str());
    xkeys.push_back(xstr);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Generate not existing keys: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    poly_hashs(keys[i].c_str());
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Hashing perf: " << elapsed.count() << '\n';

  cout << "*******************\n";
  cout << "*** strmap test ***\n";
  cout << "*******************\n";

#ifdef RESERVE
  ht = sm_create(MAP_SIZE);
#else  
  ht = sm_create(0);
#endif    
  if (!ht) {
      exit(-1);
  }
  
  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_insert(ht, keys[i].c_str(), &val, &rentry) != SM_INSERTED) {
      cout << "Error: " << keys[i].c_str() << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Insert: " << elapsed.count() << '\n';
  cout << "Size: " << sm_size(ht) << '\n';
  cout << "Load factor: " << sm_load_factor(ht) << '\n';  
  cout << "Mean: " << sm_probes_mean(ht) << '\n';
  cout << "Variance: " << sm_probes_var(ht) << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_lookup(ht, keys[i].c_str(), &rentry) != SM_FOUND) {
      cout << "Error: " << keys[i] << '\n';
      break;
    }
    if (*(int *)rentry.data != 1551) {
      cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_lookup(ht, xkeys[i].c_str(), &rentry) != SM_NOT_FOUND) {
      cout << "Error: " << xkeys[i] << '\n';
      ;
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup not existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  sm_foreach(ht, check_hash, 0);
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Foreach check_hash(): " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_lookup(ht, keys[i].c_str(), &rentry) != SM_FOUND) {
      cout << "Error: " << keys[i] << '\n';
      break;
    }
    if (*(int *)rentry.data != 1551) {
      cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_update(ht, keys[i].c_str(), &uval, &rentry) == SM_NOT_FOUND) {
      cout << "Error: " << keys[i].c_str() << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Update existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_lookup(ht, keys[i].c_str(), &rentry) != SM_FOUND) {
      cout << "Error: " << keys[i] << '\n';
      break;
    }
    if (*(int *)rentry.data != 7117) {
      cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup updated: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_lookup(ht, xkeys[i].c_str(), &rentry) != SM_NOT_FOUND) {
      cout << "Error: " << xkeys[i] << '\n';
      ;
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup not existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_remove(ht, keys[i].c_str(), &rentry) != SM_REMOVED) {
      cout << keys[i];
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Remove: " << elapsed.count() << '\n';
  cout << "Size: " << sm_size(ht) << '\n';  

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_upsert(ht, keys[i].c_str(), (void *)&uval, &rentry) == SM_MAP_FULL) {
      cout << "Error: " << keys[i].c_str() << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Upsert removed: " << elapsed.count() << '\n';
  cout << "Size: " << sm_size(ht) << '\n';  

  cout << "Mean: " << sm_probes_mean(ht) << '\n';
  cout << "Variance: " << sm_probes_var(ht) << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (sm_lookup(ht, keys[i].c_str(), &rentry) != SM_FOUND) {
      cout << "Error: " << keys[i] << '\n';
      break;
    }
    if (*(int *)rentry.data != 7117) {
      cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup: " << elapsed.count() << '\n';

  sm_free(ht);

  cout << "***********************\n";
  cout << "*** robin_hood test ***\n";
  cout << "***********************\n";

  robin_hood::unordered_set<string> strset;

#ifdef RESERVE  
  strset.reserve(MAP_SIZE);
#endif

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    strset.insert(keys[i]);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Insert robin_hood unordered_set: " << elapsed.count() << '\n';
  cout << "Load factor robin_hood unordered_set: " << strset.load_factor()
       << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (strset.find(keys[i]) == strset.end()) {
      cout << "Key not found!" << i << '-' << keys[i] << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup existing robin_hood unordered_set: " << elapsed.count()
       << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (strset.find(xkeys[i]) != strset.end()) {
      cout << "Not existing key found!" << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup not existing robin_hood unordered_set: " << elapsed.count()
       << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    strset.erase(keys[i]);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Remove: " << elapsed.count() << '\n';

  cout << "*************************************\n";
  cout << "*** robin_hood unordered_map test ***\n";
  cout << "*************************************\n";

  robin_hood::unordered_map<string, int> strmap;

#ifdef RESERVE    
  strmap.reserve(MAP_SIZE);
#endif  

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    strmap.insert({keys[i], 0});
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Insert robin_hood unordered_map: " << elapsed.count() << '\n';
  cout << "Load factor robin_hood unordered_map: " << strmap.load_factor()
       << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (strmap.find(keys[i]) == strmap.end()) {
      cout << "Key not found!" << i << '-' << keys[i] << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup existing robin_hood unordered_map: " << elapsed.count()
       << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    if (strmap.find(xkeys[i]) != strmap.end()) {
      cout << "Not existing key found!" << '\n';
      break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup not existing robin_hood unordered_map: " << elapsed.count()
       << '\n';

  t1 = Clock::now();
  for (unsigned long i = 0; i < MAP_SIZE; i++) {
    strmap.erase(keys[i]);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Remove: " << elapsed.count() << '\n';
}
