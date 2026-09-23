#pragma once
#include "data_structure.h"
#include "ins_scheduling.h"


// Software Pipeline for the most inner loop
void SoftwarePipeline();

// Build the cross-loop dependency 
void BuildExLoopDependency(Block* block);//**************************

// Module Scheduling Algorithm
void ModuleSchedule(Block* block);//, string loop_table, const char* transition_file);

// Get the Init Start Interval
int GetInitT0(Block* block);//*************

// Select the instructions whose indge = 0 and push them into soft_instr_queue
void LoadZeroInstrIntoQueue(Block* block);

// Insert the block into the block pointer list
void InsertBlock(int pos);

// Check the cross-loop restraint
bool ExLoopRestraint(Instruction* x, int fit_pos, int T);//****************

// Instruction Unplacing in Module Scheduling 
void MSUnplace(Block* block, Instruction* x, int no_func, int time_pos, int T);

// Module scheduling failure, restore the instruction state
void MSFailure(Block* block, int T, int LT);

// Find the proper position for the instruction in module scheduling
int MSFindFit(Block* block, Instruction* x, int no_func, int begin, int T);

// Find the proper position in the single loop for the instruction
int LTFindFit(Block* block, Instruction* x, int no_func, int begin, int T);

// Instruction Placing in Module Scheduling 
void MSPlace(Block* block, Instruction* x, int no_func, int fit_pos, int T, int flag = 1);

// Output the Instruction Layout in the single loop
void OutputLT(Block* block, int LT);

// Steady State Layout in Software Pipeline 
void SoftPipeSteady(Block* block);

// Epilog Layout in Software Pipeline
void SoftPipeEpilog(Block* block);

// Prolog Layout in Software Pipeline
void SoftPipeProlog(Block* block);

// Return variable list containing general variables, address variables or AR in one instruction
vector<string> SeperateVar(const string& operand, const int& general, const int& address, const int& ar);

// Get variables which are really read
vector<string> GetReadVar(const Instruction* ins);

// Get variables which are really assigned
vector<string> GetWriteVar(const Instruction* ins);

int CycleTotolNumOfBlock(Block* block);

vector<poslist> GetVarInfoByInstr(Instruction* ins, int current_cycle);

void SeparateVariableInfo(vector<poslist>& var_info, const string operand, Instruction* ins, int current_cycle, int isWrite = 0);

Instruction* across_dependency(int t_cycle/*稳定态周期*/, int ace,/*一个不小于最终总拍数的数*/ vector<poslist> ins);//跨循环依赖分析

void del(int t_cycle/*稳定态周期*/, int ace,/*一个不小于最终总拍数的数*/vector<poslist> ins);

int getAlterNum(Instruction* ins);

bool changeInsPosition(Block* block, Instruction* current_ins, int site, int use_unit, Instruction* ins, int T, int max_cycle);

