#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <unordered_set>
#include <vector>
#include <iomanip>
using namespace std;

// ---------- Lexical Rules ----------
unordered_set<string> keywords = {
    "int", "float", "char", "return", "if", "else", "while", "for", "void", "double", "break", "continue"
};

unordered_set<string> operators = {
    "+", "-", "*", "/", "%", "=", "==", "!=", "<", "<=", ">", ">=", "&&", "||", "!", "&", "|"
};

unordered_set<string> specialSymbols = {
    "(", ")", "{", "}", ";", ","
};

// ---------- Symbol Table ----------
struct Symbol {
    int entryNo;
    string lexeme;
    string tokenType;
    int declaredLine;
    vector<int> usedLines;
};

vector<Symbol> symbolTable;
int entryCounter = 1;

bool isIdentifier(const string& word) {
    return regex_match(word, regex("^[a-zA-Z_][a-zA-Z0-9_]*$"));
}

bool isInteger(const string& word) {
    return regex_match(word, regex("^[0-9]+$"));
}

bool isFloat(const string& word) {
    return regex_match(word, regex("^[0-9]+\\.[0-9]+$"));
}

bool isLiteral(const string& word) {
    return regex_match(word, regex("^\"[^\"]*\"$"));
}

int findSymbol(const string& lexeme) {
    for (int i = 0; i < symbolTable.size(); ++i)
        if (symbolTable[i].lexeme == lexeme)
            return i;
    return -1;
}

void addOrUpdateSymbol(const string& lexeme, const string& tokenType, int lineNo) {
    int index = findSymbol(lexeme);
    if (index == -1) {
        symbolTable.push_back({entryCounter++, lexeme, tokenType, lineNo, {}});
    } else {
        if (symbolTable[index].declaredLine != lineNo)
            symbolTable[index].usedLines.push_back(lineNo);
    }
}

string removeComments(const string& input) {
    string output;
    bool inBlock = false, inLine = false;

    for (size_t i = 0; i < input.length(); ++i) {
        if (!inBlock && !inLine && input[i] == '/' && input[i + 1] == '*') {
            inBlock = true; i++;
        } else if (inBlock && input[i] == '*' && input[i + 1] == '/') {
            inBlock = false; i++;
        } else if (!inBlock && !inLine && input[i] == '/' && input[i + 1] == '/') {
            inLine = true; i++;
        } else if (inLine && input[i] == '\n') {
            inLine = false;
            output += '\n';
        } else if (!inBlock && !inLine) {
            output += input[i];
        }
    }
    return output;
}

void analyzeLine(const string& line, int lineNo) {
    // 🔧 SAFELY FIXED regex for tokens
    regex tokenRegex(R"((\"[^\"]*\"|\d+\.\d+|\d+|==|!=|<=|>=|&&|\|\||[a-zA-Z_][a-zA-Z0-9_]*|[+\-*/%=<>&|!;:,.\[\]{}()]))");
    auto words_begin = sregex_iterator(line.begin(), line.end(), tokenRegex);
    auto words_end = sregex_iterator();

    for (auto it = words_begin; it != words_end; ++it) {
        string token = it->str();

        if (keywords.count(token)) {
            cout << "[Keyword] " << token << " (Line " << lineNo << ")\n";
        } else if (isIdentifier(token)) {
            cout << "[Identifier] " << token << " (Line " << lineNo << ")\n";
            addOrUpdateSymbol(token, "Identifier", lineNo);
        } else if (isInteger(token)) {
            cout << "[Integer] " << token << " (Line " << lineNo << ")\n";
            addOrUpdateSymbol(token, "Integer", lineNo);
        } else if (isFloat(token)) {
            cout << "[Float] " << token << " (Line " << lineNo << ")\n";
            addOrUpdateSymbol(token, "Float", lineNo);
        } else if (isLiteral(token)) {
            cout << "[Literal] " << token << " (Line " << lineNo << ")\n";
            addOrUpdateSymbol(token, "Literal", lineNo);
        } else if (operators.count(token) || specialSymbols.count(token)) {
            cout << "[Operator/Symbol] " << token << " (Line " << lineNo << ")\n";
        } else {
            cerr << "Lexical Error: Invalid token '" << token << "' on Line " << lineNo << "\n";
        }
    }
}

void printSymbolTable() {
    cout << "\n========= SYMBOL TABLE =========\n";
    cout << left << setw(8) << "Entry"
         << setw(16) << "Lexeme"
         << setw(16) << "Token Type"
         << setw(12) << "Declared"
         << "Used Lines\n";
    for (const auto& sym : symbolTable) {
        cout << setw(8) << sym.entryNo
             << setw(16) << sym.lexeme
             << setw(16) << sym.tokenType
             << setw(12) << sym.declaredLine;
        for (int l : sym.usedLines)
            cout << l << " ";
        cout << "\n";
    }
}

int main() {
    string filename;
    cout << "Enter source file name: ";
    cin >> filename;

    ifstream file(filename);
    if (!file) {
        cerr << "Error: Could not open file.\n";
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string code = removeComments(buffer.str());

    istringstream iss(code);
    string line;
    int lineNo = 0;

    while (getline(iss, line)) {
        ++lineNo;
        analyzeLine(line, lineNo);
    }

    printSymbolTable();
    return 0;
}
