#pragma once
#include <stdio.h>
#include <cstdlib>
#include "data_structure.h"

using namespace std;

// Split variable pairs into variables 
VarList* SplitVar(string& var);

// Judging that the variables are equal, including variables pairs a1:a0 and *AR++[OR]
bool RegEqual(string& str_conj, string& str);

// Determine whether the two mem_info are dependent
bool MemRely(const MemInfo& fa, const MemInfo& chl);

// Build Dependency for linear program
void BuildDependency();

// Build instruction dependency for each instruction in each block
void BuildInstrDependency(Block* b, Instruction* x);

// Build read dependency
void BuildReadDependency(Block* b, Instruction* x, VarList* vl);
// Build write dependency
void BuildWriteDependency(Block* b, Instruction* x, VarList* vl);
// Build mem dependency
void BuildMemDependency(Block* b, Instruction* x, VarList* vl, int load_store);

// Print Instruction Dependency File graph.txt
void PrintDependencyGraph(const char* graph_file);
