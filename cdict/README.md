### Python-like dictionary datatype written in C

Dictionary is a key-value storage useful for different purposes.

The key is always of type `char *` (string) and the value can be any type.

Since we don't know the type of the `value` in advance, (it can be a pointer to char or int or a custom structure),
We have to know how to copy the `value` and how to free the allocated memory for the `value`.

Therefore, the cdict_init() function will accept two parameters:

1. cdict_free_func: A pointer to the user provided function to free the memory for the value.
2. cdict_copy_func: A pointer to the user provided function to copy the `value`.

Other things are easy to use and understand.

There are currently four functions available for the dictionary structure:

1. Adding a key-value pair to the dictionary
2. Removing a key-value pair from a dictionary
3. Getting a value from the dictionary by providing the key
4. Lists all the keys


# Example

Check the examples in test directory

1. `test_1.c`: shows how to insert key-value pairs to the dictionary and check if a key exists.

if values are of type `char *`, then I use
```c
cdict_ctx * dict = cdict_init(free, (char*) strdup);
```

to initialize the dictionary.

However, if I want to store a custom structure in the dictionary, I have to provide two custom 
functions to __free__ and __copy__ the structure for me.

Look at `test_2.c` in the `test` directory for more info.

2. `test_remove.c` in `test` directory shows how to remove the key in a for-loop.

3. `test_2.c` in `test` directory shows how to store custom structure as values in the dictionary.

# Documentation

Use Doxygen to generate the documents
