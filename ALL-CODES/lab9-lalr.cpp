
#include <bits/stdc++.h>
using namespace std;

struct Item {
    string head;
    vector<string> body;
    int dot;
    string lookahead;

    bool operator<(const Item &o) const {
        if (head != o.head) return head < o.head;
        if (body != o.body) return body < o.body;
        if (dot != o.dot) return dot < o.dot;
        return lookahead < o.lookahead;
    }
};

string joinVec(const vector<string>& v, const string& sep=" ") {
    string s;
    for (size_t i=0;i<v.size();++i) {
        s += v[i];
        if (i+1 < v.size()) s += sep;
    }
    return s;
}

string serializeItem(const Item &it) {
    // deterministic serialization
    string s = it.head + "->";
    for (const auto &t : it.body) {
        s += t + " ";
    }
    s += "|" + to_string(it.dot) + "|" + it.lookahead;
    return s;
}

string serializeItemSet(const set<Item> &S) {
    // sorted deterministic string
    string s;
    for (const auto &it : S) {
        s += serializeItem(it) + ";;";
    }
    return s;
}

string serializeCore(const set<Item> &S) {
    // core = head, body, dot (no lookahead)
    vector<string> parts;
    for (const auto &it : S) {
        string p = it.head + "->";
        for (auto &sym : it.body) p += sym + " ";
        p += "|" + to_string(it.dot);
        parts.push_back(p);
    }
    sort(parts.begin(), parts.end());
    string out;
    for (auto &p : parts) { out += p + ";;"; }
    return out;
}

class LALRParser {
public:
    // grammar: vector of (head, body_string)
    vector<pair<string,string>> grammar_spec;
    string start_symbol;

    // internals
    set<string> non_terminals;
    set<string> terminals;
    vector<pair<string, vector<string>>> augmented_grammar; // index -> production

    map<string, set<string>> first_sets;

    vector< set<Item> > lr1_items_collection; // LR(1) states
    map<pair<int,string>, int> lr1_goto; // (state, symbol) -> next state idx

    vector< set<Item> > lalr_states;
    map<pair<int,string>, int> lalr_goto; // (lalr_state, symbol) -> next lalr_state

    vector< map<string,string> > action_table; // each row: symbol -> "sX"/"rY"/"accept"
    vector< map<string,int> > goto_table;     // each row: nonterminal -> state idx

    LALRParser(const vector<pair<string,string>>& rules, const string& start) {
        grammar_spec = rules;
        start_symbol = start;
        buildGrammar();
    }

    void buildGrammar() {
        // collect non-terminals
        for (auto &p : grammar_spec) {
            non_terminals.insert(p.first);
        }
        // build augmented grammar as vector of (head, bodyVec)
        string augmented_start = start_symbol + "'";
        augmented_grammar.clear();
        augmented_grammar.push_back({augmented_start, vector<string>{start_symbol}});
        for (auto &p : grammar_spec) {
            string head = p.first;
            string bodyStr = p.second;
            vector<string> body;
            // treat epsilon represented as "ε" or empty string as empty body
            if (bodyStr.size() == 0 || bodyStr == "ε") {
                body = {};
            } else {
                // split by whitespace
                istringstream iss(bodyStr);
                string tok;
                while (iss >> tok) body.push_back(tok);
            }
            augmented_grammar.push_back({head, body});
        }

        // compute terminals (symbols that are not non-terminals)
        terminals.clear();
        for (auto &prod : augmented_grammar) {
            for (auto &sym : prod.second) {
                if (non_terminals.find(sym) == non_terminals.end()) {
                    if (sym != "ε") terminals.insert(sym);
                }
            }
        }
        // ensure $ is considered as end marker but not as terminal for column generation
        first_sets.clear();
    }

    void computeFirstSets() {
        first_sets.clear();
        // initialize terminals
        for (auto &t : terminals) {
            first_sets[t] = {t};
        }
        first_sets["$"] = {"$"}; // end marker

        // initialize non-terminals to empty sets (ensures keys exist)
        for (auto &nt : non_terminals) {
            if (first_sets.find(nt) == first_sets.end()) first_sets[nt] = {};
        }

        bool changed = true;
        while (changed) {
            changed = false;
            for (auto &pr : grammar_spec) {
                string head = pr.first;
                string bodyStr = pr.second;
                vector<string> prod;
                if (bodyStr != "" && bodyStr != "ε") {
                    istringstream iss(bodyStr);
                    string tok;
                    while (iss >> tok) prod.push_back(tok);
                }
                set<string> first_of_prod = firstOfSequence(prod);
                // add to first_sets[head] except epsilon
                size_t before = first_sets[head].size();
                for (auto &x : first_of_prod) if (x != "ε") first_sets[head].insert(x);
                if (first_sets[head].size() > before) changed = true;
            }
        }
    }

    set<string> firstOfSequence(const vector<string> &seq) {
        set<string> res;
        if (seq.empty()) {
            res.insert("ε");
            return res;
        }
        for (size_t i=0;i<seq.size();++i) {
            string sym = seq[i];
            set<string> symFirst;
            if (first_sets.find(sym) != first_sets.end()) symFirst = first_sets[sym];
            // if unknown symbol (not yet in first_sets) treat as empty set (no epsilon)
            for (auto &s : symFirst) if (s != "ε") res.insert(s);
            if (symFirst.find("ε") == symFirst.end()) {
                return res;
            }
        }
        // all had epsilon
        res.insert("ε");
        return res;
    }

    set<Item> closure(const set<Item> &items) {
        set<Item> C = items;
        deque<Item> work;
        for (auto &it : items) work.push_back(it);

        while (!work.empty()) {
            Item it = work.front(); work.pop_front();
            if (it.dot < (int)it.body.size()) {
                string B = it.body[it.dot];
                if (non_terminals.find(B) != non_terminals.end()) {
                    // construct beta a = remaining sequence after B plus lookahead
                    vector<string> remaining;
                    for (size_t k = it.dot + 1; k < it.body.size(); ++k) remaining.push_back(it.body[k]);
                    remaining.push_back(it.lookahead);
                    set<string> first_of_lookahead = firstOfSequence(remaining);
                    // for each production B -> gamma in augmented grammar
                    for (auto &prod : augmented_grammar) {
                        if (prod.first == B) {
                            for (auto &la : first_of_lookahead) {
                                Item newIt;
                                newIt.head = prod.first;
                                newIt.body = prod.second;
                                newIt.dot = 0;
                                newIt.lookahead = la;
                                if (C.find(newIt) == C.end()) {
                                    C.insert(newIt);
                                    work.push_back(newIt);
                                }
                            }
                        }
                    }
                }
            }
        }
        return C;
    }

    set<Item> goTo(const set<Item> &I, const string &symbol) {
        set<Item> J;
        for (auto &it : I) {
            if (it.dot < (int)it.body.size() && it.body[it.dot] == symbol) {
                Item moved = it;
                moved.dot += 1;
                J.insert(moved);
            }
        }
        if (J.empty()) return {};
        return closure(J);
    }

    void buildLR1Collection() {
        lr1_items_collection.clear();
        lr1_goto.clear();

        // initial item: augmented_grammar[0] with lookahead $
        Item init;
        init.head = augmented_grammar[0].first;
        init.body = augmented_grammar[0].second;
        init.dot = 0;
        init.lookahead = "$";
        set<Item> I0 = closure({init});
        lr1_items_collection.push_back(I0);

        map<string,int> stateMap; // serialized set -> index
        stateMap[serializeItemSet(I0)] = 0;
        deque<int> work; work.push_back(0);

        // all symbols: nonterminals + terminals
        vector<string> all_symbols;
        for (auto &nt : non_terminals) all_symbols.push_back(nt);
        for (auto &t : terminals) all_symbols.push_back(t);
        // also allow "$" as symbol in goto? In classical LR, goto only for grammar symbols, actions use '$'. Do not include $ in all_symbols.

        while (!work.empty()) {
            int idx = work.front(); work.pop_front();
            set<Item> items = lr1_items_collection[idx];
            for (auto &sym : all_symbols) {
                set<Item> nxt = goTo(items, sym);
                if (nxt.empty()) continue;
                string key = serializeItemSet(nxt);
                if (stateMap.find(key) == stateMap.end()) {
                    int newIdx = (int)lr1_items_collection.size();
                    lr1_items_collection.push_back(nxt);
                    stateMap[key] = newIdx;
                    work.push_back(newIdx);
                }
                int nextIdx = stateMap[key];
                lr1_goto[{idx, sym}] = nextIdx;
            }
        }
    }

    void buildLALRStates() {
        // group LR(1) states by core
        map<string, vector<int>> core_map; // serialized core -> list of lr1 indices
        for (size_t i=0;i<lr1_items_collection.size();++i) {
            string core_key = serializeCore(lr1_items_collection[i]);
            core_map[core_key].push_back((int)i);
        }

        map<int,int> lr1_to_lalr;
        lalr_states.clear();
        for (auto &kv : core_map) {
            set<Item> merged;
            for (int lr1_idx : kv.second) {
                for (auto &it : lr1_items_collection[lr1_idx]) merged.insert(it);
            }
            int newIdx = (int)lalr_states.size();
            lalr_states.push_back(merged);
            for (int lr1_idx : kv.second) lr1_to_lalr[lr1_idx] = newIdx;
        }

        // build lalr_goto by mapping lr1_goto pairs through lr1_to_lalr
        lalr_goto.clear();
        for (auto &kv : lr1_goto) {
            int s = kv.first.first;
            string sym = kv.first.second;
            int t = kv.second;
            int ls = lr1_to_lalr[s];
            int lt = lr1_to_lalr[t];
            lalr_goto[{ls, sym}] = lt;
        }
    }

    void buildParsingTable() {
        // action columns = sorted terminals + '$'
        vector<string> action_columns;
        for (auto &t : terminals) action_columns.push_back(t);
        sort(action_columns.begin(), action_columns.end());
        action_columns.push_back("$");

        // goto columns = non-terminals excluding augmented start symbol
        string aug_start = augmented_grammar[0].first;
        vector<string> goto_columns;
        for (auto &nt : non_terminals) {
            if (nt != aug_start) goto_columns.push_back(nt);
        }
        sort(goto_columns.begin(), goto_columns.end());

        int n = (int)lalr_states.size();
        action_table.assign(n, map<string,string>());
        goto_table.assign(n, map<string,int>());

        // map production (head, body) -> index in augmented grammar
        map<pair<string, vector<string>>, int> rev_prod_map;
        for (size_t i=0;i<augmented_grammar.size();++i) rev_prod_map[{augmented_grammar[i].first, augmented_grammar[i].second}] = (int)i;

        for (int i=0;i<n;++i) {
            auto &state = lalr_states[i];
            for (auto &it : state) {
                if (it.dot < (int)it.body.size()) {
                    string sym = it.body[it.dot];
                    // if sym is terminal -> shift
                    if (terminals.find(sym) != terminals.end()) {
                        auto f = lalr_goto.find({i, sym});
                        if (f != lalr_goto.end()) {
                            int nxt = f->second;
                            action_table[i][sym] = "s" + to_string(nxt);
                        }
                    }
                } else {
                    // at end of production
                    if (it.head == augmented_grammar[0].first) {
                        if (it.lookahead == "$") action_table[i]["$"] = "accept";
                    } else {
                        auto pr = rev_prod_map.find({it.head, it.body});
                        if (pr != rev_prod_map.end()) {
                            int prod_num = pr->second;
                            string act = "r" + to_string(prod_num);
                            // naive: only write reduce if cell empty
                            if (action_table[i].find(it.lookahead) == action_table[i].end()) {
                                action_table[i][it.lookahead] = act;
                            } else {
                                // conflict -> keep existing (naive). Could print conflict info later.
                            }
                        }
                    }
                }
            }
        }

        // fill goto table based on lalr_goto
        for (auto &kv : lalr_goto) {
            int s = kv.first.first;
            string sym = kv.first.second;
            int t = kv.second;
            // only fill if sym is a nonterminal and present in goto_columns
            if (non_terminals.find(sym) != non_terminals.end() && sym != augmented_grammar[0].first) {
                goto_table[s][sym] = t;
            }
        }
    }

    void generate() {
        computeFirstSets();
        buildLR1Collection();
        buildLALRStates();
        buildParsingTable();
    }

    // printing helpers
    void printLR1Collection() {
        cout << "## Canonical Collection of LR(1) Items\n";
        for (size_t i=0;i<lr1_items_collection.size();++i) {
            cout << "--- I" << i << " ---\n";
            vector<Item> items(lr1_items_collection[i].begin(), lr1_items_collection[i].end());
            sort(items.begin(), items.end());
            for (auto &it : items) {
                string left = joinVec(vector<string>(it.body.begin(), it.body.begin() + min((size_t)it.dot, it.body.size())));
                string right;
                if ((size_t)it.dot <= it.body.size()) {
                    vector<string> r(it.body.begin() + min((size_t)it.dot, it.body.size()), it.body.end());
                    right = joinVec(r);
                }
                if (left.size()==0) left = "";
                if (right.size()==0) right = "";
                cout << "  " << it.head << " -> " << (left.size()? left + " ": "") << ". " << right << ", " << it.lookahead << "\n";
            }
        }
        cout << string(80, '=') << "\n\n";
    }

    void printDFATransitions() {
        cout << "## DFA of Item Sets (State Transitions)\n";
        vector< pair<pair<int,string>,int> > trans;
        for (auto &kv : lr1_goto) trans.push_back(kv);
        sort(trans.begin(), trans.end(), [](const auto &a, const auto &b){
            if (a.first.first != b.first.first) return a.first.first < b.first.first;
            return a.first.second < b.first.second;
        });
        for (auto &t : trans) {
            cout << "  goto(I" << t.first.first << ", " << t.first.second << ") = I" << t.second << "\n";
        }
        cout << string(80, '=') << "\n\n";
    }

    void printParsingTable() {
        cout << "## LALR Parsing Table\n";
        // collect sorted columns
        vector<string> actions;
        for (auto &t : terminals) actions.push_back(t);
        sort(actions.begin(), actions.end());
        actions.push_back("$");
        vector<string> gotos;
        for (auto &nt : non_terminals) if (nt != augmented_grammar[0].first) gotos.push_back(nt);
        sort(gotos.begin(), gotos.end());

        // header
        cout << setw(6) << "State" ;
        for (auto &a : actions) cout << setw(8) << a;
        for (auto &g : gotos) cout << setw(8) << g;
        cout << "\n";

        for (size_t i=0;i<lalr_states.size();++i) {
            cout << setw(6) << i;
            for (auto &a : actions) {
                auto it = action_table[i].find(a);
                if (it != action_table[i].end()) cout << setw(8) << it->second;
                else cout << setw(8) << "";
            }
            for (auto &g : gotos) {
                auto it = goto_table[i].find(g);
                if (it != goto_table[i].end()) cout << setw(8) << it->second;
                else cout << setw(8) << "";
            }
            cout << "\n";
        }
        cout << string(80, '=') << "\n\n";
    }

    vector< tuple<string,string,string> > parse(const string &input_str) {
        // tokenization: treat every character as a token (like the python original).
        vector<string> tokens;
        for (char c : input_str) {
            string s(1,c);
            tokens.push_back(s);
        }
        tokens.push_back("$");

        // stacks: states and symbols
        vector<int> stateStack;
        vector<string> symbolStack;
        stateStack.push_back(0);

        size_t pointer = 0;
        vector< tuple<string,string,string> > trace;

        while (true) {
            int state = stateStack.back();
            string sym = tokens[pointer];

            // check if symbol present in any action column, else error
            // if not present, syntax error
            string action = "";
            auto itAction = action_table[state].find(sym);
            if (itAction != action_table[state].end()) action = itAction->second;
            if (action.empty()) {
                trace.push_back({stackToStr(stateStack, symbolStack), joinTokens(tokens, pointer), string("Error: Invalid Syntax")});
                break;
            }

            trace.push_back({stackToStr(stateStack, symbolStack), joinTokens(tokens, pointer), actionDescription(action)});

            if (action.rfind("s",0) == 0) {
                int nxt = stoi(action.substr(1));
                symbolStack.push_back(sym);
                stateStack.push_back(nxt);
                pointer++;
            } else if (action.rfind("r",0) == 0) {
                int prod_num = stoi(action.substr(1));
                auto prod = augmented_grammar[prod_num];
                vector<string> body = prod.second;
                int toPop = (int)body.size();
                // pop symbol and state for each body symbol
                for (int k=0;k<toPop;k++) {
                    if (!symbolStack.empty()) symbolStack.pop_back();
                    if (!stateStack.empty()) stateStack.pop_back();
                }
                // after popping, top state is prev_state
                int prev_state = stateStack.back();
                string head = prod.first;
                // find goto
                int goto_state = -1;
                auto itGoto = goto_table[prev_state].find(head);
                if (itGoto != goto_table[prev_state].end()) goto_state = itGoto->second;
                if (goto_state == -1) {
                    trace.push_back({stackToStr(stateStack, symbolStack), joinTokens(tokens, pointer), string("Error: No GOTO for ") + head});
                    break;
                }
                symbolStack.push_back(head);
                stateStack.push_back(goto_state);
            } else if (action == "accept") {
                trace.push_back({stackToStr(stateStack, symbolStack), joinTokens(tokens, pointer), string("Accept")});
                break;
            } else {
                trace.push_back({stackToStr(stateStack, symbolStack), joinTokens(tokens, pointer), string("Error: Invalid Syntax")});
                break;
            }
        }

        return trace;
    }

private:
    string stackToStr(const vector<int>& states, const vector<string>& syms) {
        // produce combined stack representation like "0 c 4 c 4"
        // We'll interleave state and symbol: initial state then (symbol state) pairs
        string s;
        if (!states.empty()) s += to_string(states[0]);
        for (size_t i=0;i<syms.size();++i) {
            s += " ";
            s += syms[i];
            s += " ";
            if (i+1 < states.size()) s += to_string(states[i+1]);
            else s += "?";
        }
        return s;
    }

    string joinTokens(const vector<string>& tokens, size_t pointer) {
        string s;
        for (size_t i=pointer;i<tokens.size();++i) s += tokens[i];
        return s;
    }

    string actionDescription(const string &action) {
        if (action.rfind("s",0) == 0) {
            return string("Shift ") + action.substr(1);
        }
        if (action.rfind("r",0) == 0) {
            int prod_num = stoi(action.substr(1));
            auto prod = augmented_grammar[prod_num];
            string b = prod.second.empty() ? string("ε") : joinVec(prod.second);
            return string("Reduce by ") + prod.first + " -> " + b;
        }
        if (action == "accept") return "Accept";
        return "Error";
    }
};


int main() {
    // Example grammar from the Python example:
    // S -> C C
    // C -> c C
    // C -> d
    vector<pair<string,string>> grammar_spec = {
        {"S", "C C"},
        {"C", "c C"},
        {"C", "d"}
    };
    string start_symbol = "S";
    string input_str = "ccdd";

    LALRParser parser(grammar_spec, start_symbol);
    parser.generate();

    parser.printLR1Collection();
    parser.printDFATransitions();
    parser.printParsingTable();

    auto trace = parser.parse(input_str);

    cout << "## Parsing Trace for Input String\n";
    cout << left << setw(6) << "Step" << setw(40) << "Stack" << setw(20) << "Input Buffer" << "Action\n";
    int step = 1;
    for (auto &t : trace) {
        cout << setw(6) << step;
        cout << setw(40) << get<0>(t);
        cout << setw(20) << get<1>(t);
        cout << get<2>(t) << "\n";
        ++step;
    }
    cout << string(80, '=') << "\n";
    return 0;
}
