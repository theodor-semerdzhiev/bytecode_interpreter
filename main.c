#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include "generics/utilities.h"
#include "parser/keywords.h"
#include "misc/dbgtools.h"
#include "parser/semanalysis.h"
#include "parser/lexer.h"
#include "compiler/compiler.h"
#include "generics/hashset.h"
#include "runtime/runtime.h"

int return_code = 0;
char *mainfile = NULL;
jmp_buf global_program_interrupt;

bool exec_prog_flag = true;
bool print_lexer_flag = false;
bool print_ast_flag = false;
bool print_bytecode_flag = false;
bool print_help_msg_flag = false;

// this pointer should never be freed
char *inline_script = NULL;

// These variables store the actual args given to your input script
char **script_args = NULL;
int script_args_count = 0;

/**
 * DESCRIPTION:
 * Helper for freeing global fields storing script arguments
*/
static void free_script_args() {
    if(!script_args)
        return;

    for(int i=0; i < script_args_count; i++) 
        free(script_args[i]);

    free(script_args);

    script_args = NULL;
    script_args_count = 0;
}

const char *help_output =
    "Proper Usage: %s [FILE ..] [ARGS ...]. \n"
    "   --deconstruct: Will print program bytecode \n"
    "   --ast: Will print out AST tree of program \n"
    "   --lexer: Will print lexing information of program \n"
    "   --run: Input file will be run \n"
    "   --norun: Input file will not be run \n"
    "   --script <CODE> : Input file will not be run, instead the code given as a argument will \n"
    "   --script-args <ARG1 ARG2 ... > : CLI Arguments given to input script \n";

/* performs cleanup operations in case of parsing error */
static void parser_error_cleanup(Parser *parser)
{
    // frees all memory malloced during parsing
    clear_memtracker_pointers(parser->memtracker);
    free_parser(parser);
}

/**
 * DESCRIPTION:
 * Parses program and performs semantic analysis. If program is valid, ast is returned. Otheriwise NULL is returned
 *
 * PARAMS:
 * file_contents: raw text of mainfile
 * tokens: mainfile tokenized
 * 
 * NOTE:
 * tokens is freed in this function
 */
AST_List *generate_AST(char *file_contents, TokenList *tokens)
{
    jmp_buf before_parsing;

    Parser *parser = init_Parser();
    parser->token_list = tokens;
    parser->lines.lines =
        tokenize_str_by_seperators(file_contents, '\n', &parser->lines.line_count);
    
    // if mainfile is NULL, then --script argument was provided
    parser->file_name = cpy_string(mainfile? mainfile: "--script ARG");

    parser->error_handler = &before_parsing;
    int error_return = setjmp((int *)&before_parsing);

    // if an error is detected, long jump is performed and if statement is called
    if (error_return != 0)
    {
    parser_error:;
        assert(parser->error_indicator);
        parser_error_cleanup(parser);
        return NULL;
    }

    // parses program
    enum token_type end_of_program[] = {END_OF_FILE};
    AST_List *ast = parse_code_block(parser, NULL, 0, false, end_of_program, 1);

    if (parser->error_indicator)
        goto parser_error;

    // for debugging purposes
    if (print_ast_flag)
        print_ast_list(ast, "  ", 0);

    // performs semantic anaylsis
    SemanticAnalyzer *sem_analyser = malloc_semantic_analyser(
        mainfile,
        parser->lines.lines,
        parser->lines.line_count,
        parser->token_list);

    free_parser(parser);

    bool is_sem_valid = AST_list_has_consistent_semantics(sem_analyser, ast);
    if (!is_sem_valid)
    {
        free_ast_list(ast);
    }

    free_semantic_analyser(sem_analyser);

    return is_sem_valid ? ast : NULL;
}

static bool parse_program_args(int argc, char *argv[])
{
    int i;

    if (argc == 1)
    {
        printf(
            "%s expects arguments. Proper Usage: %s [FILE ..] [ARGS ...].\n"
            "Run %s --help to see options.\n",
            argv[0], argv[0], argv[0]);
        return false;
    }
    else if (argc >= 2)
    {
        if(argv[1][0] != '\0' && argv[1][0] != '-' && argv[1][1] != '\0' && argv[1][1] != '-') {
            mainfile = argv[1];
            i = 2;
        } else {
            i = 1;
        }
    }

    for (; i < argc; i++)
    {
        if (strings_equal(argv[i], "--deconstruct"))
        {
            print_bytecode_flag = true;
        }
        else if (strings_equal(argv[i], "--ast"))
        {
            print_ast_flag = true;
        }
        else if (strings_equal(argv[i], "--lexer"))
        {
            print_lexer_flag = true;
        }
        else if (strings_equal(argv[i], "--run"))
        {
            exec_prog_flag = true;
        }
        else if (strings_equal(argv[i], "--norun"))
        {
            exec_prog_flag = false;
        }
        else if (strings_equal(argv[i], "--help"))
        {
            printf("%s", help_output);
            print_help_msg_flag = true;
            return true;
        }
        else if (strings_equal(argv[i], "--script"))
        {
            if(argc == i + 1) {
                printf("Must provide string to --script argument. Run --help to see arguments.");
                return false;
            }

            inline_script = argv[++i];
        }
        else if (strings_equal(argv[i], "--script-args")) {
            script_args_count=0;
            script_args = (char**)malloc(sizeof(char*)*(argc-i-1));
            for(int j = i + 1; j < argc; j++) {
                script_args[script_args_count++]=cpy_string(argv[j]);
            }
            return true;
        }
        else
        {
            printf("%s is not a valid argument. Run %s --help to see options.\n", argv[i], argv[0]);
            return false;
        }
    }
    return true;
}

/* MAIN PROGRAM LOGIC */
int main(int argc, char *argv[])
{
    if (!parse_program_args(argc, argv))
        return 1;

    // mainfile = "tests/test24.txt";

    // --help flag was used
    if (print_help_msg_flag)
        return 0;

    char *file_contents = cpy_string(inline_script);
    
    if(!file_contents) {
        if(!mainfile) {
            printf("Script was not provided, provide a file as first argument, or use the --script flag.");
            return 1;
        }

        file_contents = get_file_contents(mainfile);
        if (!file_contents)
        {
            printf("Could not open %s\n", mainfile);
            return 1;
        }

    }

    init_keyword_table();
    init_Precedence();

    TokenList *tokens = tokenize_file(file_contents);

    // prints lexing info, if user requests it
    if (print_lexer_flag)
        print_token_list(tokens);

    AST_List *program_ast = generate_AST(file_contents, tokens);
    free(file_contents);

    // makes sure that program is valid
    if (!program_ast)
    {
        free_keyword_table();
        return 1;
    }

    // Compiles program
    Compiler *compiler = init_Compiler(mainfile);
    ByteCodeList *list = compile_code_body(compiler, program_ast, true, false);

    // frees structs that are no longer needed
    compiler_free(compiler);
    free_ast_list(program_ast);
    free_keyword_table();

    // user requests to deconstruct bytecode
    if (print_bytecode_flag)
        deconstruct_bytecode(list, 0);

    if(exec_prog_flag) {    
        // printf("%d", script_args_count);
        // Inits important structures used by runtime environment
        if (!prep_runtime_env(list, mainfile, script_args_count, script_args))
        {
            free_ByteCodeList(list);
            printf("Error occurred Setting up runtime environment.\n");
            return 1;
        }

        // This is the global program interrupt
        // if an unhandled exception occurs, a long jump is performed to this instruction
        // with a non zero return code
        int error_return = setjmp((int *)&global_program_interrupt);

        // Runs program
        if (error_return == 0)
            return_code = run_program();
        else
            return_code = error_return;

        perform_runtime_cleanup();
    }

    free_script_args();
    free_ByteCodeList(list);
    free_keyword_table();
    return return_code;
}
