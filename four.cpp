#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <stack>
#include <iomanip>
#include <algorithm>
using namespace std;

// Node types
enum Type { LEAF, OR, CAT, STAR };

// Syntax tree node
struct Node {
    Type type;
    Node *left, *right, *child;
    char symbol;
    int position; // Only valid for LEAF nodes
    bool nullable;
    set<int> firstpos, lastpos;

    Node(Type t, char sym = 0) : type(t), left(nullptr), right(nullptr), child(nullptr), symbol(sym), position(-1), nullable(false) {}
};

// Helper for precedence
int prec(char op) {
    if (op == '*') return 3;
    if (op == '.') return 2;
    if (op == '|') return 1;
    return 0;
}

// Converts infix regex to postfix (shunting yard, explicit dots!)
string regexToPostfix(const string &regex) {
    string output;
    stack<char> ops;
    for (char ch : regex) {
        if (isalpha(ch) || ch == '#') {
            output += ch;
        } else if (ch == '(') {
            ops.push(ch);
        } else if (ch == ')') {
            while (!ops.empty() && ops.top() != '(') {
                output += ops.top();
                ops.pop();
            }
            if (!ops.empty() && ops.top() == '(') ops.pop();
        } else { // operator
            while (!ops.empty() && prec(ops.top()) >= prec(ch)) {
                output += ops.top();
                ops.pop();
            }
            ops.push(ch);
        }
    }
    while (!ops.empty()) {
        output += ops.top();
        ops.pop();
    }
    return output;
}

// Builds syntax tree from postfix regex; returns root
Node *buildSyntaxTree(const string &postfix, vector<Node *> &leaves) {
    stack<Node *> S;
    int pos = 1;
    for (char ch : postfix) {
        if (isalpha(ch) || ch == '#') {
            Node *n = new Node(LEAF, ch);
            n->position = pos++;
            leaves.push_back(n);
            S.push(n);
        } else if (ch == '*') {
            Node *child = S.top();
            S.pop();
            Node *n = new Node(STAR);
            n->child = child;
            S.push(n);
        } else if (ch == '.') {
            Node *r = S.top();
            S.pop();
            Node *l = S.top();
            S.pop();
            Node *n = new Node(CAT);
            n->left = l;
            n->right = r;
            S.push(n);
        } else if (ch == '|') {
            Node *r = S.top();
            S.pop();
            Node *l = S.top();
            S.pop();
            Node *n = new Node(OR);
            n->left = l;
            n->right = r;
            S.push(n);
        }
    }
    return S.top();
}

// Computes nullable, firstpos, lastpos recursively for all nodes
void compute_nullable_first_last(Node *n) {
    if (!n) return;
    if (n->type == LEAF) {
        n->nullable = false;
        n->firstpos.insert(n->position);
        n->lastpos.insert(n->position);
    } else if (n->type == OR) {
        compute_nullable_first_last(n->left);
        compute_nullable_first_last(n->right);
        n->nullable = n->left->nullable || n->right->nullable;
        n->firstpos.insert(n->left->firstpos.begin(), n->left->firstpos.end());
        n->firstpos.insert(n->right->firstpos.begin(), n->right->firstpos.end());
        n->lastpos.insert(n->left->lastpos.begin(), n->left->lastpos.end());
        n->lastpos.insert(n->right->lastpos.begin(), n->right->lastpos.end());
    } else if (n->type == CAT) {
        compute_nullable_first_last(n->left);
        compute_nullable_first_last(n->right);
        n->nullable = n->left->nullable && n->right->nullable;
        n->firstpos = n->left->firstpos;
        if (n->left->nullable)
            n->firstpos.insert(n->right->firstpos.begin(), n->right->firstpos.end());
        n->lastpos = n->right->lastpos;
        if (n->right->nullable)
            n->lastpos.insert(n->left->lastpos.begin(), n->left->lastpos.end());
    } else if (n->type == STAR) {
        compute_nullable_first_last(n->child);
        n->nullable = true;
        n->firstpos = n->child->firstpos;
        n->lastpos = n->child->lastpos;
    }
}

// Computes followpos for all positions, fills followpos_map
void compute_followpos(Node *n, map<int, set<int>> &followpos) {
    if (!n) return;
    if (n->type == CAT) {
        for (int i : n->left->lastpos)
            followpos[i].insert(n->right->firstpos.begin(), n->right->firstpos.end());
    } else if (n->type == STAR) {
        for (int i : n->child->lastpos)
            followpos[i].insert(n->child->firstpos.begin(), n->child->firstpos.end());
    }
    if (n->type == OR || n->type == CAT) {
        compute_followpos(n->left, followpos);
        compute_followpos(n->right, followpos);
    } else if (n->type == STAR) {
        compute_followpos(n->child, followpos);
    }
}

// Dstates is a set of sets of positions
typedef set<int> State;
struct StateCmp {
    bool operator()(const State &a, const State &b) const { return a < b; }
};

// Returns the symbol at a given position
char symbol_at(int pos, const vector<Node *> &leaves) {
    for (auto *leaf : leaves)
        if (leaf->position == pos)
            return leaf->symbol;
    return 0;
}

// Constructs DFA from followpos. Returns transition table and state set.
void construct_dfa(Node *root, const vector<Node *> &leaves, const map<int, set<int>> &followpos,
                   map<State, int, StateCmp> &dfa_states, vector<map<char, int>> &dfa_trans,
                   int &accept_state) {
    vector<State> states;
    map<State, int, StateCmp> state_ids;
    vector<bool> is_marked;

    State start = root->firstpos;
    states.push_back(start);
    state_ids[start] = 0;
    is_marked.push_back(false);

    int marker_pos = -1;
    for (size_t i = 0; i < leaves.size(); ++i)
        if (leaves[i]->symbol == '#')
            marker_pos = leaves[i]->position;

    while (true) {
        int i = -1;
        for (size_t j = 0; j < states.size(); ++j)
            if (!is_marked[j]) {
                i = int(j);
                break;
            }
        if (i == -1) break;
        is_marked[i] = true;
        State T = states[i];
        map<char, State> move_map;
        for (int p : T) {
            char a = symbol_at(p, leaves);
            if (a == '#') continue;
            move_map[a].insert(p);
        }
        for (auto pr : move_map) {
            char a = pr.first;
            State U;
            for (int p : pr.second)
                U.insert(followpos.at(p).begin(), followpos.at(p).end());
            if (!state_ids.count(U)) {
                state_ids[U] = int(states.size());
                states.push_back(U);
                is_marked.push_back(false);
            }
        }
    }
    dfa_states = state_ids;
    dfa_trans = vector<map<char, int>>(states.size());
    for (size_t i = 0; i < states.size(); ++i) {
        State T = states[i];
        map<char, State> move_map;
        for (int p : T) {
            char a = symbol_at(p, leaves);
            if (a == '#') continue;
            move_map[a].insert(p);
        }
        for (auto pr : move_map) {
            char a = pr.first;
            State U;
            for (int p : pr.second)
                U.insert(followpos.at(p).begin(), followpos.at(p).end());
            int id = state_ids[U];
            dfa_trans[i][a] = id;
        }
    }
    // Find accept state(s)
    for (auto pr : dfa_states) {
        State T = pr.first;
        if (T.count(marker_pos)) {
            accept_state = pr.second;
            break;
        }
    }
}

// Helper to print a set of ints as {x1,x2,...}
void print_set(const set<int>& S) {
    cout << "{";
    bool comma = false;
    for (int x : S) {
        if (comma) cout << ",";
        cout << x;
        comma = true;
    }
    cout << "}";
}

// Print firstpos/lastpos table
void print_first_last(const vector<Node*>& leaves) {
    cout << "\n"
         << left << setw(10) << "Position"
         << setw(8) << "Symbol"
         << setw(13) << "Firstpos"
         << setw(13) << "Lastpos"
         << "\n";
    cout << string(44, '-') << "\n";
    for (auto* leaf : leaves) {
        cout << right << setw(8) << leaf->position << "  "
             << left << setw(6) << leaf->symbol;
        cout << setw(13); print_set(leaf->firstpos);
        cout << setw(13); print_set(leaf->lastpos);
        cout << "\n";
    }
}

// Print followpos table
void print_follow(const vector<Node*>& leaves, const map<int, set<int>>& followpos) {
    cout << "\n"
         << left << setw(10) << "Position"
         << setw(8) << "Symbol"
         << setw(15) << "Followpos"
         << "\n";
    cout << string(33, '-') << "\n";
    for (auto* leaf : leaves) {
        cout << right << setw(8) << leaf->position << "  "
             << left << setw(6) << leaf->symbol;
        cout << setw(15); print_set(followpos.at(leaf->position));
        cout << "\n";
    }
}

// Print DFA transition table with padding fix to avoid length_error
void print_dfa(const map<State, int, StateCmp>& dfa_states,
               const vector<map<char, int>>& dfa_trans,
               int accept_state, const vector<Node*>& leaves) {
    set<char> alphabet;
    for (auto* leaf : leaves)
        if (leaf->symbol != '#') alphabet.insert(leaf->symbol);

    cout << "\n";
    cout << left << setw(7) << "State"
         << setw(18) << "Set";
    for (char a : alphabet) cout << setw(7) << a;
    cout << setw(8) << "Accept" << "\n";
    cout << string(7 + 18 + 7 * alphabet.size() + 8, '-') << "\n";

    for (auto pr : dfa_states) {
        int id = pr.second;
        cout << right << setw(5) << id << "   ";
        print_set(pr.first);

        int set_width = 16; // Adjust if you expect larger sets
        int len = 2; // for "{}"
        for (int x : pr.first) {
            len += to_string(x).size() + 1; // +1 for comma
        }
        int pad = set_width - len;
        if (pad < 0) pad = 0;  // *** This line prevents string length errors ***
        cout << string(pad, ' ');

        for (char a : alphabet) {
            if (dfa_trans[id].count(a))
                cout << setw(7) << dfa_trans[id].at(a);
            else
                cout << setw(7) << "-";
        }
        cout << setw(8) << (id == accept_state ? "Yes" : "No") << "\n";
    }
}

int main() {
    cout << "Enter the regular expression (fully parenthesized with explicit '.' for concatenation, ending with .#):\n";
    string regex;
    getline(cin, regex);

    string postfix = regexToPostfix(regex);

    vector<Node*> leaves;
    Node* root = buildSyntaxTree(postfix, leaves);

    compute_nullable_first_last(root);

    map<int, set<int>> followpos;
    for (auto* leaf : leaves) followpos[leaf->position] = set<int>();
    compute_followpos(root, followpos);

    print_first_last(leaves);
    print_follow(leaves, followpos);

    map<State, int, StateCmp> dfa_states;
    vector<map<char, int>> dfa_trans;
    int accept_state = -1;
    construct_dfa(root, leaves, followpos, dfa_states, dfa_trans, accept_state);

    print_dfa(dfa_states, dfa_trans, accept_state, leaves);

    return 0;
}
