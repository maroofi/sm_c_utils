#include <errno.h>
#include <stdio.h>
#include <cstrlib.h>
#include <cunittest.h>

// compile with: gcc  -Wall -Werror -o test test_1.c ../src/cstrlib.c -I../include -g -DDEBUG
// all functions called and tested by valgrind for memory leak

// function declaration
void test_str_count(void);
void test_str_find(void);
void test_str_startswith(void);
void test_str_endswith(void);
void test_str_lstrip(void);
void test_str_rstrip(void);
void test_str_strip(void);
void test_str_reverse(void);
void test_str_upper(void);
void test_str_lower(void);
void test_str_swapcase(void);
void test_str_isdigit(void);
void test_str_replace(void);
void test_str_split(void);
char * readline(FILE *);
void test_read_file(char *);
void test_str_append_string(void);
void test_str_append_char(void);
void test_str_join(void);

void test_str_prepend_string(void);
void test_str_prepend_char(void);


int main(int argc, char ** argv){
    ABORT_ON_FAIL(1);
    test_str_count();
    test_str_find();
    test_str_startswith();
    test_str_endswith();
    test_str_lstrip();
    test_str_rstrip();
    test_str_strip();
    test_str_reverse();
    test_str_lower();
    test_str_upper();
    test_str_isdigit();
    test_str_replace();
    test_str_split();
    test_str_append_string();
    test_str_append_char();
    test_str_prepend_string();
    test_str_prepend_char();
    test_str_join();
    test_read_file(argv[1]);
    return 0;
}

void test_str_prepend_char(void){
    PSTR pstr = str_init(NULL);
    pstr->str_prepend_char(pstr, 'A');
    pstr->str_prepend_char(pstr, 'B');
    pstr->str_prepend_char(pstr, 'C');
    pstr->str_prepend_char(pstr, 'D');
    pstr->str_prepend_char(pstr, 'E');
    ASSERT_EQ_STR((char*) pstr->str_getval(pstr), "EDCBA");
    str_free(pstr);
    pstr = str_init("");
    pstr->str_prepend_char(pstr, 'A');
    pstr->str_prepend_char(pstr, 'B');
    pstr->str_prepend_char(pstr, 'C');
    pstr->str_prepend_char(pstr, 'D');
    pstr->str_prepend_char(pstr, 'E');
    ASSERT_EQ_STR((char*) pstr->str_getval(pstr), "EDCBA");
    str_free(pstr);
}


void test_str_prepend_string(void){
    PSTR pstr = str_init(NULL);
    pstr->str_prepend_string(pstr, "John Doe");
    ASSERT_EQ_STR((char*) pstr->str_getval(pstr), "John Doe");
    pstr->str_prepend_string(pstr, "John Doe");
    ASSERT_EQ_STR((char*) pstr->str_getval(pstr), "John DoeJohn Doe");
    str_free(pstr);
}

void test_str_join(void){
    PSTR pstr = str_init("1.2.3.4");
    PSPLITLIST splt = pstr->str_split(pstr, ".", -1);
    int result = pstr->str_join(pstr, splt, "/");
    ASSERT_EQ_INT(result, 0);
    ASSERT_EQ_STR((char*) pstr->str_getval(pstr), "1/2/3/4");
    str_free_splitlist(splt);
    str_free(pstr);
}


void test_str_append_string(void){
    PSTR pstr = str_init("This is an easy test!");
    pstr->str_append_string(pstr, "add this");
    ASSERT_EQ_STR((char*)pstr->str_getval(pstr), (char *)"This is an easy test!add this");
    ASSERT_EQ_INT(pstr->len, 29);
    str_free(pstr);
}

void test_str_append_char(void){
    PSTR pstr = str_init(NULL);
    pstr->str_append_char(pstr, 'a');
    pstr->str_append_char(pstr, 'b');
    pstr->str_append_char(pstr, 'c');
    pstr->str_append_char(pstr, 'd');
    pstr->str_append_char(pstr, 'e');
    pstr->str_append_char(pstr, 'f');
    pstr->str_append_char(pstr, 'g');
    ASSERT_EQ_STR((char *)pstr->str_getval(pstr), "abcdefg");
    ASSERT_EQ_INT(pstr->len, 7);
    str_free(pstr);

    pstr = str_init("John Do");
    pstr->str_append_char(pstr, 'e');
    ASSERT_EQ_STR((char *) pstr->str_getval(pstr), "John Doe");
    str_free(pstr);
}




void test_read_file(char * filename){
    if (filename == NULL){
        printf("Must specify a absolute file name...\n");
        return;
    }
    FILE * fp = fopen(filename, "r");
    if (!fp){
        fprintf(stderr, "Can not open file: %s\n", filename);
        return;
    }
    char * line = NULL;
    PSTR pstr_line = str_init(NULL);
    int result = 0;
    while ((line = readline(fp)) != NULL){
        result = pstr_line->str_setval(pstr_line, line);
        free(line);
        if (result != 0){
            printf("ERROR: %s\n", pstr_line->errmsg);
            return;
        }
        line = pstr_line->str_strip(pstr_line, NULL);
        if (line == NULL)
            continue;
        if (line[0] == '\0'){  //line is empty
            free(line);
            continue;
        }
        printf("%s\n", line);
        free(line);
    }
    str_free(pstr_line);
    fclose(fp);
}

char * readline(FILE * fp){
    const int MAX_LINE_SIZE = 255;
    char * line = (char*) calloc(MAX_LINE_SIZE + 1, sizeof(char));
    int ch = -1;
    char * new_memory = NULL;
    unsigned long int count = 0;
    while ((ch = fgetc(fp)) != EOF){
        line[count++] = ch;
        if (ch == '\n'){
            line[count] = '\0';
            return line;
        }
        if (count % MAX_LINE_SIZE == 0){
            // we need to reallocate
            new_memory = (char*)realloc(line, strlen(line) + MAX_LINE_SIZE);
            if (new_memory == NULL){
                printf("ERROR: Can not reallocate memory for reading line...");
                free(line);
                return NULL;
            }
            line = new_memory;
        }
    }
    if (count == 0){
        // we are at the end of file
        free(line);
        return NULL;
    }
    line[count] = '\0';
    return line;
}


void test_str_split(void){
    PSTR pstr = str_init("This is a test");
    PSPLITLIST plst = pstr->str_split(pstr, (char*)" ", 2);
    ASSERT_EQ_STR(plst->list[0], "This");
    ASSERT_EQ_STR(plst->list[1], "is");
    ASSERT_EQ_STR(plst->list[2], "a test");
    str_free_splitlist(plst);
    str_free(pstr);
}

void test_str_replace(void){
    PSTR pstr = str_init("");
    char * str = NULL;
    str = pstr->str_replace(pstr, "", "This is a new val", 1);
    ASSERT_EQ_STR(str, (char*)"This is a new val");
    ASSERT_EQ_INT(strlen(str), 17);
    free(str);
    pstr->str_setval(pstr, "This is a new string!");
    str = pstr->str_replace(pstr, "", "", 1);
    ASSERT_EQ_INT(strlen(str), 21);
    ASSERT_EQ_STR(str, (char*)"This is a new string!");
    ASSERT_EQ_INT(strlen(str), 21);
    free(str);
    pstr->str_setval(pstr, "This is a simple test to see if replace work fine!");
    str = pstr->str_replace(pstr, "e", "*", -1);
    ASSERT_EQ_INT(strlen(str), 50);
    ASSERT_EQ_STR(str, (char*)"This is a simpl* t*st to s** if r*plac* work fin*!");
    pstr->str_setval(pstr, "this is a new string");
    str = pstr->str_replace(pstr, "e", "*", 0);
    ASSERT_EQ_STR(str, "this is a new string");
    free(str);
    pstr->str_setval(pstr, "this is a new string");
    str = pstr->str_replace(pstr, "e", "*", 1);
    ASSERT_EQ_STR(str, "this is a n*w string");
    free(str);
    pstr->str_setval(pstr, "this is a new string");
    str = pstr->str_replace(pstr, "s", "*", 1);
    ASSERT_EQ_STR(str, "thi* is a new string");
    free(str);

    str_free(pstr);
}

void test_str_isdigit(void){
    PSTR pstr = str_init("This is an easy test!");
    ASSERT_EQ_INT(pstr->str_isdigit(pstr), 0);
    pstr->str_setval(pstr, "12345");
    ASSERT_EQ_INT(pstr->str_isdigit(pstr), 1);
    str_free(pstr);
}

void test_str_swapcase(void){
    PSTR pstr = str_init("This is an easy test!");
    ASSERT_EQ_STR(pstr->str_swapcase(pstr), (char *)"tHIS IS AN EASY TEST!");
    pstr->str_setval(pstr, "");
    ASSERT_EQ_STR(pstr->str_swapcase(pstr), (char *)"");
    pstr->str_setval(pstr, "12345");
    ASSERT_EQ_STR(pstr->str_swapcase(pstr), (char *)"12345");
    str_free(pstr);
}

void test_str_lower(void){
    PSTR pstr = str_init("This is an easy test!");
    char * str = NULL;
    
    str = pstr->str_lower(pstr);
    ASSERT_EQ_STR(str, (char *)"this is an easy test!");
    free(str);
    
    pstr->str_setval(pstr, "");
    str = pstr->str_lower(pstr);
    ASSERT_EQ_STR(str, (char *)"");
    free(str);

    pstr->str_setval(pstr, "12345");
    str = pstr->str_lower(pstr);
    ASSERT_EQ_STR(str, (char *)"12345");
    free(str);

    pstr->str_setval(pstr, "1");
    str = pstr->str_lower(pstr);
    ASSERT_EQ_STR(str, (char *)"1");
    free(str);
    pstr->str_setval(pstr, "this ");
    str = pstr->str_lower(pstr);
    ASSERT_EQ_STR(str, (char *)"this ");
    free(str);
    str_free(pstr);
}


void test_str_upper(void){
    PSTR pstr = str_init("This is an easy test!");
    char * str = NULL;
    str = pstr->str_upper(pstr);
    ASSERT_EQ_STR(str, (char *)"THIS IS AN EASY TEST!");
    free(str);

    pstr->str_setval(pstr, "");
    str = pstr->str_upper(pstr);
    ASSERT_EQ_STR(str, (char *)"");
    free(str);
    pstr->str_setval(pstr, "12345");
    str = pstr->str_upper(pstr);
    ASSERT_EQ_STR(str, (char *)"12345");
    free(str);
    pstr->str_setval(pstr, "1");
    str = pstr->str_upper(pstr);
    ASSERT_EQ_STR(str, (char *)"1");
    free(str);
    pstr->str_setval(pstr, "this ");
    str = pstr->str_upper(pstr);
    ASSERT_EQ_STR(str, (char *)"THIS ");
    free(str);
    str_free(pstr);

}

void test_str_reverse(void){
    PSTR pstr = str_init("This is an easy test!");
    char * str = NULL;
    
    str = pstr->str_reverse(NULL);
    ASSERT_NULL(str);
    free(str);
    
    str = pstr->str_reverse(pstr);
    ASSERT_EQ_STR(str, (char *)"!tset ysae na si sihT");
    free(str);


    pstr->str_setval(pstr, "");
    str = pstr->str_reverse(pstr);
    ASSERT_EQ_STR(str, (char *)"");
    free(str);

    pstr->str_setval(pstr, "12345");
    str = pstr->str_reverse(pstr);
    ASSERT_EQ_STR(str, (char *)"54321");
    free(str);

    pstr->str_setval(pstr, "1");
    str = pstr->str_reverse(pstr);
    ASSERT_EQ_STR(str, (char *)"1");
    free(str);

    str_free(pstr);
}

void test_str_strip(void){
    PSTR pstr = str_init("This is an easy test!");
    char * str = NULL;

    str = pstr->str_strip(pstr, (const char *)NULL);
    ASSERT_EQ_STR(str, pstr->str);
    free(str);

    str = pstr->str_strip(pstr, (const char *)"T!");
    ASSERT_EQ_STR(str, "his is an easy test");
    free(str);

    str = pstr->str_strip(pstr, (const char *)"T");
    ASSERT_EQ_STR(str, "his is an easy test!");
    free(str);

    str = pstr->str_strip(pstr, (const char *)"Tes!tsiaehn y");
    ASSERT_EQ_STR(str, "");
    free(str);

    pstr->str_setval(pstr, "     \x0a\x0dThis is just a test           \x0c   ");
    str = pstr->str_strip(pstr, (const char *)NULL);
    ASSERT_EQ_STR(str, "This is just a test");
    free(str);

    str_free(pstr);
}

void test_str_rstrip(void){
    PSTR pstr = str_init("This is an easy test!");
    char * str = NULL;

    str = pstr->str_rstrip(pstr, (const char *)NULL);
    ASSERT_EQ_STR(str, pstr->str);
    free(str);

    str = pstr->str_rstrip(pstr, (const char *)"!");
    ASSERT_EQ_STR(str, "This is an easy test");
    free(str);

    str = pstr->str_rstrip(pstr, (const char *)"es!");
    ASSERT_EQ_STR(str, "This is an easy test");
    free(str);
    
    str = pstr->str_rstrip(pstr, (const char *)"es!t");
    ASSERT_EQ_STR(str, "This is an easy ");
    free(str);

    str = pstr->str_rstrip(pstr, (const char *)" es!t");
    ASSERT_EQ_STR(str, "This is an easy");
    free(str);

    str = pstr->str_rstrip(pstr, (const char *)"y es!tanih");
    ASSERT_EQ_STR(str, "T");
    free(str);

    str = pstr->str_rstrip(pstr, (const char *)"y es!tanTih");
    ASSERT_EQ_STR(str, "");
    free(str);

    str = pstr->str_rstrip(pstr, (const char *)" es!t");
    ASSERT_EQ_STR(str, "This is an easy");
    free(str);

    pstr->str_setval(pstr, "this is   tab + spaces    \x0c   ");
    str = pstr->str_rstrip(pstr, (const char *)NULL);
    ASSERT_EQ_STR(str, "this is   tab + spaces");
    free(str);
    
    pstr->str_setval(pstr, "\n");
    str = pstr->str_rstrip(pstr, (const char *)NULL);
    ASSERT_EQ_STR(str, "");
    free(str);

    str_free(pstr);
}


void test_str_lstrip(void){
    PSTR pstr = str_init("This is an easy test!");
    char * str = NULL;

    str = pstr->str_lstrip(pstr, (const char *)NULL);
    ASSERT_EQ_STR(str, pstr->str);
    free(str);
    
    str = pstr->str_lstrip(pstr, (const char *)"this");
    ASSERT_EQ_STR(str, pstr->str);
    free(str);
    
    str = pstr->str_lstrip(pstr, (const char *)"This");
    ASSERT_EQ_STR(str, " is an easy test!");
    free(str);

    str = pstr->str_lstrip(pstr, (const char *)"This ");
    ASSERT_EQ_STR(str, "an easy test!");
    free(str);

    str = pstr->str_lstrip(pstr, (const char *)"T ish");
    ASSERT_EQ_STR(str, "an easy test!");
    free(str);

    str = pstr->str_lstrip(pstr, (const char *)"T ishayn");
    ASSERT_EQ_STR(str, "easy test!");
    free(str);

    
    pstr->str_setval(pstr, "!");
    str = pstr->str_lstrip(pstr, (const char *)"");
    ASSERT_EQ_STR(str, "!");
    free(str);
    
    str = pstr->str_lstrip(pstr, (const char *)"!");
    ASSERT_EQ_STR(str, "");
    free(str);

    str = pstr->str_lstrip(pstr, (const char *)NULL);
    ASSERT_EQ_STR(str, "!");
    free(str);

    str_free(pstr);
}

void test_str_endswith(void){
    PSTR pstr = str_init("This is an easy test!");
    ASSERT_EQ_INT(pstr->str_endswith(pstr, (const char *)"this"), 0);
    ASSERT_EQ_INT(pstr->str_endswith(pstr, (const char *)"This"), 0);
    ASSERT_EQ_INT(pstr->str_endswith(pstr, (const char *)"!"), 1);
    ASSERT_EQ_INT(pstr->str_endswith(pstr, (const char *)"test!"), 1);
    ASSERT_EQ_INT(pstr->str_endswith(pstr, (const char *)""), 1);
    ASSERT_EQ_INT(pstr->str_endswith(pstr, (const char *)"This is an easy test!"), 1);
    str_free(pstr);
}

void test_str_count(void){
    PSTR pstr = str_init("This is an easy test!");
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"this"), 0);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"This"), 1);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"t"), 2);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"!"), 1);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"easy"), 1);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)" "), 4);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)""), pstr->len +1);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"EASY"), 0);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"This is an easy test!"), 1);
    ASSERT_EQ_INT(pstr->str_count(pstr, (const char *)"This is an easy test! "), 0);
    str_free(pstr);
}

void test_str_find(void){
    PSTR pstr = str_init("This is an easy test!");
    ASSERT_EQ_INT(pstr->str_find(pstr, (const char *)"this"), -1);
    ASSERT_EQ_INT(pstr->str_find(pstr, (const char *)"This"), 0);
    ASSERT_EQ_INT(pstr->str_find(pstr, (const char *)""), 0);
    ASSERT_EQ_INT(pstr->str_find(pstr, (const char *)"!"), 20);
    ASSERT_EQ_INT(pstr->str_find(pstr, (const char *)"This is an easy test! sdafsdfdasfas"), -1);
    ASSERT_EQ_INT(pstr->str_find(pstr, (const char *)"This is an easy test!"), 0);
    str_free(pstr);
}

void test_str_startswith(void){
    PSTR pstr = str_init("This is an easy test!");
    ASSERT_EQ_INT(pstr->str_startswith(pstr, (const char *)"this"), 0);
    ASSERT_EQ_INT(pstr->str_startswith(pstr, (const char *)"This"), 1);
    ASSERT_EQ_INT(pstr->str_startswith(pstr, (const char *)"!"), 0);
    str_free(pstr);
}
