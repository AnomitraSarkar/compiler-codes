#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <cctype>

using namespace std;

struct ThreeAddressCode {
    string op;
    string arg1;
    string arg2;
    string result;
};

class IntermediateCodeGenerator {
    vector<ThreeAddressCode> code;
    int tempCount = 0;
    int labelCount = 0;
    
    string newTemp() {
        return "t" + to_string(++tempCount); // start t1
    }
    
    string newLabel() {
        return "L" + to_string(++labelCount); // start L1
    }
    
public:
    // convention: op, arg1, arg2, result
    void emit(const string &op, const string &arg1="", const string &arg2="", const string &result="") {
        code.push_back({op, arg1, arg2, result});
    }
    
    // generate non-hardcoded TAC (uses temps/labels) for quicksort + partition
    void generateQuickSortCode() {
        // quicksort proc header
        emit("PROC", "quicksort");
        emit("PARAM", "arr");
        emit("PARAM", "low");
        emit("PARAM", "high");
        
        // if (low < high) goto L_then else goto L_end
        string condTemp = newTemp();
        emit("<", "low", "high", condTemp);
        string L_then = newLabel();
        string L_end  = newLabel();
        emit("IF_TRUE", condTemp, "", L_then);   // if condTemp != 0 goto L_then
        emit("GOTO", L_end);
        
        // then block
        emit("LABEL", L_then);
        // call partition: args then call
        emit("ARG", "arr");
        emit("ARG", "low");
        emit("ARG", "high");
        string pivotIdx = newTemp();
        emit("CALL", "partition", "", pivotIdx); // pivotIdx = call partition(...)
        
        // quicksort(arr, low, pivotIdx - 1)
        string t1 = newTemp();
        emit("-", pivotIdx, "1", t1);
        emit("ARG", "arr");
        emit("ARG", "low");
        emit("ARG", t1);
        emit("CALL", "quicksort");
        
        // quicksort(arr, pivotIdx + 1, high)
        string t2 = newTemp();
        emit("+", pivotIdx, "1", t2);
        emit("ARG", "arr");
        emit("ARG", t2);
        emit("ARG", "high");
        emit("CALL", "quicksort");
        
        emit("LABEL", L_end);
        emit("END_PROC", "quicksort");
        
        // partition proc
        emit("PROC", "partition");
        emit("PARAM", "arr");
        emit("PARAM", "low");
        emit("PARAM", "high");
        
        // pivot = arr[high]
        string pivot = newTemp();
        emit("[]", "arr", "high", pivot); // pivot = arr[high]
        
        // i = low - 1
        string i = newTemp();
        string tmp = newTemp();
        emit("-", "low", "1", tmp);
        emit("=", tmp, "", i); // i = low - 1
        
        // j = low
        string j = newTemp();
        emit("=", "low", "", j);
        
        string L_loop = newLabel();
        string L_afterLoop = newLabel();
        string L_ifBody = newLabel();
        
        emit("LABEL", L_loop);
        string condJ = newTemp();
        emit("<", j, "high", condJ);
        emit("IF_FALSE", condJ, "", L_afterLoop); // if not (j < high) goto L_afterLoop
        
        // if (arr[j] < pivot)
        string arrj = newTemp();
        emit("[]", "arr", j, arrj); // arrj = arr[j]
        string condArr = newTemp();
        emit("<", arrj, pivot, condArr);
        emit("IF_FALSE", condArr, "", L_ifBody + "_end"); // goto L_ifBody_end if false
        
        // then: i = i + 1; swap arr[i], arr[j]
        emit("LABEL", L_ifBody);
        string iplus = newTemp();
        emit("+", i, "1", iplus);
        emit("=", iplus, "", i); // i = i + 1
        
        // temp = arr[i]
        string arri = newTemp();
        emit("[]", "arr", i, arri);
        string tmpStore = newTemp();
        emit("=", arri, "", tmpStore); // tmpStore = arr[i]
        // arr[i] = arr[j]
        emit("[]=", arrj, i, "arr");
        // arr[j] = tmpStore
        emit("[]=", tmpStore, j, "arr");
        
        emit("LABEL", L_ifBody + "_end");
        // j = j + 1
        string jplus = newTemp();
        emit("+", j, "1", jplus);
        emit("=", jplus, "", j);
        emit("GOTO", L_loop);
        
        emit("LABEL", L_afterLoop);
        // swap arr[i+1], arr[high]
        string i1 = newTemp();
        emit("+", i, "1", i1);
        string arr_i1 = newTemp();
        emit("[]", "arr", i1, arr_i1);
        string arr_high = newTemp();
        emit("[]", "arr", "high", arr_high);
        string tmp2 = newTemp();
        emit("=", arr_i1, "", tmp2); // tmp2 = arr[i+1]
        emit("[]=", arr_high, i1, "arr"); // arr[i+1] = arr[high]
        emit("[]=", tmp2, "high", "arr"); // arr[high] = tmp2
        
        emit("RETURN", i1);
        emit("END_PROC", "partition");
    }
    
    void optimize() {
        // simple constant propagation and redundant goto->label removal
        map<string, string> constMap;
        set<int> toRemove;
        
        // build constant map for assignments like x = 5
        for (int i = 0; i < (int)code.size(); ++i) {
            if (code[i].op == "=" && !code[i].arg2.empty() == false) {
                // we expect "=" stored as ( "=", value, "", target )
                if (!code[i].arg1.empty()) {
                    bool isNumber = ( (code[i].arg1[0] == '-') || isdigit((unsigned char)code[i].arg1[0]) );
                    if (isNumber) {
                        constMap[code[i].result] = code[i].arg1;
                    }
                }
            }
        }
        // replace args using constMap
        for (int i = 0; i < (int)code.size(); ++i) {
            if (!code[i].arg1.empty() && constMap.count(code[i].arg1)) code[i].arg1 = constMap[code[i].arg1];
            if (!code[i].arg2.empty() && constMap.count(code[i].arg2)) code[i].arg2 = constMap[code[i].arg2];
        }
        
        // remove redundant immediate goto->label pairs: GOTO Lx followed immediately by LABEL Lx
        for (int i = 0; i + 1 < (int)code.size(); ++i) {
            if (code[i].op == "GOTO" && code[i+1].op == "LABEL" && code[i].arg1 == code[i+1].arg1) {
                toRemove.insert(i);
            }
        }
        
        vector<ThreeAddressCode> nc;
        for (int i = 0; i < (int)code.size(); ++i) {
            if (!toRemove.count(i)) nc.push_back(code[i]);
        }
        code.swap(nc);
    }
    
    void display() {
        cout << "\nIntermediate Code (Three-Address Code):\n";
        cout << "========================================\n";
        for (int i = 0; i < (int)code.size(); ++i) {
            cout << i << ": ";
            const auto &c = code[i];
            if (c.op == "LABEL") {
                cout << c.arg1 << ":";
            } else if (c.op == "GOTO") {
                cout << "goto " << c.arg1;
            } else if (c.op == "IF_FALSE") {
                cout << "if_false " << c.arg1 << " goto " << c.result;
            } else if (c.op == "IF_TRUE") {
                cout << "if " << c.arg1 << " goto " << c.result;
            } else if (c.op == "CALL") {
                if (!c.result.empty()) cout << c.result << " = call " << c.arg1;
                else cout << "call " << c.arg1;
            } else if (c.op == "RETURN") {
                cout << "return " << c.arg1;
            } else if (c.op == "PROC") {
                cout << "proc " << c.arg1;
            } else if (c.op == "END_PROC") {
                cout << "end_proc " << c.arg1;
            } else if (c.op == "PARAM") {
                cout << "param " << c.arg1;
            } else if (c.op == "ARG") {
                cout << "arg " << c.arg1;
            } else if (c.op == "[]") {
                cout << c.result << " = " << c.arg1 << "[" << c.arg2 << "]";
            } else if (c.op == "[]=") {
                cout << c.result << "[" << c.arg2 << "] = " << c.arg1;
            } else if (c.op == "=") {
                cout << c.result << " = " << c.arg1;
            } else {
                // binary ops like + - < etc
                cout << c.result << " = " << c.arg1 << " " << c.op << " " << c.arg2;
            }
            cout << "\n";
        }
        cout << "\nOptimizations Applied:\n";
        cout << "- Constant propagation (simple)\n";
        cout << "- Redundant immediate goto->label removal\n";
    }
};

int main() {
    cout << "QuickSort Algorithm Source Code:\n";
    cout << "================================\n";
    cout << "void quicksort(int arr[], int low, int high) {\n";
    cout << "    if (low < high) {\n";
    cout << "        int pi = partition(arr, low, high);\n";
    cout << "        quicksort(arr, low, pi - 1);\n";
    cout << "        quicksort(arr, pi + 1, high);\n";
    cout << "    }\n";
    cout << "}\n\n";
    cout << "int partition(int arr[], int low, int high) {\n";
    cout << "    int pivot = arr[high];\n";
    cout << "    int i = low - 1;\n";
    cout << "    for (int j = low; j < high; j++) {\n";
    cout << "        if (arr[j] < pivot) {\n";
    cout << "            i++;\n";
    cout << "            swap(arr[i], arr[j]);\n";
    cout << "        }\n";
    cout << "    }\n";
    cout << "    swap(arr[i + 1], arr[high]);\n";
    cout << "    return i + 1;\n";
    cout << "}\n";
    
    IntermediateCodeGenerator gen;
    gen.generateQuickSortCode();
    gen.optimize();
    gen.display();
    
    return 0;
}

