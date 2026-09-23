#pragma once
#include "data_structure.h"

// initial the particular instruction, SNOP, MOVI, SADDA
void InitParticularInstruction();

// add instruction operate with stack register
// index: 4 expand scalar; 6 expand vector; 8 restore scalar; 10 restore vector
void AddStackInstruction(const int& block, int index);

// add new instruction
void AddNewInstruction(const char* ins_name, string input, string output = "", string note = "", int snop_num = -1);

// add the final SBR instruction
void AddSbr();

// check whether there is any error in the register list/queue
void DebugCheckRegState();

// initial the register size and other information
void InitRegInfo();

// determine whether the variable is alive after this cycle
int IsVarAlive(const string& var, const int& type);

// initial the register information in the current block
void InitBlockState();

// record which reserve register is used
void GetReserveRegister(int reg_type);

// the register is used in current cycle and push it to the back of the idle queue
void UpdateRegIdle(int reg_type, int reg_num);

// select the best register among the idle registers
void SelectIdleRegister(int reg_type);

// insert snop before register overflow
void RegSpillInit();

// push the variable value into the memory and release register
void SaveSpillRegister(int reg_type, int reg_num, int address = -1, int snop = 0, int release = 1);

// restore the variable value back to resgister
void RestoreSpillRegister(int reg_type, int reg_num, int address, string var = "", int snop = -1);

// occupy register for the variables
void OccupyRegister(int reg_type, string var = "", string var2 = "");

// release variable which is dead
void ReleaseDeadVar(const int& reg_type, const int& reg_num);

// release variable which is alive
void ReleaseAliveVar(const int& reg_type, const int& reg_num);

// release register whether the variable is dead or alive
// mode: 0 release buf reg;  1 variable is still alive and need to be saved;  2 variables are no more alive
void ReleaseRegister(int reg_type, int reg_num, int alive = 1);

// assign the fix variable a register or memory
// or update the available queue so that the fix register is at the head of the queue
// or select a suitable register and push it to the head of the available queue
void DealWithFixVar(string var, string var2 = "");

// determine whether the rigister is used by the variables uesd in the same cycle
bool IsInterferingVar(const string& var);

// add the register to the release list
void AddReleaseReg(const int& reg_type, const int& reg_num, const int& fix);

// transfer the variable to a new register and restore its partner to the corresponding register
int TransferVarPair(const int& reg_type, const int& reg_num, const string& var);

// put the variables, one of which is saved in memory, in the suitable registers in pair
int RepairRegPair(string var, string restore_var);

// get variables used in the same cycle
int GetInterferingVar(const string& operand, const int& cycle = 0);

// convert variables to registers which means assigning register
void ConvertVarToReg(string& operand, const int& repeat_type);

// release the variables which are dead this cycle
void ReleaseRegThisCycle();

// update registers information while restore fix variables
void ReleaseForFixVar(const int& reg_type, const int& src_num, const int& fix_num);

// exchange registers information while restore fix variables
void ExchangeForFixVar(const int& reg_type, const int& src_num, const int& fix_num);

// restore the fix variables back to their fix registers
void PushBackFixVar(const int& reg_type, const int& src_num, const int& fix_num);

// restore fix variables at the end of the block when it is a loop/branch exit block
void RestoreFixVar();

// finish prepare and restore blocks
void PrepareAndRestore();

// main register assignment function
void AssignRegister();

// used to restore the registers released in last cycle   modified by DC
void UpdateRegAvailable();