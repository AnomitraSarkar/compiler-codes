#include <bits/stdc++.h>
using namespace std;

struct Production {
    char lhs;
    string rhs;
};
struct Item {
    char lhs;
    string rhs;
    int dot;
};
struct State {
    vector<Item> items;
};

vector<Production> grammar;
set<char> terminals, nonterminals;
map<char,set<char>> FIRST, FOLLOW;

vector<State> states;
map<pair<int,char>, int> GOTO_TABLE;   // GOTO transitions
map<pair<int,char>, string> ACTION;    // ACTION table

// ---------- Utilities ----------
bool is_terminal(char c) {
    return nonterminals.count(c)==0;
}
bool equal_items(const vector<Item> &a, const vector<Item> &b) {
    if(a.size()!=b.size()) return false;
    for(size_t i=0;i<a.size();i++) {
        if(a[i].lhs!=b[i].lhs || a[i].rhs!=b[i].rhs || a[i].dot!=b[i].dot)
            return false;
    }
    return true;
}
int find_state(const State &s) {
    for(int i=0;i<states.size();i++)
        if(equal_items(states[i].items, s.items)) return i;
    return -1;
}

// ---------- Closure & GOTO ----------
State closure(State I) {
    bool changed;
    do {
        changed=false;
        vector<Item> add;
        for(auto &it:I.items) {
            if(it.dot < it.rhs.size()) {
                char B=it.rhs[it.dot];
                if(nonterminals.count(B)) {
                    for(auto &p:grammar) {
                        if(p.lhs==B) {
                            Item newIt={p.lhs,p.rhs,0};
                            bool exists=false;
                            for(auto &x:I.items)
                                if(x.lhs==newIt.lhs && x.rhs==newIt.rhs && x.dot==0) exists=true;
                            if(!exists) { add.push_back(newIt); changed=true; }
                        }
                    }
                }
            }
        }
        for(auto &x:add) I.items.push_back(x);
    } while(changed);
    return I;
}
State GOTO(const State &I,char X) {
    State J;
    for(auto &it:I.items) {
        if(it.dot < it.rhs.size() && it.rhs[it.dot]==X) {
            J.items.push_back({it.lhs,it.rhs,it.dot+1});
        }
    }
    if(J.items.empty()) return J;
    return closure(J);
}

// ---------- FIRST & FOLLOW ----------
void compute_FIRST() {
    bool changed;
    do {
        changed=false;
        for(auto &p:grammar) {
            char A=p.lhs;
            if(p.rhs=="#") {
                if(FIRST[A].insert('#').second) changed=true;
            } else {
                bool eps=true;
                for(char X:p.rhs) {
                    if(is_terminal(X)) {
                        if(FIRST[A].insert(X).second) changed=true;
                        eps=false; break;
                    } else {
                        for(char x:FIRST[X])
                            if(x!='#' && FIRST[A].insert(x).second) changed=true;
                        if(!FIRST[X].count('#')) { eps=false; break; }
                    }
                }
                if(eps) if(FIRST[A].insert('#').second) changed=true;
            }
        }
    }while(changed);
}
void compute_FOLLOW() {
    FOLLOW['S'].insert('$'); // start symbol
    bool changed;
    do {
        changed=false;
        for(auto &p:grammar) {
            for(int i=0;i<p.rhs.size();i++) {
                char B=p.rhs[i];
                if(nonterminals.count(B)) {
                    bool eps=true;
                    for(int j=i+1;j<p.rhs.size();j++) {
                        char X=p.rhs[j];
                        eps=false;
                        if(is_terminal(X)) {
                            if(FOLLOW[B].insert(X).second) changed=true;
                            break;
                        } else {
                            for(char x:FIRST[X])
                                if(x!='#' && FOLLOW[B].insert(x).second) changed=true;
                            if(FIRST[X].count('#')) eps=true;
                            else {eps=false; break;}
                        }
                    }
                    if(i==p.rhs.size()-1 || eps) {
                        for(char x:FOLLOW[p.lhs])
                            if(FOLLOW[B].insert(x).second) changed=true;
                    }
                }
            }
        }
    }while(changed);
}

// ---------- Build States ----------
void build_states() {
    State I0;
    I0.items.push_back({grammar[0].lhs,grammar[0].rhs,0});
    I0=closure(I0);
    states.push_back(I0);

    for(int i=0;i<states.size();i++) {
        set<char> symbols;
        for(auto &it:states[i].items)
            if(it.dot<it.rhs.size()) symbols.insert(it.rhs[it.dot]);
        for(char X:symbols) {
            State J=GOTO(states[i],X);
            if(!J.items.empty()) {
                int idx=find_state(J);
                if(idx==-1) {
                    states.push_back(J);
                    idx=states.size()-1;
                }
                GOTO_TABLE[{i,X}]=idx;
            }
        }
    }
}

// ---------- Build Parsing Table ----------
void build_parsing_table() {
    compute_FIRST();
    compute_FOLLOW();

    for(int i=0;i<states.size();i++) {
        for(auto &it:states[i].items) {
            if(it.dot<it.rhs.size()) {
                char a=it.rhs[it.dot];
                if(is_terminal(a)) {
                    int j=GOTO_TABLE[{i,a}];
                    ACTION[{i,a}]="s"+to_string(j);
                }
            } else {
                if(it.lhs=='Q') ACTION[{i,'$'}]="acc";
                else {
                    int prod=-1;
                    for(int k=0;k<grammar.size();k++)
                        if(grammar[k].lhs==it.lhs && grammar[k].rhs==it.rhs) prod=k;
                    for(char a:FOLLOW[it.lhs]) {
                        ACTION[{i,a}]="r"+to_string(prod);
                    }
                }
            }
        }
        for(char A:nonterminals) {
            if(GOTO_TABLE.count({i,A}))
                ACTION[{i,A}]="g"+to_string(GOTO_TABLE[{i,A}]);
        }
    }
}

// ---------- Print Functions ----------
void print_grammar() {
    cout<<"Grammar Rules:\n";
    for(int i=0;i<grammar.size();i++)
        cout<<i<<": "<<grammar[i].lhs<<" -> "<<grammar[i].rhs<<"\n";
    cout<<"\n";
}
void print_states() {
    cout<<"Canonical Collection of LR(0) Items:\n";
    for(int i=0;i<states.size();i++) {
        cout<<"I"<<i<<":\n";
        for(auto &it:states[i].items) {
            cout<<"  "<<it.lhs<<" -> ";
            for(int j=0;j<it.rhs.size();j++) {
                if(j==it.dot) cout<<".";
                cout<<it.rhs[j];
            }
            if(it.dot==it.rhs.size()) cout<<".";
            cout<<"\n";
        }
        cout<<"\n";
    }
}
void print_dfa() {
    cout<<"DFA of Item Sets (state transitions):\n";
    for(auto &e:GOTO_TABLE)
        cout<<"I"<<e.first.first<<" --"<<e.first.second<<"--> I"<<e.second<<"\n";
    cout<<"\n";
}
void print_table() {
    cout<<"SLR Parsing Table:\n";
    vector<char> terms(terminals.begin(),terminals.end());
    terms.push_back('$');
    vector<char> nonterms(nonterminals.begin(),nonterminals.end());
    cout<<setw(7)<<"State";
    for(char t:terms) cout<<setw(8)<<t;
    for(char A:nonterms) cout<<setw(8)<<A;
    cout<<"\n";
    for(int i=0;i<states.size();i++) {
        cout<<setw(7)<<i;
        for(char t:terms) {
            string act=ACTION[{i,t}];
            cout<<setw(8)<<act;
        }
        for(char A:nonterms) {
            string g=ACTION[{i,A}];
            cout<<setw(8)<<g;
        }
        cout<<"\n";
    }
    cout<<"\n";
}

// ---------- Parse Input ----------
void parse(string input) {
    cout<<"Parsing input string: "<<input<<"\n";
    input+="$";
    vector<int> stateStack={0};
    vector<char> symStack={'$'};
    int ip=0;
    cout<<setw(15)<<"StateStack"<<setw(15)<<"SymbolStack"<<setw(15)<<"Input"<<setw(15)<<"Action"<<"\n";
    while(true) {
        int s=stateStack.back();
        char a=input[ip];
        string act=ACTION[{s,a}];
        cout<<setw(15);
        for(int x:stateStack) cout<<x<<" ";
        cout<<setw(15);
        for(char c:symStack) cout<<c<<" ";
        cout<<setw(15)<<input.substr(ip)<<setw(15)<<act<<"\n";
        if(act=="") { cout<<"Error!\n"; break; }
        if(act=="acc") { cout<<"Accepted!\n"; break; }
        if(act[0]=='s') {
            int j=stoi(act.substr(1));
            stateStack.push_back(j);
            symStack.push_back(a);
            ip++;
        } else if(act[0]=='r') {
            int k=stoi(act.substr(1));
            Production p=grammar[k];
            for(int j=0;j<p.rhs.size() && p.rhs!="#";j++) {
                stateStack.pop_back();
                symStack.pop_back();
            }
            int t=stateStack.back();
            symStack.push_back(p.lhs);
            string g=ACTION[{t,p.lhs}];
            int j=stoi(g.substr(1));
            stateStack.push_back(j);
        }
    }
}

// ---------- Main ----------
int main() {
    // Hardcoded grammar for your example:
    grammar={
        {'Q',"S"},
        {'S',"CC"},
        {'C',"cC"},
        {'C',"d"}
    };
    nonterminals={'Q','S','C'};
    terminals={'c','d'};

    build_states();
    build_parsing_table();

    print_grammar();
    print_states();
    print_dfa();
    print_table();
    parse("ccdd"); // fixed input

    return 0;
}
