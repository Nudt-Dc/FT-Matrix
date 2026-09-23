#pragma once
#pragma once
#include "data_structure.h"

// TODO

// remove the .loop label in start_pos
// and .endloop label in end_pos
void RemoveLoop(int start_pos, int end_pos);
void ResetIns(Instruction* x);

// reset a bolck (index in program_block)
void ResetBlock(int index);

// remove the block in pos (block_pointer_list)
void RemoveBlock(int pos);

// merge the block in pos and the previous block (pos - 1) (block_pointer_list)
// return: 
// 0: not merged
// 1: merged
int MergeBlock(int pos);




// find all temporary variables
// from block[start_pos] to block[end_pos]
void TemporaryVar(int start_pos, int end_pos);


string ExactReplace(string src, string pattern, string suffix);

// rename the temporary variables with in tem_var_set in Instruction x 
// through adding a suffix 
void RenameVar(Instruction* ins, Instruction* ins_ori, string suffix);

// rebuild the ins_str of a instruction 
// according to its content
void ReBuildInsStr(Instruction* ins);


// double the possible instructions of a original block
// and stored the new block into returned block*
// only support most inner loop that contains exactly 1 block now
// cases:
// 1. VLDW
// 2. VSTW
// 3. IMM
bool MergeIns(Block* blk);

bool DoubleCheck(Block* blk, int index);

// copy a block from linear_program.block_pointer_list[src]
// to linear_program.program_block[dst], dst = linear_program.total_block_num
Block* CopyBlockByIndex(int src);

int UnrollingLoop(int end_pos, int unrolling_factor, int double_extension);


int UnrollingMostInnerLoop(int start_pos, int end_pos, int unrolling_factor, int double_extension);

void UnrollingProgram(int double_extension);

int UnrollChanceCheck(int start_pos, int end_pos, int unrolling_factor, int trip_num, string loop_name);