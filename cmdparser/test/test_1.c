#include <stdio.h>
#include <cmdparser.h>


int main(int argc, char ** argv){
    int parse_error = 0;
    ARG_CMD_OPTION cmd_option[] = {
        {.short_option='q', .long_option="silent", .has_param=0, .help="do things silently", .tag="is_silent"},
        {.short_option='i', .long_option="input", .has_param=1, .help="Specifies the input file", .tag="input_file"},
        {.short_option='l', .long_option="list", .has_param=0, .help="List the database", .tag="is_list"},
        {.short_option='o', .long_option="output", .has_param=1, .help="Specifies the output file", .tag="output_file"},
        {.short_option='-', .long_option=NULL, .has_param=0, .help=NULL, .tag=NULL}  // it's important to have the last row with tag=NULL
    };
    ARG_CMDLINE cmd = {
        .cmd_option = cmd_option, .summary="This is a very useful program\nIt can help you parse all the arguments!",
        .accept_file = 1, .extra = NULL
    };

    printf("cmdline: ");
    for (int i=0;i<argc;i++){
        printf("%s\t", argv[i]);
    }
    printf("\n");
    PARG_PARSED_ARGS parg = arg_parse_arguments(&cmd, argc, argv, &parse_error);
    if (!parg){
        arg_show_help(&cmd, argc, argv);
        return 1;
    }
    PARG_CMD_OPTION opt = cmd.cmd_option;

    while (opt->tag){
        printf("tag: %s, value: %s\n", arg_is_tag_set(parg, opt->tag)?opt->tag:NULL, arg_get_tag_value(parg, opt->tag)?arg_get_tag_value(parg, opt->tag):NULL);
        opt++;
    }
    if (cmd.extra)
        printf("We also have this extra param: %s\n", cmd.extra);
    return 0;
}
