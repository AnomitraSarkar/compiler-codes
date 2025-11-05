
/* lab7.c — Operator Precedence Parser
 *
 * Grammar:
 *   E -> E + E | E - E | E * E | E / E | ( E ) | i
 *
 * Terminals used in the precedence table:
 *   +  -  *  /  i  (  )  $
 *
 * Behavior:
 *   - Prints grammar and the operator precedence table
 *   - Prompts for an input string (use 'i' for id, no '$' needed)
 *   - Traces parsing with columns: STACK | INPUT | ACTION
 *   - Accepts examples: i+i*i  and  (i+i)*i
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAXLEN   1024
#define MAXSTK   2048

static const char SYMS[] = "+-*/i()$";  /* order used in the table, 8 terms */

/* precedence table: relation between terminals ( < , > , = , ' ' ) */
static char prec[8][8];

/* ---- utilities on terminals ------------------------------------------------ */
static int idx_of(char c) {
    const char *p = strchr(SYMS, c);
    return p ? (int)(p - SYMS) : -1;
}
static bool is_terminal(char c) {
    return idx_of(c) >= 0;
}

/* ---- build and print precedence table ------------------------------------- */
static void setrel(char a, char b, char rel) {
    int i = idx_of(a), j = idx_of(b);
    if (i >= 0 && j >= 0) prec[i][j] = rel;
}
static void build_table(void) {
    /* initialize with blanks meaning "no relation" (i.e., error) */
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            prec[i][j] = ' ';

    /* rows mirror the Python dict you had */
    /* + row */
    setrel('+','+','>'); setrel('+','-','>'); setrel('+','*','<'); setrel('+','/','<');
    setrel('+','i','<'); setrel('+','(','<'); setrel('+',')','>'); setrel('+','$','>');
    /* - row */
    setrel('-','+','>'); setrel('-','-','>'); setrel('-','*','<'); setrel('-','/','<');
    setrel('-','i','<'); setrel('-','(','<'); setrel('-',')','>'); setrel('-','$','>');
    /* * row */
    setrel('*','+','>'); setrel('*','-','>'); setrel('*','*','>'); setrel('*','/','>');
    setrel('*','i','<'); setrel('*','(','<'); setrel('*',')','>'); setrel('*','$','>');
    /* / row */
    setrel('/','+','>'); setrel('/','-','>'); setrel('/','*','>'); setrel('/','/','>');
    setrel('/','i','<'); setrel('/','(','<'); setrel('/',')','>'); setrel('/','$','>');
    /* i row */
    setrel('i','+','>'); setrel('i','-','>'); setrel('i','*','>'); setrel('i','/','>');
    setrel('i',')','>'); setrel('i','$','>');
    /* ( row */
    setrel('(','+','<'); setrel('(','-','<'); setrel('(','*','<'); setrel('(','/','<');
    setrel('(','i','<'); setrel('(','(','<'); setrel('(',')','=');
    /* ) row */
    setrel(')','+','>'); setrel(')','-','>'); setrel(')','*','>'); setrel(')','/','>');
    setrel(')',')','>'); setrel(')','$','>');
    /* $ row */
    setrel('$','+','<'); setrel('$','-','<'); setrel('$','*','<'); setrel('$','/','<');
    setrel('$','i','<'); setrel('$','(','<'); setrel('$','$','=');
}

static void print_grammar_and_table(void) {
    puts("Grammar:\n");
    puts("E -> E + E | E - E | E * E | E / E | ( E ) | i\n");

    puts("Operator Precedence Table:");
    /* header */
    printf("    ");
    for (int j = 0; j < 8; ++j) printf(" %c ", SYMS[j]);
    putchar('\n');
    for (int i = 0; i < 8; ++i) {
        printf("%c   ", SYMS[i]);
        for (int j = 0; j < 8; ++j) {
            char r = prec[i][j] ? prec[i][j] : ' ';
            printf(" %c ", r);
        }
        putchar('\n');
    }
}

/* ---- stack helpers --------------------------------------------------------- */
static void push(char *stk, int *top, char c) {
    if (*top + 1 >= MAXSTK) return;
    stk[++(*top)] = c;
}
static char pop(char *stk, int *top) {
    if (*top < 0) return '\0';
    return stk[(*top)--];
}
static void stack_to_string(const char *stk, int top, char *out) {
    for (int i = 0; i <= top; ++i) out[i] = stk[i];
    out[top + 1] = '\0';
}

/* ---- reduction logic ------------------------------------------------------- */
static bool reduce(char *stk, int *top, char *msg) {
    /* E -> i */
    if (*top >= 0 && stk[*top] == 'i') {
        pop(stk, top);
        push(stk, top, 'E');
        strcpy(msg, "Reduced: E->i");
        return true;
    }
    /* E -> (E) */
    if (*top >= 2 && stk[*top] == ')' && stk[*top - 1] == 'E' && stk[*top - 2] == '(') {
        pop(stk, top); pop(stk, top); pop(stk, top);
        push(stk, top, 'E');
        strcpy(msg, "Reduced: E->(E)");
        return true;
    }
    /* E -> E op E  (op in + - * /) */
    if (*top >= 2 && stk[*top] == 'E' && stk[*top - 2] == 'E') {
        char op = stk[*top - 1];
        if (op == '+' || op == '-' || op == '*' || op == '/') {
            pop(stk, top); pop(stk, top); pop(stk, top);
            push(stk, top, 'E');
            sprintf(msg, "Reduced: E->E%cE", op);
            return true;
        }
    }
    return false;
}

/* ---- nearest terminal on the stack ---------------------------------------- */
static char nearest_terminal(const char *stk, int top) {
    for (int i = top; i >= 0; --i) {
        if (is_terminal(stk[i])) return stk[i];
    }
    return '\0';
}

/* ---- pretty print a trace row --------------------------------------------- */
static void print_row(const char *stk, int top, const char *input, int ip, const char *action) {
    char sbuf[MAXSTK + 1], ibuf[MAXLEN + 3];
    stack_to_string(stk, top, sbuf);
    /* remaining input (from ip to end) */
    int k = 0;
    for (int i = ip; input[i] != '\0'; ++i) ibuf[k++] = input[i];
    ibuf[k] = '\0';

    /* match screenshot style loosely with column widths */
    printf("%-16s %-16s %s\n", sbuf, ibuf, action);
}

/* ---- driver: operator precedence parse ------------------------------------ */
static bool parse(const char *expr_raw) {
    /* prepare input: strip spaces, append '$' */
    char input[MAXLEN + 3];
    int n = 0;
    for (int i = 0; expr_raw[i] != '\0'; ++i) {
        if (!isspace((unsigned char)expr_raw[i])) input[n++] = expr_raw[i];
    }
    input[n++] = '$';
    input[n] = '\0';

    /* simple validation: allowed symbols only */
    for (int i = 0; input[i] != '\0'; ++i) {
        if (input[i] == '$') break;
        if (!strchr("+-*/()i", input[i])) {
            fprintf(stderr, "Error: invalid symbol '%c'\n", input[i]);
            return false;
        }
    }

    char stk[MAXSTK];
    int top = -1;
    push(stk, &top, '$');

    int ip = 0; /* index into input */

    puts("\nSTACK\t\tINPUT\t\tACTION");

    for (;;) {
        /* accept state: after shifting final '$', stack should be $E$ and input empty */
        if (input[ip] == '\0') {
            if (top == 2 && stk[0] == '$' && stk[1] == 'E' && stk[2] == '$') {
                printf("\nFinal stack: ");
                char s[MAXSTK + 1]; stack_to_string(stk, top, s);
                puts(s);
                puts("\nString Accepted.");
                return true;
            } else {
                puts("Reject");
                return false;
            }
        }

        /* show state BEFORE deciding the action (as in the Python version) */
        /* We'll print the action text right after deciding; so print a placeholder row later */

        /* nearest terminal on stack */
        char t = nearest_terminal(stk, top);
        char a = input[ip];

        if (!is_terminal(a) || t == '\0') {
            print_row(stk, top, input, ip, "Reject");
            return false;
        }

        char rel = prec[idx_of(t)][idx_of(a)];

        if (rel == '<' || rel == '=') {
            /* shift */
            push(stk, &top, a);
            ++ip;
            print_row(stk, top - 1, input, ip - 1, "Shift");  /* row shows state BEFORE action */
        } else if (rel == '>') {
            char msg[64];
            bool ok = reduce(stk, &top, msg);
            print_row(stk, top, input, ip, ok ? msg : "Reject");
            if (!ok) return false;
        } else {
            print_row(stk, top, input, ip, "Reject");
            return false;
        }
    }
}

/* ---- main ------------------------------------------------------------------ */
int main(void) {
    build_table();
    print_grammar_and_table();

    char line[MAXLEN + 1];
    printf("\nEnter the string (use i for id, no $ needed): ");
    if (!fgets(line, sizeof(line), stdin)) return 0;

    /* trim trailing newline */
    size_t L = strlen(line);
    if (L && line[L - 1] == '\n') line[L - 1] = '\0';

    (void)parse(line);
    return 0;
}
