#include <bits/stdc++.h>
using namespace std;

map<char, vector<string>> grammar;
map<char, set<char>> leading, trailing;
set<char> nonTerminals;

void findLeading(char A) {
    for (auto prod : grammar[A]) {
        // Rule 1: If first symbol is terminal
        if (!isupper(prod[0]))
            leading[A].insert(prod[0]);

        // Rule 2: If first symbol is non-terminal, add its LEADING
        if (isupper(prod[0]) && prod.size() > 0)
            for (auto x : leading[prod[0]])
                leading[A].insert(x);

        // Rule 3: If second symbol exists and is terminal (A → B a)
        if (isupper(prod[0]) && prod.size() > 1 && !isupper(prod[1]))
            leading[A].insert(prod[1]);
    }
}

void findTrailing(char A) {
    for (auto prod : grammar[A]) {
        int n = prod.size();

        // Rule 1: If last symbol is terminal
        if (!isupper(prod[n - 1]))
            trailing[A].insert(prod[n - 1]);

        // Rule 2: If last symbol is non-terminal, add its TRAILING
        if (isupper(prod[n - 1]) && n > 0)
            for (auto x : trailing[prod[n - 1]])
                trailing[A].insert(x);

        // Rule 3: If before last is terminal (A → a B)
        if (isupper(prod[n - 1]) && n > 1 && !isupper(prod[n - 2]))
            trailing[A].insert(prod[n - 2]);
    }
}

int main() {
    int n;
    cout << "Enter number of productions: ";
    cin >> n;

    cout << "Enter productions (example: E->E+T or T->i):\n";
    for (int i = 0; i < n; i++) {
        string s;
        cin >> s;
        char lhs = s[0];
        string rhs = s.substr(3);
        grammar[lhs].push_back(rhs);
        nonTerminals.insert(lhs);
    }

    bool changed;
    do {
        changed = false;
        auto oldLeading = leading;
        auto oldTrailing = trailing;

        for (auto nt : nonTerminals) {
            findLeading(nt);
            findTrailing(nt);
        }

        if (oldLeading != leading || oldTrailing != trailing)
            changed = true;

    } while (changed);

    cout << "\nLEADING sets:\n";
    for (auto nt : nonTerminals) {
        cout << "LEADING(" << nt << ") = { ";
        for (auto x : leading[nt])
            cout << x << " ";
        cout << "}\n";
    }

    cout << "\nTRAILING sets:\n";
    for (auto nt : nonTerminals) {
        cout << "TRAILING(" << nt << ") = { ";
        for (auto x : trailing[nt])
            cout << x << " ";
        cout << "}\n";
    }

    return 0;
}
