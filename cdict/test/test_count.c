#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cdict.h>

struct kv{
    char * key;
    unsigned int value;
};


void free_func(void * value){
    free(value);
}

void *copy_func(void * value){
    // the value is the number of occurrence (integer)
    int *new_val = (int*) malloc(sizeof(int));
    *new_val = *((int *)value);
    return new_val;
}

int compare(const void * a1, const void* a2){
    struct kv *a = (struct kv *)a1;
    struct kv *b = (struct kv *)a2;
    if (a->value > b->value)
        return -1;
    if (a->value < b->value)
        return 1;
    return 0;
}

void print_most_common(cdict_ctx * ctx){
    cdict_keylist * klst = cdict_keys(ctx, 0);
    struct kv * kvpair = (struct kv*) malloc(sizeof(struct kv) * klst->len);
    for (unsigned int i=0; i< klst->len; ++i){
        kvpair[i].key = strdup(klst->lst[i]);
        kvpair[i].value = *((int*)(cdict_get(ctx, klst->lst[i])));
    }
    // sort it
    qsort(kvpair, klst->len, sizeof(struct kv), compare);
    // print sorted result and free
    for (unsigned int i=0; i< klst->len; ++i){
        fprintf(stdout, "%s -> %d\n", kvpair[i].key, kvpair[i].value);
        free(kvpair[i].key);
    }
    // free memory
    cdict_free_keylist(klst, 0);
    free(kvpair);
    return;
}


int main(){
    int array_2_count[] = {1,2,3,4,5,6,7,8,9,7,65,45,3,2,2,2,
                           34,5,6,78,9,9,76,54,3,23,34,4,3,212};
    cdict_ctx * ctx = cdict_init(free_func, copy_func);
    if (!ctx){
        fprintf(stderr, "Can not allocate memory!\n");
        return 1;
    }
    int len = sizeof(array_2_count) /  sizeof(int);
    char key[50] = {0};
    int new_val;
    for (int i=0; i<len; i++){
        sprintf(key, "%d", array_2_count[i]);
        if (cdict_has_key(ctx, key)){
            new_val = *((int*)cdict_get(ctx, key));
            new_val = new_val + 1;
            cdict_set(ctx, key, (void*)&new_val);
        }else{
            new_val = 1;
            cdict_set(ctx, key, (void*)&new_val);
        }
    }
    // now print the result
    fprintf(stdout, "****Final results****\n");
    // let's print the final result based on the most common values
    print_most_common(ctx);
    // free memory
    cdict_free(ctx);
    return 0;
}
