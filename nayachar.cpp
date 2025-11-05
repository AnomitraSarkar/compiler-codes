#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <stack>
#include <iomanip>
#include <algorithm>
using namespace std;

enum Type
{
    LEAF,
    OR,
    CAT,
    STAR
};

struct Node
{
    Type type;
    Node *left, *right, *child;
    char symbol;
    int position;
    bool nullable;
    set<int> firstpos, lastpos;

    Node(Type t, char sym = 0) : type(t), left(nullptr), right(nullptr), child(nullptr), symbol(sym), position(-1), nullable(false) {}
};

int prec(char op)
{
    if (op == '*')
        return 3;
    if (op == '.')
        return 2;
    if (op == '|')
        return 1;
    return 0;
}

bool isSymbol(char c)
{
    return isalpha(c) || c == '#' || isdigit(c);
}

// Insert explicit concatenation
string insertConcat(const string &regex)
{
    string result;
    for (size_t i = 0; i < regex.size(); ++i)
    {
        char curr = regex[i];
        result += curr;
        if (i + 1 < regex.size())
        {
            char next = regex[i + 1];
            if ((isSymbol(curr) || curr == '*' || curr == ')') &&
                (isSymbol(next) || next == '('))
            {
                result += '.';
            }
        }
    }
    return result;
}

string regexToPostfix(const string &regex)
{
    string output;
    stack<char> ops;
    for (char ch : regex)
    {
        if (isSymbol(ch))
        {
            output += ch;
        }
        else if (ch == '(')
        {
            ops.push(ch);
        }
        else if (ch == ')')
        {
            while (!ops.empty() && ops.top() != '(')
            {
                output += ops.top();
                ops.pop();
            }
            if (!ops.empty())
                ops.pop();
        }
        else
        {
            while (!ops.empty() && prec(ops.top()) >= prec(ch))
            {
                output += ops.top();
                ops.pop();
            }
            ops.push(ch);
        }
    }
    while (!ops.empty())
    {
        output += ops.top();
        ops.pop();
    }
    return output;
}

Node *buildSyntaxTree(const string &postfix, vector<Node *> &leaves)
{
    stack<Node *> S;
    int pos = 1;
    for (char ch : postfix)
    {
        if (isSymbol(ch))
        {
            Node *n = new Node(LEAF, ch);
            n->position = pos++;
            leaves.push_back(n);
            S.push(n);
        }
        else if (ch == '*')
        {
            Node *child = S.top();
            S.pop();
            Node *n = new Node(STAR);
            n->child = child;
            S.push(n);
        }
        else if (ch == '.')
        {
            Node *r = S.top();
            S.pop();
            Node *l = S.top();
            S.pop();
            Node *n = new Node(CAT);
            n->left = l;
            n->right = r;
            S.push(n);
        }
        else if (ch == '|')
        {
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

void compute_nullable_first_last(Node *n)
{
    if (!n)
        return;
    if (n->type == LEAF)
    {
        n->nullable = false;
        n->firstpos.insert(n->position);
        n->lastpos.insert(n->position);
    }
    else if (n->type == OR)
    {
        compute_nullable_first_last(n->left);
        compute_nullable_first_last(n->right);
        n->nullable = n->left->nullable || n->right->nullable;
        n->firstpos.insert(n->left->firstpos.begin(), n->left->firstpos.end());
        n->firstpos.insert(n->right->firstpos.begin(), n->right->firstpos.end());
        n->lastpos.insert(n->left->lastpos.begin(), n->left->lastpos.end());
        n->lastpos.insert(n->right->lastpos.begin(), n->right->lastpos.end());
    }
    else if (n->type == CAT)
    {
        compute_nullable_first_last(n->left);
        compute_nullable_first_last(n->right);
        n->nullable = n->left->nullable && n->right->nullable;
        n->firstpos = n->left->firstpos;
        if (n->left->nullable)
            n->firstpos.insert(n->right->firstpos.begin(), n->right->firstpos.end());
        n->lastpos = n->right->lastpos;
        if (n->right->nullable)
            n->lastpos.insert(n->left->lastpos.begin(), n->left->lastpos.end());
    }
    else if (n->type == STAR)
    {
        compute_nullable_first_last(n->child);
        n->nullable = true;
        n->firstpos = n->child->firstpos;
        n->lastpos = n->child->lastpos;
    }
}

void compute_followpos(Node *n, map<int, set<int>> &followpos)
{
    if (!n)
        return;
    if (n->type == CAT)
    {
        for (int i : n->left->lastpos)
            followpos[i].insert(n->right->firstpos.begin(), n->right->firstpos.end());
    }
    else if (n->type == STAR)
    {
        for (int i : n->child->lastpos)
            followpos[i].insert(n->child->firstpos.begin(), n->child->firstpos.end());
    }
    if (n->type == OR || n->type == CAT)
    {
        compute_followpos(n->left, followpos);
        compute_followpos(n->right, followpos);
    }
    else if (n->type == STAR)
    {
        compute_followpos(n->child, followpos);
    }
}

typedef set<int> State;
struct StateCmp
{
    bool operator()(const State &a, const State &b) const { return a < b; }
};

char symbol_at(int pos, const vector<Node *> &leaves)
{
    for (auto *leaf : leaves)
        if (leaf->position == pos)
            return leaf->symbol;
    return 0;
}

void construct_dfa(Node *root, const vector<Node *> &leaves, const map<int, set<int>> &followpos,
                   map<State, int, StateCmp> &dfa_states, vector<map<char, int>> &dfa_trans,
                   int &accept_state)
{
    vector<State> states;
    map<State, int, StateCmp> state_ids;
    vector<bool> is_marked;

    State start = root->firstpos;
    states.push_back(start);
    state_ids[start] = 0;
    is_marked.push_back(false);

    int marker_pos = -1;
    for (auto *leaf : leaves)
        if (leaf->symbol == '#')
            marker_pos = leaf->position;

    while (true)
    {
        int i = -1;
        for (size_t j = 0; j < states.size(); ++j)
            if (!is_marked[j])
            {
                i = int(j);
                break;
            }
        if (i == -1)
            break;
        is_marked[i] = true;
        State T = states[i];
        map<char, State> move_map;
        for (int p : T)
        {
            char a = symbol_at(p, leaves);
            if (a == '#')
                continue;
            move_map[a].insert(p);
        }
        for (auto pr : move_map)
        {
            char a = pr.first;
            State U;
            for (int p : pr.second)
                U.insert(followpos.at(p).begin(), followpos.at(p).end());
            if (!state_ids.count(U))
            {
                state_ids[U] = int(states.size());
                states.push_back(U);
                is_marked.push_back(false);
            }
        }
    }

    dfa_states = state_ids;
    dfa_trans = vector<map<char, int>>(states.size());
    for (size_t i = 0; i < states.size(); ++i)
    {
        State T = states[i];
        map<char, State> move_map;
        for (int p : T)
        {
            char a = symbol_at(p, leaves);
            if (a == '#')
                continue;
            move_map[a].insert(p);
        }
        for (auto pr : move_map)
        {
            char a = pr.first;
            State U;
            for (int p : pr.second)
                U.insert(followpos.at(p).begin(), followpos.at(p).end());
            int id = state_ids[U];
            dfa_trans[i][a] = id;
        }
    }

    for (auto pr : dfa_states)
    {
        State T = pr.first;
        if (T.count(marker_pos))
        {
            accept_state = pr.second;
            break;
        }
    }
}

void print_set(const set<int> &S)
{
    cout << "{";
    bool comma = false;
    for (int x : S)
    {
        if (comma)
            cout << ",";
        cout << x;
        comma = true;
    }
    cout << "}";
}

void print_first_last_follow(const vector<Node *> &leaves, const map<int, set<int>> &followpos)
{
    cout << "\nPos\tSym\tFirstPos\tLastPos\t\tFollowPos\n";
    cout << string(60, '-') << "\n";
    for (auto *leaf : leaves)
    {
        cout << leaf->position << "\t" << leaf->symbol << "\t";
        print_set(leaf->firstpos);
        cout << "\t\t";
        print_set(leaf->lastpos);
        cout << "\t\t";
        print_set(followpos.at(leaf->position));
        cout << "\n";
    }
}

void print_dfa(const map<State, int, StateCmp> &dfa_states,
               const vector<map<char, int>> &dfa_trans,
               int accept_state, const vector<Node *> &leaves)
{
    set<char> alphabet;
    for (auto *leaf : leaves)
        if (leaf->symbol != '#')
            alphabet.insert(leaf->symbol);

    cout << "\nDFA Transition Table: -------------\n";
    cout << left << setw(10) << "State" << setw(20) << "Set";
    for (char a : alphabet)
        cout << setw(8) << a;
    cout << "Accept\n";
    cout << string(10 + 20 + 8 * alphabet.size() + 8, '-') << "\n";

    for (auto pr : dfa_states)
    {
        int id = pr.second;
        bool isStart = (id == 0);
        bool isAccept = (id == accept_state);
        string label = (isStart ? "->" : "  ") + to_string(id) + (isAccept ? "*" : "");

        cout << left << setw(10) << label;
        print_set(pr.first);
        int pad = 20 - (int)to_string(pr.first.size()).length();
        if (pad < 0)
            pad = 0;
        cout << string(pad, ' ');

        for (char a : alphabet)
        {
            if (dfa_trans[id].count(a))
                cout << setw(8) << dfa_trans[id].at(a);
            else
                cout << setw(8) << "-";
        }
        cout << (isAccept ? "Yes" : "No") << "\n";
    }
}

int main()
{
    cout << "Enter regular expression (without dots, ending with #): ";

    string input;
    getline(cin, input);
    cout << "--------------------------";
    string withConcat = insertConcat(input);
    string postfix = regexToPostfix(withConcat);

    vector<Node *> leaves;
    Node *root = buildSyntaxTree(postfix, leaves);

    compute_nullable_first_last(root);

    map<int, set<int>> followpos;
    for (auto *leaf : leaves)
        followpos[leaf->position] = set<int>();
    compute_followpos(root, followpos);

    print_first_last_follow(leaves, followpos);

    map<State, int, StateCmp> dfa_states;
    vector<map<char, int>> dfa_trans;
    int accept_state = -1;
    construct_dfa(root, leaves, followpos, dfa_states, dfa_trans, accept_state);

    print_dfa(dfa_states, dfa_trans, accept_state, leaves);

    return 0;
}
