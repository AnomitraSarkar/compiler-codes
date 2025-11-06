#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEME_LENGTH 100
#define MAX_SYMBOL_TABLE_ENTRIES 1000
#define MAX_TOKENS 1000

// Token types
#define KEYWORD "keyword"
#define IDENTIFIER "identifier"
#define INTEGER "integer"
#define FLOAT "float"
#define OPERATOR "operator"
#define SPECIAL "special"
#define LITERAL "literal"

// Symbol table entry structure
typedef struct {
    int entry_no;
    char lexeme[MAX_LEXEME_LENGTH];
    char token_type[20];
    int line_declared;
    int line_used;
} SymbolEntry;

// Token structure
typedef struct {
    char token_type[20];
    char lexeme[MAX_LEXEME_LENGTH];
    int line;
} Token;

// Function prototypes
int is_keyword(const char *lexeme);
int is_one_char_op(char c);
int is_two_char_op(const char *s);
int is_special_symbol(char c);

int main() {
    const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum",
        "extern", "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed",
        "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
    };
    const char *one_char_ops = "+-*%=<>!&|";
    const char *two_char_ops[] = {"++", "--", "==", "!=", "<=", ">=", "&&", "||", "+=", "-=", "*=", "/="};
    const char *special_symbols = "(){};,";

    FILE *fp = fopen("test.c", "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Determine file size
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate memory for file content
    char *buffer = (char *)malloc(fsize + 1);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        fclose(fp);
        return 1;
    }

    // Read file content
    fread(buffer, 1, fsize, fp);
    buffer[fsize] = '\0';
    fclose(fp);

    SymbolEntry symbol_table[MAX_SYMBOL_TABLE_ENTRIES];
    Token tokens[MAX_TOKENS];
    int symbol_table_size = 0;
    int token_count = 0;
    int line_number = 1;
    int i = 0;

    while (i < fsize) {
        char c = buffer[i];

        // Skip whitespace
        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }

        // Handle newline
        if (c == '\n') {
            line_number++;
            i++;
            continue;
        }

        // Handle comments
        if (c == '/') {
            if (i + 1 < fsize) {
                if (buffer[i + 1] == '/') {
                    // Single-line comment
                    i += 2;
                    while (i < fsize && buffer[i] != '\n') {
                        i++;
                    }
                    continue;
                } else if (buffer[i + 1] == '*') {
                    // Multi-line comment
                    i += 2;
                    int comment_closed = 0;
                    while (i < fsize - 1) {
                        if (buffer[i] == '*' && buffer[i + 1] == '/') {
                            comment_closed = 1;
                            i += 2;
                            break;
                        }
                        if (buffer[i] == '\n') {
                            line_number++;
                        }
                        i++;
                    }
                    if (!comment_closed) {
                        fprintf(stderr, "Lexical error at line %d: unclosed multi-line comment\n", line_number);
                    }
                    continue;
                }
            }
        }

        // Handle identifiers and keywords
        if (isalpha(c) || c == '_') {
            char lexeme[MAX_LEXEME_LENGTH] = {0};
            int j = 0;

            lexeme[j++] = c;
            i++;
            while (i < fsize && (isalnum(buffer[i]) || buffer[i] == '_')) {
                if (j < MAX_LEXEME_LENGTH - 1) {
                    lexeme[j++] = buffer[i];
                }
                i++;
            }
            lexeme[j] = '\0';

            if (is_keyword(lexeme)) {
                tokens[token_count].line = line_number;
                strcpy(tokens[token_count].token_type, KEYWORD);
                strcpy(tokens[token_count].lexeme, lexeme);
                token_count++;
            } else {
                tokens[token_count].line = line_number;
                strcpy(tokens[token_count].token_type, IDENTIFIER);
                strcpy(tokens[token_count].lexeme, lexeme);
                token_count++;

                symbol_table[symbol_table_size].entry_no = symbol_table_size + 1;
                strcpy(symbol_table[symbol_table_size].lexeme, lexeme);
                strcpy(symbol_table[symbol_table_size].token_type, IDENTIFIER);
                symbol_table[symbol_table_size].line_declared = line_number;
                symbol_table[symbol_table_size].line_used = line_number;
                symbol_table_size++;
            }
            continue;
        }

        // Handle numbers and floats
        if (isdigit(c) || (c == '.' && i + 1 < fsize && isdigit(buffer[i + 1]))) {
            char lexeme[MAX_LEXEME_LENGTH] = {0};
            int j = 0;
            int is_float = 0;

            lexeme[j++] = c;
            i++;
            if (c == '.') {
                is_float = 1;
            }

            while (i < fsize && j < MAX_LEXEME_LENGTH - 1) {
                if (isdigit(buffer[i])) {
                    lexeme[j++] = buffer[i];
                    i++;
                } else if (buffer[i] == '.' && !is_float) {
                    lexeme[j++] = buffer[i];
                    i++;
                    is_float = 1;
                } else {
                    break;
                }
            }
            lexeme[j] = '\0';

            // Validate number format
            if (is_float) {
                if (lexeme[j - 1] == '.') {
                    lexeme[j - 1] = '\0';
                    i--;
                    if (strlen(lexeme) == 0) {
                        fprintf(stderr, "Lexical error at line %d: invalid number '%s'\n", line_number, lexeme);
                        continue;
                    } else {
                        tokens[token_count].line = line_number;
                        strcpy(tokens[token_count].token_type, INTEGER);
                        strcpy(tokens[token_count].lexeme, lexeme);
                        token_count++;

                        symbol_table[symbol_table_size].entry_no = symbol_table_size + 1;
                        strcpy(symbol_table[symbol_table_size].lexeme, lexeme);
                        strcpy(symbol_table[symbol_table_size].token_type, INTEGER);
                        symbol_table[symbol_table_size].line_declared = line_number;
                        symbol_table[symbol_table_size].line_used = line_number;
                        symbol_table_size++;
                    }
                } else {
                    tokens[token_count].line = line_number;
                    strcpy(tokens[token_count].token_type, FLOAT);
                    strcpy(tokens[token_count].lexeme, lexeme);
                    token_count++;

                    symbol_table[symbol_table_size].entry_no = symbol_table_size + 1;
                    strcpy(symbol_table[symbol_table_size].lexeme, lexeme);
                    strcpy(symbol_table[symbol_table_size].token_type, FLOAT);
                    symbol_table[symbol_table_size].line_declared = line_number;
                    symbol_table[symbol_table_size].line_used = line_number;
                    symbol_table_size++;
                }
            } else {
                tokens[token_count].line = line_number;
                strcpy(tokens[token_count].token_type, INTEGER);
                strcpy(tokens[token_count].lexeme, lexeme);
                token_count++;

                symbol_table[symbol_table_size].entry_no = symbol_table_size + 1;
                strcpy(symbol_table[symbol_table_size].lexeme, lexeme);
                strcpy(symbol_table[symbol_table_size].token_type, INTEGER);
                symbol_table[symbol_table_size].line_declared = line_number;
                symbol_table[symbol_table_size].line_used = line_number;
                symbol_table_size++;
            }
            continue;
        }

        // Handle literals (strings)
        if (c == '"') {
            i++;
            int start = i;
            int error = 0;
            while (i < fsize) {
                if (buffer[i] == '"') {
                    break;
                }
                if (buffer[i] == '\n') {
                    line_number++;
                    error = 1;
                    fprintf(stderr, "Lexical error at line %d: newline in string literal\n", line_number - 1);
                    break;
                }
                if (buffer[i] == '\\') {
                    i++;
                    if (i >= fsize) {
                        break;
                    }
                }
                i++;
            }

            if (i >= fsize || buffer[i] != '"') {
                error = 1;
                fprintf(stderr, "Lexical error at line %d: unclosed string literal\n", line_number);
            }

            if (error) {
                if (i < fsize && buffer[i] == '"') {
                    i++;
                }
                continue;
            }

            int len = i - start;
            char lexeme[MAX_LEXEME_LENGTH];
            if (len >= MAX_LEXEME_LENGTH) {
                strncpy(lexeme, buffer + start, MAX_LEXEME_LENGTH - 1);
                lexeme[MAX_LEXEME_LENGTH - 1] = '\0';
            } else {
                strncpy(lexeme, buffer + start, len);
                lexeme[len] = '\0';
            }
            i++;

            tokens[token_count].line = line_number;
            strcpy(tokens[token_count].token_type, LITERAL);
            strcpy(tokens[token_count].lexeme, lexeme);
            token_count++;

            symbol_table[symbol_table_size].entry_no = symbol_table_size + 1;
            strcpy(symbol_table[symbol_table_size].lexeme, lexeme);
            strcpy(symbol_table[symbol_table_size].token_type, LITERAL);
            symbol_table[symbol_table_size].line_declared = line_number;
            symbol_table[symbol_table_size].line_used = line_number;
            symbol_table_size++;
            continue;
        }

        // Handle operators
        if (is_one_char_op(c)) {
            char op[3] = {c, '\0'};
            if (i + 1 < fsize) {
                char two_char[3] = {c, buffer[i + 1], '\0'};
                if (is_two_char_op(two_char)) {
                    strcpy(op, two_char);
                    tokens[token_count].line = line_number;
                    strcpy(tokens[token_count].token_type, OPERATOR);
                    strcpy(tokens[token_count].lexeme, op);
                    token_count++;
                    i += 2;
                    continue;
                }
            }
            tokens[token_count].line = line_number;
            strcpy(tokens[token_count].token_type, OPERATOR);
            strcpy(tokens[token_count].lexeme, op);
            token_count++;
            i++;
            continue;
        }

        // Handle special symbols
        if (is_special_symbol(c)) {
            char sym[2] = {c, '\0'};
            tokens[token_count].line = line_number;
            strcpy(tokens[token_count].token_type, SPECIAL);
            strcpy(tokens[token_count].lexeme, sym);
            token_count++;
            i++;
            continue;
        }

        // Handle unrecognized characters
        fprintf(stderr, "Lexical error at line %d: unrecognized character '%c'\n", line_number, c);
        i++;
    }

    // Print tokens
    printf("Tokens:\n");
    printf("%-10s %-20s %-15s\n", "Line", "Token Type", "Lexeme");
    for (int idx = 0; idx < token_count; idx++) {
        printf("%-10d %-20s %-15s\n", tokens[idx].line, tokens[idx].token_type, tokens[idx].lexeme);
    }

    // Print symbol table
    printf("\nSymbol Table:\n");
    printf("%-10s %-20s %-15s %-15s %-15s\n", "Entry", "Lexeme", "Token Type", "Line Decl", "Line Used");
    for (int idx = 0; idx < symbol_table_size; idx++) {
        printf("%-10d %-20s %-15s %-15d %-15d\n", 
               symbol_table[idx].entry_no, 
               symbol_table[idx].lexeme, 
               symbol_table[idx].token_type, 
               symbol_table[idx].line_declared, 
               symbol_table[idx].line_used);
    }

    free(buffer);
    return 0;
}

// Check if lexeme is a keyword
int is_keyword(const char *lexeme) {
    const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum",
        "extern", "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed",
        "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
    };
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(lexeme, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Check if character is a single-character operator
int is_one_char_op(char c) {
    const char *one_char_ops = "+-*%=<>!&|";
    return (strchr(one_char_ops, c) != NULL);
}

// Check if string is a two-character operator
int is_two_char_op(const char *s) {
    const char *two_char_ops[] = {"++", "--", "==", "!=", "<=", ">=", "&&", "||", "+=", "-=", "*=", "/="};
    int num_ops = sizeof(two_char_ops) / sizeof(two_char_ops[0]);
    for (int i = 0; i < num_ops; i++) {
        if (strcmp(s, two_char_ops[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Check if character is a special symbol
int is_special_symbol(char c) {
    const char *special_symbols = "(){};,";
    return (strchr(special_symbols, c) != NULL);
}