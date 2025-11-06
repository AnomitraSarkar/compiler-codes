#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <unordered_map>

using namespace std;

// Predefined action and goto tables for the grammar
vector<unordered_map<string, string>> actionTable = {
    {{"id", "s5"}, {"(", "s4"}},
    {{"+", "s6"}, {"$", "acc"}},
    {{"+", "r2"}, {"*", "s7"}, {")", "r2"}, {"$", "r2"}},
    {{"+", "r4"}, {"*", "r4"}, {")", "r4"}, {"$", "r4"}},
    {{"id", "s5"}, {"(", "s4"}},
    {{"+", "r6"}, {"*", "r6"}, {")", "r6"}, {"$", "r6"}},
    {{"id", "s5"}, {"(", "s4"}},
    {{"id", "s5"}, {"(", "s4"}},
    {{"+", "s6"}, {")", "s11"}},
    {{"+", "r1"}, {"*", "s7"}, {")", "r1"}, {"$", "r1"}},
    {{"+", "r3"}, {"*", "r3"}, {")", "r3"}, {"$", "r3"}},
    {{"+", "r5"}, {"*", "r5"}, {")", "r5"}, {"$", "r5"}}
};

vector<unordered_map<string, int>> gotoTable = {
    {{"E", 1}, {"T", 2}, {"F", 3}},
    {},
    {},
    {},
    {{"E", 8}, {"T", 2}, {"F", 3}},
    {},
    {{"T", 9}, {"F", 3}},
    {{"F", 10}},
    {},
    {},
    {},
    {}
};

vector<pair<string, int>> productions = {
    {"E", 3},  // E -> E + T
    {"E", 1},  // E -> T
    {"T", 3},  // T -> T * F
    {"T", 1},  // T -> F
    {"F", 3},  // F -> ( E )
    {"F", 1}   // F -> id
};

vector<string> productionDisplay;

vector<string> tokenize(const string& input) {
    vector<string> tokens;
    stringstream ss(input);
    string token;
    while (ss >> token) {
        if (token == "0") break;
        tokens.push_back(token);
    }
    tokens.push_back("$");
    return tokens;
}

int main() {
    int numProductions;
    cout << "Enter number of productions in the grammar: ";
    cin >> numProductions;
    cin.ignore(); // To ignore the newline after reading number

    productionDisplay.resize(numProductions);
    cout << "Enter productions (LHS -> RHS, space-separated RHS symbols):" << endl;
    for (int i = 0; i < numProductions; i++) {
        string production;
        getline(cin, production);
        productionDisplay[i] = production;
    }

    string input;
    cout << "Enter input strings to parse. Enter '0' to quit." << endl;
    cout << "Input string (Enter 0 to exit): ";
    getline(cin, input);

    if (input == "0") return 0;

    vector<string> tokens = tokenize(input);
    vector<int> stateStack = {0};
    vector<string> symbolStack = {"$"};

    cout << left << setw(20) << "Stack" << setw(20) << "Input Buffer" << "Action" << endl;

    int index = 0;
    while (index < tokens.size()) {
        string currentToken = tokens[index];
        int currentState = stateStack.back();
        string action;

        if (currentState < actionTable.size() && actionTable[currentState].find(currentToken) != actionTable[currentState].end()) {
            action = actionTable[currentState][currentToken];
        } else {
            action = "e";
        }

        if (action[0] == 's') {
            int nextState = stoi(action.substr(1));
            stateStack.push_back(nextState);
            symbolStack.push_back(currentToken);
            index++;

            string stackStr;
            for (const auto& sym : symbolStack) stackStr += sym + " ";
            string inputStr;
            for (int i = index; i < tokens.size(); i++) inputStr += tokens[i] + " ";

            cout << left << setw(20) << stackStr << setw(20) << inputStr << "Shift " << currentToken << endl;
        } else if (action[0] == 'r') {
            int prodIndex = stoi(action.substr(1)) - 1;
            string lhs = productions[prodIndex].first;
            int popCount = productions[prodIndex].second;

            for (int i = 0; i < popCount; i++) {
                stateStack.pop_back();
                symbolStack.pop_back();
            }

            int newState = stateStack.back();
            if (gotoTable[newState].find(lhs) != gotoTable[newState].end()) {
                int nextState = gotoTable[newState][lhs];
                stateStack.push_back(nextState);
                symbolStack.push_back(lhs);
            }

            string stackStr;
            for (const auto& sym : symbolStack) stackStr += sym + " ";
            string inputStr;
            for (int i = index; i < tokens.size(); i++) inputStr += tokens[i] + " ";

            cout << left << setw(20) << stackStr << setw(20) << inputStr << "Reduce by: " << productionDisplay[prodIndex] << endl;
        } else if (action == "acc") {
            string stackStr;
            for (const auto& sym : symbolStack) stackStr += sym + " ";
            string inputStr;
            for (int i = index; i < tokens.size(); i++) inputStr += tokens[i] + " ";

            cout << left << setw(20) << stackStr << setw(20) << inputStr << "Accept" << endl;
            break;
        } else {
            cout << "Error: Invalid action for state " << currentState << " and token " << currentToken << endl;
            break;
        }
    }

    return 0;
}
