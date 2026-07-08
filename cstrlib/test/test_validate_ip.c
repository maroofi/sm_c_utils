#include <cstrlib.h>
#include <stdio.h>
#include <stdlib.h>


// The code perform an IPv4 validation test by using the string library.
// Note that this is just a demo to show how to use the library.
// An IPv4 can be validated in an easier way using its grammar more efficiently.

int main(int argc, char ** argv){
    if (argc != 2){
        fprintf(stdout, "Enter an IP address to validate\n");
        fprintf(stdout, "Usage: %s <ip-address>\n", argv[0]);
        return 1;
    }
    PSTR pstr = str_init(argv[1]);
    if (!pstr){
        fprintf(stderr, "Can not initialize a string..\n");
        return 1;
    }
    //valid ipv4 length is < 15
    if (pstr->str_len(pstr) > 15){
        fprintf(stdout, "INVALID: too long to be a valid IPv4\n");
        str_free(pstr);
        return 1;
    }
    // valid ipv4 has 3 dots
    int dot_count = pstr->str_count(pstr, ".");
    if (dot_count != 3){
        fprintf(stdout, "INVALID: valid IPv4 has three dots not %d\n", dot_count);
        str_free(pstr);
        return 1;
    }
    // must be splitted into 4 parts
    PSPLITLIST splt = pstr->str_split(pstr, ".", -1);
    if (!splt || splt->len != 4){
        fprintf(stdout, "INVALID: valid IPv4 has four parts splitted by dot\n");
        str_free_splitlist(splt);
        str_free(pstr);
        return 1;
    }

    // validate each part
    PSTR part = NULL;
    char ip[20] = {0};
    int each_part = -1;
    for (int i=0; i< splt->len; i++){
        part = str_init(splt->list[i]);
        // parts can not start with '0' unless it's the only digit
        if (part->str_startswith(part, "0") && part->str_len(part) > 1){
            fprintf(stdout, "INVALID: Parts can not start with zero unless it's the only digit\n");
            str_free(part);
            str_free(pstr);
            str_free_splitlist(splt);
            return 1;
        }
        // check if each part is all digit
        if (! part->str_isdigit(part)){
            fprintf(stdout, "INVALID: Parts must be all digits not characters:%s\n", part->str);
            str_free(part);
            str_free(pstr);
            str_free_splitlist(splt);
            return 1;

        }
        // check if each part is between 0 and 255
        int result = sscanf(part->str, "%d%s", &each_part, ip);
        if (result != 1 || each_part < 0 || each_part > 255 || ip[0] != '\0'){
            fprintf(stdout, "INVALID: %s is not a valid part of IPv4\n", (char*) part->str);
            str_free(part);
            str_free(pstr);
            str_free_splitlist(splt);
            return 1;
        }
        str_free(part);
    }
    str_free_splitlist(splt);
    str_free(pstr);
    fprintf(stdout, "%s is a VALID IPv4!\n", argv[1]);
    return 0;
}
