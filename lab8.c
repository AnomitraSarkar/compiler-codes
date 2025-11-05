#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 100
#define STATES 20
#define SYMBOLS 10

// Grammar rules
typedef struct {
    char lhs;
    char rhs[MAX];
} Production;

// Item structure
typedef struct {
    char lhs;
    char rhs[MAX];
    int dot_position;
} Item;

// State structure
typedef struct {
    Item items[MAX];
    int item_count;
} State;

// Stack structure for parsing
typedef struct {
    int items[MAX];
    int top;
} Stack;

// Global variables
Production grammar[MAX];
int prod_count = 0;
State states[STATES];
int state_count = 0;
char terminals[SYMBOLS];
int term_count = 0;
char non_terminals[SYMBOLS];
int non_term_count = 0;

// ACTION and GOTO tables
char action[STATES][SYMBOLS][10];
int goto_table[STATES][SYMBOLS];

// ---------- Utility functions ----------
int item_exists(State *state, Item item) {
    for (int i = 0; i < state->item_count; i++) {
        if (state->items[i].lhs == item.lhs &&
            strcmp(state->items[i].rhs, item.rhs) == 0 &&
            state->items[i].dot_position == item.dot_position) {
            return 1;
        }
    }
    return 0;
}

void add_item(State *state, Item item) {
    if (!item_exists(state, item)) {
        state->items[state->item_count++] = item;
    }
}

int symbol_index(char sym, char arr[], int count) {
    for (int i = 0; i < count; i++) {
        if (arr[i] == sym) return i;
    }
    return -1;
}

// ---------- Printing ----------
void print_state(int index) {
    printf("State %d:\n", index);
    for (int i = 0; i < states[index].item_count; i++) {
        Item it = states[index].items[i];

        // print lhs (handle S')
        if (it.lhs == 'Q')
            printf("  S' -> ");
        else
            printf("  %c -> ", it.lhs);

        for (int j = 0; j < strlen(it.rhs); j++) {
            if (j == it.dot_position) printf(".");
            printf("%c", it.rhs[j]);
        }
        if (it.dot_position == strlen(it.rhs)) printf(".");
        printf("\n");
    }
    printf("\n");
}

// ---------- Closure & GOTO ----------
void closure(State *state) {
    int added;
    do {
        added = 0;
        for (int i = 0; i < state->item_count; i++) {
            Item it = states[state - states].items[i]; // pointer offset
            if (it.dot_position < strlen(it.rhs)) {
                char next_symbol = it.rhs[it.dot_position];
                if (symbol_index(next_symbol, non_terminals, non_term_count) != -1) {
                    for (int j = 0; j < prod_count; j++) {
                        if (grammar[j].lhs == next_symbol) {
                            Item new_item = { grammar[j].lhs, "", 0 };
                            strcpy(new_item.rhs, grammar[j].rhs);
                            if (!item_exists(state, new_item)) {
                                add_item(state, new_item);
                                added = 1;
                            }
                        }
                    }
                }
            }
        }
    } while (added);
}

State goto_func(State *state, char symbol) {
    State new_state;
    new_state.item_count = 0;

    for (int i = 0; i < state->item_count; i++) {
        Item it = state->items[i];
        if (it.dot_position < strlen(it.rhs) && it.rhs[it.dot_position] == symbol) {
            Item moved_item = it;
            moved_item.dot_position++;
            add_item(&new_state, moved_item);
        }
    }
    closure(&new_state);
    return new_state;
}

int states_equal(State *s1, State *s2) {
    if (s1->item_count != s2->item_count) return 0;
    for (int i = 0; i < s1->item_count; i++) {
        Item it1 = s1->items[i];
        int found = 0;
        for (int j = 0; j < s2->item_count; j++) {
            Item it2 = s2->items[j];
            if (it1.lhs == it2.lhs &&
                strcmp(it1.rhs, it2.rhs) == 0 &&
                it1.dot_position == it2.dot_position) {
                found = 1;
                break;
            }
        }
        if (!found) return 0;
    }
    return 1;
}

int state_index(State *new_state) {
    for (int i = 0; i < state_count; i++) {
        if (states_equal(&states[i], new_state)) {
            return i;
        }
    }
    return -1;
}

// ---------- Build states ----------
void build_states() {
    state_count = 0;
    states[0].item_count = 0;

    // Augmented grammar: S' -> S
    Item start_item = { 'Q', "S", 0 };
    add_item(&states[0], start_item);
    closure(&states[0]);
    state_count++;

    int front = 0;
    printf("\nDFA of Item Sets (Transitions):\n");
    while (front < state_count) {
        State curr = states[front];
        char symbols[SYMBOLS];
        int sym_count = 0;

        for (int i = 0; i < curr.item_count; i++) {
            Item it = curr.items[i];
            if (it.dot_position < strlen(it.rhs)) {
                char sym = it.rhs[it.dot_position];
                if (symbol_index(sym, symbols, sym_count) == -1) {
                    symbols[sym_count++] = sym;
                }
            }
        }

        for (int i = 0; i < sym_count; i++) {
            State new_state = goto_func(&curr, symbols[i]);
            int idx = state_index(&new_state);
            if (idx == -1) {
                idx = state_count;
                states[state_count++] = new_state;
            }

            // Print DFA transition
            if (symbols[i] == 'Q')
                printf("I%d --S'--> I%d\n", front, idx);
            else
                printf("I%d --%c--> I%d\n", front, symbols[i], idx);

            if (symbol_index(symbols[i], terminals, term_count) != -1) {
                int t_idx = symbol_index(symbols[i], terminals, term_count);
                strcpy(action[front][t_idx], "s");
                char buffer[10];
                sprintf(buffer, "%d", idx);
                strcat(action[front][t_idx], buffer);
            } else {
                int nt_idx = symbol_index(symbols[i], non_terminals, non_term_count);
                goto_table[front][nt_idx] = idx;
            }
        }
        front++;
    }
}

// ---------- Parsing Table ----------
void build_parsing_table() {
    for (int i = 0; i < STATES; i++) {
        for (int j = 0; j < SYMBOLS; j++) {
            strcpy(action[i][j], "error");
            goto_table[i][j] = -1;
        }
    }

    build_states();

    for (int i = 0; i < state_count; i++) {
        for (int j = 0; j < states[i].item_count; j++) {
            Item it = states[i].items[j];
            if (it.dot_position == strlen(it.rhs)) {
                if (it.lhs == 'Q' && strcmp(it.rhs, "S") == 0) {
                    int idx = symbol_index('$', terminals, term_count);
                    strcpy(action[i][idx], "acc");
                } else {
                    for (int t = 0; t < term_count; t++) {
                        char buffer[10];
                        int prod_idx = -1;
                        for (int k = 0; k < prod_count; k++) {
                            if (grammar[k].lhs == it.lhs && strcmp(grammar[k].rhs, it.rhs) == 0) {
                                prod_idx = k;
                                break;
                            }
                        }
                        if (prod_idx != -1) {
                            sprintf(buffer, "r%d", prod_idx);
                            strcpy(action[i][t], buffer);
                        }
                    }
                }
            }
        }
    }
}

void print_parsing_table() {
    printf("\nACTION and GOTO Table:\n");
    printf("State\t");
    for (int i = 0; i < term_count; i++) printf("%c\t", terminals[i]);
    for (int i = 0; i < non_term_count; i++) printf("%c\t", non_terminals[i]);
    printf("\n");

    for (int i = 0; i < state_count; i++) {
        printf("%d\t", i);
        for (int j = 0; j < term_count; j++) {
            printf("%s\t", action[i][j]);
        }
        for (int j = 0; j < non_term_count; j++) {
            if (goto_table[i][j] != -1)
                printf("%d\t", goto_table[i][j]);
            else
                printf("-\t");
        }
        printf("\n");
    }
}

// ---------- Parsing ----------
void parse_input(char input_str[]) {
    Stack state_stack = { .top = -1 };
    Stack symbol_stack = { .top = -1 };

    state_stack.items[++state_stack.top] = 0;
    symbol_stack.items[++symbol_stack.top] = '$';

    char buffer[MAX];
    sprintf(buffer, "%s$", input_str);
    int ip = 0;

    printf("\nParsing Trace:\n");
    printf("Stack\tInput\tAction\n");

    while (1) {
        int state = state_stack.items[state_stack.top];
        char lookahead = buffer[ip];
        int term_idx = symbol_index(lookahead, terminals, term_count);

        printf("[");
        for (int i = 0; i <= state_stack.top; i++) {
            printf("%d ", state_stack.items[i]);
        }
        printf("]\t%s\t", &buffer[ip]);

        if (term_idx == -1 || strcmp(action[state][term_idx], "error") == 0) {
            printf("Error\n");
            break;
        }

        if (strcmp(action[state][term_idx], "acc") == 0) {
            printf("Accept\n");
            break;
        }

        if (action[state][term_idx][0] == 's') {
            int next_state = atoi(&action[state][term_idx][1]);
            printf("Shift %c\n", lookahead);
            state_stack.items[++state_stack.top] = next_state;
            symbol_stack.items[++symbol_stack.top] = lookahead;
            ip++;
        } else if (action[state][term_idx][0] == 'r') {
            int prod_idx = atoi(&action[state][term_idx][1]);
            Production p = grammar[prod_idx];
            if (p.lhs == 'Q')
                printf("Reduce by S' -> %s\n", p.rhs);
            else
                printf("Reduce by %c -> %s\n", p.lhs, p.rhs);

            int rhs_len = strlen(p.rhs);
            state_stack.top -= rhs_len;
            symbol_stack.top -= rhs_len;

            state = state_stack.items[state_stack.top];
            int nt_idx = symbol_index(p.lhs, non_terminals, non_term_count);

            symbol_stack.items[++symbol_stack.top] = p.lhs;
            state_stack.items[++state_stack.top] = goto_table[state][nt_idx];
        }
    }
}

int main() {
    // Define grammar rules (excluding augmented one)
    prod_count = 0;
    grammar[prod_count].lhs = 'S';
    strcpy(grammar[prod_count++].rhs, "CC");
    grammar[prod_count].lhs = 'C';
    strcpy(grammar[prod_count++].rhs, "cC");
    grammar[prod_count].lhs = 'C';
    strcpy(grammar[prod_count++].rhs, "d");

    // Augmented production: S' -> S (internally Q -> S)
    grammar[prod_count].lhs = 'Q';
    strcpy(grammar[prod_count++].rhs, "S");

    // Terminals and non-terminals
    terminals[0] = 'c';
    terminals[1] = 'd';
    terminals[2] = '$';
    term_count = 3;

    non_terminals[0] = 'S';
    non_terminals[1] = 'C';
    non_terminals[2] = 'Q';  // internal S'
    non_term_count = 3;

    // Build parsing table
    build_parsing_table();

    // Print canonical collection
    printf("\nCanonical Collection of LR(0) Items:\n");
    for (int i = 0; i < state_count; i++) {
        print_state(i);
    }

    // Print parsing table
    print_parsing_table();

    // Input string
    char input_str[MAX];
    printf("\nEnter input string (e.g. ccdd): ");
    scanf("%s", input_str);

    // Parse the input
    parse_input(input_str);

    return 0;
}
