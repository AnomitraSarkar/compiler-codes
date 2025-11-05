#include <iostream>
#include <vector>
#include <string>
#include <sstream>

class Quadruple {
public:
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string result;
    std::string jumpTarget; // ✅ New field for printing only

    Quadruple(const std::string& op, const std::string& arg1, const std::string& arg2, const std::string& result)
        : op(op), arg1(arg1), arg2(arg2), result(result), jumpTarget("_") {}

    void display() const {
        // ✅ Enhanced printing logic
        if (op == "if") {
            // (if, a, <, b, 5)
            std::cout << "(" << op << ", " << arg1 << ", " << arg2 << ", " << result;
            if (jumpTarget != "_" && !jumpTarget.empty())
                std::cout << ", " << jumpTarget << ")";
            else
                std::cout << ")";
        }
        else if (op == "goto") {
            std::cout << "(" << op << ", , , ";
            if (jumpTarget != "_" && !jumpTarget.empty())
                std::cout << jumpTarget << ")";
            else
                std::cout << "_)";
        }
        else if (op == "label") {
            std::cout << "(" << op << ", " << arg1 << ", , )";
        }
        else {
            std::cout << "(" << op << ", " << arg1 << ", " << arg2 << ", " << result << ")";
        }
    }
};

class Backpatching {
private:
    std::vector<Quadruple> tac;
    int nextQuad;
    int tempCounter;

public:
    Backpatching() : nextQuad(1), tempCounter(1) {}

    std::vector<int> makeList(int quad) { return {quad}; }

    std::vector<int> merge(const std::vector<int>& list1, const std::vector<int>& list2) {
        std::vector<int> result = list1;
        result.insert(result.end(), list2.begin(), list2.end());
        return result;
    }

    void backpatch(std::vector<int>& list, int target) {
        for (int quad : list) {
            if (quad - 1 < tac.size()) {
                std::stringstream ss;
                ss << target;
                // ✅ Store target separately for printing
                tac[quad - 1].jumpTarget = ss.str();
            }
        }
    }

    std::string newTemp() { return "t" + std::to_string(tempCounter++); }

    int nextquad() { return nextQuad++; }

    void emit(const std::string& op, const std::string& arg1, const std::string& arg2, const std::string& result) {
        tac.emplace_back(op, arg1, arg2, result);
        std::cout << nextQuad - 1 << ": ";
        tac.back().display();
        std::cout << std::endl;
    }

    void displayTAC() {
        std::cout << "\nFinal Three Address Code:\n";
        for (size_t i = 0; i < tac.size(); i++) {
            std::cout << i + 1 << ": ";
            tac[i].display();
            std::cout << std::endl;
        }
    }

    void generateTAC() {
        std::cout << "Generating Three Address Code for: ((a < b) || (a == b)) && (c > d)\n";
        std::cout << "========================================\n";

        std::vector<int> trueList1, falseList1;

        std::cout << "\n=== Generating code for (a < b) ===\n";
        int quad1 = nextquad();
        emit("if", "a", "<", "b");
        trueList1 = makeList(quad1);

        int quad2 = nextquad();
        emit("goto", "", "", "_");
        std::vector<int> falseList1_part = makeList(quad2);

        std::cout << "\n=== Generating code for (a == b) ===\n";
        int quad3 = nextquad();
        emit("if", "a", "==", "b");
        std::vector<int> trueList2 = makeList(quad3);

        int quad4 = nextquad();
        emit("goto", "", "", "_");
        std::vector<int> falseList2 = makeList(quad4);

        std::cout << "\n=== Backpatching for OR operation ===\n";
        backpatch(falseList1_part, quad3);
        std::vector<int> E1_trueList = merge(trueList1, trueList2);
        std::vector<int> E1_falseList = falseList2;

        std::cout << "\n=== Generating code for (c > d) ===\n";
        int quad5 = nextquad();
        emit("if", "c", ">", "d");
        std::vector<int> trueList3 = makeList(quad5);

        int quad6 = nextquad();
        emit("goto", "", "", "_");
        std::vector<int> falseList3 = makeList(quad6);

        std::cout << "\n=== Backpatching for AND operation ===\n";
        backpatch(E1_trueList, quad5);

        std::vector<int> finalTrueList = trueList3;
        std::vector<int> finalFalseList = merge(E1_falseList, falseList3);

        std::cout << "\n=== Final labels ===\n";
        int trueLabel = nextquad();
        emit("label", "L_TRUE", "", "");
        backpatch(finalTrueList, trueLabel);

        int falseLabel = nextquad();
        emit("label", "L_FALSE", "", "");
        backpatch(finalFalseList, falseLabel);

        displayControlFlow(finalTrueList, finalFalseList, trueLabel, falseLabel);
    }

    void displayControlFlow(const std::vector<int>& trueList, const std::vector<int>& falseList,
                            int trueLabel, int falseLabel) {
        std::cout << "\n=== Final Control Flow ===\n";
        std::cout << "True List (points to L_TRUE): ";
        for (int quad : trueList)
            std::cout << quad << " ";
        std::cout << "\nFalse List (points to L_FALSE): ";
        for (int quad : falseList)
            std::cout << quad << " ";
        std::cout << "\nTrue Label: L_TRUE (Quad " << trueLabel << ")";
        std::cout << "\nFalse Label: L_FALSE (Quad " << falseLabel << ")\n";
    }
};

int main() {
    Backpatching bp;
    bp.generateTAC();
    bp.displayTAC();
    return 0;
}

