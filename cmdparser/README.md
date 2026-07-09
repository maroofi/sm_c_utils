# cmdparser
Small command line argument parser in C

(You should always use GetOpt if possible)

### Command-line parser in C

Not using GetOpt or Argp since I may use it on other operating systems.

All based on standard libraries.

Check the `test` directory for an example.

Compile as a library with make:
```
cd cmdparser
make
```

Clean up the compiled library
```
make clean
```

### Example

```c
#include <stdio.h>
#include <cmdparser.h>


int main(int argc, char ** argv){

    // we need an integer to pass to the arg_parse_arguments() function to capture the possible errors.
    int parse_error = 0;

    // Create an array of ARG_CMD_OPTION. This array specifies the list of command line options.
    // short_option: This can be just one character. The user will use this with '-' sign
    // long_option: This can be a word. The user will use this with '--' sign.
    // has_param: 0 means this option does not accept parameters. 1 means the user must provide a value for this option.
    // in the following example, -q/--silent just tel the code to be executed silently. It does not need params.
    // help: the string to show information about the switch.
    // tag: This is a handle that will be used by you to check if the switch is set or not and to get the value of
    // the switch in case it has a value. It must be a string.
    // The last record of this array must be an empty record with .tag=NULL. This is important since this is the 
    // way we find the last record (we don't keep the length of the cmd_option array).
    ARG_CMD_OPTION cmd_option[] = {
        {.short_option='q', .long_option="silent", .has_param=0, .help="do things silently", .tag="is_silent"},
        {.short_option='i', .long_option="input", .has_param=1, .help="Specifies the input file", .tag="input_file"},
        {.short_option='l', .long_option="list", .has_param=0, .help="List the database", .tag="is_list"},
        {.short_option='o', .long_option="output", .has_param=1, .help="Specifies the output file", .tag="output_file"},
        {.short_option='-', .long_option=NULL, .has_param=0, .help=NULL, .tag=NULL}  // it's important to have the last row with tag=NULL
    };

    // This is the mail structure of the command line.
    // summary: A summary of what this tool does
    // accept_file: can be 0 or 1. If you have an input file, you don't need to specify a switch for it. You can just set this
    // to one and the command line without switch will be parsed as input file. It will be stored in 'extra' member of the structure.
    ARG_CMDLINE cmd = {
        .cmd_option = cmd_option, .summary="This is a very useful program\nIt can help you parse all the arguments!",
        .accept_file = 1, .extra = NULL
    };

    printf("cmdline: ");
    for (int i=0;i<argc;i++){
        printf("%s\t", argv[i]);
    }
    printf("\n");

    // we call this function to parse the input provided by the user.
    PARG_PARSED_ARGS parg = arg_parse_arguments(&cmd, argc, argv, &parse_error);
    
    if (!parg){
        arg_show_help(&cmd, argc, argv);
        return 1;
    }
    PARG_CMD_OPTION opt = cmd.cmd_option;

    // arg_is_tag_set() is used to see if a tag is set by the user or not.
    // arg_get_tag_value() is used to get the value of the tags that accept values.
    while (opt->tag){
        printf("tag: %s, value: %s\n", arg_is_tag_set(parg, opt->tag)?opt->tag:NULL, arg_get_tag_value(parg, opt->tag)?arg_get_tag_value(parg, opt->tag):NULL);
        opt++;
    }
    // if you set accept_file=1, then you will receive the extra (of input file) by calling cmd.extra.
    if (cmd.extra)
        printf("We also have this extra param: %s\n", cmd.extra);
    return 0;
}
```
