#include <cdict.h>
#include <cunittest.h>
#include <stdlib.h>

// we don't use this function because
// we can just simply use free() function
// from stdlib as the 'value' is a null-terminated
// string
void dict_free_func(void * value){
    free(value);
}

// we can just simply use strdup() form string.h
// header since the 'values' are null-terminated string.
void* dict_copy_func(void * value){
    return (void*) strdup((char*)value);
}


int main(int argc, char ** argv){
    srand(1);
    ABORT_ON_FAIL(1);
    cdict_ctx* new_dict = NULL;
    int COUNT = 500000;
    new_dict = cdict_init(free, (void*)strdup);
    // add random key, values to the dictionary
    int random_val = 0;
    char key[50] = {0};
    for (int i=0; i< COUNT; ++i){
        random_val = rand();
        sprintf(key, "key_%d", random_val);
        // befor;e insertion check if the key exists
        cdict_set(new_dict, key, (void *)"John Doe");
        memset(key, 0, 50);
    }
    // I want to remove all the key-value pairs one by one
    cdict_keylist * klst = NULL;
    // passing 1 to cdict_keys will copy the keys
    klst = cdict_keys(new_dict, 1);
    fprintf(stdout, "We have %d keys\n", klst->len);
    for (unsigned int i=0; i<klst->len; ++i){
        cdict_remove(new_dict, klst->lst[i]);
    }
    cdict_free_keylist(klst, 1);
    cdict_free(new_dict);
    return 0;
}
