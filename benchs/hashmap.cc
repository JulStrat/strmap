#include <algorithm>
#include <vector>
#include <iostream>
#include <chrono>
#include <cstring>

#include "strmap.h"
#include "hashmap.h"

unsigned int MAP_SIZE = 1024;

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

int main(int argc, char **argv)
{
    string str  = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    string xstr = "ZbcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
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
    //int *data;
    size_t csize;
    
    STRMAP *ht;
    struct hashmap_s hm;
   
    char *ptr;    
    MAP_SIZE = strtoul(argv[1], &ptr, 10);

    auto t1 = Clock::now();      
    for (int i=0; i<MAP_SIZE; i++) {
        //random_shuffle(str.begin(), str.end());
        fisher_yates_shuffle((char *)str.c_str());
        keys.push_back(str);
    }
    auto t2 = Clock::now();    
    elapsed = t2 - t1;
    cout << "Generate keys: " << elapsed.count() << '\n';
    
    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
        //random_shuffle(xstr.begin(), xstr.end());
        fisher_yates_shuffle((char *)xstr.c_str());
        xkeys.push_back(xstr);
    }
    t2 = Clock::now();    
    elapsed = t2 - t1;
    cout << "Generate not existing keys: " << elapsed.count() << '\n';

    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
        poly_hashs(keys[i].c_str());
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Hashing perf: " << elapsed.count() << '\n';

    cout << "*******************\n";
    cout << "*** strmap test ***\n";
    cout << "*******************\n";    

    ht = sm_create(MAP_SIZE);
    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
        if (sm_insert(ht, keys[i].c_str(), &val, &rentry) != SM_INSERTED) {
            cout << "Error: " << keys[i].c_str() << '\n';
            //break;
        }
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Insert: " << elapsed.count() << '\n';
    cout << "Size: " << sm_size(ht) << '\n';
    
    cout << "Mean: " << sm_probes_mean(ht) << '\n';
    cout << "Variance: " << sm_probes_var(ht) << '\n';    

    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
        if (sm_lookup(ht, keys[i].c_str(), &rentry)  != SM_FOUND) {
            cout << "Error: " << keys[i] << '\n';
            //break;
        }
        if (*(int *)rentry.data != 1551) {
            cout << "Error: " << keys[i] << " Data:" << *(int *)rentry.data << '\n';
            //break;
        }
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Lookup existing: " << elapsed.count() << '\n';

    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
        if (sm_lookup(ht, xkeys[i].c_str(), &rentry) != SM_NOT_FOUND) {
            cout << "Error: " << xkeys[i] << '\n';  ;
            //break;
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
    for (int i=0; i<MAP_SIZE; i++) {
        if (sm_remove(ht, keys[i].c_str(), &rentry) != SM_REMOVED) {
            cout << keys[i]; 
            //break;            
        }
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Remove: " << elapsed.count() << '\n';

    sm_free(ht);
    
    cout << "**********************\n";    
    cout << "*** hashmap.h test ***\n";
    cout << "**********************\n";
    
    if (hashmap_create(MAP_SIZE, &hm)) {
        exit(-1);
    }

    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
      if (hashmap_put(&hm, keys[i].c_str(), str.length(), &val)) {
          cout << "hashmap_put error: " << i << " - " << keys[i].c_str() << '\n';
          //break;
      }
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Insert hashmap.h: " << elapsed.count() << '\n';
    cout << "Size: " << hashmap_num_entries(&hm) << '\n';
    
    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
      if (!hashmap_get(&hm, keys[i].c_str(), str.length())) {
         cout << "hashmap_get not found: " << i << " - " << keys[i].c_str() << '\n'; 
      }
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Lookup hashmap.h: " << elapsed.count() << '\n';

    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
      if (hashmap_get(&hm, xkeys[i].c_str(), xstr.length())) {
        cout << "hashmap_get not existing found: " << i << " - " << xkeys[i].c_str() << '\n';           
      }
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Lookup not existing hashmap.h: " << elapsed.count() << '\n';
    
    t1 = Clock::now();
    for (int i=0; i<MAP_SIZE; i++) {
      if (hashmap_remove(&hm, keys[i].c_str(), str.length())) {
        cout << "hashmap_remove not found: " << i << " - " << keys[i].c_str() << '\n';                     
      }
    }
    t2 = Clock::now();
    elapsed = t2 - t1;
    cout << "Remove: " << elapsed.count() << '\n';
    
    cout << "Size: " << hashmap_num_entries(&hm) << '\n';
    
    hashmap_destroy(&hm);
}
