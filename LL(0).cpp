#include <bits/stdc++.h>

using namespace std;

using Symbol = string;

struct Production {
    Symbol lhs;
    vector<Symbol> rhs;
};

vector<Production> productions;
set<Symbol> terminals, nonTerminals;
map<Symbol, set<Symbol>> FIRST, FOLLOW;
map<pair<Symbol, Symbol>, vector<Symbol>> parsingTable;
Symbol startSymbol;

// Check if a symbol is terminal
bool isTerminal(const Symbol& s) {
    return terminals.count(s) > 0;
}

// Tokenizer that splits spaces and also parentheses/brackets into separate tokens
vector<Symbol> tokenizeWithParentheses(const string& str) {
    vector<Symbol> tokens;
    stringstream ss(str);
    string temp;
    while (ss >> temp) {
        size_t pos = 0;
        while (pos < temp.size()) {
            if (temp[pos] == '(' || temp[pos] == ')' ||
                temp[pos] == '{' || temp[pos] == '}' ||
                temp[pos] == '[' || temp[pos] == ']') {
                tokens.push_back(string(1, temp[pos]));
                pos++;
            } else {
                size_t start = pos;
                while (pos < temp.size() &&
                       temp[pos] != '(' && temp[pos] != ')' &&
                       temp[pos] != '{' && temp[pos] != '}' &&
                       temp[pos] != '[' && temp[pos] != ']')
                    pos++;
                tokens.push_back(temp.substr(start, pos - start));
            }
        }
    }
    return tokens;
}

// Compute FIRST set
set<Symbol> computeFIRST(const Symbol &sym) {
    if (FIRST.count(sym)) return FIRST[sym];
    set<Symbol> result;
    if (isTerminal(sym) || sym == "epsilon") {
        result.insert(sym);
        return FIRST[sym] = result;
    }
    for (const auto& prod : productions) {
        if (prod.lhs == sym) {
            bool epsilonAll = true;
            for (const Symbol& s : prod.rhs) {
                set<Symbol> firstS = computeFIRST(s);
                for (const Symbol& f : firstS) {
                    if (f != "epsilon") result.insert(f);
                }
                if (!firstS.count("epsilon")) {
                    epsilonAll = false;
                    break;
                }
            }
            if (epsilonAll) result.insert("epsilon");
        }
    }
    return FIRST[sym] = result;
}

// Compute FOLLOW set
set<Symbol> computeFOLLOW(const Symbol &sym) {
    if (FOLLOW.count(sym)) return FOLLOW[sym];
    set<Symbol> result;
    if (sym == startSymbol) result.insert("$");
    for (const auto &prod : productions) {
        for (size_t i = 0; i < prod.rhs.size(); ++i) {
            if (prod.rhs[i] == sym) {
                bool epsilonAll = true;
                for (size_t j = i + 1; j < prod.rhs.size(); ++j) {
                    set<Symbol> firstS = computeFIRST(prod.rhs[j]);
                    for (const Symbol& f : firstS) {
                        if (f != "epsilon") result.insert(f);
                    }
                    if (!firstS.count("epsilon")) {
                        epsilonAll = false;
                        break;
                    }
                }
                if (i + 1 == prod.rhs.size() || epsilonAll) {
                    if (prod.lhs != sym) {
                        set<Symbol> followLHS = computeFOLLOW(prod.lhs);
                        result.insert(followLHS.begin(), followLHS.end());
                    }
                }
            }
        }
    }
    return FOLLOW[sym] = result;
}

// Build LL(1) table
void buildParsingTable() {
    for (const auto &prod : productions) {
        bool epsilonAll = true;
        for (const Symbol &s : prod.rhs) {
            set<Symbol> firstS = computeFIRST(s);
            for (const Symbol& f : firstS) {
                if (f != "epsilon")
                    parsingTable[{prod.lhs, f}] = prod.rhs;
            }
            if (!firstS.count("epsilon")) {
                epsilonAll = false;
                break;
            }
        }
        if (epsilonAll) {
            for (const Symbol &f : computeFOLLOW(prod.lhs))
                parsingTable[{prod.lhs, f}] = prod.rhs;
        }
    }
}

// Display FIRST and FOLLOW sets side by side in one table
void displayFirstFollowCombined() {
    cout << "\nFIRST and FOLLOW Sets (side-by-side):\n";
    cout << "+--------------+-------------------------+-------------------------+\n";
    cout << "| Non-Terminal | FIRST                   | FOLLOW                  |\n";
    cout << "+--------------+-------------------------+-------------------------+\n";
    for (const auto& nt : nonTerminals) {
        cout << "| " << setw(12) << nt << " | ";

        // Print FIRST set
        for (const auto& f : FIRST[nt]) cout << f << " ";
        int firstSetWidth = 25;
        int firstSetLen = 0;
        for (const auto& f : FIRST[nt]) firstSetLen += (int)f.size() + 1;
        for (int i = 0; i < firstSetWidth - firstSetLen; i++) cout << " ";

        cout << "| ";

        // Print FOLLOW set
        for (const auto& f : FOLLOW[nt]) cout << f << " ";
        int followSetWidth = 25;
        int followSetLen = 0;
        for (const auto& f : FOLLOW[nt]) followSetLen += (int)f.size() + 1;
        for (int i = 0; i < followSetWidth - followSetLen; i++) cout << " ";

        cout << "|\n";
    }
    cout << "+--------------+-------------------------+-------------------------+\n";
}

// Display parsing table
void displayParsingTable() {
    vector<Symbol> termList(terminals.begin(), terminals.end());
    termList.push_back("$");
    cout << "\nLL(1) Parsing Table:\n";
    cout << "+--------------";
    for (const auto& t : termList) cout << "+-------------";
    cout << "+\n| Non-Terminal";
    for (const auto& t : termList) cout << "| " << setw(11) << t << " ";
    cout << "|\n+--------------";
    for (size_t i = 0; i < termList.size(); ++i) cout << "+-------------";
    cout << "+\n";
    for (const auto& nt : nonTerminals) {
        cout << "| " << setw(12) << nt << " ";
        for (const auto &t : termList) {
            auto key = make_pair(nt, t);
            if (parsingTable.count(key)) {
                cout << "| " << nt << "->";
                for (const auto& s : parsingTable[key]) cout << s << " ";
                cout << " ";
            } else cout << "|     -       ";
        }
        cout << "|\n+--------------";
        for (size_t i = 0; i < termList.size(); ++i) cout << "+-------------";
        cout << "+\n";
    }
}

// Parse and show steps
bool parseString(const vector<Symbol>& tokens) {
    stack<Symbol> st;
    st.push("$");
    st.push(startSymbol);

    size_t ip = 0;
    cout << "\nParsing Steps:\n";
    cout << left << setw(30) << "Stack" << setw(30) << "Input" << "Action\n";
    cout << string(90, '-') << "\n";

    while (!st.empty()) {
        Symbol top = st.top();
        string stackContent;
        {
            stack<Symbol> temp = st;
            vector<Symbol> v;
            while (!temp.empty()) { v.push_back(temp.top()); temp.pop(); }
            reverse(v.begin(), v.end());
            for (const auto& c : v) stackContent += c + " ";
        }
        string inputBuffer;
        for (size_t i = ip; i < tokens.size(); ++i) inputBuffer += tokens[i] + " ";

        cout << setw(30) << stackContent << setw(30) << inputBuffer;

        if (top == tokens[ip]) {
            if (top == "$") {
                cout << "ACCEPT\n";
                return true;
            }
            st.pop(); ++ip;
            cout << "Match " << top << "\n";
        } else if (nonTerminals.count(top)) {
            auto key = make_pair(top, tokens[ip]);
            if (parsingTable.count(key)) {
                st.pop();
                const auto& prod = parsingTable[key];
                cout << top << "->";
                for (const auto& s : prod) cout << s << " ";
                cout << "\n";
                if (prod.size() == 1 && prod[0] == "epsilon") continue;
                for (int i = (int)prod.size() - 1; i >= 0; --i) st.push(prod[i]);
            } else {
                cout << "ERROR: No rule for (" << top << ", " << tokens[ip] << ")\n";
                return false;
            }
        } else {
            cout << "ERROR: Terminal mismatch (" << top << " vs " << tokens[ip] << ")\n";
            return false;
        }
    }
    return false;
}


int main() {
    int n;
    cout << "Enter number of productions: ";
    cin >> n; cin.ignore();
    cout << "Enter productions (e.g., E->T E', E'->+ T E', E'->epsilon, T->( E )):\n";
    for (int i = 0; i < n; ++i) {
        string prod; getline(cin, prod);
        size_t delim = prod.find("->");
        Symbol lhs = prod.substr(0, delim);
        Symbol rhs = prod.substr(delim+2);
        vector<Symbol> rhsTokens = tokenizeWithParentheses(rhs);
        productions.push_back({lhs, rhsTokens});
        nonTerminals.insert(lhs);
        for (const auto& tok : rhsTokens) {
            if (!(isupper(tok[0]) && tok != "epsilon")) {
                if (tok != "epsilon")
                    terminals.insert(tok);
            }
        }
    }
    startSymbol = productions[0].lhs;
    for (const auto& nt : nonTerminals) computeFIRST(nt);
    for (const auto& nt : nonTerminals) computeFOLLOW(nt);
    displayFirstFollowCombined();
    buildParsingTable();
    displayParsingTable();

    while (true) {
        cout << "\nEnter string to parse (tokens separated by space, enter 0 to exit): ";
        string input; 
        getline(cin, input);
        if (input == "0") break;

        vector<Symbol> tokens = tokenizeWithParentheses(input);
        tokens.push_back("$");
        bool accepted = parseString(tokens);

        if (accepted)
            cout << "\nResult: The string IS accepted by the grammar.\n";
        else
            cout << "\nResult: The string is NOT accepted by the grammar.\n";
    }

    cout << "Parser terminated. Goodbye!\n";
    return 0;
}
