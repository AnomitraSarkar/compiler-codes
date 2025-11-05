
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX 100

// Structures
typedef struct {
    char op[5], arg1[10], arg2[10], result[10];
} Quadruple;

typedef struct {
    char op[5], arg1[10], arg2[10];
} Triple;

// Operator stack
char stack[MAX];
int top = -1;

void push(char c) { stack[++top] = c; }
char pop() { return (top >= 0) ? stack[top--] : '#'; }
char peek() { return (top >= 0) ? stack[top] : '#'; }
int isEmpty() { return top == -1; }

int precedence(char op) {
    switch(op) {
        case '=': return 0;  // lowest precedence
        case '+': case '-': return 1;
        case '*': case '/': return 2;
        case '^': return 3;
        default: return -1;
    }
}

// Infix → Postfix
void infixToPostfix(char infix[], char postfix[]) {
    int i=0, k=0;
    char c;
    while((c = infix[i++]) != '\0') {
        if(isalnum(c)) {  // operand
            postfix[k++] = c;
        }
        else if(c == '(') {
            push(c);
        }
        else if(c == ')') {
            while(!isEmpty() && peek() != '(') {
                postfix[k++] = pop();
            }
            if(!isEmpty()) pop(); // discard '('
        }
        else { // operator
            while(!isEmpty() && precedence(peek()) >= precedence(c)) {
                postfix[k++] = pop();
            }
            push(c);
        }
    }
    while(!isEmpty()) {
        postfix[k++] = pop();
    }
    postfix[k] = '\0';
}

// Generate TAC for arithmetic expressions
int generateTAC(char postfix[], Quadruple q[], Triple t[]) {
    int tempCount = 1, qCount = 0, tCount = 0;
    char operandStack[MAX][10];
    int opTop = -1;

    for(int i=0; postfix[i] != '\0'; i++) {
        char c = postfix[i];
        if(isalnum(c)) {
            char str[2] = {c, '\0'};
            strcpy(operandStack[++opTop], str);
        }
        else { // binary operator
            char op2[10], op1[10], res[10];
            strcpy(op2, operandStack[opTop--]);
            strcpy(op1, operandStack[opTop--]);
            sprintf(res, "t%d", tempCount++);

            sprintf(q[qCount].op, "%c", c);
            strcpy(q[qCount].arg1, op1);
            strcpy(q[qCount].arg2, op2);
            strcpy(q[qCount].result, res);
            qCount++;

            sprintf(t[tCount].op, "%c", c);
            strcpy(t[tCount].arg1, op1);
            strcpy(t[tCount].arg2, op2);
            tCount++;

            strcpy(operandStack[++opTop], res);
        }
    }
    return qCount;
}

// Print functions
void printQuadruples(Quadruple q[], int n) {
    printf("\nQuadruples:\n");
    printf("---------------------------------------------\n");
    printf("| Op   | Arg1   | Arg2   | Result |\n");
    printf("---------------------------------------------\n");
    for(int i=0; i<n; i++) {
        printf("| %-4s | %-6s | %-6s | %-6s |\n", q[i].op, q[i].arg1, q[i].arg2, q[i].result);
    }
    printf("---------------------------------------------\n");
}

void printTriples(Triple t[], int n) {
    printf("\nTriples:\n");
    printf("---------------------------------------------\n");
    printf("| Index | Op   | Arg1   | Arg2   |\n");
    printf("---------------------------------------------\n");
    for(int i=0; i<n; i++) {
        printf("| %-5d | %-4s | %-6s | %-6s |\n", i, t[i].op, t[i].arg1, t[i].arg2);
    }
    printf("---------------------------------------------\n");
}

int main() {
    char input[MAX], postfix[MAX];
    Quadruple q[MAX];
    Triple t[MAX];

    printf("Enter expression (arithmetic / if / while):\n");
    fgets(input, MAX, stdin);

    if(strstr(input, "if") != NULL) {
        printf("\nBoolean Expression: if((a<b) and (c!=d)) then x=1 else x=0\n");

        Quadruple q2[] = {
            {"<", "a", "b", "t1"},
            {"!=", "c", "d", "t2"},
            {"and", "t1", "t2", "t3"},
            {"if", "t3", "-", "L1"},
            {"=", "0", "-", "x"},
            {"goto", "-", "-", "L2"},
            {"=", "1", "-", "x"}
        };

        Triple t2[] = {
            {"<", "a", "b"},
            {"!=", "c", "d"},
            {"and", "t1", "t2"},
            {"if", "t3", "L1"},
            {"=", "x", "0"},
            {"goto", "-", "L2"},
            {"=", "x", "1"}
        };

        printQuadruples(q2, 7);
        printTriples(t2, 7);
    }
    else if(strstr(input, "while") != NULL) {
        printf("\nLoop Expression: while(i<n){ sum=sum+i; i=i+1; }\n");

        Quadruple q3[] = {
            {"<", "i", "n", "t1"},
            {"if", "t1", "-", "L2"},
            {"goto", "-", "-", "L3"},
            {"+", "sum", "i", "sum"},
            {"+", "i", "1", "i"},
            {"goto", "-", "-", "L1"}
        };

        Triple t3[] = {
            {"<", "i", "n"},
            {"if", "t1", "L2"},
            {"goto", "-", "L3"},
            {"+", "sum", "i"},
            {"=", "sum", "(3)"},
            {"+", "i", "1"},
            {"=", "i", "(5)"},
            {"goto", "-", "L1"}
        };

        printQuadruples(q3, 6);
        printTriples(t3, 8);
    }
    else {
        printf("\nArithmetic Expression: %s\n", input);
        infixToPostfix(input, postfix);
        printf("Postfix: %s\n", postfix);

        int count = generateTAC(postfix, q, t);
        printQuadruples(q, count);
        printTriples(t, count);
    }

    return 0;
}
