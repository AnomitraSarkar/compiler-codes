#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <stack>
#include <algorithm>
#include <memory>
using namespace std;

const char EPSILON = '@';  // Using @ to represent epsilon
const char END_MARKER = '#';  // Standard end marker

struct Node {
    char val;
    unique_ptr<Node> left, right;
    int pos;
    bool nullable;
    set<int> firstpos, lastpos;
    
    Node(char v, unique_ptr<Node> l = nullptr, unique_ptr<Node> r = nullptr, int p = -1) 
        : val(v), left(move(l)), right(move(r)), pos(p) {}
};

void printSet(const set<int>& s) {
    cout << "{";
    for(auto it = s.begin(); it != s.end(); ++it) {
        if(it != s.begin()) cout << ", ";
        cout << *it;
    }
    cout << "}";
}

unique_ptr<Node> constructSyntaxTree(const string& regex, map<int, char>& posToChar) {
    stack<unique_ptr<Node>> st;
    int position = 1;
    
    auto applyOperator = [&](unique_ptr<Node> op_node) {
        if(st.size() < 1) {
            cerr << "Error: Not enough operands for operator " << op_node->val << endl;
            return;
        }
        op_node->right = move(st.top()); st.pop();
        if(op_node->val != '*') {
            if(st.empty()) {
                cerr << "Error: Not enough operands for operator " << op_node->val << endl;
                return;
            }
            op_node->left = move(st.top()); st.pop();
        }
        st.push(move(op_node));
    };
    
    for(size_t i = 0; i < regex.size(); ++i) {
        char c = regex[i];
        
        if(c == ' ') continue;
        
        if(c == '(') {
            st.push(make_unique<Node>(c));
        } 
        else if(c == '|' || c == '.') {
            st.push(make_unique<Node>(c));
        }
        else if(c == '*') {
            if(st.empty()) {
                cerr << "Error: Nothing to repeat\n";
                return nullptr;
            }
            applyOperator(make_unique<Node>(c));
        }
        else if(c == ')') {
            stack<unique_ptr<Node>> temp_stack;
            while(!st.empty() && st.top()->val != '(') {
                temp_stack.push(move(st.top())); st.pop();
            }
            
            if(st.empty()) {
                cerr << "Error: Unbalanced parentheses\n";
                return nullptr;
            }
            st.pop(); // Remove the '('
            
            // Rebuild the subtree
            unique_ptr<Node> subtree;
            while(!temp_stack.empty()) {
                auto node = move(temp_stack.top()); temp_stack.pop();
                if(node->val == '|' || node->val == '.') {
                    applyOperator(move(node));
                } else {
                    st.push(move(node));
                }
            }
        }
        else {
            st.push(make_unique<Node>(c, nullptr, nullptr, position));
            posToChar[position] = c;
            position++;
        }
    }

    // Handle implicit concatenation
    stack<unique_ptr<Node>> temp_stack;
    while(!st.empty()) {
        temp_stack.push(move(st.top())); st.pop();
    }
    
    while(temp_stack.size() > 1) {
        auto right = move(temp_stack.top()); temp_stack.pop();
        auto left = move(temp_stack.top()); temp_stack.pop();
        
        // Check if we need to apply an operator
        if(left->val == '|' || left->val == '.') {
            left->right = move(right);
            if(left->val != '*') {
                if(temp_stack.empty()) {
                    cerr << "Error: Not enough operands for operator " << left->val << endl;
                    return nullptr;
                }
                left->left = move(temp_stack.top()); temp_stack.pop();
            }
            temp_stack.push(move(left));
        } else {
            // Implicit concatenation
            auto concat = make_unique<Node>('.', move(left), move(right));
            temp_stack.push(move(concat));
        }
    }

    if(temp_stack.empty()) {
        cerr << "Error: Empty regular expression\n";
        return nullptr;
    }

    // Add end marker
    auto root = move(temp_stack.top());
    auto endMarker = make_unique<Node>('#', nullptr, nullptr, position);
    posToChar[position] = '#';
    return make_unique<Node>('.', move(root), move(endMarker));
}

void calculateFunctions(Node* node) {
    if(!node) return;
    
    calculateFunctions(node->left.get());
    calculateFunctions(node->right.get());
    
    if(node->val == '|') {
        node->nullable = node->left->nullable || node->right->nullable;
        node->firstpos = node->left->firstpos;
        node->firstpos.insert(node->right->firstpos.begin(), node->right->firstpos.end());
        node->lastpos = node->left->lastpos;
        node->lastpos.insert(node->right->lastpos.begin(), node->right->lastpos.end());
    } 
    else if(node->val == '.') {
        node->nullable = node->left->nullable && node->right->nullable;
        node->firstpos = node->left->firstpos;
        if(node->left->nullable) {
            node->firstpos.insert(node->right->firstpos.begin(), node->right->firstpos.end());
        }
        node->lastpos = node->right->lastpos;
        if(node->right->nullable) {
            node->lastpos.insert(node->left->lastpos.begin(), node->left->lastpos.end());
        }
    } 
    else if(node->val == '*') {
        node->nullable = true;
        node->firstpos = node->left->firstpos;
        node->lastpos = node->left->lastpos;
    } 
    else {
        node->nullable = (node->val == EPSILON);
        if(node->pos != -1) {
            node->firstpos.insert(node->pos);
            node->lastpos.insert(node->pos);
        }
    }
}

void calculateFollowPos(Node* node, map<int, set<int>>& followpos) {
    if(!node) return;
    
    if(node->val == '.') {
        for(int i : node->left->lastpos) {
            followpos[i].insert(node->right->firstpos.begin(), node->right->firstpos.end());
        }
    } 
    else if(node->val == '*') {
        for(int i : node->lastpos) {
            followpos[i].insert(node->firstpos.begin(), node->firstpos.end());
        }
    }
    
    calculateFollowPos(node->left.get(), followpos);
    calculateFollowPos(node->right.get(), followpos);
}

void constructDFA(unique_ptr<Node>& root, const map<int, char>& posToChar, const map<int, set<int>>& followpos) {
    set<int> startState = root->firstpos;
    vector<set<int>> states = {startState};
    map<pair<set<int>, char>, set<int>> transition;
    set<set<int>> acceptingStates;
    
    int endPos = posToChar.rbegin()->first;
    
    for(size_t i = 0; i < states.size(); ++i) {
        set<int> currState = states[i];
        
        if(currState.find(endPos) != currState.end()) {
            acceptingStates.insert(currState);
        }
        
        map<char, set<int>> charToPositions;
        for(int pos : currState) {
            char c = posToChar.at(pos);
            if(c != '#') {
                charToPositions[c].insert(pos);
            }
        }
        
        for(auto& entry : charToPositions) {
            char c = entry.first;
            set<int> nextState;
            
            for(int pos : entry.second) {
                if(followpos.count(pos)) {
                    nextState.insert(followpos.at(pos).begin(), followpos.at(pos).end());
                }
            }
            
            if(!nextState.empty()) {
                auto it = find(states.begin(), states.end(), nextState);
                if(it == states.end()) {
                    states.push_back(nextState);
                }
                transition[{currState, c}] = nextState;
            }
        }
    }
    
    cout << "\nDFA States:\n";
    for(size_t i = 0; i < states.size(); ++i) {
        cout << "State " << i << ": ";
        printSet(states[i]);
        if(acceptingStates.count(states[i])) {
            cout << " (accepting)";
        }
        cout << endl;
    }
    
    cout << "\nTransition Table:\n";
    cout << "State\tSymbol\tNext State\n";
    for(auto& entry : transition) {
        set<int> fromState = entry.first.first;
        char symbol = entry.first.second;
        set<int> toState = entry.second;
        
        auto fromIt = find(states.begin(), states.end(), fromState);
        auto toIt = find(states.begin(), states.end(), toState);
        
        cout << distance(states.begin(), fromIt) << "\t" << symbol << "\t" 
             << distance(states.begin(), toIt) << endl;
    }
}

void processRegex(const string& regex) {
    cout << "\nProcessing regex: " << regex << endl;
    
    map<int, char> posToChar;
    auto root = constructSyntaxTree(regex, posToChar);
    if(!root) {
        cerr << "Failed to construct syntax tree\n";
        return;
    }
    
    calculateFunctions(root.get());
    
    map<int, set<int>> followpos;
    calculateFollowPos(root.get(), followpos);
    
    cout << "\nPositions to characters mapping:\n";
    for(auto& entry : posToChar) {
        cout << "Position " << entry.first << ": " << entry.second << endl;
    }
    
    cout << "\nFollowpos:\n";
    for(auto& entry : followpos) {
        cout << "followpos(" << entry.first << ") = ";
        printSet(entry.second);
        cout << endl;
    }
    
    constructDFA(root, posToChar, followpos);
}

int main() {
    // Test cases
    cout << "Testing simple patterns first...\n";
    // processRegex("ab");
    // processRegex("a|b");
    processRegex("(a|b)*abb#");
    
    cout << "\nTesting more complex patterns...\n";
    processRegex("(a|b)*abb");
    // processRegex("a*b*a(a|b)*b*a"); // Test after simpler cases work
    
    return 0;
}