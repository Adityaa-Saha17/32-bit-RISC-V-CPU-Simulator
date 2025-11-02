#include <bits/stdc++.h>
#include "Executer.cpp"
using namespace std;

int main(){
    // Fetch
    ifstream file("input.txt");
    string line;

    // Decode
    while (getline(file, line)){
        if (line.empty()) continue;
        assembler(line);
    }
    
    // Execute
    execute();
    return 0;
}