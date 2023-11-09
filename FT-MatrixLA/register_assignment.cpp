#include "utils.h"
#include "aliveness_analysis.h"
#include "register_assignment.h"
#include "ins_scheduling.h"

// global variables
// the final instruction list after register assignment
vector<vector<vector<Instruction*>>> ins_output;
// the label of each block
vector<string> block_label;
// particular Instruction
// 0: SNOP
// 1: SBR
// 2: SMOVIL -1 to high word(32)
// 3: SMOVIL 0 to high word(32)
// 4-5: SMOVIL(32, low word)/SMOVI(64), SADDA(expan scalar stack size)
// 6-7: SMOVIL(32, low word)/SMOVI(64), SADDA(expan vector stack size)
// 8-9: SMOVIL(32, low word)/SMOVI(64), SADDA(restore scalar stack size)
// 10-11: SMOVIL(32, low word)/SMOVI(64), SADDA(restore vector stack size)
Instruction particular_instruction[12];

// constant string
// special reg label
static string kRes = "RESERVE";
static string kCond = "COND";
static string kStack = "STACK";
static string kBuf = "BUF";
// special block label
static string kPrepare = ".preparation";
static string kRestore = ".restoration";
// the names of every registers
static vector<string> reg_name[10];

// the maxium of the immediate as an address offset
static int kImmOffset = 256;
// the number of VPE
static const int kVpe = 16;

// preparation and restoration relevant
// whether the stack size has been expand
static int scalar_expansion_flag = 0, vector_expansion_flag = 0;
// whether the high word register has been intialized as -1/0
static int high_word_init = 0;
// the amount of scalar(0) and vector(1) condiction register
static int cond_reg_amount[2];
// the list of general scalar registers which weren't used when the program started and could be used as buf register
static list<int> reg_buf;
// the number of the reserved general scalar register(R) (record high word register in 32)
static int offset_buf = -1;
// the address offset of stack border
static int scalar_save = 0, vector_save = 0;
// the reserve OR for spill operation
static int scalar_stack_OR = -1, vector_stack_OR = -1;
// the reserve reigster which is used in the program
static vector<int> reserve_register_used[8];

static Block* current_block;
static int block_num, cycle_num, unit_num;
// the list of variables which used in the same cycle
static list<vector<string>> interfering_var;
// the list of registers which should be released in the current cycle, information: reg_num, fix_flag
static vector<vector<pair<int, int>>> release_reg;

static bool spill_flag = false;
static bool spill_warnning = false;
static bool debug = false;

void InitParticularInstruction() {
	ReferInstruction* refer_ins;
	string smovi = "SMOVI";
	string smovi_buf = reg_name[0][offset_buf];
	string sadda_buf = reg_name[0][offset_buf];
	if (core == 32 || core == 34) {
		// initialization of high word in R pair in 32
		smovi += "L";
		refer_ins = &refer_table.table[GetReferIns(smovi.c_str())];
		strcpy(particular_instruction[2].ins_name, refer_ins->name);
		strcpy(particular_instruction[2].func_unit, refer_ins->func_unit);
		particular_instruction[2].input[0] = "-1";
		particular_instruction[2].output = reg_name[0][offset_buf];
		particular_instruction[2].cycle = refer_ins->cycle;
		particular_instruction[2].read_cycle = refer_ins->read_cycle;
		particular_instruction[2].write_cycle = refer_ins->write_cycle;
		particular_instruction[3] = particular_instruction[2];
		particular_instruction[2].note = ";initial the high word of the stack size in R-buf";
		particular_instruction[3].input[0] = "0";
		particular_instruction[3].note = ";initial the high word of in R-buf";
		smovi_buf = reg_name[0][offset_buf - 1];
		sadda_buf = sadda_buf + ":" + reg_name[0][offset_buf - 1];
	}
	// SNOP
	refer_ins = &refer_table.table[GetReferIns("SNOP")];
	strcpy(particular_instruction[0].ins_name, refer_ins->name);
	strcpy(particular_instruction[0].func_unit, refer_ins->func_unit);
	particular_instruction[0].input[0] = "1";
	particular_instruction[0].cycle = refer_ins->cycle;
	particular_instruction[0].read_cycle = refer_ins->read_cycle;
	particular_instruction[0].write_cycle = refer_ins->write_cycle;
	// SBR
	refer_ins = &refer_table.table[GetReferIns("SBR")];
	strcpy(particular_instruction[1].ins_name, refer_ins->name);
	strcpy(particular_instruction[1].func_unit, refer_ins->func_unit);
	particular_instruction[1].input[0] = reg_name[0][reg_return];
	particular_instruction[1].cycle = refer_ins->cycle;
	particular_instruction[1].read_cycle = refer_ins->read_cycle;
	particular_instruction[1].write_cycle = refer_ins->write_cycle;
	// SMOVI relevant
	refer_ins = &refer_table.table[GetReferIns(smovi.c_str())];
	strcpy(particular_instruction[4].ins_name, refer_ins->name);
	strcpy(particular_instruction[4].func_unit, refer_ins->func_unit);
	particular_instruction[4].output = smovi_buf;
	particular_instruction[4].cycle = refer_ins->cycle;
	particular_instruction[4].read_cycle = refer_ins->read_cycle;
	particular_instruction[4].write_cycle = refer_ins->write_cycle;
	particular_instruction[10] = particular_instruction[4];
	particular_instruction[8] = particular_instruction[4];
	particular_instruction[4].input[0] = "-";
	particular_instruction[6] = particular_instruction[4];
	particular_instruction[4].note = ";assign the of size of the scalar stack expansion to the register buf";
	particular_instruction[6].note = ";assign the of size of the vector stack expansion to the register buf";
	particular_instruction[8].note = ";assign the of size of the scalar stack restoration to the register buf";
	particular_instruction[10].note = ";assign the of size of the vector stack restoration to the register buf";
	// SADDA relevant
	refer_ins = &refer_table.table[GetReferIns("SADDA")];
	strcpy(particular_instruction[5].ins_name, refer_ins->name);
	strcpy(particular_instruction[5].func_unit, refer_ins->func_unit);
	particular_instruction[5].input[0] = sadda_buf;
	particular_instruction[5].input[1] = reg_name[2][scalar_stack_AR];
	particular_instruction[5].output = reg_name[2][scalar_stack_AR];
	particular_instruction[5].cycle = refer_ins->cycle;
	particular_instruction[7] = particular_instruction[5];
	particular_instruction[9] = particular_instruction[5];
	particular_instruction[5].note = ";expand the scalar stack size";
	particular_instruction[9].note = ";restore the scalar stack register";
	particular_instruction[7].input[1] = reg_name[3][vector_stack_AR];
	particular_instruction[7].output = reg_name[3][vector_stack_AR];
	particular_instruction[11] = particular_instruction[7];
	particular_instruction[7].note = ";expand the vector stack size";
	particular_instruction[11].note = ";restore the vector stack register";
}


void AddStackInstruction(const int& block, int index) {
	int cycle = ins_output[block].size();
	int unit;
	vector<Instruction*> new_cycle(kFuncUnitAmount);
	if ((core == 32 || core == 34) && index == 4 || index == 6 && !high_word_init) {
		high_word_init = 1;
		ins_output[block].push_back(new_cycle);
		unit = FunctionUnitSplit(particular_instruction[2].func_unit)[0];
		ins_output[block][cycle++][unit] = &particular_instruction[2];
	}
	ins_output[block].push_back(new_cycle);
	unit = FunctionUnitSplit(particular_instruction[index].func_unit)[0];
	ins_output[block][cycle++][unit] = &particular_instruction[index];
	ins_output[block].push_back(new_cycle);
	unit = FunctionUnitSplit(particular_instruction[++index].func_unit)[0];
	ins_output[block][cycle++][unit] = &particular_instruction[index];
}


void AddNewInstruction(const char* ins_name, string input, string output, string note, int snop_num) {
	int& ins_num = current_block->block_ins_num;
	if (ins_num == kMaxInsNumPerBlock) {
		PrintError("The amount of instructions in block %d is larger than the maximun we support.\n", block_num, kMaxInsNumPerBlock);
		Error();
	}
	ReferInstruction* refer_ins = &refer_table.table[GetReferIns(ins_name)];
	int unit = FunctionUnitSplit(refer_ins->func_unit).back();
	vector<Instruction*> new_cycle(kFuncUnitAmount);

	ins_output[block_num].push_back(new_cycle);
	ins_output[block_num].back()[unit] = &current_block->block_ins[ins_num];

	strcpy(current_block->block_ins[ins_num].ins_name, refer_ins->name);
	strcpy(current_block->block_ins[ins_num].func_unit, refer_ins->func_unit);
	if (strstr(refer_ins->func_unit, "MAC") != NULL) ReallocMAC(&current_block->block_ins[ins_num], unit);
	//if (unit == 0 || unit == 5) strcat(current_block->block_ins[ins_num].ins_name, ".M1");
	//else if (unit == 1 || unit == 6) strcat(current_block->block_ins[ins_num].ins_name, ".M2");
	//else if (unit == 7) strcat(current_block->block_ins[ins_num].ins_name, ".M3");
	current_block->block_ins[ins_num].cycle = refer_ins->cycle;
	current_block->block_ins[ins_num].read_cycle = refer_ins->read_cycle;
	current_block->block_ins[ins_num].write_cycle = refer_ins->write_cycle;
	current_block->block_ins[ins_num].input[0] = input;
	current_block->block_ins[ins_num].output = output;
	current_block->block_ins[ins_num].note = note;
	if (snop_num == -1) snop_num = current_block->block_ins[ins_num].cycle - 1;
	ins_num++;
	for (; snop_num > 0; --snop_num) {
		ins_output[block_num].push_back(new_cycle);
		ins_output[block_num].back()[4] = &particular_instruction[0];
	}
}


void AddSbr() {
	if (current_block->sbr.ins_name[0] == '\0') return;
	vector<Instruction*> new_cycle(kFuncUnitAmount);
	int cycle = particular_instruction[1].cycle;
	vector<vector<Instruction*>>& sbr_block = ins_output[block_num];

	while (sbr_block.size() < cycle) {
		sbr_block.push_back(new_cycle);
		sbr_block.back()[4] = &particular_instruction[0];
	}
	for (cycle = sbr_block.size() - cycle; sbr_block[cycle][4] != NULL && sbr_block[cycle][4] != &particular_instruction[0]; ++cycle) {
		sbr_block.push_back(new_cycle);
		sbr_block.back()[4] = &particular_instruction[0];
	}
	sbr_block[cycle][4] = &current_block->sbr;
}


void DebugCheckRegState() {
	string var;
	int reg_type, reg_num, mem_address;
	int reg1, reg2, flag;
	//for (reg_type = 0; reg_type < 8; ++reg_type) {
	//	for (reg_num = 0; reg_num < current_block->reg_used[reg_type].size(); ++reg_num) {
	//		if (current_block->reg_used[reg_type][reg_num] == &kBuf) {
	//			MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
	//		}
	//	}
	//}
	for (auto iter = current_block->var_storage.begin(); iter != current_block->var_storage.end(); ++iter) {
		var = iter->first;
		reg_num = iter->second.reg_num;
		mem_address = iter->second.mem_address;
		reg_type = int(var_map[var].reg_type);
		//if (reg_type > 7) reg_type -= 8;
		if (reg_num != -1 && reg_num != -2) {
			if (current_block->reg_used[reg_type][reg_num] != &var_map[var].name) {
				MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
			}
			reg_num -= var_map[var].pair_pos;
			var = var_map[var].partner;
			if (!var.empty()) {
				reg2 = current_block->var_storage[var].reg_num;
				if (reg2 != -1 && reg2 != -2) {
					if (reg_num != reg2 - var_map[var].pair_pos) {
						MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
					}
				}
			}
		}
	}
	for (reg_type = 8; reg_type < 10; ++reg_type) {
		for (auto it : current_block->reg_idle[reg_type]) {
			if (it % 2 == 0) {
				MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
			}
		}
		for (int i = 0; i < current_block->reg_available[reg_type].size(); ++i) {
			flag = 0;
			if (current_block->reg_available[reg_type].back() % 2 == 0) {
				MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
			}
			for (int j = 0; j < current_block->reg_available[reg_type - 8].size(); ++j) {
				reg1 = current_block->reg_available[reg_type - 8].back();
				if (reg1 == current_block->reg_available[reg_type].back()) {
					if (flag == 0) flag = 1;
					else if (flag == 2) flag = 3;
					else {
						flag = 0;
						break;
					}
				}
				if (reg1 == current_block->reg_available[reg_type].back() - 1) {
					if (flag == 0) flag = 2;
					else if (flag == 1) flag = 3;
					else {
						flag = 0;
						break;
					}
				}
				current_block->reg_available[reg_type - 8].pop_back();
				current_block->reg_available[reg_type - 8].push_front(reg1);
			}
			if (flag != 3) {
				MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
			}
			reg1 = current_block->reg_available[reg_type].back();
			current_block->reg_available[reg_type].pop_back();
			for (int j = 0; j < current_block->reg_available[reg_type].size(); ++j) {
				reg2 = current_block->reg_available[reg_type].back();
				if (reg1 == reg2) {
					MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
				}
				current_block->reg_available[reg_type].pop_back();
				current_block->reg_available[reg_type].push_front(reg2);
			}
			current_block->reg_available[reg_type].push_front(reg1);
		}
	}
	for (reg_type = 0; reg_type < kRegType; ++reg_type) {
		map<int, int> nums;
		for (int i = 0; i < current_block->reg_available[reg_type].size(); ++i) {
			reg_num = current_block->reg_available[reg_type].back();
			if (nums.count(reg_num)) {
				MessageFormalize(-1, "ERROR: Unexpected problem occur in register assignment.");
			}
			nums[reg_num] = 0;
			current_block->reg_available[reg_type].pop_back();
			current_block->reg_available[reg_type].push_front(reg_num);
		}
	}
}


void InitRegInfo() {
	int reg_type, reg_num;
	int i;
	string buf = "";

	//block_num = 0;
	block_num = 1;
	current_block = linear_program.block_pointer_list[block_num];

	// name all register
	for (reg_type = 0; reg_type < 8; ++reg_type) {
		if (reg_type == 0 || reg_type == 6) buf = "R";
		else if (reg_type == 1 || reg_type == 7) buf = "VR";
		else if (reg_type == 2 || reg_type == 3) buf = "AR";
		else if (reg_type == 4 || reg_type == 5) buf = "OR";
		for (reg_num = 0; reg_num < reg_amount[reg_type]; ++reg_num) {
			if (reg_type == 2 || reg_type == 4) {
				reg_name[reg_type].push_back(buf + to_string(reg_num + sv_interval));
			}
			else {
				reg_name[reg_type].push_back(buf + to_string(reg_num));
			}
		}
	}
	for (; reg_type < kRegType; ++reg_type) {
		reg_name[reg_type].resize(reg_amount[reg_type - 8]);
		for (reg_num = 1; reg_num < reg_amount[reg_type - 8]; reg_num += 2) {
			reg_name[reg_type][reg_num] = reg_name[reg_type - 8][reg_num] + ":" + reg_name[reg_type - 8][reg_num - 1];
		}
	}
	// push the reserve reg to the queue tail
	for (reg_type = 0; reg_type < 6; ++reg_type) {
		current_block->reg_used[reg_type].resize(reg_amount[reg_type]);
		current_block->reg_fix[reg_type].resize(reg_amount[reg_type]);
		for (i = reg_res[reg_type].size() - 1; i >= 0; --i) {
			reg_num = reg_res[reg_type][i];
			if (reg_type == 2 || reg_type == 4) {
				reg_num -= sv_interval;
			}
			current_block->reg_used[reg_type][reg_num] = &kRes;
			current_block->reg_available[reg_type].push_back(reg_num);
			if (reg_type == 0 || reg_type == 1) {
				if (reg_num % 2) {
					current_block->reg_available[reg_type + 8].push_back(reg_num);
				}
				else if (current_block->reg_used[reg_type][reg_num + 1] != &kRes) {
					current_block->reg_available[reg_type + 8].push_back(reg_num + 1);
				}
			}
		}
	}
	// stack offset buffer register occupy
	if (core == 64) {
		for (reg_num = reg_amount[0] - 1; reg_num >= 0; --reg_num) {
			if (current_block->reg_used[0][reg_num] != &kRes) {
				offset_buf = reg_num;
				current_block->reg_used[0][reg_num] = &kStack;
				current_block->reg_fix[0][reg_num] = &kStack;
				current_block->reg_available[8].remove(reg_num - reg_num % 2 + 1);
				break;
			}
		}
	}
	if (core == 32 || core == 34) {
		reg_num = reg_amount[0] - 1;
		if (reg_num % 2 == 0) reg_num--;
		for (; reg_num >= 0; reg_num -= 2) {
			if (current_block->reg_used[0][reg_num] != &kRes && current_block->reg_used[0][reg_num - 1] != &kRes) {
				offset_buf = reg_num;
				current_block->reg_used[0][reg_num] = &kStack;
				current_block->reg_used[0][reg_num - 1] = &kStack;
				current_block->reg_fix[0][reg_num] = &kStack;
				current_block->reg_fix[0][reg_num - 1] = &kStack;
				current_block->reg_available[8].remove(reg_num);
				break;
			}
		}
	}
	// condiction register preparation
	for (reg_type = 6; reg_type < 8; ++reg_type) {
		if (reg_amount[reg_type] >= linear_program.max_var_alive[reg_type]) {
			cond_reg_amount[reg_type - 6] = linear_program.max_var_alive[reg_type];
		}
		else {
			cond_reg_amount[reg_type - 6] = reg_amount[reg_type];
		}
		current_block->reg_used[reg_type].resize(cond_reg_amount[reg_type - 6]);
		current_block->reg_fix[reg_type].resize(cond_reg_amount[reg_type - 6]);
		for (reg_num = 0; reg_num < cond_reg_amount[reg_type - 6]; ++reg_num) {
			if (current_block->reg_used[reg_type - 6][reg_num] == &kRes) {
				current_block->reg_used[reg_type][reg_num] = &kRes;
				current_block->reg_available[reg_type - 6].remove(reg_num);
				if (reg_num % 2 == 0) {
					current_block->reg_available[reg_type + 2].remove(reg_num + 1);
				}
			}
			current_block->reg_used[reg_type - 6][reg_num] = &kCond;
			current_block->reg_available[reg_type].push_front(reg_num);
		}
	}
	// stack base address register occupy
	current_block->reg_used[2][scalar_stack_AR] = &kStack;
	current_block->reg_used[3][vector_stack_AR] = &kStack;
	current_block->reg_fix[2][scalar_stack_AR] = &kStack;
	current_block->reg_fix[3][vector_stack_AR] = &kStack;
	// parameter occupy
	InitParticularInstruction();
	for (int var_index = 0, reg_index = 0; var_index < linear_program.var_in.size(); ++var_index, ++reg_index) {
		reg_num = reg_in[reg_index];
		reg_type = int(var_map[linear_program.var_in[var_index]].reg_type);
		// general parameter
		if (reg_type == 0) {
			if (core == 32 || core == 34) {
				reg_buf.push_back(reg_num + 1);
			}
			if (!var_map[linear_program.var_in[var_index]].partner.empty()) {
				reg_num++;
				current_block->reg_idle[8].push_back(reg_num);
				current_block->reg_used[reg_type][reg_num] = &var_map[linear_program.var_in[var_index]].name;
				current_block->var_storage[linear_program.var_in[var_index]].reg_num = reg_num;
				current_block->reg_idle[reg_type].push_back(reg_num);
				reg_num--;
				var_index++;
			}
			linear_program.block_pointer_list[block_num]->reg_used[reg_type][reg_num] = &var_map[linear_program.var_in[var_index]].name;
			linear_program.block_pointer_list[block_num]->var_storage[linear_program.var_in[var_index]].reg_num = reg_num;
			linear_program.block_pointer_list[block_num]->reg_idle[reg_type].push_back(reg_num);
			continue;
		}
		// scalar condiction register
		if (!current_block->reg_available[reg_type].size()) {
			scalar_expansion_flag = 1;
			current_block->var_storage[linear_program.var_in[var_index]].reg_num = -2;
			current_block->var_storage[linear_program.var_in[var_index]].mem_address = scalar_save;
			current_block->reg_used[reg_type - 6][reg_num] = &var_map[linear_program.var_in[var_index]].name;
			SaveSpillRegister(reg_type - 6, reg_num);
			current_block->reg_used[reg_type - 6][reg_num] = NULL;
		}
		else {
			reg_num = current_block->reg_available[reg_type].back();
			current_block->reg_available[reg_type].pop_back();
			if (current_block->reg_used[reg_type][reg_num] == &kRes) {
				reserve_register_used[reg_type].push_back(reg_num);
			}
			current_block->reg_used[reg_type][reg_num] = &var_map[linear_program.var_in[var_index]].name;
			AddNewInstruction("SMOV", reg_name[0][reg_in[reg_index]], reg_name[6][reg_num], ";transfer \"" + linear_program.var_in[var_index] + "\" from GPR to Cond_R", 0);
			current_block->var_storage[linear_program.var_in[var_index]].reg_num = reg_num;
			current_block->reg_idle[reg_type].push_back(reg_num);
		}
	}
	// push other registers
	for (reg_type = 0; reg_type < 6; ++reg_type) {
		for (reg_num = reg_amount[reg_type] - 1; reg_num >= 0; --reg_num) {
			if (current_block->reg_used[reg_type][reg_num] != NULL) continue;
			current_block->reg_available[reg_type].push_back(reg_num);
			if (reg_type == 2 || reg_type == 3) continue;
			else if (reg_type == 4) {
				if (scalar_stack_OR != -1) continue;
				scalar_stack_OR = reg_num;
				current_block->reg_used[reg_type][reg_num] = &kStack;
				current_block->reg_fix[reg_type][reg_num] = &kStack;
				current_block->reg_available[reg_type].pop_back();
			}
			else if (reg_type == 5) {
				if (vector_stack_OR != -1) continue;
				vector_stack_OR = reg_num;
				current_block->reg_used[reg_type][reg_num] = &kStack;
				current_block->reg_fix[reg_type][reg_num] = &kStack;
				current_block->reg_available[reg_type].pop_back();
			}
			else {
				if (reg_type == 0 && core == 64) reg_buf.push_back(reg_num);
				if (reg_num % 2) {
					if (current_block->reg_used[reg_type][reg_num - 1] == NULL) {
						current_block->reg_available[reg_type + 8].push_back(reg_num);
					}
				}
			}
		}
	}
	// complete register buf list
	if (core == 32 || core == 34) {
		reg_num = reg_amount[0] - 1;
		if (reg_num % 2 == 0) reg_num--;
		for (; reg_num >= cond_reg_amount[0]; reg_num -= 2) {
			if (current_block->reg_used[0][reg_num] == NULL && current_block->reg_used[0][reg_num - 1] == NULL) {
				reg_buf.push_back(reg_num);
			}
		}
		if (reg_num > 0) {
			if (current_block->reg_used[0][reg_num] == NULL && current_block->reg_used[6][reg_num - 1] == NULL) {
				reg_buf.push_back(reg_num);
			}
			reg_num -= 2;
		}
		for (; reg_num >= 0; --reg_num) {
			if (current_block->reg_used[6][reg_num] == NULL && current_block->reg_used[6][reg_num - 1] == NULL) {
				reg_buf.push_back(reg_num);
			}
		}
	}
	reg_buf.remove(reg_out);
	reg_buf.remove(reg_out + 1);
}


int IsVarAlive(const string& var, const int& type) {
	vector<string*>& alive_var_vector = current_block->alive_chart[cycle_num + 1][type];
	for (int i = 0; i < alive_var_vector.size(); ++i) {
		if (*alive_var_vector[i] == var) {
			return 1;
		}
	}
	return 0;
}


void InitBlockState() {
	int reg_type, reg_num, pair_num;
	int entry_block_num = block_num;
	int i, j;
	int fix, release;
	string var, var2;
	vector<vector<string*>> last_alive, current_alive;

	// get block label
	if (!current_block->label.empty()) {
		if (current_block->label != ".input" && current_block->label != ".output" 
			&& current_block->label != ".gen_var" && current_block->label != ".add_var"
			&& current_block->label != ".endloop" && current_block->label != ".size"
			&& current_block->label != ".open")
			block_label[block_num] = current_block->label;
	}
	// initial interfering variables
	list<vector<string>>(1).swap(interfering_var);
	// get the most recent entry block
	for (i = 0; i < current_block->entry_block.size(); ++i) {
		entry_block_num = current_block->entry_block[i];
		if (entry_block_num < block_num) break;
	}
	if (entry_block_num == block_num) entry_block_num--;
	Block& entry_block = *linear_program.block_pointer_list[entry_block_num];
	// copy register information from the entry block
	for (reg_type = 0; reg_type < kRegType; ++reg_type) {
		current_block->reg_available[reg_type] = entry_block.reg_available[reg_type];
		current_block->reg_idle[reg_type] = entry_block.reg_idle[reg_type];
		if (reg_type > 7) continue;
		current_block->reg_used[reg_type] = entry_block.reg_used[reg_type];
		current_block->reg_fix[reg_type] = entry_block.reg_fix[reg_type];
	}
	current_block->var_storage = entry_block.var_storage;
	// get fix variables
	if (current_block->fix_var.size()) {
		for (auto iter = current_block->fix_var.begin(); iter != current_block->fix_var.end(); ++iter) {
			var = iter->first;
			if (linear_program.block_pointer_list[block_num - 1]->fix_var.count(var)) {
				current_block->fix_var[var] = linear_program.block_pointer_list[block_num - 1]->fix_var[var];
			}
			else {
				current_block->fix_var[var] = current_block->var_storage[var];
			}
			reg_type = int(var_map[var].reg_type);
			reg_num = current_block->fix_var[var].reg_num;
			if (reg_num >= 0) {
				current_block->reg_fix[reg_type][reg_num] = &var_map[var].name;
			}
		}
	}
	// release register which is not alive
	last_alive = entry_block.alive_chart.back();
	current_alive = current_block->alive_chart[0];
	for (reg_type = 0; reg_type < 8; ++reg_type) {
		for (i = 0; i < last_alive[reg_type].size(); ++i) {
			var = *last_alive[reg_type][i];
			fix = current_block->fix_var.count(var);
			if (fix) continue;
			release = 1;
			for (j = 0; j < current_alive[reg_type].size(); ++j) {
				if (current_alive[reg_type][j] == last_alive[reg_type][i]) {
					release = 0;
					break;
				}
			}
			if (!release) continue;
			if (!var_map[var].partner.empty()) {
				for (j = 0; j < current_alive[reg_type + 8].size(); ++j) {
					if (current_alive[reg_type + 8][j] == last_alive[reg_type][i]) {
						release = 0;
						break;
					}
				}
			}
			if (!release) continue;
			reg_num = current_block->var_storage[var].reg_num;
			current_block->var_storage[var].reg_num = -1;
			if (reg_num < 0) continue;
			current_block->reg_used[reg_type][reg_num] = NULL;
			if (fix) {
				current_block->reg_available[reg_type].push_front(reg_num);
			}
			else {
				current_block->reg_available[reg_type].push_back(reg_num);
			}
			current_block->reg_idle[reg_type].remove(reg_num);
			//reg pair
			if (!var_map[var].partner.empty()) {
				if (reg_num % 2)
					pair_num = reg_num - 1;
				else pair_num = reg_num + 1;
				current_block->var_storage[var_map[var].partner].reg_num = -1;
				if (pair_num < 0) continue;
				current_block->reg_used[reg_type][pair_num] = NULL;
				if (fix) {
					current_block->reg_available[reg_type].push_front(pair_num);
				}
				else {
					current_block->reg_available[reg_type].push_back(pair_num);
				}
				current_block->reg_idle[reg_type].remove(pair_num);
			}

			if (reg_type > 1) continue;
			// reg pair
			if (reg_num % 2 == 0) reg_num++;
			if ((current_block->reg_used[reg_type][reg_num] != NULL && current_block->reg_used[reg_type][reg_num] != &kRes)
				|| (current_block->reg_used[reg_type][reg_num - 1] != NULL && current_block->reg_used[reg_type][reg_num - 1] != &kRes)) continue;
			current_block->reg_available[reg_type + 8].remove(reg_num);
			if (fix) {
				current_block->reg_available[reg_type + 8].push_front(reg_num);
			}
			else {
				current_block->reg_available[reg_type + 8].push_back(reg_num);
			}
		}
	}
	// make sure every fix variables is assigned a storage
	if (!current_block->fix_var.size()) return;
	for (auto iter = current_block->fix_var.begin(); iter != current_block->fix_var.end(); ++iter) {
		if (iter->second.reg_num != -1) continue;
		var = iter->first;
		reg_type = int(var_map[var].reg_type);
		reg_num = -1;
		// reg pair
		if (!var_map[var].partner.empty()) {
			if (var_map[var].pair_pos == -1) {
				var = var_map[var].partner;
			}
			var2 = var_map[var].partner;
			// find the available register pair haven't been used by fix variables
			for (i = current_block->reg_available[reg_type + 8].size() - 1; i >= 0; --i) {
				j = current_block->reg_available[reg_type + 8].back();
				current_block->reg_available[reg_type + 8].pop_back();
				current_block->reg_available[reg_type + 8].push_front(j);
				if (reg_num == -1) {
					if (current_block->reg_fix[reg_type][j] == NULL && current_block->reg_fix[reg_type][j - 1] == NULL) {
						reg_num = j;
					}
				}
			}
			// assign the fix variables the register pair
			if (reg_num != -1) {
				reg_type += 8;
				current_block->reg_available[reg_type].remove(reg_num);
				current_block->reg_available[reg_type].push_back(reg_num);
				GetReserveRegister(reg_type);
				current_block->fix_var[var].reg_num = reg_num;
				current_block->fix_var[var2].reg_num = reg_num - 1;
				OccupyRegister(reg_type, var, var2);
			}
			// assign the vaiables the memory
			else {
				current_block->fix_var[var].reg_num = -2;
				current_block->fix_var[var2].reg_num = -2;
				current_block->var_storage[var].reg_num = -2;
				current_block->var_storage[var2].reg_num = -2;
				if (reg_type) {
					current_block->fix_var[var].mem_address = vector_save;
					current_block->var_storage[var].mem_address = vector_save;
					vector_save += kVpe;
					current_block->fix_var[var2].mem_address = vector_save;
					current_block->var_storage[var2].mem_address = vector_save;
					vector_save += kVpe;
				}
				else {
					current_block->fix_var[var].mem_address = scalar_save;
					current_block->var_storage[var].mem_address = scalar_save;
					scalar_save++;
					current_block->fix_var[var2].mem_address = scalar_save;
					current_block->var_storage[var2].mem_address = scalar_save;
					scalar_save++;
				}
			}
			continue;
		}
		// other type of variables
		for (i = current_block->reg_available[reg_type].size() - 1; i >= 0; --i) {
			j = current_block->reg_available[reg_type].back();
			current_block->reg_available[reg_type].pop_back();
			current_block->reg_available[reg_type].push_front(j);
			if (reg_num == -1 && current_block->reg_fix[reg_type][j] == NULL) {
				reg_num = j;
			}
		}
		// assign the fix variables the register pair
		if (reg_num != -1) {
			current_block->reg_available[reg_type].remove(reg_num);
			current_block->reg_available[reg_type].push_back(reg_num);
			GetReserveRegister(reg_type);
			iter->second.reg_num = reg_num;
			OccupyRegister(reg_type, var);
		}
		// assign the vaiables the memory
		else {
			iter->second.reg_num = -2;
			current_block->var_storage[var].reg_num = -2;
			if (reg_type == 1 || reg_type == 7) {
				iter->second.mem_address = vector_save;
				current_block->var_storage[var].mem_address = vector_save;
				vector_save += kVpe;
				continue;
			}
			iter->second.mem_address = scalar_save;
			current_block->var_storage[var].mem_address = scalar_save;
			if (core == 64) scalar_save++;
			else if (core == 32 || core == 34) {
				if (reg_type == 0 || reg_type == 6) scalar_save++;
				else scalar_save += 2;
			}
		}
	}
}


void GetReserveRegister(int reg_type) {
	int reg_num = current_block->reg_available[reg_type].back();
	if (reg_type > 7) {
		reg_type -= 8;
		reg_num--;
		if (current_block->reg_used[reg_type][reg_num] == &kRes) {
			reserve_register_used[reg_type].push_back(reg_num);
			current_block->reg_used[reg_type][reg_num] = NULL;
		}
		reg_num++;
	}
	if (current_block->reg_used[reg_type][reg_num] == &kRes) {
		reserve_register_used[reg_type].push_back(reg_num);
		current_block->reg_used[reg_type][reg_num] = NULL;
	}
}


void UpdateRegIdle(int reg_type, int reg_num) {
	current_block->reg_idle[reg_type].remove(reg_num);
	current_block->reg_idle[reg_type].push_front(reg_num);
	// update reg pair idle queue
	if (reg_type == 0 || reg_type == 1) {
		if (reg_num % 2) reg_num--;
		if (reg_num < cond_reg_amount[reg_type]) {
			return;
		}
		reg_type += 8;
		reg_num++;
		current_block->reg_idle[reg_type].remove(reg_num);
		current_block->reg_idle[reg_type].push_front(reg_num);
	}
	// update single reg idle queue
	else if (reg_type == 8 || reg_type == 9) {
		reg_type -= 8;
		current_block->reg_idle[reg_type].remove(reg_num);
		current_block->reg_idle[reg_type].remove(reg_num - 1);
		current_block->reg_idle[reg_type].push_front(reg_num);
		current_block->reg_idle[reg_type].push_front(reg_num - 1);
	}
}


void SelectIdleRegister(int reg_type) {
	// select four type of idle register
	// 0: the occupying variables are not-fixed variables and one of the registers is available in register pair
	// 1: the registers are occupied by not-fixed variables
	// 2: the registers are occupied by fix variables and one of the registers is available in register pair
	// 3: the registers are occupied by fix variables
	int idle_type = 3, idle_num[4] = { -1, -1, -1, -1 };
	int reg_num;
	string var, var2;

	for (int i = 0; i < current_block->reg_idle[reg_type].size(); ++i) {
		reg_num = current_block->reg_idle[reg_type].front();
		current_block->reg_idle[reg_type].pop_front();
		current_block->reg_idle[reg_type].push_back(reg_num);
		// single register
		if (reg_type < 8) {
			var = *current_block->reg_used[reg_type][reg_num];
			if (!IsInterferingVar(var) && !current_block->fix_var.count(var)) {
				idle_type = 1;
			}
		}
		// register pair
		else {
			reg_type -= 8;
			if (current_block->reg_used[reg_type][reg_num] == NULL) idle_type = 2;
			else var = *current_block->reg_used[reg_type][reg_num];
			if (current_block->reg_used[reg_type][reg_num - 1] == NULL) idle_type = 2;
			else var2 = *current_block->reg_used[reg_type][reg_num - 1];
			reg_type += 8;
			if (IsInterferingVar(var) || IsInterferingVar(var2)) continue;
			if (!current_block->fix_var.count(var)
				&& !current_block->fix_var.count(var2)) {
				if (idle_type == 2) idle_type = 0;
				else idle_type = 1;
			}
		}
		idle_num[idle_type] = reg_num;
	}
	// select the best register(s)
	for (idle_type = 0; idle_type < 4; ++idle_type) {
		if (idle_num[idle_type] != -1) {
			reg_num = idle_num[idle_type];
			break;
		}
	}
	current_block->reg_idle[reg_type].remove(reg_num);
	current_block->reg_idle[reg_type].push_back(reg_num);
}


void RegSpillInit() {
	int supplement = 0, final_line = ins_output[block_num].size() - 1, tmp;
	vector<Instruction*> new_cycle(kFuncUnitAmount);
	new_cycle[4] = &particular_instruction[0];

	for (int i = 0; i < LONGEST_INSTR_DELAY && i <= final_line; ++i) {
		if (ins_output[block_num][final_line - i][4] != NULL) {
			if (!strcmp(ins_output[block_num][final_line - i][4]->ins_name, "SNOP")) continue;
		}
		for (int unit = 0; unit < kFuncUnitAmount; ++unit) {
			if (ins_output[block_num][final_line - i][unit] == NULL) continue;
			tmp = ins_output[block_num][final_line - i][unit]->cycle - i - 1;
			if (tmp > supplement) supplement = tmp;
		}
	}
	for (; supplement > 0; --supplement) {
		ins_output[block_num].push_back(new_cycle);
	}
}


void SaveSpillRegister(int reg_type, int reg_num, int address, int snop, int release) {
	string operand = "*+";
	string var;
	string note;
	int buf_type, buf_num;

	if (block_num == 1 || block_num == ins_output.size() - 1) snop = 0;
	if (block_num == 1) var = kRes;
	else {
		var = *current_block->reg_used[reg_type][reg_num];
		if (current_block->soft_pipeline) {
			PrintError("Soft pipeline fail due to registers overflow, please improve the code or give up the soft pipeline option.\n");
			Error();
		}
		if (!spill_warnning) {
			spill_warnning = true;
			PrintWarn("Register overflow may affect the performance of the program. It is recommended to improve the code.\n");
		}
		if (!spill_flag) {
			RegSpillInit();
			spill_flag = true;
		}
	}
	note = ";save \"" + var;
	// R
	if (reg_type == 0 || reg_type == 6) {
		operand += reg_name[2][scalar_stack_AR];
		operand += "[";
		if (address == -1) address = scalar_save++;
		// offset in limit
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			if (core == 64) {
				AddNewInstruction("SMOVI", to_string(address), reg_name[0][offset_buf], ";stack offset spill, transfer the offset to R");
				AddNewInstruction("SMVAGA", reg_name[0][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			}
			else if (core == 32 || core == 34) {
				AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s low word to R");
				AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			}
			operand += reg_name[4][scalar_stack_OR];
		}
		operand += "]";
		note += "\" in memory";
		AddNewInstruction("SSTW", reg_name[reg_type][reg_num], operand, note, snop);
	}
	// VR
	else if (reg_type == 1 || reg_type == 7) {
		operand += reg_name[3][vector_stack_AR];
		operand += "[";
		if (address == -1) {
			address = vector_save;
			vector_save += kVpe;
		}
		// offset in limit
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			if (core == 64) {
				AddNewInstruction("SMOVI", to_string(address), reg_name[0][offset_buf], ";stack offset spill, transfer the offset to R");
				AddNewInstruction("SMVAGA", reg_name[0][offset_buf], reg_name[5][vector_stack_OR], ";transfer the stack offset into the OR");
			}
			else if (core == 32 || core == 34) {
				AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s low word to R");
				AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[5][vector_stack_OR], ";transfer the stack offset into the OR");
			}
			operand += reg_name[5][vector_stack_OR];
		}
		operand += "]";
		note += "\" in memory";
		AddNewInstruction("VSTW", reg_name[reg_type][reg_num], operand, note, snop);
	}
	// AR/OR
	else if (core == 64) {
		if (address == -1) address = scalar_save++;
		operand += reg_name[2][scalar_stack_AR];
		operand += "[";
		buf_type = 0;
		if (block_num != 0) {
			if (current_block->reg_available[buf_type].size() == 0) {
				SelectIdleRegister(buf_type);
				ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
			}
			buf_num = current_block->reg_available[buf_type].back();
			GetReserveRegister(buf_type);
			OccupyRegister(buf_type);
			ReleaseRegister(buf_type, buf_num, 0);
		}
		else buf_num = reg_buf.back();
		AddNewInstruction("SMVAAG", reg_name[reg_type][reg_num], reg_name[buf_type][buf_num], ";transfer \"" + var + "\" and release " + reg_name[reg_type][reg_num]);
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		else {
			AddNewInstruction("SMOVI", to_string(address), reg_name[0][offset_buf], ";stack offset spill, transfer the offset to R");
			AddNewInstruction("SMVAGA", reg_name[0][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			operand += reg_name[4][scalar_stack_OR];
			
		}
		operand += "]";
		note += "\" and release " + reg_name[buf_type][buf_num];
		AddNewInstruction("SSTW", reg_name[buf_type][buf_num], operand, note, snop);
	}
	else if (core == 32 || core == 34) {
		if (address == -1) {
			address = scalar_save;
			scalar_save += 2;
		}
		operand += reg_name[2][scalar_stack_AR];
		operand += "[";
		buf_type = 8;
		if (block_num != 0) {
			if (current_block->reg_available[buf_type].size() == 0) {
				SelectIdleRegister(buf_type);
				ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
			}
			buf_num = current_block->reg_available[buf_type].back();
			GetReserveRegister(buf_type);
			OccupyRegister(buf_type);
			ReleaseRegister(buf_type, buf_num, 0);
		}
		else buf_num = reg_buf.back();
		AddNewInstruction("SMVAAGL", reg_name[reg_type][reg_num], reg_name[0][buf_num - 1], ";transfer \"" + var + "\"\'s low word", 0);
		AddNewInstruction("SMVAAGH", reg_name[reg_type][reg_num], reg_name[0][buf_num], ";transfer \"" + var + "\"\'s high word and release " + reg_name[reg_type][reg_num]);
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s low word to R");
			AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			operand += reg_name[4][scalar_stack_OR];
		}
		operand += "]";
		note += "\"\'s low word and release " + reg_name[0][buf_num - 1];
		AddNewInstruction("SSTW", reg_name[0][buf_num - 1], operand, note, snop);
		address++;
		operand = "*+" + reg_name[2][scalar_stack_AR] + "[";
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s high word to R");
			AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			operand += reg_name[4][scalar_stack_OR];
		}
		operand += "]";
		note = ";save \"" + var + "\"\'s high word and release " + reg_name[0][buf_num];
		AddNewInstruction("SSTW", reg_name[0][buf_num], operand, note, snop);
	}
	//if (block_num != 1 && release) ReleaseRegister(reg_type, reg_num, 0);
}


void RestoreSpillRegister(int reg_type, int reg_num, int address, string var, int snop) {
	string operand = "*+";
	string note = ";restore \"" + var;
	int buf_type, buf_num;
	//if (var.empty() || var != kRes) var = *current_block->reg_used[reg_type][reg_num];

	if (current_block->soft_pipeline) {
		PrintError("Soft pipeline fail due to registers overflow, please improve the code or give up the soft pipeline option.\n");
		Error();
	}
	if (!spill_warnning) {
		spill_warnning = true;
		PrintWarn("Register overflow may affect the performance of the program. It is recommended to improve the code.\n");
	}
	if (block_num != ins_output.size() - 1 && !spill_flag) {
		RegSpillInit();
		spill_flag = true;
	}
	// R
	if (reg_type == 0 || reg_type == 6) {
		operand += reg_name[2][scalar_stack_AR];
		operand += "[";
		// offset in limit
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			if (core == 64) {
				AddNewInstruction("SMOVI", to_string(address), reg_name[0][offset_buf], ";stack offset spill, transfer the offset to R");
				AddNewInstruction("SMVAGA", reg_name[0][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			}
			else if (core == 32 || core == 34) {
				AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s low word to R");
				AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			}
			operand += reg_name[4][scalar_stack_OR];
		}
		operand += "]";
		note += "\" back to register";
		AddNewInstruction("SLDW", operand, reg_name[reg_type][reg_num], note, snop);
	}
	// VR
	else if (reg_type == 1 || reg_type == 7) {
		operand += reg_name[3][vector_stack_AR];
		operand += "[";
		// offset in limit
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			if (core == 64) {
				AddNewInstruction("SMOVI", to_string(address), reg_name[0][offset_buf], ";stack offset spill, transfer the offset to R");
				AddNewInstruction("SMVAGA", reg_name[0][offset_buf], reg_name[5][vector_stack_OR], ";transfer the stack offset into the OR");
			}
			else if (core == 32 || core == 34) {
				AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s low word to R");
				AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[5][vector_stack_OR], ";transfer the stack offset into the OR");
			}
			operand += reg_name[5][vector_stack_OR];
		}
		operand += "]";
		note += "\" back to register";
		AddNewInstruction("VLDW", operand, reg_name[reg_type][reg_num], note, snop);
	}
	// AR/OR
	else if (core == 64) {
		operand += reg_name[2][scalar_stack_AR];
		operand += "[";
		if (block_num != 0) {
			buf_type = 0;
			if (current_block->reg_available[buf_type].size() == 0) {
				SelectIdleRegister(buf_type);
				ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
			}
			buf_num = current_block->reg_available[buf_type].back();
			GetReserveRegister(buf_type);
			OccupyRegister(buf_type);
			ReleaseRegister(buf_type, buf_num, 0);
		}
		else buf_num = reg_buf.back();
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		else {
			AddNewInstruction("SMOVI", to_string(address), reg_name[0][offset_buf], ";stack offset spill, transfer the offset to R");
			AddNewInstruction("SMVAGA", reg_name[0][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			operand += reg_name[4][scalar_stack_OR];

		}
		operand += "]";
		note += "\" in " + reg_name[buf_type][buf_num];
		AddNewInstruction("SLDW", operand, reg_name[buf_type][buf_num], note, snop);
		AddNewInstruction("SMVAGA", reg_name[buf_type][buf_num], reg_name[reg_type][reg_num], ";transfer \"" + var + "\" and release " + reg_name[buf_type][buf_num]);
	}
	else if (core == 32 || core == 34) {
		operand += reg_name[2][scalar_stack_AR];
		operand += "[";
		buf_type = 8;
		if (block_num != 0) {
			if (current_block->reg_available[buf_type].size() == 0) {
				SelectIdleRegister(buf_type);
				ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
			}
			buf_num = current_block->reg_available[buf_type].back();
			GetReserveRegister(buf_type);
			OccupyRegister(buf_type);
			ReleaseRegister(buf_type, buf_num, 0);
		}
		else buf_num = reg_buf.back();
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s low word to R");
			AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			operand += reg_name[4][scalar_stack_OR];
		}
		operand += "]";
		note += "\"\'s low word back to " + reg_name[0][buf_num - 1];
		AddNewInstruction("SLDW", operand, reg_name[0][buf_num - 1], note, snop);
		address++;
		operand = "*+" + reg_name[2][scalar_stack_AR] + "[";
		if (address < kImmOffset) {
			operand += to_string(address);
		}
		// offset out of range
		else {
			AddNewInstruction("SMOVIL", to_string(address), reg_name[0][offset_buf - 1], ";stack offset spill, transfer the offset\'s high word to R");
			AddNewInstruction("SMVAGA36", reg_name[8][offset_buf], reg_name[4][scalar_stack_OR], ";transfer the stack offset into the OR");
			operand += reg_name[4][scalar_stack_OR];
		}
		operand += "]";
		note = ";restore \"" + var + "\"\'s high word back to " + reg_name[0][buf_num];
		AddNewInstruction("SLDW", operand, reg_name[0][buf_num], note, snop);
		AddNewInstruction("SMVAGA36", reg_name[buf_type][buf_num], reg_name[reg_type][reg_num], ";transfer \"" + var + "\" and release " + reg_name[buf_type][buf_num]);
	}
}


void OccupyRegister(int reg_type, string var, string var2) {
	int reg_num = current_block->reg_available[reg_type].back();

	current_block->reg_available[reg_type].pop_back();
	current_block->reg_idle[reg_type].push_front(reg_num);

	// update the pair reg available stack and reg idle list
	if (reg_type == 0 || reg_type == 1) {
		if (reg_num % 2) {
			if (current_block->reg_used[reg_type][reg_num - 1] == NULL || current_block->reg_used[reg_type][reg_num - 1] == &kRes) {
				current_block->reg_available[reg_type + 8].remove(reg_num);
				current_block->reg_idle[reg_type + 8].push_front(reg_num);
			}
			else if (current_block->reg_used[reg_type][reg_num - 1] != &kCond && current_block->reg_used[reg_type][reg_num - 1] != &kStack) {
				current_block->reg_idle[reg_type + 8].remove(reg_num);
				current_block->reg_idle[reg_type + 8].push_front(reg_num);
			}
		}
		else {
			if (current_block->reg_used[reg_type][reg_num + 1] == NULL || current_block->reg_used[reg_type][reg_num + 1] == &kRes) {
				current_block->reg_available[reg_type + 8].remove(reg_num + 1);
				current_block->reg_idle[reg_type + 8].push_front(reg_num + 1);
			}
			else if (current_block->reg_used[reg_type][reg_num + 1] != &kStack) {
				current_block->reg_idle[reg_type + 8].remove(reg_num + 1);
				current_block->reg_idle[reg_type + 8].push_front(reg_num + 1);
			}
		}
	}
	// update the single reg available queue
	else if (reg_type == 8 || reg_type == 9) {
		reg_type -= 8;
		current_block->reg_available[reg_type].remove(reg_num);
		current_block->reg_available[reg_type].remove(reg_num - 1);
		current_block->reg_idle[reg_type].push_front(reg_num);
		current_block->reg_idle[reg_type].push_front(reg_num - 1);
		if (!var2.empty()) {
			current_block->var_storage[var2].reg_num = reg_num - 1;
			current_block->reg_used[reg_type][reg_num - 1] = &var_map[var2].name;
		}
		else current_block->reg_used[reg_type][reg_num - 1] = &kBuf;
	}
	// record the variable and register information
	if (!var.empty()) {
		current_block->var_storage[var].reg_num = reg_num;
		current_block->reg_used[reg_type][reg_num] = &var_map[var].name;
	}
	else current_block->reg_used[reg_type][reg_num] = &kBuf;
}


void ReleaseDeadVar(const int& reg_type, const int& reg_num) {
	string var;
	if (current_block->reg_used[reg_type][reg_num] == NULL) {
		current_block->reg_available[reg_type].remove(reg_num);
		current_block->reg_used[reg_type][reg_num] = NULL;
	}
	else if (current_block->reg_used[reg_type][reg_num] == &kRes) {
		current_block->reg_available[reg_type].remove(reg_num);
	}
	else if (current_block->reg_used[reg_type][reg_num] == &kBuf) {
		current_block->reg_used[reg_type][reg_num] = NULL;
	}
	else {
		var = *current_block->reg_used[reg_type][reg_num];
		current_block->var_storage[var].reg_num = -1;
		current_block->reg_used[reg_type][reg_num] = NULL;
	}
	current_block->reg_released_buf[reg_type].push_back(reg_num);  // modified by DC
	current_block->reg_idle[reg_type].remove(reg_num);
	if (reg_type == 0 || reg_type == 1) {
		if (reg_num % 2) {
			if (current_block->reg_used[reg_type][reg_num - 1] == NULL || current_block->reg_used[reg_type][reg_num - 1] == &kRes) {
				current_block->reg_released_buf[reg_type + 8].push_back(reg_num); // modified by DC
				current_block->reg_idle[reg_type + 8].remove(reg_num);
			}
		}
		else {
			if (current_block->reg_used[reg_type][reg_num + 1] == NULL || current_block->reg_used[reg_type][reg_num + 1] == &kRes) {
				current_block->reg_released_buf[reg_type + 8].push_back(reg_num + 1); // modified by DC
				current_block->reg_idle[reg_type + 8].remove(reg_num + 1);
			}
		}
	}
}


void ReleaseAliveVar(const int& reg_type, const int& reg_num) {
	string var;
	if (current_block->reg_used[reg_type][reg_num] == NULL) {
		current_block->reg_available[reg_type].remove(reg_num);
		//current_block->reg_used[reg_type][reg_num] = NULL;
	}
	else if (current_block->reg_used[reg_type][reg_num] == &kRes) {
		current_block->reg_available[reg_type].remove(reg_num);
	}
	else {
		var = *current_block->reg_used[reg_type][reg_num];
		current_block->var_storage[var].reg_num = -2;
		if (current_block->var_storage[var].mem_address == -1) {
			if (reg_type == 1 || reg_type == 7) {
				current_block->var_storage[var].mem_address = vector_save;
			}
			else current_block->var_storage[var].mem_address = scalar_save;
			if (current_block->fix_var.count(var)) {
				if (current_block->fix_var[var].mem_address == -1) {
					current_block->fix_var[var].mem_address = current_block->var_storage[var].mem_address;
				}
				else {
					PrintError("The fix mem_address has been assigned, but var_storage mem_address hasn\'t\n");
					Error();
				}
			}
			SaveSpillRegister(reg_type, reg_num);
		}
		else {
			SaveSpillRegister(reg_type, reg_num, current_block->var_storage[var].mem_address);
		}
		current_block->reg_used[reg_type][reg_num] = NULL;
	}
	current_block->reg_available[reg_type].push_back(reg_num); // modified by DC
	current_block->reg_idle[reg_type].remove(reg_num);
	if (reg_type == 0 || reg_type == 1) {
		if (reg_num % 2) {
			if (current_block->reg_used[reg_type][reg_num - 1] == NULL || current_block->reg_used[reg_type][reg_num - 1] == &kRes) {
				current_block->reg_available[reg_type + 8].remove(reg_num);
				current_block->reg_available[reg_type + 8].push_back(reg_num);  // modified by DC
				current_block->reg_idle[reg_type + 8].remove(reg_num);
			}
		}
		else {
			if (current_block->reg_used[reg_type][reg_num + 1] == NULL || current_block->reg_used[reg_type][reg_num + 1] == &kRes) {
				current_block->reg_available[reg_type + 8].remove(reg_num + 1);
				current_block->reg_available[reg_type + 8].push_back(reg_num + 1);  // modified by DC
				current_block->reg_idle[reg_type + 8].remove(reg_num + 1);
			}
		}
	}
}


void ReleaseRegister(int reg_type, int reg_num, int alive) {
	string var;
	// release registers whose vhose variables are dead
	if (!alive) {
		if (reg_type == 8 || reg_type == 9) {
			reg_type -= 8;
			ReleaseDeadVar(reg_type, reg_num--);
		}
		ReleaseDeadVar(reg_type, reg_num);
	}
	// release alive variables
	else {
		if (reg_type == 8 || reg_type == 9) {
			reg_type -= 8;
			ReleaseAliveVar(reg_type, reg_num--);
		}
		ReleaseAliveVar(reg_type, reg_num);
		return;
	}
}


void DealWithFixVar(string var, string var2) {
	int reg_type, reg_num;
	string var_occupy = "", var_occupy2 = "";
	reg_type = int(var_map[var].reg_type);

	// the variable has already been fix in one register
	if (current_block->fix_var[var].reg_num >= 0) {
		reg_num = current_block->fix_var[var].reg_num;
		// single register
		if (var2.empty()) {
			// register is available
			if (current_block->reg_used[reg_type][reg_num] == NULL) {
				current_block->reg_available[reg_type].remove(reg_num);
				current_block->reg_available[reg_type].push_back(reg_num);
				return;
			}
			// register is occupied
			var_occupy = *current_block->reg_used[reg_type][reg_num];
			// the variable occupying isn't used in the same cycle
			if (!IsInterferingVar(var_occupy)) {
				ReleaseRegister(reg_type, reg_num);
				return;
			}
		}
		// pair format
		else {
			reg_num = reg_num - var_map[var].pair_pos;
			// register is available
			if (current_block->reg_used[reg_type][reg_num] == NULL && current_block->reg_used[reg_type][reg_num - 1] == NULL) {
				current_block->reg_available[reg_type + 8].remove(reg_num);
				current_block->reg_available[reg_type + 8].push_back(reg_num);
				return;
			}
			if (current_block->reg_used[reg_type][reg_num])	var_occupy = *current_block->reg_used[reg_type][reg_num];
			if (current_block->reg_used[reg_type][reg_num - 1])	var_occupy2 = *current_block->reg_used[reg_type][reg_num - 1];
			reg_type += 8;
			// the variables occupying aren't used in the same cycle
			if (!IsInterferingVar(var_occupy) && !IsInterferingVar(var_occupy2)) {
				ReleaseRegister(reg_type, reg_num);
				return;
			}
		}
		// the variables occupying are used in the same cycle, so prepare a new register (pair)
		if (current_block->reg_available[reg_type].size()) return;
		SelectIdleRegister(reg_type);
		reg_num = current_block->reg_idle[reg_type].back();
		ReleaseRegister(reg_type, reg_num);
		return;
	}
	// the variable has already been fixed in the memory
	if (current_block->fix_var[var].reg_num == -2) {
		if (!var_map[var].partner.empty()) reg_type += 8;
		if (!current_block->reg_available[reg_type].size()) {
			SelectIdleRegister(reg_type);
			reg_num = current_block->reg_idle[reg_type].back();
			ReleaseRegister(reg_type, reg_num);
		}
		return;
	}
	// the variable hasn't been fix
	if (current_block->fix_var[var].reg_num == -1) {
		// single register
		if (var2.empty()) {
			if (!current_block->reg_available[reg_type].size()) {
				SelectIdleRegister(reg_type);
				reg_num = current_block->reg_idle[reg_type].back();
				ReleaseRegister(reg_type, reg_num);
			}
			else reg_num = current_block->reg_available[reg_type].back();
			// register is available
			if (current_block->reg_used[reg_type][reg_num] == NULL) {
				current_block->reg_available[reg_type].remove(reg_num);
				current_block->reg_available[reg_type].push_back(reg_num);
				return;
			}
			// all registers are fixed
			current_block->fix_var[var].reg_num = -2;
			if (reg_type == 1 || reg_type == 7) {
				current_block->fix_var[var].mem_address = vector_save;
				vector_save += kVpe;
			}
			else {
				current_block->fix_var[var].mem_address = scalar_save;
				scalar_save++;
				if ((core == 32 || core == 34) && reg_type != 0 && reg_type != 6) scalar_save++;
			}
			RestoreSpillRegister(reg_type, reg_num, current_block->fix_var[var].mem_address, var);
			current_block->var_storage[var].mem_address = current_block->fix_var[var].mem_address;
			return;
		}
		// pair format
		reg_type += 8;
		if (var_map[var].pair_pos == -1) var = var_map[var].partner;
		var2 = var_map[var].partner;
		if (!current_block->reg_available[reg_type].size()) {
			SelectIdleRegister(reg_type);
			reg_num = current_block->reg_idle[reg_type].back();
			ReleaseRegister(reg_type, reg_num);
		}
		else reg_num = current_block->reg_available[reg_type].back();
		reg_type -= 8;
		// register is available
		if (current_block->reg_fix[reg_type][reg_num] == NULL && current_block->reg_used[reg_type][reg_num - 1] == NULL) {
			current_block->fix_var[var].reg_num = reg_num;
			current_block->fix_var[var2].reg_num = reg_num - 1;
			current_block->reg_fix[reg_type][reg_num] = &var_map[var].name;
			current_block->reg_fix[reg_type][reg_num - 1] = &var_map[var2].name;
			return;
		}
		// all registers are fixed
		current_block->fix_var[var].reg_num = -2;
		current_block->fix_var[var2].reg_num = -2;
		if (reg_type) {
			current_block->fix_var[var].mem_address = vector_save;
			vector_save += kVpe;
			current_block->fix_var[var2].mem_address = vector_save;
			vector_save += kVpe;
		}
		else {
			current_block->fix_var[var].mem_address = scalar_save;
			scalar_save++;
			current_block->fix_var[var2].mem_address = scalar_save;
			scalar_save++;
		}
		RestoreSpillRegister(reg_type, reg_num, current_block->fix_var[var].mem_address, var);
		RestoreSpillRegister(reg_type, reg_num - 1, current_block->fix_var[var2].mem_address, var2);
		current_block->var_storage[var].mem_address = current_block->fix_var[var].mem_address;
		current_block->var_storage[var2].mem_address = current_block->fix_var[var2].mem_address;
	}
}


bool IsInterferingVar(const string& var) {
	for (int i = 0; i < interfering_var.front().size(); ++i) {
		if (var == interfering_var.front()[i]) return true;
	}
	return false;
}


void AddReleaseReg(const int& reg_type, const int& reg_num, const int& fix) {
	for (int i = 0; i < release_reg[reg_type].size(); ++i) {
		if (release_reg[reg_type][i].first == reg_num) return;
	}
	release_reg[reg_type].push_back(make_pair(reg_num, fix));
}


int TransferVarPair(const int& reg_type, const int& reg_num, const string& var) {
	string restore_var = var_map[var].partner;
	int pair_num;
	int restore_num = current_block->var_storage[var].reg_num;
	int mem_address = current_block->var_storage[restore_var].mem_address;
	if (!current_block->reg_available[reg_type].size()) {
		SelectIdleRegister(reg_type);
		ReleaseRegister(reg_type, current_block->reg_idle[reg_type].back());
	}
	pair_num = current_block->reg_available[reg_type].back();
	GetReserveRegister(reg_type);
	restore_num = pair_num + var_map[restore_var].pair_pos;
	if (!spill_flag) {
		RegSpillInit();
		spill_flag = true;
	}
	if (reg_type == 8) {
		AddNewInstruction("SMOV", reg_name[0][reg_num], reg_name[0][pair_num + var_map[var].pair_pos], ";transfer the variable \"" + var + "\" to new register");
	}
	else {
		AddNewInstruction("VMOV", reg_name[1][reg_num], reg_name[1][pair_num + var_map[var].pair_pos], ";transfer the variable \"" + var + "\" to new register");
	}
	ReleaseRegister(reg_type - 8, reg_num, 0);
	RestoreSpillRegister(reg_type - 8, restore_num, mem_address, restore_var);
	current_block->reg_available[reg_type - 8].remove(pair_num - 1);
	current_block->reg_available[reg_type - 8].remove(pair_num);
	if (var_map[var].pair_pos == 0) {
		current_block->reg_available[reg_type - 8].push_back(pair_num - 1);
		current_block->reg_available[reg_type - 8].push_back(pair_num);
	}
	else {
		current_block->reg_available[reg_type - 8].push_back(pair_num);
		current_block->reg_available[reg_type - 8].push_back(pair_num - 1);
	}
	OccupyRegister(reg_type - 8, var);
	return pair_num;
}


int RepairRegPair(string var, string restore_var) {
	int reg_type, reg_num, restore_num, mem_address;
	int pair_num, fix;
	string buf, occupy_var;
	reg_type = int(var_map[var].reg_type);
	reg_num = current_block->var_storage[var].reg_num;
	restore_num = current_block->var_storage[restore_var].reg_num;
	if (reg_num >= 0) {
		pair_num = reg_num;
		restore_num = reg_num - 1;
	}
	else {
		reg_num = restore_num++;
		pair_num = restore_num;
		buf = var;
		var = restore_var;
		restore_var = buf;
	}
	mem_address = current_block->var_storage[restore_var].mem_address;
	fix = current_block->fix_var.count(var) + current_block->fix_var.count(restore_var);
	if (fix == 0) {
		// the register is empty
		if (current_block->reg_used[reg_type][restore_num] == &kRes || current_block->reg_used[reg_type][restore_num] == NULL) {
			current_block->reg_available[reg_type].remove(restore_num);
			current_block->reg_available[reg_type].push_back(restore_num);
			GetReserveRegister(reg_type);
			RestoreSpillRegister(reg_type, restore_num, mem_address, restore_var);
		}
		// the register is occupied by other variables
		else {
			pair_num = TransferVarPair(reg_type + 8, reg_num, var);
		}
		OccupyRegister(reg_type, restore_var);
		return pair_num;
	}
	// the variables are fix variables
	pair_num = current_block->fix_var[var].reg_num - var_map[var].pair_pos;
	// the existing variable is at its fix position
	if (current_block->fix_var[var].reg_num == reg_num) {
		// the register is empty
		if (current_block->reg_used[reg_type][restore_num] == NULL) {
			current_block->reg_available[reg_type].remove(restore_num);
			current_block->reg_available[reg_type].push_back(restore_num);
			RestoreSpillRegister(reg_type, restore_num, mem_address, restore_var);
		}
		else {
			// the variable is not a interfering variable
			occupy_var = *current_block->reg_used[reg_type][restore_num];
			if (!IsInterferingVar(occupy_var)) {
				ReleaseRegister(reg_type, reg_num);
				RestoreSpillRegister(reg_type, restore_num, mem_address, restore_var);
			}
			// the variable is a interfering variable
			else {
				pair_num = TransferVarPair(reg_type + 8, reg_num, var);
			}
		}
		OccupyRegister(reg_type, restore_var);
		return pair_num;
	}
	// the existing variable is not at its fix position
	if (current_block->reg_used[reg_type][pair_num] == NULL && current_block->reg_used[reg_type][pair_num - 1] == NULL) {
		current_block->reg_available[reg_type + 8].remove(pair_num);
		current_block->reg_available[reg_type + 8].push_back(pair_num);
	}
	else if (current_block->reg_used[reg_type][pair_num] == NULL && current_block->reg_used[reg_type][pair_num - 1] != NULL) {
		occupy_var = *current_block->reg_used[reg_type][pair_num - 1];
		if (!IsInterferingVar(occupy_var)) {
			ReleaseRegister(reg_type, pair_num - 1);
			current_block->reg_available[reg_type + 8].remove(pair_num);
			current_block->reg_available[reg_type + 8].push_back(pair_num);
		}
	}
	else if (current_block->reg_used[reg_type][pair_num] != NULL && current_block->reg_used[reg_type][pair_num - 1] == NULL) {
		occupy_var = *current_block->reg_used[reg_type][pair_num];
		if (!IsInterferingVar(occupy_var)) {
			ReleaseRegister(reg_type, pair_num);
			current_block->reg_available[reg_type + 8].remove(pair_num);
			current_block->reg_available[reg_type + 8].push_back(pair_num);
		}
	}
	else {
		if (!IsInterferingVar(*current_block->reg_used[reg_type][pair_num]) && !IsInterferingVar(*current_block->reg_used[reg_type][pair_num - 1])) {
			ReleaseRegister(reg_type + 8, pair_num);
		}
	}
	pair_num = TransferVarPair(reg_type + 8, reg_num, var);
	OccupyRegister(reg_type, restore_var);
	return pair_num;
}


int GetInterferingVar(const string& operand, const int& cycle) {
	if (IsSpecialOperand(operand) != -1) return 0;

	int i = 0, j, new_var_flag = 0;
	string var = "";

	// memory access format
	if (operand[0] == '*') {
		// AR
		while (operand[i] == '*' || operand[i] == '+' || operand[i] == '-') i++;
		while (operand[i] != '+' && operand[i] != '-' && operand[i] != '[' && operand[i] != '\0') var += operand[i++];
		while (operand[i] == '+' || operand[i] == '-' || operand[i] == '[') i++;
		if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		if ((operand[i] >= '0' && operand[i] <= '9') || operand[i] == '\0') return new_var_flag;
		// OR
		var = "";
		while (operand[i] != ']') var += operand[i++];
		if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		return new_var_flag;
	}
	// other format
	while (operand[i] != ':' && i < operand.size()) var += operand[i++];
	if (cycle) {
		for (j = 0; j < interfering_var.size(); ++j) {
			if (j < cycle && !IsInterferingVar(var)) interfering_var.front().push_back(var);
			interfering_var.push_back(interfering_var.front());
			interfering_var.pop_front();
		}
		for (; j < cycle; ++j) {
			interfering_var.push_back(vector<string>());
			interfering_var.back().push_back(var);
		}
		if (linear_program.block_pointer_list[block_num]->var_storage[var].reg_num == -1) {
			new_var_flag = 1;
		}
	}
	else if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
	var = var_map[var].partner;
	if (var.empty()) return new_var_flag;
	// reg pair relevant
	if (i != operand.size()) {
		if (cycle) {
			for (j = 0; j < interfering_var.size(); ++j) {
				if (j < cycle && !IsInterferingVar(var)) interfering_var.front().push_back(var);
				interfering_var.push_back(interfering_var.front());
				interfering_var.pop_front();
			}
			for (; j < cycle; ++j) {
				interfering_var.push_back(vector<string>());
				interfering_var.back().push_back(var);
			}
			if (new_var_flag == 1 && linear_program.block_pointer_list[block_num]->var_storage[var].reg_num != -1) {
				new_var_flag = 0;
			}
		}
		else if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		//return new_var_flag;
	}
	//if (IsVarAlive(var, int(var_map[var].reg_type)) && !IsInterferingVar(var)) {
	//	interfering_var.front().push_back(var);
	//}
	return new_var_flag;
}


void ConvertVarToReg(string& operand, const int& repeat_type) {
	if (debug) DebugCheckRegState();
	//if (operand == "mant13") {
	//	printf("Error occur this cycle.\n");
	//}
	if (IsSpecialOperand(operand) != -1) return;

	int reg_type, reg_num, mem_address = -1;
	int reg_num2, mem_address2;
	string new_operand = "", var = "", var2 = "";
	int fix;
	int i = 0;

	// memory access format
	if (operand[0] == '*') {
		// AR
		while (operand[i] == '*' || operand[i] == '+' || operand[i] == '-') new_operand += operand[i++];
		while (operand[i] != '+' && operand[i] != '-' && operand[i] != '[' && operand[i] != '\0') var += operand[i++];
		reg_type = int(var_map[var].reg_type);
		reg_num = current_block->var_storage[var].reg_num;
		fix = current_block->fix_var.count(var);
		// the variable has already assigned a register
		if (reg_num >= 0) {
			UpdateRegIdle(reg_type, reg_num);
		}
		// -1: the value is empty; -2: the value is saved in memory
		else {
			if (reg_num == -2) mem_address = current_block->var_storage[var].mem_address;
			if (fix) DealWithFixVar(var);
			else if (!current_block->reg_available[reg_type].size()) SelectIdleRegister(reg_type);
			int idle_num = -1;
			if (!current_block->reg_available[reg_type].size()) {
				idle_num = current_block->reg_idle[reg_type].back();
				ReleaseRegister(reg_type, current_block->reg_idle[reg_type].back());
			}
			// add a reg from reg_released_buf to reg_available   modified by DC
			list<int>::iterator it;
			for (it = current_block->reg_released_buf[reg_type].begin(); it != current_block->reg_released_buf[reg_type].end(); it++) {
				if (*it == idle_num) {
					current_block->reg_available[reg_type].push_back(*it);
					current_block->reg_released_buf[reg_type].erase(it);
					//break;
				}
			}
			reg_num = current_block->reg_available[reg_type].back();
			if (mem_address == -1) GetReserveRegister(reg_type);
			else RestoreSpillRegister(reg_type, reg_num, mem_address, var);
			OccupyRegister(reg_type, var);
		}
		//if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		new_operand += reg_name[reg_type][reg_num];
		if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		if (!IsVarAlive(var, reg_type)) AddReleaseReg(reg_type, reg_num, fix);
		while (operand[i] == '+' || operand[i] == '-' || operand[i] == '[') new_operand += operand[i++];
		if (operand[i] == '\0') {
			if (repeat_type == 0 || repeat_type == 3) operand = new_operand;
			return;
		}
		// IMM
		if ((operand[i] >= '0' && operand[i] <= '9') || operand[i] == '-') {
			while (operand[i] != '\0') new_operand += operand[i++];
			if (repeat_type == 0 || repeat_type == 3) operand = new_operand;
			return;
		}
		// OR
		var = "";
		while (operand[i] != ']') var += operand[i++];
		reg_type = int(var_map[var].reg_type);
		reg_num = current_block->var_storage[var].reg_num;
		fix = current_block->fix_var.count(var);
		// the variable has already assigned a register
		if (reg_num >= 0) {
			UpdateRegIdle(reg_type, reg_num);
		}
		// -1: the value is empty; -2: the value is saved in memory
		else {
			if (reg_num == -2) mem_address = current_block->var_storage[var].mem_address;
			else mem_address = -1;
			if (fix) DealWithFixVar(var);
			else if (!current_block->reg_available[reg_type].size()) SelectIdleRegister(reg_type);
			int idle_num = -1;
			if (!current_block->reg_available[reg_type].size()) {
				idle_num = current_block->reg_idle[reg_type].back();
				ReleaseRegister(reg_type, current_block->reg_idle[reg_type].back());
			}
			// add a reg from reg_released_buf to reg_available   modified by DC
			list<int>::iterator it;
			for (it = current_block->reg_released_buf[reg_type].begin(); it != current_block->reg_released_buf[reg_type].end(); it++) {
				if (*it == idle_num) {
					current_block->reg_available[reg_type].push_back(*it);
					current_block->reg_released_buf[reg_type].erase(it);
					//break;
				}
			}
			reg_num = current_block->reg_available[reg_type].back();
			if (mem_address == -1) GetReserveRegister(reg_type);
			else RestoreSpillRegister(reg_type, reg_num, mem_address, var);
			OccupyRegister(reg_type, var);
		}
		//if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		new_operand += reg_name[reg_type][reg_num];
		new_operand += ']';
		if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		if (repeat_type == 0 || repeat_type == 3) operand = new_operand;
		else return;
		if (!IsVarAlive(var, reg_type)) AddReleaseReg(reg_type, reg_num, fix);
		return;
	}
	// other format
	while (operand[i] != ':' && operand[i] != '\0') var += operand[i++];
	reg_type = int(var_map[var].reg_type);
	reg_num = current_block->var_storage[var].reg_num;
	fix = current_block->fix_var.count(var);
	// pair format
	if (operand[i] == ':') {
		reg_type += 8;
		var2 = var_map[var].partner;
		reg_num = current_block->var_storage[var].reg_num;
		reg_num2 = current_block->var_storage[var2].reg_num;
		// both variables have already been assigned
		if (reg_num >= 0 && reg_num2 >= 0) {
			UpdateRegIdle(reg_type, reg_num);
		}
		// both variables are empty or saved in memory
		else if ((reg_num == -1 && reg_num2 == -1) || (reg_num == -2 && reg_num2 == -2)) {
			if (reg_num == -2) {
				mem_address = current_block->var_storage[var].mem_address;
				mem_address2 = current_block->var_storage[var2].mem_address;
			}
			if (fix) DealWithFixVar(var, var2);
			else if (!current_block->reg_available[reg_type].size()) SelectIdleRegister(reg_type);
			int idle_num = -1;
			if (!current_block->reg_available[reg_type].size()) {
				idle_num = current_block->reg_idle[reg_type].back();
				ReleaseRegister(reg_type, current_block->reg_idle[reg_type].back());
			}
			// add a reg from reg_released_buf to reg_available   modified by DC
			list<int>::iterator it;
			for (it = current_block->reg_released_buf[reg_type].begin(); it != current_block->reg_released_buf[reg_type].end(); it++) {
				if (*it == idle_num) {
					current_block->reg_available[reg_type].push_back(*it);
					current_block->reg_released_buf[reg_type].erase(it);
					//break;
				}
			}
			reg_num = current_block->reg_available[reg_type].back();
			if (mem_address == -1) GetReserveRegister(reg_type);
			else {
				RestoreSpillRegister(reg_type - 8, reg_num, mem_address, var);
				RestoreSpillRegister(reg_type - 8, reg_num - 1, mem_address2, var2);
			}
			OccupyRegister(reg_type, var, var2);
		}
		// one of the variable is saved in memory
		else {
			reg_num = RepairRegPair(var, var2);
		}
		if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		if (!IsInterferingVar(var2)) interfering_var.front().push_back(var2);
		new_operand = reg_name[reg_type][reg_num];
		if (repeat_type == 0 || repeat_type == 3) operand = new_operand;
		else return;
		if (IsVarAlive(var, reg_type)) return;
		reg_type -= 8;
		if (!IsVarAlive(var, reg_type)) {
			AddReleaseReg(reg_type, reg_num, fix);
		}
		reg_num--;
		if (!IsVarAlive(var2, reg_type)) {
			AddReleaseReg(reg_type, reg_num, fix);
		}
		return;
	}
	if (reg_type < 2 && IsVarAlive(var, reg_type + 8)) var2 = var_map[var].partner;
	// single variable
	if (var2.empty()) {
		// the variable has already assigned a register
		if (reg_num >= 0) {
			UpdateRegIdle(reg_type, reg_num);
		}
		// -1: the value is empty; -2: the value is saved in memory
		else {
			if (reg_num == -2) mem_address = current_block->var_storage[var].mem_address;
			if (fix) DealWithFixVar(var);
			else if (!current_block->reg_available[reg_type].size()) SelectIdleRegister(reg_type);
			int idle_num = -1;
			if (!current_block->reg_available[reg_type].size()) {
				idle_num = current_block->reg_idle[reg_type].back();
				ReleaseRegister(reg_type, current_block->reg_idle[reg_type].back());
			}
			// add a reg from reg_released_buf to reg_available   modified by DC
			list<int>::iterator it;
			for (it = current_block->reg_released_buf[reg_type].begin(); it != current_block->reg_released_buf[reg_type].end(); it++) {
				if (*it == idle_num) {
					current_block->reg_available[reg_type].push_back(*it);
					current_block->reg_released_buf[reg_type].erase(it);
					//break;
				}
			}
			reg_num = current_block->reg_available[reg_type].back();
			if (mem_address == -1) GetReserveRegister(reg_type);
			else RestoreSpillRegister(reg_type, reg_num, mem_address, var);
			OccupyRegister(reg_type, var);
		}
		if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
		new_operand += reg_name[reg_type][reg_num];
		if (repeat_type == 0 || repeat_type == 3) operand = new_operand;
		else return;
		if (!IsVarAlive(var, reg_type)) {
			AddReleaseReg(reg_type, reg_num, fix);
			var2 = var_map[var].partner;
			// deal with pair_declared but single_used
			if (!var2.empty()) {
				if (!IsVarAlive(var, reg_type) && current_block->var_storage[var2].reg_num >= 0) {
					AddReleaseReg(reg_type, current_block->var_storage[var2].reg_num, current_block->fix_var.count(var2));
				}
			}
		}
		return;
	}
	// variable pair
	reg_type += 8;
	if (var_map[var].pair_pos == -1) var = var2;
	var2 = var_map[var].partner;
	reg_num = current_block->var_storage[var].reg_num;
	reg_num2 = current_block->var_storage[var2].reg_num;
	// both variables have already been assigned
	if (reg_num >= 0 && reg_num2 >= 0) {
		UpdateRegIdle(reg_type, reg_num);
	}
	// both variables are empty or saved in memory
	else if ((reg_num == -1 && reg_num2 == -1) || (reg_num == -2 && reg_num2 == -2)) {
		if (reg_num == -2) {
			mem_address = current_block->var_storage[var].mem_address;
			mem_address2 = current_block->var_storage[var2].mem_address;
		}
		if (fix) DealWithFixVar(var, var2);
		else if (!current_block->reg_available[reg_type].size()) SelectIdleRegister(reg_type);
		int idle_num = -1;
		if (!current_block->reg_available[reg_type].size()) {
			idle_num = current_block->reg_idle[reg_type].back();
			ReleaseRegister(reg_type, current_block->reg_idle[reg_type].back());
		}
		// add a reg from reg_released_buf to reg_available   modified by DC
		list<int>::iterator it;
		for (it = current_block->reg_released_buf[reg_type].begin(); it != current_block->reg_released_buf[reg_type].end(); it++) {
			if (*it == idle_num) {
				current_block->reg_available[reg_type].push_back(*it);
				current_block->reg_released_buf[reg_type].erase(it);
				//break;
			}
		}
		reg_num = current_block->reg_available[reg_type].back();
		if (mem_address == -1) GetReserveRegister(reg_type);
		else {
			RestoreSpillRegister(reg_type - 8, reg_num, mem_address, var);
			RestoreSpillRegister(reg_type - 8, reg_num - 1, mem_address2, var2);
		}
		OccupyRegister(reg_type, var, var2);
	}
	// one of the variable is saved in memory
	else {
		reg_num = RepairRegPair(var, var2);
	}
	if (!IsInterferingVar(var)) interfering_var.front().push_back(var);
	if (!IsInterferingVar(var2)) interfering_var.front().push_back(var2);
	reg_num += var_map[operand].pair_pos;
	new_operand = reg_name[reg_type - 8][reg_num];
	if (repeat_type == 0 || repeat_type == 3) operand = new_operand;
	else return;
	if (IsVarAlive(var, reg_type)) return;
	reg_type -= 8;
	if (!IsVarAlive(var, reg_type)) {
		AddReleaseReg(reg_type, reg_num, fix);
	}
	reg_num--;
	if (!IsVarAlive(var2, reg_type)) {
		AddReleaseReg(reg_type, reg_num, fix);
	}
}


void ReleaseRegThisCycle() {
	int reg_type, reg_num;
	for (reg_type = 0; reg_type < kRegType; ++reg_type) {
		for (int i = 0; i < release_reg[reg_type].size(); ++i) {
			reg_num = release_reg[reg_type][i].first;
			ReleaseRegister(reg_type, reg_num, 0);
			if (release_reg[reg_type][i].second) {
				current_block->reg_available[reg_type].remove(reg_num);
				current_block->reg_released_buf[reg_type].push_front(reg_num);  // modified by DC
			}
			if (debug) DebugCheckRegState();
		}
	}
}

void UpdateRegAvailable() {
	/*if (current_block->reg_released_buf->size()) {
		current_block->reg_released_buf->clear();
	}*/

	int reg_type, reg_num;
	for (reg_type = 0; reg_type < kRegType; reg_type++) {
		list<int>::iterator it;
		for (it = current_block->reg_released_buf[reg_type].begin(); it != current_block->reg_released_buf[reg_type].end();) {
			reg_num = *it;
			current_block->reg_available[reg_type].push_front(reg_num);
			current_block->reg_released_buf[reg_type].erase(it++);
		}
	}
}


void ReleaseForFixVar(const int& reg_type, const int& src_num, const int& fix_num) {
	string* var = current_block->reg_used[reg_type][src_num];

	current_block->reg_used[reg_type][src_num] = NULL;
	current_block->reg_used[reg_type][fix_num] = var;
	current_block->reg_available[reg_type].remove(fix_num);
	current_block->reg_available[reg_type].push_back(src_num);
	current_block->reg_idle[reg_type].remove(fix_num);
	current_block->reg_idle[reg_type].push_front(fix_num);
	current_block->var_storage[*var].reg_num = fix_num;
	if (reg_type > 1) return;
	if (src_num % 2) {
		if (current_block->reg_used[reg_type][src_num - 1] == NULL || current_block->reg_used[reg_type][src_num - 1] == &kRes) {
			current_block->reg_idle[reg_type + 8].remove(src_num);
			current_block->reg_available[reg_type + 8].push_back(src_num);
		}
	}
	else {
		if (current_block->reg_used[reg_type][src_num + 1] == NULL || current_block->reg_used[reg_type][src_num + 1] == &kRes) {
			current_block->reg_idle[reg_type + 8].remove(src_num + 1);
			current_block->reg_available[reg_type + 8].push_back(src_num + 1);
		}
	}
	if (fix_num % 2) {
		if (current_block->reg_used[reg_type][fix_num - 1] == NULL || current_block->reg_used[reg_type][fix_num - 1] == &kRes) {
			current_block->reg_available[reg_type + 8].remove(fix_num);
			current_block->reg_idle[reg_type + 8].push_front(fix_num);
		}
	}
	else {
		if (current_block->reg_used[reg_type][fix_num + 1] == NULL || current_block->reg_used[reg_type][fix_num + 1] == &kRes) {
			current_block->reg_available[reg_type + 8].remove(fix_num + 1);
			current_block->reg_idle[reg_type + 8].push_front(fix_num + 1);
		}
	}
}


void ExchangeForFixVar(const int& reg_type, const int& src_num, const int& fix_num) {
	string* src_var, * fix_var;
	
	src_var = current_block->reg_used[reg_type][src_num];
	fix_var = current_block->reg_used[reg_type][fix_num];
	current_block->reg_used[reg_type][src_num] = fix_var;
	current_block->reg_used[reg_type][fix_num] = src_var;
	current_block->var_storage[*src_var].reg_num = fix_num;
	current_block->var_storage[*fix_var].reg_num = src_num;
	current_block->reg_idle[reg_type].remove(src_num);
	current_block->reg_idle[reg_type].remove(fix_num);
	current_block->reg_idle[reg_type].push_front(src_num);
	current_block->reg_idle[reg_type].push_front(fix_num);
}


void PushBackFixVar(const int& reg_type, const int& src_num, const int& fix_num) {
	int buf_type, buf_num;
	string note, buf_var;
	string* occupy_var = current_block->reg_used[reg_type][fix_num];

	if (!spill_flag) {
		RegSpillInit();
		spill_flag = true;
	}
	if (current_block->soft_pipeline) {
		PrintError("Soft pipeline fail due to registers overflow, please improve the code or give up the soft pipeline option.\n");
		Error();
	}
	// the register is not occupied or occupied by variable that is not fixed
	if (occupy_var == NULL || !current_block->fix_var.count(*occupy_var) || current_block->fix_var[*occupy_var].reg_num == -2) {
		if (occupy_var) ReleaseRegister(reg_type, fix_num);
		note = ";restore \"" + *current_block->reg_used[reg_type][src_num] + "\" back to " + reg_name[reg_type][fix_num] + " and release " + reg_name[reg_type][src_num];
		// R/COND_R
		if (reg_type == 0 || reg_type == 6) {
			AddNewInstruction("SMOV", reg_name[reg_type][src_num], reg_name[reg_type][fix_num], note);
		}
		// VR/COND_VR
		else if (reg_type == 1 || reg_type == 7) {
			AddNewInstruction("VMOV", reg_name[reg_type][src_num], reg_name[reg_type][fix_num], note);
		}
		// AR/OR
		else {
			if (core == 64) buf_type = 0;
			else if (core == 32 || core == 34) buf_type = 8;
			if (!current_block->reg_available[buf_type].size()) {
				SelectIdleRegister(buf_type);
				ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
			}
			buf_num = current_block->reg_available[buf_type].back();
			GetReserveRegister(buf_type);
			OccupyRegister(buf_num);
			ReleaseRegister(buf_type, buf_num, 0);
			if (core == 64) {
				note = ";transfer \"" + *current_block->reg_used[reg_type][src_num] + "\" in buf and release " + reg_name[reg_type][src_num];
				AddNewInstruction("SMVAAG", reg_name[reg_type][src_num], reg_name[buf_type][buf_num], note);
				note = ";restore \"" + *current_block->reg_used[reg_type][src_num] + "\" back to " + reg_name[reg_type][fix_num] + " and release " + reg_name[buf_type][buf_num];
				AddNewInstruction("SMVAGA", reg_name[buf_type][buf_num], reg_name[reg_type][fix_num], note);
			}
			else if (core == 32 || core == 34) {
				note = ";transfer \"" + *current_block->reg_used[reg_type][src_num] + "\"\'s low word in buf";
				AddNewInstruction("SMVAAGL", reg_name[reg_type][src_num], reg_name[0][buf_num - 1], note);
				note = ";transfer \"" + *current_block->reg_used[reg_type][src_num] + "\"\'s high word in buf and release " + reg_name[reg_type][src_num];
				AddNewInstruction("SMVAAGH", reg_name[reg_type][src_num], reg_name[0][buf_num], note);
				note = ";restore \"" + *current_block->reg_used[reg_type][src_num] + "\" back to " + reg_name[reg_type][fix_num] + " and release " + reg_name[buf_type][buf_num];
				AddNewInstruction("SMVAGA36", reg_name[buf_type][buf_num], reg_name[reg_type][fix_num], note);
			}
		}
		ReleaseForFixVar(reg_type, src_num, fix_num);
		return;
	}
	// the reigster is occupied by fix variables
	note = ";restore \"" + *current_block->reg_used[reg_type][src_num] + "\" back to " + reg_name[reg_type][fix_num] + " and release " + reg_name[reg_type][src_num];
	// R/COND_R
	if (reg_type == 0 || reg_type == 6) {
		if (!current_block->reg_available[0].size() && !current_block->reg_available[6].size()) {
			buf_type = 0;
			SelectIdleRegister(buf_type);
			buf_num = current_block->reg_idle[buf_type].back();
			buf_var = *current_block->reg_used[buf_type][buf_num];
			if (!current_block->fix_var.count(buf_var)) ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
			else buf_num = -1;
		}
		else if (!current_block->reg_available[0].size()) {
			buf_type = 0;
			buf_num = current_block->reg_available[buf_type].back();
		}
		else {
			buf_type = 6;
			buf_num = current_block->reg_available[buf_type].back();
		}
		if (buf_num == -1) {
			ReleaseRegister(reg_type, fix_num);
			note = ";restore \"" + *current_block->reg_used[reg_type][src_num] + "\" back to " + reg_name[reg_type][fix_num];
			AddNewInstruction("SMOV", reg_name[reg_type][src_num], reg_name[reg_type][fix_num], note, 0);
			ReleaseForFixVar(reg_type, src_num, fix_num);
			return;
		}
		note = ";save \"" + *current_block->reg_used[reg_type][src_num] + "\" in buf";
		AddNewInstruction("SMOV", reg_name[reg_type][src_num], reg_name[buf_type][buf_num], note);
		note = ";exchange \"" + *current_block->reg_used[reg_type][src_num] + "\" and \"" + *current_block->reg_used[reg_type][fix_num] + "\"";
		AddNewInstruction("SMOV", reg_name[reg_type][fix_num], reg_name[reg_type][src_num], note);
		AddNewInstruction("SMOV", reg_name[buf_type][buf_num], reg_name[reg_type][fix_num], note);
		ExchangeForFixVar(reg_type, src_num, fix_num);
		return;
	}
	// VR/COND_VR
	if (reg_type == 1 || reg_type == 7) {
		if (!current_block->reg_available[1].size() && !current_block->reg_available[7].size()) {
			buf_type = 1;
			SelectIdleRegister(buf_type);
			buf_num = current_block->reg_idle[buf_type].back();
			buf_var = *current_block->reg_used[buf_type][buf_num];
			if (!current_block->fix_var.count(buf_var)) ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
			else buf_num = -1;
		}
		else if (!current_block->reg_available[1].size()) {
			buf_type = 1;
			buf_num = current_block->reg_available[buf_type].back();
		}
		else {
			buf_type = 7;
			buf_num = current_block->reg_available[buf_type].back();
		}
		if (buf_num == -1) {
			ReleaseRegister(reg_type, fix_num);
			note = ";restore \"" + *current_block->reg_used[reg_type][src_num] + "\" back to " + reg_name[reg_type][fix_num];
			AddNewInstruction("VMOV", reg_name[reg_type][src_num], reg_name[reg_type][fix_num], note, 0);
			ReleaseForFixVar(reg_type, src_num, fix_num);
			return;
		}
		note = ";save \"" + *current_block->reg_used[reg_type][src_num] + "\" in buf";
		AddNewInstruction("VMOV", reg_name[reg_type][src_num], reg_name[buf_type][buf_num], note);
		note = ";exchange \"" + *current_block->reg_used[reg_type][src_num] + "\" and \"" + *current_block->reg_used[reg_type][fix_num] + "\"";
		AddNewInstruction("VMOV", reg_name[reg_type][fix_num], reg_name[reg_type][src_num], note);
		AddNewInstruction("VMOV", reg_name[buf_type][buf_num], reg_name[reg_type][fix_num], note);
		ExchangeForFixVar(reg_type, src_num, fix_num);
		return;
	}
	// AR/OR
	if (core == 64) buf_type = 0;
	else if (core == 32 || core == 34) buf_type = 8;
	if (!current_block->reg_available[buf_type].size()) {
		SelectIdleRegister(buf_type);
		ReleaseRegister(buf_type, current_block->reg_idle[buf_type].back());
	}
	buf_num = current_block->reg_available[buf_type].back();
	GetReserveRegister(buf_type);
	OccupyRegister(buf_num);
	ReleaseRegister(buf_type, buf_num, 0);
	if (core == 64) {
		note = ";save \"" + *current_block->reg_used[reg_type][src_num] + "\" in buf";
		AddNewInstruction("SMVAAG", reg_name[reg_type][src_num], reg_name[buf_type][buf_num], note, 0);
		note = ";save \"" + *current_block->reg_used[reg_type][fix_num] + "\" in buf";
		AddNewInstruction("SMVAAG", reg_name[reg_type][src_num], reg_name[buf_type][buf_num], note, 0);
		note = ";exchange \"" + *current_block->reg_used[reg_type][src_num] + "\" and \"" + *current_block->reg_used[reg_type][fix_num] + "\"";
		AddNewInstruction("SMVAGA", reg_name[buf_type][buf_num], reg_name[reg_type][fix_num], note, 0);
		AddNewInstruction("SMVAGA", reg_name[buf_type][buf_num], reg_name[reg_type][src_num], note);
	}
	else if (core == 32 || core == 34) {
		note = ";save \"" + *current_block->reg_used[reg_type][src_num] + "\"\'s low word in buf";
		AddNewInstruction("SMVAAGL", reg_name[reg_type][src_num], reg_name[0][buf_num - 1], note, 0);
		note = ";save \"" + *current_block->reg_used[reg_type][src_num] + "\"\'s high word in buf";
		AddNewInstruction("SMVAAGH", reg_name[reg_type][src_num], reg_name[0][buf_num], note, 0);
		note = ";save \"" + *current_block->reg_used[reg_type][fix_num] + "\"\'s low word in buf";
		AddNewInstruction("SMVAAGL", reg_name[reg_type][fix_num], reg_name[0][buf_num - 1], note, 0);
		note = ";save \"" + *current_block->reg_used[reg_type][fix_num] + "\"\'s high word in buf";
		AddNewInstruction("SMVAAGH", reg_name[reg_type][fix_num], reg_name[0][buf_num], note, 0);
		note = ";exchange \"" + *current_block->reg_used[reg_type][src_num] + "\" and \"" + *current_block->reg_used[reg_type][fix_num] + "\"";
		AddNewInstruction("SMVAGA36", reg_name[buf_type][buf_num], reg_name[reg_type][fix_num], note, 0);
		AddNewInstruction("SMVAGA36", reg_name[buf_type][buf_num], reg_name[reg_type][src_num], note);
	}
	ExchangeForFixVar(reg_type, src_num, fix_num);
}


void RestoreFixVar() {
	vector<Instruction*> new_cycle(kFuncUnitAmount);
	int reg_type, reg_num, mem_address;
	int fix, fix_num, fix_address;
	int i;
	string cond_var = current_block->sbr.cond_field;
	string fix_var, restore_var = "";
	vector<list<string>> fix_var_list(8);

	// needn't restore fix var, complete sbr instruction
	if (!fix_destination.count(block_num)) {
		if (cond_var.empty()) return;
		reg_num = current_block->var_storage[cond_var].reg_num;
		fix = current_block->fix_var.count(cond_var);
		if (reg_num >= 0) {
			UpdateRegIdle(6, reg_num);
		}
		// -1: the value is empty; -2: the value is saved in memory
		else {
			if (reg_num == -2) mem_address = current_block->var_storage[cond_var].mem_address;
			if (fix) DealWithFixVar(cond_var);
			else if (!current_block->reg_available[6].size()) SelectIdleRegister(6);
			if (!current_block->reg_available[6].size()) ReleaseRegister(6, current_block->reg_idle[6].back());
			reg_num = current_block->reg_available[6].back();
			if (mem_address == -1) GetReserveRegister(6);
			else RestoreSpillRegister(6, reg_num, mem_address, cond_var);
			OccupyRegister(6, cond_var);
			for (int i = particular_instruction[1].cycle; i > 0; --i) {
				ins_output[block_num].push_back(new_cycle);
				ins_output[block_num].back()[4] = &particular_instruction[0];
			}
		}
		current_block->sbr.cond_field = reg_name[6][reg_num];
		if (!IsVarAlive(cond_var, 6)) AddReleaseReg(6, reg_num, fix);
		return;
	}
	// restore condiction variable in SBR
	if (!cond_var.empty() && current_block->var_storage[cond_var].reg_num < 0) {
		reg_num = current_block->var_storage[cond_var].reg_num;
		if (reg_num == -2) {
			if (current_block->soft_pipeline) {
				PrintError("Soft pipeline fail due to registers overflow, please improve the code or give up the soft pipeline option.\n");
				Error();
			}
			mem_address = current_block->var_storage[cond_var].mem_address;
		}
		if (!current_block->reg_available[6].size()) {
			if (current_block->soft_pipeline) {
				PrintError("Soft pipeline fail due to registers overflow, please improve the code or give up the soft pipeline option.\n");
				Error();
			}
			if (!current_block->fix_var.count(cond_var)) SelectIdleRegister(6);
			else DealWithFixVar(cond_var);
		}
		if (!current_block->reg_available[6].size()) ReleaseRegister(6, current_block->reg_idle[6].back());
		reg_num = current_block->reg_available[6].back();
		if (mem_address == -1) GetReserveRegister(6);
		else RestoreSpillRegister(6, reg_num, mem_address, cond_var);
		OccupyRegister(6, cond_var);
	}
	for (auto iter = current_block->fix_var.begin(); iter != current_block->fix_var.end(); ++iter) {
		reg_type = int(var_map[iter->first].reg_type);
		fix_var_list[reg_type].push_front(iter->first);
	}
	for (reg_type = 2; reg_type < 8; ++reg_type) {
		for (i = fix_var_list[reg_type].size(); i > 0; --i) {
			fix_var = fix_var_list[reg_type].front();
			fix_var_list[reg_type].pop_front();
			reg_num = current_block->var_storage[fix_var].reg_num;
			mem_address = current_block->var_storage[fix_var].mem_address;
			fix_num = current_block->fix_var[fix_var].reg_num;
			if (fix_num == -2) {
				if (fix_var == cond_var) continue;
				if (reg_num != -2) ReleaseRegister(reg_type, reg_num);
			}
			else if (reg_num != fix_num) {
				if (reg_num != -2) {
					PushBackFixVar(reg_type, reg_num, fix_num);
					continue;
				}
				if (current_block->reg_used[reg_type][fix_num]) {
					if (*current_block->reg_used[reg_type][fix_num] == cond_var) {
						restore_var = *current_block->reg_fix[reg_type][fix_num];
						continue;
					}
					ReleaseRegister(reg_type, fix_num);
				}
				RestoreSpillRegister(reg_type, fix_num, mem_address, fix_var);
				current_block->var_storage[fix_var].reg_num = fix_num;
				current_block->reg_available[reg_type].remove(fix_num);
				current_block->reg_used[reg_type][fix_num] = &var_map[fix_var].name;
				UpdateRegIdle(reg_type, fix_num);
			}
			if (debug) DebugCheckRegState();
		}
	}
	for (reg_type = 0; reg_type < 2; ++reg_type) {
		for (i = fix_var_list[reg_type].size(); i > 0; --i) {
			fix_var = fix_var_list[reg_type].front();
			fix_var_list[reg_type].pop_front();
			reg_num = current_block->var_storage[fix_var].reg_num;
			mem_address = current_block->var_storage[fix_var].mem_address;
			fix_num = current_block->fix_var[fix_var].reg_num;
			if (fix_num == -2) {
				if (reg_num != -2) ReleaseRegister(reg_type, reg_num);
			}
			else if (reg_num != fix_num) {
				if (reg_num != -2) {
					PushBackFixVar(reg_type, reg_num, fix_num);
					continue;
				}
				if (current_block->reg_used[reg_type][fix_num]) {
					ReleaseRegister(reg_type, fix_num);
				}
				RestoreSpillRegister(reg_type, fix_num, mem_address, fix_var);
				if (fix_num % 2) {
					if (current_block->reg_used[reg_type][fix_num - 1] == NULL || current_block->reg_used[reg_type][fix_num - 1] == &kRes) {
						current_block->reg_available[reg_type + 8].remove(fix_num);
						current_block->reg_idle[reg_type + 8].push_front(fix_num);
					}
				}
				else {
					if (current_block->reg_used[reg_type][fix_num + 1] == NULL || current_block->reg_used[reg_type][fix_num + 1] == &kRes) {
						current_block->reg_available[reg_type + 8].remove(fix_num + 1);
						current_block->reg_idle[reg_type + 8].push_front(fix_num + 1);
					}
				}
				current_block->var_storage[fix_var].reg_num = fix_num;
				current_block->reg_available[reg_type].remove(fix_num);
				current_block->reg_used[reg_type][fix_num] = &var_map[fix_var].name;
				UpdateRegIdle(reg_type, fix_num);
			}
			if (debug) DebugCheckRegState();
		}
	}
	if (!cond_var.empty()) {
		reg_type = 6;
		reg_num = current_block->var_storage[cond_var].reg_num;
		if (current_block->fix_var.count(cond_var) && current_block->fix_var[cond_var].reg_num == -2 || !restore_var.empty()) {
			current_block->sbr.cond_field = reg_name[reg_type][reg_num];
			ReleaseRegister(reg_type, reg_num);
			if (!restore_var.empty()) {
				RestoreSpillRegister(reg_type, reg_num, current_block->var_storage[restore_var].mem_address, restore_var);
				OccupyRegister(reg_type, restore_var);
			}
		}
		else {
			current_block->sbr.cond_field = reg_name[reg_type][reg_num];
		}
	}
}


void PrepareAndRestore() {
	vector<vector<Instruction*>> block_buf;
	vector<Instruction*> new_cycle(kFuncUnitAmount);
	int end_block = block_label.size() - 1;
	int reg_type, reg_num, mem_address;
	int type_list[8] = { 2, 3, 4, 5, 6, 0, 7, 1 };
	int snop_num = 0;
	int unit;
	string var;

	// initial first and last block's labels
	block_label[0] = linear_program.block_pointer_list[0]->block_ins[0].input[0];
	block_label[1] = kPrepare;
	block_label[end_block] = kRestore;

	block_num = end_block;
	current_block = linear_program.block_pointer_list[block_num];
	// move return variables to return registers
	if (linear_program.var_out.size() == 1) {
		var = linear_program.var_out[0];
		reg_num = linear_program.block_pointer_list[end_block]->var_storage[var].reg_num;
		if (reg_num != reg_out) {
			AddNewInstruction("SMOV", reg_name[0][reg_num], reg_name[0][reg_out], ";transfer \"" + var + "\" to " + reg_name[0][reg_out]);
		}
	}
	else if (linear_program.var_out.size() == 1) {
		var = linear_program.var_out[0];
		reg_num = linear_program.block_pointer_list[end_block]->var_storage[var].reg_num;
		if (reg_num != reg_out + 1) {
			AddNewInstruction("SMOV", reg_name[0][reg_num], reg_name[0][reg_out + 1], ";transfer \"" + var + "\" to " + reg_name[0][reg_out + 1]);
		}
		var = linear_program.var_out[1];
		reg_num = linear_program.block_pointer_list[end_block]->var_storage[var].reg_num;
		if (reg_num != reg_out) {
			AddNewInstruction("SMOV", reg_name[0][reg_num], reg_name[0][reg_out], ";transfer \"" + var + "\" to " + reg_name[0][reg_out]);
		}
	}
	// save and restore reserve registers
	block_buf.swap(ins_output[1]);
	for (int type_index = 0; type_index < 8; ++type_index) {
		reg_type = type_list[type_index];
		for (int i = 0; i < reserve_register_used[reg_type].size(); ++i) {
			reg_num = reserve_register_used[reg_type][i];
			if (reg_type == 0 || reg_type == 6) mem_address = scalar_save++;
			else if (reg_type == 1 || reg_type == 7) mem_address = vector_save++;
			else {
				mem_address = scalar_save;
				scalar_save += 2;
			}
			block_num = 1;
			current_block = linear_program.block_pointer_list[block_num];
			SaveSpillRegister(reg_type, reg_num, mem_address);
			block_num = end_block;
			current_block = linear_program.block_pointer_list[block_num];
			RestoreSpillRegister(reg_type, reg_num, mem_address, kRes);
			if (type_index < 4) reg_buf.pop_back();
		}
	}
	ins_output[1].insert(ins_output[1].end(), block_buf.begin(), block_buf.end());
	// complete last block snop
	if (ins_output.back().size()) {
		for (int i = 0; i < kFuncUnitAmount; ++i) {
			if (ins_output.back().back()[i]) {
				if (ins_output.back().back()[i]->cycle > snop_num) snop_num = ins_output.back().back()[i]->cycle;
			}
		}
	}
	for (int i = 1; i < snop_num; ++i) {
		ins_output.back().push_back(new_cycle);
		ins_output.back().back()[4] = &particular_instruction[0];
	}
	block_buf.swap(ins_output[1]);
	vector<vector<Instruction*>>().swap(ins_output[1]);
	if (scalar_save > 0) {
		AddStackInstruction(1, 4);
		AddStackInstruction(end_block, 8);
		particular_instruction[4].input[0] += to_string(scalar_save * 8);
		particular_instruction[8].input[0] += to_string(scalar_save * 8);
	}
	if (vector_save > 0) {
		AddStackInstruction(1, 6);
		AddStackInstruction(end_block, 10);
		particular_instruction[6].input[0] += to_string(vector_save * 8);
		particular_instruction[10].input[0] += to_string(vector_save * 8);
	}
	if (high_word_init) {
		ins_output[1].push_back(new_cycle);
		unit = FunctionUnitSplit(particular_instruction[3].func_unit)[0];
		ins_output[1].back()[unit] = &particular_instruction[3];
	}
	block_buf.insert(block_buf.begin(), ins_output[1].begin(), ins_output[1].end());
	block_buf.swap(ins_output[1]);
	// add final sbr
	block_num = end_block;
	current_block = linear_program.block_pointer_list[block_num];
	current_block->sbr = particular_instruction[1];
	AddSbr();
}


void AssignRegister() {
	debug = true;
	PrintInfo("Assignment of registers starts...\n");

	// init
	ins_output.clear();
	block_label.clear();

	for (int i = 0; i < 10; i++) {
		reg_name[i].clear();
	}

	scalar_expansion_flag = 0, vector_expansion_flag = 0;
	high_word_init = 0;
	reg_buf.clear();
	offset_buf = -1;
	scalar_save = 0, vector_save = 0;
	scalar_stack_OR = -1, vector_stack_OR = -1;
	for (int i = 0; i < 8; i++) {
		reserve_register_used[i].clear();
	}

	interfering_var.clear();
	release_reg.clear();

	spill_flag = false;
	spill_warnning = false;
	debug = false;
	// end init


	if (core == 64) kImmOffset = 1024;
	else if (core == 32 || core == 34) kImmOffset = 256;
	ins_output.resize(linear_program.block_pointer_list.size());
	block_label.resize(ins_output.size());
	InitRegInfo();

	Instruction* current_ins;

	for (block_num = 2; block_num < linear_program.block_pointer_list.size(); ++block_num) {
		current_block = linear_program.block_pointer_list[block_num];
		InitBlockState();
		
		for (cycle_num = 0; cycle_num < current_block->queue_length; ++cycle_num) {
			spill_flag = false;
			vector<Instruction*> new_cycle(kFuncUnitAmount);
			interfering_var.pop_front();
			if (interfering_var.size() == 0) interfering_var.push_back(vector<string>());
			// skip SBR
			/*if (current_block->ins_schedule_queue[cycle_num][4]) {
				if (!current_block->soft_pipeline && !strcmp(current_block->ins_schedule_queue[cycle_num][4]->ins_name, "SBR")) {
					current_block->ins_schedule_queue[cycle_num][4] = NULL;
				}
			}*/
			// add registers released in last cycle  ---------modified by DC
			UpdateRegAvailable();

			vector<vector<pair<int, int>>>(kRegType).swap(release_reg);
			int new_var[kFuncUnitAmount], repeat_type[kFuncUnitAmount];
			// get interfering variables
			for (unit_num = 0; unit_num < kFuncUnitAmount; ++unit_num) {
				new_var[unit_num] = 0;
				repeat_type[unit_num] = 0;
				current_ins = current_block->ins_schedule_queue[cycle_num][unit_num];
				if (current_ins == NULL) continue;
				if (!strcmp(current_ins->ins_name, "SNOP")) {
					repeat_type[unit_num] = 3;
				}
				else if (cycle_num == 0) {
					if (current_ins == current_block->ins_schedule_queue[cycle_num + 1][unit_num]) repeat_type[unit_num] = 1;
				}
				else if (cycle_num == current_block->queue_length - 1) {
					if (current_ins == current_block->ins_schedule_queue[cycle_num - 1][unit_num]) repeat_type[unit_num] = 3;
				}
				else {
					if (current_ins == current_block->ins_schedule_queue[cycle_num + 1][unit_num] && current_ins == current_block->ins_schedule_queue[cycle_num - 1][unit_num]) repeat_type[unit_num] = 2;
					else if (current_ins == current_block->ins_schedule_queue[cycle_num + 1][unit_num]) repeat_type[unit_num] = 1;
					else if (current_ins == current_block->ins_schedule_queue[cycle_num - 1][unit_num]) repeat_type[unit_num] = 3;
				}
				if (!current_ins->cond_field.empty()) GetInterferingVar(current_ins->cond_field);
				if (!current_ins->input[0].empty()) GetInterferingVar(current_ins->input[0]);
				if (!current_ins->input[1].empty()) GetInterferingVar(current_ins->input[1]);
				if (!current_ins->input[2].empty()) GetInterferingVar(current_ins->input[2]);
				if (!current_ins->output.empty() && repeat_type[unit_num] < 2) {
					new_var[unit_num] = GetInterferingVar(current_ins->output, current_ins->cycle);
				}
				if (repeat_type[unit_num] < 2) new_cycle[unit_num] = current_ins;
			}
			// convert variables that are used to read or have already assigned
			for (unit_num = 0; unit_num < kFuncUnitAmount; ++unit_num) {
				current_ins = current_block->ins_schedule_queue[cycle_num][unit_num];
				if (current_ins == NULL) continue;
				/*if (current_ins->input[0] == "0xB0C0") {
					printf("1");
				}*/
				if (!current_ins->cond_field.empty()) ConvertVarToReg(current_ins->cond_field, repeat_type[unit_num]);
				if (!current_ins->input[0].empty()) ConvertVarToReg(current_ins->input[0], repeat_type[unit_num]);
				if (!current_ins->input[1].empty()) ConvertVarToReg(current_ins->input[1], repeat_type[unit_num]);
				if (!current_ins->input[2].empty()) ConvertVarToReg(current_ins->input[2], repeat_type[unit_num]);
				if (!current_ins->output.empty() && !new_var[unit_num]) ConvertVarToReg(current_ins->output, repeat_type[unit_num]);
				if (debug) DebugCheckRegState();
			}
			
			// convert variable that is to be assigned a new value
			for (unit_num = 0; unit_num < kFuncUnitAmount; ++unit_num) {
				current_ins = current_block->ins_schedule_queue[cycle_num][unit_num];
				if (current_ins == NULL) continue;
				if (new_var[unit_num]) ConvertVarToReg(current_ins->output, repeat_type[unit_num]);
				if (debug) DebugCheckRegState();
			}
			// add snop
			for (unit_num = 0; unit_num < kFuncUnitAmount; ++unit_num) {
				if (new_cycle[unit_num] != NULL) break;
			}
			if (unit_num == kFuncUnitAmount) new_cycle[4] = &particular_instruction[0];
			ins_output[block_num].push_back(new_cycle);
			ReleaseRegThisCycle();

			if (cycle_num == current_block->queue_length - 1) {
				UpdateRegAvailable();
			}
		}
		cycle_num--;
		RestoreFixVar();
		AddSbr();
	}

	PrepareAndRestore();
	PrintInfo("Assignment of registers done.\n\n\n");
}