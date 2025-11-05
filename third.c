#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEXEME 100
#define MAX_ENTRIES 1000

typedef struct
{
    char lexeme[MAX_LEXEME];
    char tokenType[20];
    int declaredLine;
    int usedLines[100];
    int useCount;
} Symbol;

Symbol symbolTable[MAX_ENTRIES];
int symbolCount = 0;

const char *keywords[] = {
    "int", "float", "char", "return", "void", "if", "else", "while", "for", "do", "switch", "case", "break", "continue"};
const char *operators = "+-*/%=<>&|!^";
const char *specialSymbols = "(){},;";

int isKeyword(const char *word)
{
    for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        if (strcmp(word, keywords[i]) == 0)
            return 1;
    }
    return 0;
}

int isOperator(char ch)
{
    return strchr(operators, ch) != NULL;
}

int isSpecialSymbol(char ch)
{
    return strchr(specialSymbols, ch) != NULL;
}

int addToSymbolTable(const char *lexeme, const char *tokenType, int line, int isDeclaration)
{
    for (int i = 0; i < symbolCount; i++)
    {
        if (strcmp(symbolTable[i].lexeme, lexeme) == 0)
        {
            symbolTable[i].usedLines[symbolTable[i].useCount++] = line;
            return i;
        }
    }

    strcpy(symbolTable[symbolCount].lexeme, lexeme);
    strcpy(symbolTable[symbolCount].tokenType, tokenType);
    symbolTable[symbolCount].declaredLine = isDeclaration ? line : -1;
    symbolTable[symbolCount].usedLines[0] = line;
    symbolTable[symbolCount].useCount = 1;
    return symbolCount++;
}

void removeComments(char *buffer)
{
    char *src = buffer;
    char *dst = buffer;

    while (*src)
    {
        if (*src == '/' && *(src + 1) == '/')
        {
            while (*src && *src != '\n')
                src++;
        }
        else if (*src == '/' && *(src + 1) == '*')
        {
            src += 2;
            while (*src && !(*src == '*' && *(src + 1) == '/'))
                src++;
            if (*src)
                src += 2;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void analyze(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Error opening input file");
        exit(1);
    }

    char buffer[10000];
    fread(buffer, sizeof(char), sizeof(buffer), file);
    fclose(file);

    removeComments(buffer);

    int i = 0, line = 1;
    while (buffer[i])
    {
        if (isspace(buffer[i]))
        {
            if (buffer[i] == '\n')
                line++;
            i++;
            continue;
        }

        if (isalpha(buffer[i]) || buffer[i] == '_')
        {
            char temp[MAX_LEXEME];
            int j = 0;
            while (isalnum(buffer[i]) || buffer[i] == '_')
            {
                temp[j++] = buffer[i++];
            }
            temp[j] = '\0';

            if (isKeyword(temp))
            {
                printf("[KEYWORD: %s] at line %d\n", temp, line);
            }
            else
            {
                printf("[IDENTIFIER: %s] at line %d\n", temp, line);
                addToSymbolTable(temp, "IDENTIFIER", line, 1);
            }
        }
        else if (isdigit(buffer[i]))
        {
            char temp[MAX_LEXEME];
            int j = 0;
            int hasDecimal = 0;
            while (isdigit(buffer[i]) || buffer[i] == '.')
            {
                if (buffer[i] == '.')
                {
                    if (hasDecimal++)
                        break; // error: more than 1 dot
                }
                temp[j++] = buffer[i++];
            }
            temp[j] = '\0';
            if (hasDecimal)
                printf("[FLOAT: %s] at line %d\n", temp, line);
            else
                printf("[INTEGER: %s] at line %d\n", temp, line);

            addToSymbolTable(temp, hasDecimal ? "FLOAT" : "INTEGER", line, 0);
        }
        else if (buffer[i] == '"')
        {
            i++;
            char temp[MAX_LEXEME];
            int j = 0;
            while (buffer[i] && buffer[i] != '"')
            {
                temp[j++] = buffer[i++];
            }
            temp[j] = '\0';
            if (buffer[i] == '"')
            {
                i++;
                printf("[LITERAL: \"%s\"] at line %d\n", temp, line);
                addToSymbolTable(temp, "LITERAL", line, 0);
            }
            else
            {
                printf("LEXICAL ERROR: Unterminated string at line %d\n", line);
            }
        }
        else if (buffer[i] == '\'')
        {
            i++;
            char temp[MAX_LEXEME];
            int j = 0;

            if (buffer[i] && buffer[i + 1] == '\'')
            {
                temp[j++] = buffer[i++];
                i++; // skip closing '
                temp[j] = '\0';
                printf("[CHAR_LITERAL: '%s'] at line %d\n", temp, line);
                addToSymbolTable(temp, "CHAR_LITERAL", line, 0);
            }
            else if (buffer[i] == '\\' && buffer[i + 2] == '\'')
            {
                temp[j++] = buffer[i++];
                temp[j++] = buffer[i++];
                i++; // skip closing '
                temp[j] = '\0';
                printf("[CHAR_LITERAL: '%s'] at line %d\n", temp, line);
                addToSymbolTable(temp, "CHAR_LITERAL", line, 0);
            }
            else
            {
                printf("LEXICAL ERROR: Invalid char literal at line %d\n", line);
                while (buffer[i] && buffer[i] != '\'')
                    i++;
                if (buffer[i] == '\'')
                    i++;
            }
        }
        else if (isOperator(buffer[i]))
        {
            printf("[OPERATOR: %c] at line %d\n", buffer[i], line);
            i++;
        }
        else if (isSpecialSymbol(buffer[i]))
        {
            printf("[SYMBOL: %c] at line %d\n", buffer[i], line);
            i++;
        }
        else
        {
            printf("LEXICAL ERROR: Unrecognized token '%c' at line %d\n", buffer[i], line);
            i++;
        }
    }

    printf("\n=== SYMBOL TABLE ===\n");
    printf("Entry No.\tLexeme\t\tToken Type\t\tLine No. Declared\tLine No. Used\n");
    for (int i = 0; i < symbolCount; i++)
    {
        printf("%d\t\t%-10s\t\t%-10s\t\t%d\t\t", i + 1, symbolTable[i].lexeme,
               symbolTable[i].tokenType,
               symbolTable[i].declaredLine == -1 ? 0 : symbolTable[i].declaredLine);
        for (int j = 0; j < symbolTable[i].useCount; j++)
        {
            printf("%d ", symbolTable[i].usedLines[j]);
        }
        printf("\n");
    }
}

int main()
{
    analyze("test.c");
    return 0;
}
