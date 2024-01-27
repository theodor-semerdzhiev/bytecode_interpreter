#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include "parser/keywords.h"
#include "misc/dbgtools.h"
#include "parser/semanalysis.h"
#include "compiler/compiler.h"
#include "generics/hashset.h"
#include "runtime/runtime.h"

/* Abstracts lexing for a given file */
TokenList *tokenize_file(char *file_contents)
{

    Lexer *lexer = malloc_lexer();
    TokenList *tokens = tokenize_str(lexer, file_contents);
    free_lexer(lexer);

    return tokens;
}

/* performs cleanup operations in case of parsing error */
static void parser_error_cleanup(Parser *parser)
{
    // frees all memory malloced during parsing
    clear_memtracker_pointers(parser->memtracker);
    free_parser(parser);
}

/* Abstracts file parsing, if a parsing or semantic error occurs, function returns NULL */
AST_List *parse_file(char *filename)
{

    char *file_contents = get_file_contents(filename);
    if (!file_contents)
    {
        printf("Could not open %s\n", filename);
        return NULL;
    }

    TokenList *tokens = tokenize_file(file_contents);

    print_token_list(tokens);

    jmp_buf before_parsing;

    Parser *parser = init_Parser();
    parser->token_list = tokens;
    parser->lines.lines =
        tokenize_str_by_seperators(file_contents, '\n', &parser->lines.line_count);
    parser->file_name = malloc_string_cpy(NULL, filename);

    free(file_contents);

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

    enum token_type end_of_program[] = {END_OF_FILE};
    AST_List *ast = parse_code_block(parser, NULL, 0, end_of_program, 1);

    if (parser->error_indicator)
        goto parser_error;

    // for debugging purposes 
    print_ast_list(ast, "  ", 0);

    SemanticAnalyzer *sem_analyser = malloc_semantic_analyser(
                                    filename, 
                                    parser->lines.lines, 
                                    parser->lines.line_count, 
                                    parser->token_list);

    free_parser(parser);

    bool is_sem_valid = AST_list_has_consistent_semantics(sem_analyser, ast);

    if (is_sem_valid)
    {
        printf("Valid semantics\n");        
    }
    else
    {
        printf("Invalid semantics\n");
        free_ast_list(ast);
    }

    free_semantic_analyser(sem_analyser);
    return is_sem_valid? ast: NULL;
}

/* MAIN PROGRAM LOGIC */

int return_code = 0;

int main(int argc, char *argv[])
{
    init_keyword_table();
    init_Precedence();

    // AST_List *ast = parse_file(argv[1]);
    AST_List *ast = parse_file("./tests/test5.txt");
    
    free_keyword_table();
    
    if(!ast) {
        return_code = 1;
    } else {

        ByteCodeList* list = compile_code_body(ast, true, false);
        
        free_ast_list(ast);

        deconstruct_bytecode(list,0);

        // Preps runtime environment 
        if(!prep_runtime_env(list)) {
            printf("Error occurred Setting up runtime environment.");
            return 1;
        }

        // Runs program
        return_code = run_program();
        
        perform_cleanup();
        
        free_ByteCodeList(list);
    }

    free_keyword_table();
    return return_code;
}
