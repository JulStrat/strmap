#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

//#define USE_FAST_REDUCE 1
//#include <cassert>
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
  string str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  string xstr = "ZbcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  chrono::duration<double> elapsed;
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
  size_t csize;

  STRMAP *nht;
  STRMAP *ht;

  auto t1 = Clock::now();
  // Load words
  // ifstream fwords("tests/slice-03");
  ifstream fwords(argv[1]);
  string word;
  while (getline(fwords, word)) {
    // cout << word << "\n";
    keys.push_back(word);
  }
  auto t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Load " << keys.size() << " words: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    // random_shuffle(xstr.begin(), xstr.end());
    fisher_yates_shuffle((char *)xstr.c_str());
    xkeys.push_back(xstr);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Generate not existing keys: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    poly_hashs(keys[i].c_str());
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Hashing perf: " << elapsed.count() << '\n';

  cout << "*************************\n";
  cout << "*** strmap words test ***\n";
  cout << "*************************\n";

  ht = sm_create(keys.size());

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_insert(ht, keys[i].c_str(), &val, &rentry) != SM_INSERTED) {
      ;
      // cout << "Error: " << keys[i].c_str() << '\n';
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Insert " << sm_size(ht) << " keys: " << elapsed.count() << '\n';

  cout << "Mean: " << sm_probes_mean(ht) << '\n';
  cout << "Variance: " << sm_probes_var(ht) << '\n';

  t1 = Clock::now();
  sm_foreach(ht, check_hash, 0);
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Foreach check_hash(): " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_lookup(ht, keys[i].c_str(), &rentry) != SM_FOUND) {
      cout << "Error: " << keys[i] << '\n';
      // break;
    }
    if (*(int *)rentry.data != 1551) {
      cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_update(ht, keys[i].c_str(), &uval, &rentry) == SM_NOT_FOUND) {
      cout << "Error: " << keys[i].c_str() << '\n';
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Update existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_lookup(ht, keys[i].c_str(), &rentry) != SM_FOUND) {
      cout << "Error: " << keys[i] << '\n';
      // break;
    }
    if (*(int *)rentry.data != 7117) {
      cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup updated: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_lookup(ht, xkeys[i].c_str(), &rentry) != SM_NOT_FOUND) {
      cout << "Error: " << xkeys[i] << '\n';
      ;
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup not existing: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_remove(ht, keys[i].c_str(), &rentry) != SM_REMOVED) {
      ;
      // cout << keys[i];
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Remove: " << elapsed.count() << '\n';

  cout << "Mean: " << sm_probes_mean(ht) << '\n';
  cout << "Variance: " << sm_probes_var(ht) << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_upsert(ht, keys[i].c_str(), (void *)&uval, &rentry) == SM_MAP_FULL) {
      cout << "Error: " << keys[i].c_str() << '\n';
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Upsert removed: " << elapsed.count() << '\n';

  cout << "Mean: " << sm_probes_mean(ht) << '\n';
  cout << "Variance: " << sm_probes_var(ht) << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    if (sm_lookup(ht, keys[i].c_str(), &rentry) != SM_FOUND) {
      cout << "Error: " << keys[i] << '\n';
      // break;
    }
    if (*(int *)rentry.data != 7117) {
      cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
      // break;
    }
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup: " << elapsed.count() << '\n';

  sm_free(ht);

  cout << "******************************\n";
  cout << "*** STL unordered_map test ***\n";
  cout << "******************************\n";

  unordered_map<string, int> strmap;
  strmap.reserve(keys.size());

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    strmap.insert(make_pair(keys[i], 0));
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Insert STL unordered_map: " << elapsed.count() << '\n';
  cout << "Load factor STL unordered_map: " << strmap.load_factor() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    strmap.find(keys[i]);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup existing STL unordered_map: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    strmap.find(xkeys[i]);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Lookup not existing STL unordered_map: " << elapsed.count() << '\n';

  t1 = Clock::now();
  for (size_t i = 0; i < keys.size(); i++) {
    strmap.erase(keys[i]);
  }
  t2 = Clock::now();
  elapsed = t2 - t1;
  cout << "Remove: " << elapsed.count() << '\n';
}
