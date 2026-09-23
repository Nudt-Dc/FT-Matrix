#pragma once
#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <list>
#include <map>
#include <queue>
using namespace std;

const int kMaxBblockNum = 1024;
const int kMaxInsNumPerBlock = 2048;
const int kRegType = 10;
const int kFuncUnitAmount = 11;
const int kMaxLineLength = 1024;
const int kMaxWordLength = 1024;
// assuming the maximum cycles of each instruction
const int LONGEST_INSTR_DELAY = 12; // avoid index of array out of bound when placing

// the type of an instruction
// -1: default
// 0: label
// 1: instructions in the reference file
enum class InsType
{
	Default = -1,
	Label, // 0
	ReferIns // 1
};

//------------------------------build dependency--------------------------
// modified by CZY
// the structure of memory access information
struct MemInfo{
	string AR, OR;
	long long AR_offset;
};
// mem_info operator overloading
struct MemInfoCMP {
	bool operator()(const MemInfo& a, const MemInfo& b)const {
		if (a.AR != b.AR) return a.AR < b.AR;
		if (a.OR != b.OR) return a.OR < b.OR;
		return a.AR_offset < b.AR_offset;
	}
};



// data structure for storage space of each variable in linear assembly
// 
struct VarStorage {
	// the number of register where the variable is stored 
	int reg_num;
	// the memory address where the variable is stored
	int mem_address;

	VarStorage() {
		reg_num = -1;
		mem_address = -1;
	}
};


// data structure for linear assembly instructions
struct Instruction
{
	// the type of an instruction
	InsType type;
	// the entire string of original instruction
	char* ins_string;
	// the name of an instruction
	char ins_name[16];
	// function unit of an instruction
	char func_unit[32];
	// condition field of an instruction
	string cond_field;
	// 3 input (at most) of an instruction
	string input[3];
	// 1 output of an instruction
	string output;
	// ? 
	string note;
	// 0. denote whether ! the cond_field
	// 1. for loop end label, loop_unrolling factor
	int cond_flag;
	int cycle, read_cycle, write_cycle;
	// vector for extracted vars
	set<string> extracted_var;
	//state of instruction
	int state;
	//instruction partner
	Instruction* partner;

	//modifed by DC
	//dp for dynamic programming, dp is the earliest issue time of instrucion	
	int dp;
	//function unit occupied
	int func_occupied;

	//--------------------build dependency && instruction scheduling--------------------
	// modified by CZY
	// In-degree, that is the number of parent instructions
	int indeg;
	// Latest launch cycle, 
	// In PrintDependencyGraph(), last_fa_end_time denotes index temporarily
	int last_fa_end_time;
	int use_unit;
	int site;
	// the list of children instruction, pair<pointer of children instr, dependent cycle>
	vector<pair<Instruction*, int> > chl;

	vector<pair<Instruction*, pair<int, int> > > ex_loop;// first: number of iteration crossed;  second: instruction delay


	// construct function
	Instruction() {
		type = InsType::Default;
		ins_string = NULL;
		ins_name[0] = '\0';
		func_unit[0] = '\0';
		cond_field = "";
		cond_flag = 0;
		input[0] = "";
		input[1] = "";
		input[2] = "";
		output = "";
		note = "";
		cycle = read_cycle = write_cycle = 0;
		indeg = last_fa_end_time = 0; // modified by CZY
		site = -1;
		state = 0;
		partner = NULL;
		dp = 0;
		func_occupied = -1;
	}
};

//---------------------instruction scheduling----------------
// modified by CZY
struct InstrCMP {
	int operator() (const Instruction* x, const Instruction* y) const {
		/*if (y->state < 1) {
			return true;
		}*/
		return x->state < y->state;// Instr in high priority with higher state
	}
};

// 0: normal label
// 1: loop start label
// 2: instructions
// 3: loop end label
enum class BlockType
{
	Default = -1,  // -1
	Label,  // 0
	LoopStart, // 1
	Instruction, // 2
	LoopEnd, //3
	Close,
	Open,

};
// data structure for basic block in linear assembly
// the linear program is split according to :
// 1) . label. each label is a block
// 2) SBR instruction. each SBR and instructions before it is a block
struct Block
{
	// used to denote the type of a code block
	BlockType type;
	// the label/name of the block
	// default: ""
	//
	string label;
	// used to denote the index of a code block
	// starts from 0
	int block_index;
	// used to denote the depth of a loop
	// default -1
	int loop_depth;
	// used to denote the number of instructions in block
	int block_ins_num;
	Instruction block_ins[kMaxInsNumPerBlock];
	//vector<Instruction> ins_vector;
	vector<Instruction*> ins_pointer_list;

	//---------------instruction scheduling-----------------------
	//modified by CZY
	// instruction pointer set after scheduling
	vector<vector<Instruction*>> ins_schedule_queue;
	vector<vector<Instruction*>> ins_schedule_queue_write;
	vector<vector<Instruction*>> module_queue_r;
	vector<vector<Instruction*>> module_queue_w;
	vector<vector<Instruction*>> LT_queue_r;
	vector<vector<Instruction*>> LT_queue_w;

	// the instruction queue of the instructions with indege=0
	priority_queue<Instruction*, vector<Instruction*>, InstrCMP> zero_indeg_instr;
	queue<Instruction*> soft_instr_queue;
	//the number of cycles of ins_schedule_queue;
	int queue_length;
	int LT_queue_length;
	int soft_pipe_cycle;
	// software pipeline flag in block
	int soft_pipeline;

	//-----------------build dependency----------------------------
	// modified by CZY
	// Reg Dictionary for reg and mem updating information
	map<string, Instruction*> reg_written, reg_root;
	map<string, long long> reg_offset;
	map<MemInfo, Instruction*, MemInfoCMP> mem_written, mem_read;


	//----------------aliveness analysis----------------
	// used to record variables which are alive in each cycle
	// [cycle][reg_type][alive_variable_num]
	vector<vector<vector<string*>>> alive_chart;
	// used to record the condiction of each variable in last instruction involved
	// int[0]: the last cycle when the variable was assigned
	// int[1]: the last cycle when the variable was read
	// int[2]: the usage mode of the variable in last instruction involved
	// int[3]: the final usage mode of the variable in this block
	// int[4]: the final cycle when the variable is involved
	map<string*, int[5]> var_condition;
	// used to record variables which need to fix their storage location in this block
	map<string, VarStorage> fix_var;
	// used to record the numbers of entrance and exit blocks
	vector<int> entry_block, exit_block;
	// used to record SBR instruction
	Instruction sbr;
	// used to record the maxium of each type of variables alive in the block
	int max_var_alive[kRegType];


	//----------------register assignment----------------
	// used to show which variable the register is occupied by
	// R, VR, S_AR, V_AR, S_OR, V_OR 
	vector<string*> reg_used[8];
	// used to show which fix variable the register is occupied by
	vector<string*> reg_fix[8];
	// available register list
	list<int> reg_available[kRegType];
	// Least Recently Used register list
	list<int> reg_idle[kRegType];
	// the register number or the memory address information of the variable
	map<string, VarStorage> var_storage;
	// used to store the released registers this cycle   modified by DC
	list<int> reg_released_buf[kRegType]; 


	Block() {
		type = BlockType::Default;
		block_index = -1;
		block_ins_num = 0;
		loop_depth = 0;
		label = "";
		for (int i = 0; i < kRegType; ++i) {
			max_var_alive[i] = 0;
		}
		queue_length = 0; // modified by CZY
		LT_queue_length = 0;
		soft_pipe_cycle = 0;
		soft_pipeline = 0;
	}
};


// data structure for the linear assembly program
struct Program
{
	// total number of basic blocks
	int total_block_num;
	// total number of instructions
	int total_ins_num;

	// list of the basic blocks
	//vector<Block> block_vector;
	Block program_block[kMaxBblockNum];
	vector<Block*> block_pointer_list;

	// list of variables passed in and return
	vector<string> var_in, var_out;
	// maxium of variable alive
	int max_var_alive[kRegType];

	Program() {
		total_block_num = 0;
		total_ins_num = 0;
		for (int i = 0; i < kRegType; ++i) {
			max_var_alive[i] = 0;
		}
	}
};

// data structure for the standard assembly instruction
struct ReferInstruction {
	char name[16];
	char func_unit[32];
	char input[3][90];
	char output[90];
	int cycle;
	int read_cycle, write_cycle;
};

// data structure for the table of all standard assembly instructions
struct ReferTable
{
	vector<ReferInstruction> table;
};

// -1 : default
// 0: input
// 1: output
// 2: gen_var
// 3: add_var
enum class VarType
{
	Default = -1,
	Input,
	Output,
	General,
	Address
};

enum class RegType
{
	Default = -1,
	R,
	VR,
	S_AR,
	V_AR,
	S_OR,
	V_OR,
	R_COND,
	VR_COND,
	AR_OR,
};


const string kRegTypeStr[10] = { "Default", "R", "VR", "S_AR", "V_AR", "S_OR",
								 "V_OR", "R_COND", "VR_COND", "AR_OR"};

// data structure for a declared variable
struct Variable {
	string name;
	VarType type;
	// reg_type: R, VR, AR(Scalar), AR(Vector), OR(Scalar), OR(Vector), R(cond), VR(cond), R(pair), VR(pair)
	RegType reg_type;
	string partner;
	int pair_pos;
	// used to denote whether this variable is used
	int use_flag;
	// used to denote whether this variable is assigned
	int assign_flag;

	bool operator<(const Variable& var) const
	{
		return name < var.name;
	}
	Variable() {
		type = VarType::Default;
		name = "";
		reg_type = RegType::Default;
		partner = "";
		pair_pos = -1;
		use_flag = 0;
		assign_flag = 0;
	}

};


//------------------build dependency------------------
// modified by CZY
// data structure for pairs of variables
struct VarList {
	Variable var;
	VarList* next;
	void free() {
		VarList* it = this;
		VarList* next = it->next;
		while (1) {
			delete it;
			if (next == NULL) break;
			it = next;
			next = it->next;
		}
	}
};




// global data structure for reference
extern vector<string> refer_src;


// global data structure for the 
// program and variables
extern Program linear_program;
extern unordered_map<string, Variable> var_map;


// global data structure for the 
// reference instructions and function units
extern ReferTable refer_table;
extern unordered_map<string, int> function_units_index;
extern vector<string> reg_ctrl[2];
extern vector<int> reg_amount;
extern vector<vector<int>> reg_res;
extern vector<int> reg_in;
extern int reg_out;
extern int reg_return;
extern int scalar_stack_AR;
extern int vector_stack_AR;
extern int core;
// TODO
extern int sv_interval;


//----------------register assignment----------------
extern vector<vector<vector<Instruction*>>> ins_output;
extern vector<string> block_label;
extern Instruction particular_instruction[12];


struct position//这个变量最终的位置信息以及读写情况
{
	int pos;//第几次循环													
	int pd = 0;//有两拍读或者写的变量会有这个东西，初始为0，在第一拍就为1，第二拍就为2
	Instruction* ins;
	position(int x, int y, Instruction* z)
	{
		pos = x;
		pd = y;
		ins = z;
	}
	bool operator == (const position a) {
		if (pos == a.pos && pd == a.pd && ins == a.ins) return true;
		else return false;
	}
};
struct poslist//变量的属性
{
	string name;
	int ab_po;//绝对启动位置
	int cycle;//周期
	int judge;//读或者写
	Instruction* ins;
};
struct coor//顺序总表，把所有循环融合
{
	vector<position> re;//这一拍中读所在的循环数
	vector<position> wr;
};

int GetReferIns(const char* ins_name);
void CleanLinearProgram();