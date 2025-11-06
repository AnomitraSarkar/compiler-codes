#include <iostream>
#include <fstream>
#include <cctype>
#include <unordered_set>
#include <string>
using namespace std;

unordered_set<string> keywords = {
    "int", "float", "if", "else", "while", "for", "return", "void", "char", "double", "long", "short", "switch", "case", "break", "continue", "default", "do", "struct", "typedef"
};

unordered_set<char> operators = {'+', '-', '*', '/', '=', '<', '>', '!', '%'};
unordered_set<char> specialSymbols = {'(', ')', '{', '}', ';', ','};

bool isKeyword(const string& word) {
    return keywords.find(word) != keywords.end();
}

bool isIdentifierStart(char ch) {
    return isalpha(ch) || ch == '_';
}

bool isIdentifierChar(char ch) {
    return isalnum(ch) || ch == '_';
}

void processFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file.\n";
        return;
    }

    // Counters
    int keywordCount = 0;
    int identifierCount = 0;
    int operatorCount = 0;

    string line;
    bool inMultilineComment = false;
    while (getline(file, line)) {
        int i = 0;
        int len = line.length();

        while (i < len) {
            char ch = line[i];

            // Skip whitespace
            if (isspace(ch)) {
                i++;
                continue;
            }

            // Handle single-line comment
            if (ch == '/' && i + 1 < len && line[i + 1] == '/') {
                break; // Skip rest of line
            }

            // Handle multi-line comment start
            if (ch == '/' && i + 1 < len && line[i + 1] == '*') {
                inMultilineComment = true;
                i += 2;
                continue;
            }

            // Inside multi-line comment
            if (inMultilineComment) {
                if (ch == '*' && i + 1 < len && line[i + 1] == '/') {
                    inMultilineComment = false;
                    i += 2;
                } else {
                    i++;
                }
                continue;
            }

            // Handle Identifiers or Keywords
            if (isIdentifierStart(ch)) {
                string token;
                while (i < len && isIdentifierChar(line[i])) {
                    token += line[i++];
                }
                if (isKeyword(token)) {
                    cout << "Keyword: " << token << endl;
                    keywordCount++;
                } else {
                    cout << "Identifier: " << token << endl;
                    identifierCount++;
                }
                continue;
            }

            // Handle Numbers
            if (isdigit(ch)) {
                string number;
                bool isFloat = false;
                while (i < len && (isdigit(line[i]) || line[i] == '.')) {
                    if (line[i] == '.') {
                        if (isFloat) break; // More than one dot
                        isFloat = true;
                    }
                    number += line[i++];
                }
                if (isFloat)
                    cout << "Float: " << number << endl;
                else
                    cout << "Integer: " << number << endl;
                continue;
            }

            // Handle Operators
            if (operators.find(ch) != operators.end()) {
                cout << "Operator: " << ch << endl;
                operatorCount++;
                i++;
                continue;
            }

            // Handle Special Symbols
            if (specialSymbols.find(ch) != specialSymbols.end()) {
                cout << "Special Symbol: " << ch << endl;
                i++;
                continue;
            }

            // If none matched
            cout << "Error: Invalid token starting with '" << ch << "'\n";
            i++;
        }
    }

    file.close();

    // Final Summary
    cout << "\nSummary:\n";
    cout << "Total Keywords: " << keywordCount << endl;
    cout << "Total Identifiers: " << identifierCount << endl;
    cout << "Total Operators: " << operatorCount << endl;
    cout << "Sum Total (Keywords + Identifiers + Operators): " 
         << (keywordCount + identifierCount + operatorCount) << endl;
}

int main() {
    string filename;
    cout << "Enter the filename to analyze: ";
    cin >> filename;

    processFile(filename);
    return 0;
}
