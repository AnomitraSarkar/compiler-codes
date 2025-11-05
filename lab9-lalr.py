# Corrected and runnable LALR parser implementation with fixes and a demonstration run.
# This will execute and display the parser's LR(1) collection, DFA transitions, parsing table, and parsing trace.

import pandas as pd
from collections import defaultdict

class LALRParser:
    """
    Corrected LALR parser generator and driver.
    Fixes applied:
    - Corrected constructor name from _init_ to __init__.
    - Properly build terminals after collecting non-terminals.
    - Handle empty/action cell values safely when parsing.
    - Ensure FIRST set initialization and end-marker '$' handled.
    - Use consistent tuple bodies for productions.
    - Robust handling of goto and reduction actions.
    """

    def __init__(self, grammar_rules, start_symbol):
        """Initializes the parser with the given grammar."""
        self.grammar = defaultdict(list)
        for head, body in grammar_rules:
            # treat epsilon as empty tuple
            if body.strip() == 'ε' or body.strip() == '':
                body_tuple = tuple()
            else:
                body_tuple = tuple(body.split())
            self.grammar[head].append(body_tuple)

        self.start_symbol = start_symbol
        # collect non-terminals first
        self.non_terminals = set(self.grammar.keys())

        # find terminals (symbols that are not non-terminals)
        self.terminals = set()
        for head in self.grammar:
            for production in self.grammar[head]:
                for symbol in production:
                    if symbol not in self.non_terminals:
                        self.terminals.add(symbol)
        # remove epsilon if present
        self.terminals.discard('ε')

        # augmented grammar
        self.augmented_start_symbol = f"{self.start_symbol}'"
        self.augmented_grammar = [(self.augmented_start_symbol, (self.start_symbol,))]
        for head, bodies in self.grammar.items():
            for body in bodies:
                self.augmented_grammar.append((head, body))

        # tables and collections
        self.lr1_items_collection = []
        self.lr1_goto = {}
        self.lalr_states = []
        self.lalr_goto = {}
        self.action_table = None
        self.goto_table = None

    def _compute_first_sets(self):
        """Computes the FIRST sets for all symbols in the grammar."""
        self.first_sets = defaultdict(set)

        # end marker
        self.first_sets['$'] = {'$'}

        # terminals
        for t in self.terminals:
            self.first_sets[t] = {t}

        changed = True
        while changed:
            changed = False
            for head, productions in self.grammar.items():
                for prod in productions:
                    first_of_prod = self._first_of_sequence(prod)
                    initial_len = len(self.first_sets[head])
                    self.first_sets[head].update(first_of_prod - {'ε'})
                    if len(self.first_sets[head]) > initial_len:
                        changed = True

    def _first_of_sequence(self, sequence):
        """Helper to find the FIRST set of a sequence of symbols."""
        first = set()
        if not sequence:
            first.add('ε')
            return first

        for symbol in sequence:
            symbol_first = self.first_sets.get(symbol, set())
            # if symbol is unknown (like a nonterminal not yet in first_sets), treat as empty set for now
            first.update(symbol_first - {'ε'})
            if 'ε' not in symbol_first:
                break
        else:
            first.add('ε')
        return first

    def _closure(self, items):
        """Computes the closure of a set of LR(1) items."""
        closure_set = set(items)
        worklist = list(items)

        while worklist:
            item = worklist.pop(0)
            prod_head, prod_body, dot_pos, lookahead = item

            if dot_pos < len(prod_body):
                next_symbol = prod_body[dot_pos]
                if next_symbol in self.non_terminals:
                    remaining_seq = prod_body[dot_pos+1:]
                    lookahead_seq = remaining_seq + (lookahead,)
                    first_of_lookahead = self._first_of_sequence(lookahead_seq)

                    for p_head, p_body in self.augmented_grammar:
                        if p_head == next_symbol:
                            for new_lookahead in first_of_lookahead:
                                new_item = (p_head, p_body, 0, new_lookahead)
                                if new_item not in closure_set:
                                    closure_set.add(new_item)
                                    worklist.append(new_item)
        return frozenset(closure_set)

    def _goto(self, item_set, symbol):
        """Computes the GOTO set for a given item set and symbol."""
        new_set = set()
        for item in item_set:
            prod_head, prod_body, dot_pos, lookahead = item
            if dot_pos < len(prod_body) and prod_body[dot_pos] == symbol:
                new_set.add((prod_head, prod_body, dot_pos + 1, lookahead))
        return self._closure(new_set) if new_set else frozenset()

    def _build_lr1_collection(self):
        """Builds the canonical collection of LR(1) item sets (the DFA)."""
        start_prod_head, start_prod_body = self.augmented_grammar[0]
        initial_item = (start_prod_head, start_prod_body, 0, '$')
        initial_state = self._closure({initial_item})

        self.lr1_items_collection = [initial_state]
        worklist = [initial_state]
        state_map = {initial_state: 0}
        self.lr1_goto = {}

        all_symbols = list(self.non_terminals) + list(self.terminals)

        while worklist:
            current_state_items = worklist.pop(0)
            current_state_idx = state_map[current_state_items]

            for symbol in all_symbols:
                next_state_items = self._goto(current_state_items, symbol)
                if next_state_items and len(next_state_items) > 0:
                    if next_state_items not in state_map:
                        state_map[next_state_items] = len(self.lr1_items_collection)
                        self.lr1_items_collection.append(next_state_items)
                        worklist.append(next_state_items)

                    next_state_idx = state_map[next_state_items]
                    self.lr1_goto[(current_state_idx, symbol)] = next_state_idx

    def _build_lalr_states(self):
        """Merges LR(1) states to form LALR states based on common cores."""
        core_map = defaultdict(list)
        for i, item_set in enumerate(self.lr1_items_collection):
            core = frozenset((p[0], p[1], p[2]) for p in item_set)
            core_map[core].append(i)

        lalr_state_map = {}
        self.lalr_states = []
        for core, lr1_indices in core_map.items():
            new_lalr_state = set()
            for lr1_idx in lr1_indices:
                new_lalr_state.update(self.lr1_items_collection[lr1_idx])

            lalr_idx = len(self.lalr_states)
            self.lalr_states.append(frozenset(new_lalr_state))
            for lr1_idx in lr1_indices:
                lalr_state_map[lr1_idx] = lalr_idx

        for (lr1_state_idx, symbol), next_lr1_state_idx in self.lr1_goto.items():
            lalr_state_idx = lalr_state_map[lr1_state_idx]
            next_lalr_state_idx = lalr_state_map[next_lr1_state_idx]
            self.lalr_goto[(lalr_state_idx, symbol)] = next_lalr_state_idx

    def _build_parsing_table(self):
        """Constructs the LALR ACTION and GOTO tables."""
        action_columns = sorted(self.terminals) + ['$']
        goto_columns = sorted(self.non_terminals - {self.augmented_start_symbol})

        self.action_table = pd.DataFrame(index=range(len(self.lalr_states)), columns=action_columns).fillna('')
        self.goto_table = pd.DataFrame(index=range(len(self.lalr_states)), columns=goto_columns).fillna('')

        rev_prod_map = {(p[0], p[1]): i for i, p in enumerate(self.augmented_grammar)}

        for i, state in enumerate(self.lalr_states):
            for item in state:
                head, body, dot_pos, lookahead = item

                if dot_pos < len(body):
                    symbol = body[dot_pos]
                    if symbol in self.terminals:
                        next_state_idx = self.lalr_goto.get((i, symbol))
                        if next_state_idx is not None:
                            action = f's{next_state_idx}'
                            self.action_table.at[i, symbol] = action
                else:
                    if head == self.augmented_start_symbol:
                        if lookahead == '$':
                            self.action_table.at[i, '$'] = 'accept'
                    else:
                        prod_num = rev_prod_map.get((head, body))
                        if prod_num is not None:
                            action = f'r{prod_num}'
                            # Only set reduce for the lookahead column if empty (naive conflict handling)
                            existing = self.action_table.at[i, lookahead] if lookahead in self.action_table.columns else ''
                            if not existing:
                                self.action_table.at[i, lookahead] = action

        for (state_idx, symbol), next_state_idx in self.lalr_goto.items():
            if symbol in self.goto_table.columns:
                self.goto_table.at[state_idx, symbol] = next_state_idx

    def generate(self):
        """Generates all parser components."""
        self._compute_first_sets()
        self._build_lr1_collection()
        self._build_lalr_states()
        self._build_parsing_table()

    def parse(self, input_string):
        """Parses the input string using the generated LALR table."""
        tokens = list(input_string) + ['$']
        stack = [0]
        pointer = 0
        trace = []

        while True:
            state = stack[-1]
            symbol = tokens[pointer]

            if symbol not in self.action_table.columns:
                trace.append((' '.join(map(str, stack)), ''.join(tokens[pointer:]), f'Error: Invalid symbol \"{symbol}\"'))
                break

            action = self.action_table.at[state, symbol]
            if action is None or action == '' or (isinstance(action, float) and pd.isna(action)):
                # no action defined -> syntax error
                trace.append((
                    ' '.join(map(str, stack)),
                    ''.join(tokens[pointer:]),
                    'Error: Invalid Syntax'
                ))
                break

            action = str(action)
            trace.append((
                ' '.join(map(str, stack)),
                ''.join(tokens[pointer:]),
                self._get_action_description(action)
            ))

            if action.startswith('s'):
                next_state = int(action[1:])
                stack.append(symbol)
                stack.append(next_state)
                pointer += 1
            elif action.startswith('r'):
                prod_num = int(action[1:])
                head, body = self.augmented_grammar[prod_num]

                # pop 2 * len(body) symbols (symbol + state) from stack
                for _ in range(2 * len(body)):
                    stack.pop()

                prev_state = stack[-1]
                goto_state = self.goto_table.at[prev_state, head] if head in self.goto_table.columns else self.goto_table.at[prev_state, head] if head in self.goto_table.columns else ''
                # try to coerce goto_state to int
                if goto_state == '' or pd.isna(goto_state):
                    trace.append((' '.join(map(str, stack)), ''.join(tokens[pointer:]), f'Error: No GOTO for {head} from state {prev_state}'))
                    break
                goto_state = int(goto_state)
                stack.append(head)
                stack.append(goto_state)
            elif action == 'accept':
                trace.append((' '.join(map(str, stack)), ''.join(tokens[pointer:]), 'Accept'))
                break
            else:
                trace.append((' '.join(map(str, stack)), ''.join(tokens[pointer:]), 'Error: Invalid Syntax'))
                break

        return trace

    def _get_action_description(self, action):
        """Provides a human-readable description of a parser action."""
        if action.startswith('s'):
            return f"Shift {action[1:]}"
        if action.startswith('r'):
            prod_num = int(action[1:])
            head, body = self.augmented_grammar[prod_num]
            body_str = ' '.join(body) if body else 'ε'
            return f"Reduce by {head} -> {body_str}"
        if action == 'accept':
            return "Accept"
        return "Error"

    def print_lr1_collection(self):
        """Prints the canonical collection of LR(1) items."""
        print("## Canonical Collection of LR(1) Items")
        for i, item_set in enumerate(self.lr1_items_collection):
            print(f"--- I{i} ---")
            for item in sorted(list(item_set)):
                head, body, dot, la = item
                left = ' '.join(body[:dot])
                right = ' '.join(body[dot:])
                print(f"  {head} -> {left} . {right}, {la}")
        print("\n" + "="*80 + "\n")

    def print_dfa_transitions(self):
        """Prints the DFA transitions (GOTO graph)."""
        print("## DFA of Item Sets (State Transitions)")
        for (state, symbol), next_state in sorted(self.lr1_goto.items()):
            print(f"  goto(I{state}, {symbol}) = I{next_state}")
        print("\n" + "="*80 + "\n")

    def print_parsing_table(self):
        """Prints the final LALR parsing table."""
        print("## LALR Parsing Table")
        full_table = pd.concat([self.action_table, self.goto_table], axis=1)
        print(full_table.to_string())
        print("\n" + "="*80 + "\n")

    def print_parsing_trace(self, trace):
        """Prints the parsing trace in a formatted table."""
        print("## Parsing Trace for Input String")
        trace_df = pd.DataFrame(trace, columns=["Stack", "Input Buffer", "Action"])
        trace_df.index.name = "Step"
        trace_df.index += 1
        print(trace_df.to_string())
        print("\n" + "="*80 + "\n")


# --- Main Execution Demo ---
if __name__ == "__main__":
    grammar_spec = [
        ('S', "C C"),
        ('C', "c C"),
        ('C', "d"),
    ]
    start_symbol = 'S'
    input_str = "ccdd"
    parser = LALRParser(grammar_spec, start_symbol)
    parser.generate()
    parser.print_lr1_collection()
    parser.print_dfa_transitions()
    parser.print_parsing_table()
    parsing_trace = parser.parse(input_str)
    parser.print_parsing_trace(parsing_trace)

# Run the demo immediately so the user sees the output in this notebook.
parser_demo_output = None

