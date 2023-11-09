#pragma once
#include "data_structure.h"

extern map<int, int> fix_destination;
extern int flag_keep_alive;

// aliveness analysis main function
// fix_mode configuration
// 1: make the fix variables in cylcle completely alive
void AnalyseAliveness(const int fix_mode);

// return whether the variable is missing in one cycle
int CheckAliveVarOmission(const string* var, const int& cycle, const int& type);

// add alive variable in alive_chart in one cycle
void AddAliveVar(string* var, const int& cycle, int type);

// delete alive variable in alive_chart in one cycle
void DeleteAliveVar(const string* var, const int& cycle, int type);

// initial the alive_chart in one block
void InitAliveChart();

// update the alive chart per operand
void UpdateVarAliveness(const string& var, const int& read_mode);

// seperate the operand
void SeparateVariable(const string& operand, int read_mode = 1);

// record fix variables and complete their alivenesses
void CompleteFixVariables();

// fix all variables which are alive at the entrance of the cycle
void FixCycleVar();

// print the list of variable alive in each cycle
void DebugPrintAliveChart();

// print the fix variable in each block
void DebugPrintFixVariables();