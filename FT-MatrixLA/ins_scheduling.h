#pragma once
#include <stdio.h>
#include <cstdlib>
#include "data_structure.h"

using namespace std;

// assumimng the maximum executing cycles of each block
const int max_num_of_instr = 8192;

// Add a instruction of SNOP 1 for register allocation
Instruction InstrAddSnop();
// Find the proper function unit and launch cycle in ins_scheduling_queue
pair<int, int> FindFit(Block* block, vector<int> func_unit, int start, int cycle, int read_cycle, int write_cycle);
// Find the proper launch cycle of a pair of instructions in ins_scheduling_queue
int FindPairFit(Block* block, vector<int> func_unit, int start, int cycle, int read_cycle, int write_cycle);
// Insert the instruction to the proper place
static void Place(Block* block, Instruction* x, int no_func, int time_pos);

// unplace the instruction for multiple read/write cycle, avoiding for output repeatly
//void unplace(Block* block, Instruction* x, int no_func, int time_pos);

// Add MAC unit to the ins_name, .M1/.M2...
void ReallocMAC(Instruction* x, int no_func);
// Select the instructions whose indge = 0 and push them into zero_indeg_instr
void LoadZeroIndegInstr(Block* block);
// Instruction Scheduling for each block
void InsScheduling(const char* opt_option);
// Scheduling with option -o0
void O0Scheduling(Block* x);
// Scheduling with option -o1
void O1Scheduling(Block* x);
// Print the scheduling results into transition file
void PrintSchedulingTransistion(const char* transition_file);
void StatusValue(Block* block);
int StatusCompute(Instruction* x);
// Scheduling with DP
void O1SchedulingDP(Block* block);