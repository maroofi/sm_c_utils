#include <cdict.h>
#include <cunittest.h>
#include <stdlib.h>

void dict_free_func(void * value){
    free(value);
}

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
        // before insertion check if the key exists
        //if (cdict_has_key(new_dict, key)){
        //    fprintf(stdout, "We already have this key: %s\n", key);
        //    continue;
        //}
        cdict_set(new_dict, key, (void *)"John Doe");
    }
    cdict_keylist * klst = NULL;
    klst = cdict_keys(new_dict, 0);
    fprintf(stdout, "We have %d keys\n", klst->len);
    //for (unsigned long int i=0; i< klst->len; ++i){
    //    //fprintf(stdout, "%s -> %s\n", klst->lst[i], (char*)cdict_get(new_dict, klst->lst[i]));
    //}
    cdict_free_keylist(klst, 0);
    cdict_free(new_dict);
    return 0;
}
