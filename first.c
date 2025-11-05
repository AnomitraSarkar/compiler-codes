#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD 100

// List of keywords
const char* keywords[] = {
    "int", "float", "if", "else", "while", "for", "return",
    "void", "char", "double", "long", "short", "switch", "case",
    "break", "continue", "default", "do", "struct", "typedef", "const"
};
int num_keywords = sizeof(keywords) / sizeof(keywords[0]);

// Check if a word is a keyword
int isKeyword(const char* word) {
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(word, keywords[i]) == 0) return 1;
    }
    return 0;
}

// Check if a character is an operator
int isOperator(char ch) {
    return (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '=' || ch == '<' || ch == '>' || ch == '%');
}

// Check if a character is a special symbol
int isSpecialSymbol(char ch) {
    return (ch == ';' || ch == '{' || ch == '}' || ch == '(' || ch == ')' || ch == ',' || ch == '[' || ch == ']');
}

int main() {
    char ch, word[MAX_WORD];
    int i = 0;
    int keywordCount = 0, identifierCount = 0, constantCount = 0, operatorCount = 0, specialCount = 0;

    FILE* fp = fopen("input.c", "r");
    if (!fp) {
        printf("Error opening file!\n");
        return 1;
    }

    while ((ch = fgetc(fp)) != EOF) {
        if (isalnum(ch) || ch == '_') {
            word[i++] = ch;
        } else {
            word[i] = '\0';

            if (i != 0) {
                if (isKeyword(word)) {
                    keywordCount++;
                } else if (isdigit(word[0])) {
                    constantCount++;
                } else {
                    identifierCount++;
                }
                i = 0; // reset word
            }

            // Count operator
            if (isOperator(ch)) operatorCount++;

            // Count special symbol
            if (isSpecialSymbol(ch)) specialCount++;
        }
    }

    fclose(fp);

    // Output
    printf("Keyword count     : %d\n", keywordCount);
    printf("Identifier count  : %d\n", identifierCount);
    printf("Constant count    : %d\n", constantCount);
    printf("Operator count    : %d\n", operatorCount);
    printf("Special symbol count : %d\n", specialCount);

    return 0;
}
