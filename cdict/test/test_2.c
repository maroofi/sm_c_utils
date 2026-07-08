#include <cdict.h>
#include <stdlib.h>

// our custom structure
struct custom_value{
    char * name;
    char * lastname;
    int age;
};

// this is my customized free function to pass to cdict_init().
void dict_free_func(void * value){
    fprintf(stdout, "let's free the shit\n");
    struct custom_value * tmp = (struct custom_value*) value;
    free(tmp->name);
    free(tmp->lastname);
    free(tmp);
}

// this is my customized copy function to pass to cdict_init()
void* dict_copy_func(void * value){
    struct custom_value * tmp = (struct custom_value*)malloc(sizeof(struct custom_value));
    if (!tmp)
        return NULL;
    tmp->age = ((struct custom_value*)value)->age;
    tmp->name = strdup(((struct custom_value*)value)->name);
    tmp->lastname = strdup(((struct custom_value*)value)->lastname);
    return tmp;
}


int main(int argc, char ** argv){
    srand(1);
    cdict_ctx* new_dict = NULL;
    new_dict = cdict_init(dict_free_func, dict_copy_func);
    if (!new_dict){
        fprintf(stdout, "Can not initialize the dictionary!\n");
        return 1;
    }
    struct custom_value cv;
    cv.age = 32;
    cv.name = "John";
    cv.lastname = "Doe";
    cdict_set(new_dict, "jd32", (void*)&cv);
    
    // now let's get the value
    struct custom_value * data = cdict_get(new_dict, "jd32");
    if (data){
        fprintf(stdout, "Name: %s, Lastname: %s, Age: %d\n",
                data->name, data->lastname, data->age);
    }
    cdict_free(new_dict);
    return 0;
}
