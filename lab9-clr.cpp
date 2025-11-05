//CLR

#include <bits/stdc++.h>
using namespace std;

// ---------- Structures ----------
struct Production {
    char lhs;
    string rhs;
};
struct Item {
    int prod;          // index in grammar
    int dot;           // position of dot
    char lookahead;    // lookahead symbol
};
struct State {
    vector<Item> items;
};

// ---------- Globals ----------
vector<Production> grammar;
vector<State> states;
map<pair<int,char>, int> GOTO_TABLE;
map<pair<int,char>, string> ACTION;

set<char> terminals = {'c','d','$'};
set<char> non_terminals = {'S','C','Q'};

// ---------- FIRST ----------
map<char,set<string>> FIRST;

void compute_FIRST() {
    // terminals
    for(char t: terminals) FIRST[t].insert(string(1,t));

    bool changed;
    do {
        changed=false;
        for(auto &p: grammar) {
            char A = p.lhs;
            string beta = p.rhs;
            if(beta=="") {
                if(FIRST[A].insert("ε").second) changed=true;
            } else {
                bool all_nullable=true;
                for(char X: beta) {
                    for(auto &a: FIRST[X]) if(a!="ε") if(FIRST[A].insert(a).second) changed=true;
                    if(FIRST[X].count("ε")==0) { all_nullable=false; break; }
                }
                if(all_nullable) if(FIRST[A].insert("ε").second) changed=true;
            }
        }
    } while(changed);
}

set<string> FIRST_of_string(const string &s, char lookahead) {
    set<string> result;
    if(s=="") { result.insert(string(1,lookahead)); return result; }
    bool nullable=true;
    for(char X: s) {
        for(auto &a: FIRST[X]) if(a!="ε") result.insert(a);
        if(FIRST[X].count("ε")==0) { nullable=false; break; }
    }
    if(nullable) result.insert(string(1,lookahead));
    return result;
}

// ---------- Item utilities ----------
bool item_exists(State &st, Item it) {
    for(auto &x: st.items)
        if(x.prod==it.prod && x.dot==it.dot && x.lookahead==it.lookahead)
            return true;
    return false;
}
void add_item(State &st, Item it) {
    if(!item_exists(st,it)) st.items.push_back(it);
}
bool states_equal(State &a, State &b) {
    if(a.items.size()!=b.items.size()) return false;
    for(auto &it: a.items) if(!item_exists(b,it)) return false;
    return true;
}
int state_index(State &st) {
    for(int i=0;i<states.size();i++) if(states_equal(states[i],st)) return i;
    return -1;
}

// ---------- Closure & GOTO ----------
State closure(State I) {
    bool added;
    do {
        added=false;
        vector<Item> new_items;
        for(auto &it: I.items) {
            Production p = grammar[it.prod];
            if(it.dot < p.rhs.size()) {
                char B = p.rhs[it.dot];
                if(non_terminals.count(B)) {
                    string beta = p.rhs.substr(it.dot+1);
                    set<string> lookaheads = FIRST_of_string(beta, it.lookahead);
                    for(int idx=0; idx<grammar.size(); idx++) {
                        Production gp = grammar[idx];
                        if(gp.lhs==B) {
                            for(auto &a: lookaheads) {
                                Item new_item{ idx, 0, a[0] };
                                if(!item_exists(I,new_item)) { new_items.push_back(new_item); added=true; }
                            }
                        }
                    }
                }
            }
        }
        for(auto &ni: new_items) I.items.push_back(ni);
    } while(added);
    return I;
}

State GOTO(const State &I, char X) {
    State J;
    for(auto &it: I.items) {
        Production p = grammar[it.prod];
        if(it.dot < p.rhs.size() && p.rhs[it.dot]==X) {
            Item moved{it.prod, it.dot+1, it.lookahead};
            add_item(J,moved);
        }
    }
    return closure(J);
}

// ---------- Build States ----------
void build_states() {
    states.clear();
    GOTO_TABLE.clear();
    ACTION.clear();

    State start;
    start.items.push_back({0,0,'$'}); // Q -> •S , $
    start = closure(start);
    states.push_back(start);

    for(int idx=0; idx<states.size(); idx++) {
        set<char> symbols;
        for(auto &it: states[idx].items) {
            Production p = grammar[it.prod];
            if(it.dot < p.rhs.size()) symbols.insert(p.rhs[it.dot]);
        }
        for(char X: symbols) {
            State J = GOTO(states[idx],X);
            if(J.items.empty()) continue;
            int j = state_index(J);
            if(j==-1) { j = states.size(); states.push_back(J); }
            GOTO_TABLE[{idx,X}] = j;
        }
    }
}

// ---------- Build Parsing Table ----------
void build_parsing_table() {
    for(int i=0;i<states.size();i++) {
        for(auto &it: states[i].items) {
            Production p = grammar[it.prod];
            if(it.dot < p.rhs.size()) {
                char a = p.rhs[it.dot];
                if(terminals.count(a)) {
                    int j = GOTO_TABLE[{i,a}];
                    ACTION[{i,a}] = "s"+to_string(j);
                }
            } else {
                if(p.lhs=='Q' && it.lookahead=='$')
                    ACTION[{i,'$'}] = "acc";
                else ACTION[{i,it.lookahead}] = "r"+to_string(it.prod);
            }
        }
    }
}

// ---------- Printing ----------
void print_states() {
    cout << "\nCanonical Collection of LR(1) Items:\n";
    for(int i=0;i<states.size();i++) {
        cout << "I" << i << ":\n";
        for(auto &it: states[i].items) {
            Production p = grammar[it.prod];
            cout << "  " << p.lhs << " -> ";
            for(int k=0;k<p.rhs.size();k++) { if(k==it.dot) cout << "."; cout << p.rhs[k]; }
            if(it.dot==p.rhs.size()) cout << ".";
            cout << ", " << it.lookahead << "\n";
        }
        cout << "\n";
    }
}

void print_dfa() {
    cout << "DFA of Item Sets (Transitions):\n";
    for(auto &kv: GOTO_TABLE)
        cout << "I" << kv.first.first << " --" << kv.first.second << "--> I" << kv.second << "\n";
}

void print_parsing_table() {
    cout << "\nCLR Parsing Table (ACTION and GOTO):\n";
    cout << "State\t";
    for(char t: terminals) cout << t << "\t";
    for(char nt: non_terminals) if(nt!='Q') cout << nt << "\t";
    cout << "\n";
    for(int i=0;i<states.size();i++) {
        cout << i << "\t";
        for(char t: terminals) {
            if(ACTION.count({i,t})) cout << ACTION[{i,t}] << "\t";
            else cout << "-\t";
        }
        for(char nt: non_terminals) if(nt!='Q') {
            if(GOTO_TABLE.count({i,nt})) cout << GOTO_TABLE[{i,nt}] << "\t";
            else cout << "-\t";
        }
        cout << "\n";
    }
}

// ---------- Parsing ----------
void parse_input(string input) {
    input += "$";
    vector<int> state_stack = {0};
    vector<char> symbol_stack = {'$'};
    int ip=0;

    cout << "\nParsing Trace:\n";
    cout << "Stack\t\tInput\t\tAction\n";

    while(true) {
        int s = state_stack.back();
        char a = input[ip];
        cout << "[";
        for(int st: state_stack) cout << st << " ";
        cout << "]\t\t" << input.substr(ip) << "\t\t";

        if(!ACTION.count({s,a})) { cout << "ERROR\n"; break; }
        string act = ACTION[{s,a}];
        if(act=="acc") { cout << "ACCEPT\n"; break; }
        else if(act[0]=='s') {
            int j = stoi(act.substr(1));
            cout << "Shift " << a << "\n";
            symbol_stack.push_back(a);
            state_stack.push_back(j);
            ip++;
        } else if(act[0]=='r') {
            int prod_idx = stoi(act.substr(1));
            Production p = grammar[prod_idx];
            cout << "Reduce by " << p.lhs << "->" << p.rhs << "\n";
            for(int k=0;k<p.rhs.size();k++) { state_stack.pop_back(); symbol_stack.pop_back(); }
            int t = state_stack.back();
            symbol_stack.push_back(p.lhs);
            int j = GOTO_TABLE[{t,p.lhs}];
            state_stack.push_back(j);
        }
    }
}

// ---------- Main ----------
int main() {
    grammar.push_back({'Q',"S"});
    grammar.push_back({'S',"CC"});
    grammar.push_back({'C',"cC"});
    grammar.push_back({'C',"d"});

    compute_FIRST();
    build_states();
    build_parsing_table();

    print_states();
    print_dfa();
    print_parsing_table();

    string input="ccdd";
    cout << "\nInput: " << input << "\n";
    parse_input(input);
}
