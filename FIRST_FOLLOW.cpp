#include <bits/stdc++.h>
using namespace std;

map<char, vector<string>> grammar;
map<char, set<char>> firstSet, followSet;
set<char> nonTerminals;
char startSymbol;

// Function to compute FIRST of a symbol
void computeFirst(char symbol) {
    if (!firstSet[symbol].empty()) return; // already computed

    // If terminal
    if (!isupper(symbol)) {
        firstSet[symbol].insert(symbol);
        return;
    }

    // For each production of the non-terminal
    for (auto production : grammar[symbol]) {
        for (int i = 0; i < production.size(); i++) {
            char ch = production[i];
            computeFirst(ch); // recursive call
            bool epsilonInFirst = false;

            // Add all symbols from FIRST(ch)
            for (auto f : firstSet[ch]) {
                if (f == '#') epsilonInFirst = true; // epsilon
                else firstSet[symbol].insert(f);
            }

            // If epsilon not in FIRST(ch), stop
            if (!epsilonInFirst) break;

            // If epsilon is in FIRST(ch) and this is the last symbol, add epsilon
            if (epsilonInFirst && i == production.size() - 1)
                firstSet[symbol].insert('#');
        }
    }
}

// Function to compute FOLLOW of a non-terminal
void computeFollow(char symbol) {
    if (!followSet[symbol].empty() && symbol != startSymbol) return;

    // Start symbol always gets '$'
    if (symbol == startSymbol)
        followSet[symbol].insert('$');

    // For all productions
    for (auto &p : grammar) {
        char lhs = p.first;
        for (auto production : p.second) {
            for (int i = 0; i < production.size(); i++) {
                if (production[i] == symbol) {
                    bool epsilonNext = false;
                    // If not the last symbol
                    if (i + 1 < production.size()) {
                        char next = production[i + 1];
                        // Add FIRST(next) except epsilon
                        for (auto f : firstSet[next]) {
                            if (f != '#')
                                followSet[symbol].insert(f);
                            else
                                epsilonNext = true;
                        }
                        // If epsilon is in FIRST(next), add FOLLOW(lhs)
                        if (epsilonNext)
                            computeFollow(lhs),
                            followSet[symbol].insert(followSet[lhs].begin(), followSet[lhs].end());
                    } 
                    // If last symbol, add FOLLOW(lhs)
                    else if (i + 1 == production.size() && lhs != symbol) {
                        computeFollow(lhs);
                        followSet[symbol].insert(followSet[lhs].begin(), followSet[lhs].end());
                    }
                }
            }
        }
    }
}

int main() {
    int n;
    cout << "Enter number of productions: ";
    cin >> n;

    cout << "Enter productions (example: E->E+T or E->T):\n";
    for (int i = 0; i < n; i++) {
        string s;
        cin >> s;
        char lhs = s[0];
        startSymbol = (i == 0) ? lhs : startSymbol; // first non-terminal = start
        nonTerminals.insert(lhs);
        string rhs = s.substr(3);
        string temp = "";
        for (int j = 0; j < rhs.size(); j++) {
            if (rhs[j] == '|') {
                grammar[lhs].push_back(temp);
                temp = "";
            } else temp += rhs[j];
        }
        grammar[lhs].push_back(temp);
    }

    // Compute FIRST sets
    for (auto nt : nonTerminals)
        computeFirst(nt);

    cout << "\nFIRST Sets:\n";
    for (auto nt : nonTerminals) {
        cout << "FIRST(" << nt << ") = { ";
        for (auto it : firstSet[nt])
            cout << it << " ";
        cout << "}\n";
    }

    // Compute FOLLOW sets
    for (auto nt : nonTerminals)
        computeFollow(nt);

    cout << "\nFOLLOW Sets:\n";
    for (auto nt : nonTerminals) {
        cout << "FOLLOW(" << nt << ") = { ";
        for (auto it : followSet[nt])
            cout << it << " ";
        cout << "}\n";
    }

    return 0;
}
