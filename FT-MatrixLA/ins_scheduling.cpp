//------------------- Instruction Scheduling--------------------
// Including -o0, -o1 scheduling and transition file printing

#include <cstdio>
#include "data_structure.h"
#include "ins_scheduling.h"
#include "utils.h"
#include <algorithm>
#include "ins_rescheduling.h"

using namespace std;

// Add a instruction of SNOP 1 for register allocation
Instruction InstrAddSnop() {
	Instruction snop;
	snop.type = InsType::ReferIns;
	strcpy(snop.ins_name, "SNOP");
	strcpy(snop.func_unit, "SBR");
	snop.input[0] = "1";
	snop.cycle = snop.read_cycle = snop.write_cycle = 1;
	return snop;
}

// Find the proper function unit and launch cycle in ins_scheduling_queue
pair<int, int> FindFit(Block* block, vector<int> func_unit, int start, int cycle, int read_cycle, int write_cycle) {
	//make sure start >= 0
	if (start < 0) start = 0;

	int best_fit = 0x3f3f3f3f, no_func;
	int i, j, k;
	bool flag;

	for (i = func_unit.size()-1; i >=0; i--) {
		for (j = start; j < max_num_of_instr - LONGEST_INSTR_DELAY; j++) {
			flag = 1;
			for (k = j; k < j + read_cycle; k++) {
				if (block->ins_schedule_queue[k][func_unit[i]] != NULL) {
					flag = 0;
					break;
				}
			}
			if (flag == 0) { j = k; continue; }
			for (k = j + cycle - write_cycle; k < j + cycle; k++) {
				if (block->ins_schedule_queue_write[k][func_unit[i]] != NULL) {
					flag = 0;
					break;
				}
			}
			if (flag == 0) { j += read_cycle - 1; continue; }
			else if (j < best_fit) {
				no_func = func_unit[i];
				best_fit = j;
				break;
			}
		}
	}
	if (best_fit == 0x3f3f3f3f) {
		PrintError("Schedule_queue is full, fail to find a fit.\ntry modifying max_num_of_instr in \'schedule.h\' to a larger size\n");
		Error();
	}
	return make_pair(no_func, best_fit);
}

// Find the proper launch cycle of a pair of instructions in ins_scheduling_queue
int FindPairFit(Block* block, vector<int> func_unit, int start, int cycle, int read_cycle, int write_cycle) {
	//make sure start >= 0
	if (start < 0) start = 0;

	int best_fit = 0x3f3f3f3f, &no_func1 = func_unit[0], &no_func2 = func_unit[1];

	for (int i = start, j = 0; i < max_num_of_instr - LONGEST_INSTR_DELAY; ++i) {
		bool flag = 1;
		for (j = i; j < i + read_cycle; ++j) {
			if (block->ins_schedule_queue[j][no_func1] != NULL || block->ins_schedule_queue[j][no_func2] != NULL) {
				flag = 0;
				break;
			}
		}
		if (flag == 0) {
			i = j;
			continue;
		}
		for (j = i + cycle - write_cycle; j < i + cycle; ++j) {
			if (block->ins_schedule_queue[j][no_func1] != NULL || block->ins_schedule_queue[j][no_func2] != NULL) {
				flag = 0;
				break;
			}
		}
		if (flag == 0) {
			i = read_cycle - 1;
			continue;
		}
		else {
			best_fit = i;
			break;
		}
	}
	if (best_fit == 0x3f3f3f3f) {
		PrintError("Schedule_queue is full, fail to find a fit.\ntry modifying max_num_of_instr in \'schedule.h\' to a larger size\n");
		Error();
	}
	return best_fit;
}

// Insert the instruction to the proper place
static void Place(Block* block, Instruction* x, int no_func, int time_pos) {
	Instruction* child; int edge_len;

	//place this instruction into schedule_queue
	for (int i = time_pos; i < time_pos + x->read_cycle; i++)
		block->ins_schedule_queue[i][no_func] = x;
	for (int i = time_pos + (x->cycle) - (x->write_cycle); i < time_pos + x->cycle; i++)
		block->ins_schedule_queue_write[i][no_func] = x;

	if (time_pos + (x->cycle) > block->queue_length) block->queue_length = time_pos + (x->cycle);

	//fresh children's information
	for (int i = 0; i < (x->chl).size(); i++) {
		child = (x->chl[i]).first;
		edge_len = (x->chl[i]).second;// be careful: edge_len != x->cycle !!!!
		if (child->last_fa_end_time < time_pos + edge_len) child->last_fa_end_time = time_pos + edge_len;
		(child->indeg)--;
		if (child->indeg == 0) block->zero_indeg_instr.push(child);
	}
}

/*
// unplace the instruction for multiple read/write cycle, avoiding for output repeatly 
void Unplace(Block* block, Instruction* x, int no_func, int time_pos) {
	for (int i = time_pos; i < time_pos + x->read_cycle; i++)
		block->ins_schedule_queue[i][no_func] = NULL;
	for (int i = time_pos + (x->cycle) - (x->write_cycle); i < time_pos + x->cycle; i++)
		block->ins_schedule_queue_write[i][no_func] = NULL;
}
*/

// Add MAC unit to the ins_name, .M1/.M2...
void ReallocMAC(Instruction* x, int no_func) {
	int pos = FindChar(x->ins_name, '.');
	if (pos != -1) {
		x->ins_name[pos] = '\0';
	}
	if ((function_units_index.count("SIEU") && no_func == function_units_index["SIEU"]) || 
		(function_units_index.count("VIEU") && no_func == function_units_index["VIEU"]) ||
		(function_units_index.count("VMAC0") && no_func == function_units_index["VMAC0"]))
		return ;
	int tail = strlen(x->ins_name);
	x->ins_name[tail++] = '.';
	x->ins_name[tail++] = 'M';
	int smac_value = no_func - function_units_index["SMAC1"]+1;
	int vmac_value = no_func - function_units_index["VMAC1"]+1;
	if (vmac_value < 0) x->ins_name[tail++] = '0' + smac_value;
	else x->ins_name[tail++] = '0' + vmac_value;
	x->ins_name[tail] = '\0';
}

// Select the instructions whose indge = 0 and push them into zero_indeg_instr
void LoadZeroIndegInstr(Block* block) {
	// clear queue
	while (block->zero_indeg_instr.size()) block->zero_indeg_instr.pop();

	for (int i = 0; i < block->block_ins_num; i++) {
		if (block->ins_pointer_list[i]->indeg == 0) {
			block->zero_indeg_instr.push(block->ins_pointer_list[i]);
		}
	}

	/*for (int i = block->block_ins_num - 1; i >= 0; i--) {
		if (block->ins_pointer_list[i]->state == 0) {
			for (int j = 0; j < block->ins_pointer_list[i]->chl.size(); j++) {
				block->ins_pointer_list[i]->chl[j].first->indeg--;
			}
			continue;
		}
		if (block->ins_pointer_list[i]->indeg == 0) {
			block->zero_indeg_instr.push(block->ins_pointer_list[i]);
		}
	}*/

	
	//if (reschedule == "CLOSE") {
	//	for (int i = 0; i < block->block_ins_num; i++) {
	//		if (block->ins_pointer_list[i]->indeg == 0) {
	//			block->zero_indeg_instr.push(block->ins_pointer_list[i]);
	//		}
	//	}
	//}
	//

	////	skip virtual instructions and delete the edge to real instructions
	////	modified by DC
	//if (reschedule == "OPEN") {
	//	for (int i = block->block_ins_num - 1; i >= 0; i--) {
	//		if (block->ins_pointer_list[i]->state == 0) {
	//			for (int j = 0; j < block->ins_pointer_list[i]->chl.size(); j++) {
	//				block->ins_pointer_list[i]->chl[j].first->indeg--;
	//			}
	//			continue;
	//		}
	//		if (block->ins_pointer_list[i]->indeg == 0) {
	//			block->zero_indeg_instr.push(block->ins_pointer_list[i]);
	//		}
	//	}
	//}
	
}

// Instruction Scheduling for each block
void InsScheduling(const char* opt_option) {
	PrintInfo("Instruction Scheduling starts...\n");
	int opt = opt_option[2] - '0';
	for (int i = 0; i < linear_program.total_block_num; i++) {
		if (linear_program.block_pointer_list[i]->type != BlockType::Instruction || linear_program.block_pointer_list[i]->soft_pipeline != 0) continue;
			if (opt) {
				O1SchedulingDP(linear_program.block_pointer_list[i]);	//modifed by DC
			}
			else {
				O0Scheduling(linear_program.block_pointer_list[i]);
			}
	}
	PrintInfo("Instruction Scheduling is successfully finished...\n\n");
}

int StatusCompute(Instruction* x)		// added by DC
{
	//Instruction* tmp;
	if (x->state != 0) return x->state;
	//printf("%s\n", x->ins_string);
	int tmp = 0;
	int maxstate = -1;
	if (x->chl.size() <= 0) return x->cycle;
	for (int i = 0; i < x->chl.size(); i++)
	{
		if (x == x->chl[i].first) continue;
		tmp = StatusCompute((x->chl[i]).first) + (x->chl[i]).second;
		if (maxstate < tmp)
			maxstate = tmp;
	}
	if (maxstate < x->cycle)
		maxstate = x->cycle;
	x->state = maxstate;
	return x->state;
}

void StatusValue(Block* block)			// added by DC
{
	Instruction* x;
	int tmp = 0;
	for (int i = 0; i < block->block_ins_num; i++)
	{
		if (block->ins_pointer_list[i]->type != InsType::ReferIns) continue; 
		x = block->ins_pointer_list[i];
		x->state = StatusCompute(x);
		if (x->state > tmp)
			tmp = x->state;
	}
}

// Scheduling with option -o1
void O1Scheduling(Block* block) {
	Instruction* x, * sbr = NULL;
	Instruction* swait = NULL, * cr = nullptr;
	int pos;
	pair<int, int> pr;
	set<Instruction*> ready_partner_ins;
	block->queue_length = 0;
	block->ins_schedule_queue.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
	block->ins_schedule_queue_write.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));

	StatusValue(block);
	LoadZeroIndegInstr(block);

	while (block->zero_indeg_instr.size()) {
		x = block->zero_indeg_instr.top();
		block->zero_indeg_instr.pop();


		// if this instruction is SNOP
		if (!strcmp(x->ins_name, "SNOP"))
			continue;
		if (!strcmp(x->ins_name, "SBR")) {
			sbr = x;
			continue;
		}
		if (!strcmp(x->ins_name, "SWAIT")) {
			swait = x;
			continue;
		}
		// FIXME: yang jing
		if (IsCtrlInstrction(x)) {
			cr = x;
			continue;
		}

		Instruction* partner = x->partner;
		vector<int> function_unit_vector;

		function_unit_vector = FunctionUnitSplit(string(x->func_unit));
		/*if (!strcmp(x->instr_name, "SMVAGA36")) {
			int i = 0, ii = 0;
			char cnum[10];
			int inum = 0;
			while (isalpha(x->output[i])) i++;
			for (ii = 0; isdigit(x->output[i]); i++, ii++)
				cnum[ii] = x->output[i];
			cnum[ii] = '\0';
			inum = atoi(cnum);
			if (inum < 8) pr = make_pair(0, 0);
		}*/

		if (partner) {
			if (ready_partner_ins.find(partner) == ready_partner_ins.end()) {
				ready_partner_ins.insert(x);
				for (int i = 0; i < (x->chl).size(); i++) {
					Instruction* child = (x->chl[i]).first;
					if (child == partner) {
						(child->indeg)--;
						if (child->indeg == 0) block->zero_indeg_instr.push(child);
					}
				}
				continue;
			}
			ready_partner_ins.erase(partner);
			int last_start = x->last_fa_end_time;
			if (partner->last_fa_end_time > last_start) last_start = partner->last_fa_end_time;
			int fit = FindPairFit(block, function_unit_vector, last_start, x->cycle, x->read_cycle, x->write_cycle);
			Place(block, x, function_unit_vector[0], fit);
			Place(block, partner, function_unit_vector[1], fit);
			continue;
		}

		if (function_unit_vector.size() != 0) {
			pr = FindFit(block, function_unit_vector, x->last_fa_end_time, x->cycle, x->read_cycle, x->write_cycle);
			if (strstr(x->func_unit, "MAC") != NULL) ReallocMAC(x, pr.first);
			//if (!strncmp(x->func_unit, "M", 1) || !strncmp(x->func_unit, "SMAC", 4)) ReallocMAC(x, pr.first);
			Place(block, x, pr.first, pr.second);
		}
		else
			Place(block, x, 0, 0);
	}

	vector<int> function_unit_vector;
	if (sbr != NULL) {
		function_unit_vector = FunctionUnitSplit(string(sbr->func_unit));
		pr = FindFit(block, function_unit_vector, max(sbr->last_fa_end_time, block->queue_length - (sbr->cycle)), sbr->cycle, sbr->read_cycle, sbr->write_cycle);
		Place(block, sbr, pr.first, pr.second);
		block->sbr = *sbr;
	}
	if (swait != NULL) {
		function_unit_vector = FunctionUnitSplit(string(swait->func_unit));
		pr = FindFit(block, function_unit_vector, max(swait->last_fa_end_time, block->queue_length), swait->cycle, swait->read_cycle, swait->write_cycle);
		Place(block, swait, pr.first, pr.second);
	}
	// FIXME: yang jing
	if (cr != nullptr) {
		function_unit_vector = FunctionUnitSplit(string(cr->func_unit));
		pr = FindFit(block, function_unit_vector, max(cr->last_fa_end_time, block->queue_length - (cr->cycle)), cr->cycle, cr->read_cycle, cr->write_cycle);
		Place(block, cr, pr.first, pr.second);
	}
}

//modifed by DC
// Scheduling with DP
void O1SchedulingDP(Block* block) {
	Instruction* x, * sbr = NULL;
	Instruction* swait = NULL, *cr = nullptr;
	pair<int, int> pr;
	set<Instruction*> ready_partner_ins;
	block->queue_length = 0;
	block->ins_schedule_queue.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
	block->ins_schedule_queue_write.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
	
	int ins_amount = 0;
	for (int i = 0; i < block->ins_pointer_list.size(); i++) {
		Instruction* ins = block->ins_pointer_list[i];
		if (ins->type != InsType::ReferIns) {
			ins_amount = i;
			break;
		}
	}

	//reverse(block->ins_pointer_list.begin(), block->ins_pointer_list.begin() + ins_amount);

	// broundrary condition: if ins has no child
	for (int ins_index = ins_amount - 1; ins_index >= 0; ins_index--) {
		Instruction* this_ins = block->ins_pointer_list[ins_index];
		if (!strcmp(this_ins->ins_name, "SNOP"))
			continue;
		if (!strcmp(this_ins->ins_name, "SWAIT")) {
			continue;
		}
		//if (this_ins == NULL)	continue;
		//if (this_ins->type != InsType::ReferIns)	continue;
		vector<int> func_available = FunctionUnitSplit(string(this_ins->func_unit));
		if (func_available.size() == 0)	continue;


		if (this_ins->chl.size() == 0) {
			for (int time_pos = 0; time_pos < time_pos + this_ins->cycle; time_pos++) {
				bool flag = 0;
				for (int func_no = func_available.size() - 1; func_no >= 0; func_no--) {
					bool flag1 = 0;
					int func = func_available[func_no];
					for (int i = time_pos; i < time_pos + this_ins->write_cycle; i++) {
						if (block->ins_schedule_queue_write[i][func] != NULL)	flag1 = 1;
					}
					bool flag2 = 0;
					for (int i = time_pos + this_ins->cycle - this_ins->read_cycle; i < time_pos + this_ins->cycle; i++) {
						if (block->ins_schedule_queue[i][func] != NULL)	flag2 = 1;
					}
					if (flag1 == 1 || flag2 == 1) {
						continue;
					}
					else {
						for (int i = time_pos; i < time_pos + this_ins->write_cycle; i++) {
							block->ins_schedule_queue_write[i][func] = this_ins;
						}
						for (int i = time_pos + this_ins->cycle - this_ins->read_cycle; i < time_pos + this_ins->cycle; i++) {
							block->ins_schedule_queue[i][func] = this_ins;
						}
						this_ins->func_occupied = func;
						this_ins->dp = time_pos + this_ins->cycle - 1;
						flag = 1;
						if (time_pos + this_ins->cycle > block->queue_length)	block->queue_length = time_pos + this_ins->cycle;
						break;
					}
				}
				if (flag == 1)	break;
			}
		}

		if (strstr(this_ins->func_unit, "MAC") != NULL) ReallocMAC(this_ins, this_ins->func_occupied);
	}

	//dynamic programming for instructions with no child
	for (int ins_index = ins_amount - 1; ins_index >= 0; ins_index--) {
		Instruction* this_ins = block->ins_pointer_list[ins_index];
		if (!strcmp(this_ins->ins_name, "SNOP"))
			continue;
		if (!strcmp(this_ins->ins_name, "SWAIT")) {
			continue;
		}
		//if (this_ins == NULL)	continue;
		//if (this_ins->type != InsType::ReferIns)	continue;
		if (this_ins->chl.size() == 0)	continue;
		vector<int> func_available = FunctionUnitSplit(string(this_ins->func_unit));
		if (func_available.size() == 0)	continue;

		int cycle_pos = -1;	// max dp for chla
		int func_pos = -1;
		vector<pair<int, int>> ins_pos;	// pair<cycle, function>
		for (int chl_index = 0; chl_index < this_ins->chl.size(); chl_index++) {
			Instruction* this_chl = this_ins->chl[chl_index].first;
			vector<pair<int, int>> tmp_ins_pos;
			
			for (int time_pos = this_chl->dp + this_ins->chl[chl_index].second; time_pos < time_pos + this_ins->cycle; time_pos++) {
				if (time_pos - this_ins->cycle + 1 < 0)	continue;
				bool flag = 0;
				for (int func_no = func_available.size() - 1; func_no >= 0; func_no--) {
					int func = func_available[func_no];
					bool flag1 = 0;
					for (int i = time_pos - this_ins->cycle + 1; i < time_pos - this_ins->cycle + 1 + this_ins->write_cycle; i++) {
						if (block->ins_schedule_queue_write[i][func] != NULL)	flag1 = 1;
					}
					bool flag2 = 0;
					for (int i = time_pos; i > time_pos - this_ins->read_cycle; i--) {
						if (block->ins_schedule_queue[i][func] != NULL)	flag2 = 1;
					}
					if (flag1 == 1 || flag2 == 1) {
						continue;
					}
					else {
						tmp_ins_pos.push_back(make_pair(time_pos, func));
						flag = 1;
						break;
					}
				}
				if (flag == 1)	break;
			}

			// find the min dp of func
			int min_dp = 10000;
			int tmp_func_no = -1;
			for (int i = 0; i < tmp_ins_pos.size(); i++) {
				if (tmp_ins_pos[i].first < min_dp) {
					min_dp = tmp_ins_pos[i].first;
					tmp_func_no = tmp_ins_pos[i].second;
				}
			}
			ins_pos.push_back(make_pair(min_dp, tmp_func_no));
		}

		// find max dp for chl
		for (int i = 0; i < ins_pos.size(); i++) {
			if (ins_pos[i].first > cycle_pos) {
				cycle_pos = ins_pos[i].first;
				func_pos = ins_pos[i].second;
				this_ins->dp = cycle_pos;
				this_ins->func_occupied = func_pos;
				if (cycle_pos + this_ins->cycle > block->queue_length)	block->queue_length = cycle_pos + 1;
			}
		}

		// place this instruction
		for (int cycle = cycle_pos - this_ins->cycle + 1; cycle < cycle_pos - this_ins->cycle + 1 + this_ins->write_cycle; cycle++) {
			block->ins_schedule_queue_write[cycle][func_pos] = this_ins;
		}
		for (int cycle = cycle_pos; cycle > cycle_pos - this_ins->read_cycle; cycle--) {
			block->ins_schedule_queue[cycle][func_pos] = this_ins;
		}
		
		if (strstr(this_ins->func_unit, "MAC") != NULL) ReallocMAC(this_ins, func_pos);
	}

	//reversal the cycle domain for ins_schedule_queue
	reverse(block->ins_schedule_queue.begin(), block->ins_schedule_queue.begin() + block->queue_length);
	//reverse(block->ins_schedule_queue_write.begin(), block->ins_schedule_queue_write.begin() + block->queue_length);
	//reverse(block->ins_pointer_list.begin(), block->ins_pointer_list.begin() + ins_amount);
}

// Scheduling with option -o0
void O0Scheduling(Block* block) {
	int current_pos = 0;
	Instruction* x = NULL, *cr = nullptr;
	pair<int, int> pr;
	set<Instruction*> ready_partner_ins;
	block->queue_length = 0;
	block->ins_schedule_queue.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
	block->ins_schedule_queue_write.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
	for (int i = 0; i < block->block_ins_num; i++) {
		x = block->ins_pointer_list[i];
		x->last_fa_end_time = current_pos;
		if (!strcmp(x->ins_name, "SBR")) {
			block->sbr = *x;
		}
		Instruction* partner = x->partner;
		vector<int> function_unit_vector;
		function_unit_vector = FunctionUnitSplit(string(x->func_unit));
		if (partner) {
			if (ready_partner_ins.find(partner) == ready_partner_ins.end()) {
				ready_partner_ins.insert(x);
				for (int i = 0; i < (x->chl).size(); i++) {
					Instruction* child = (x->chl[i]).first;
					if (child == partner) {
						(child->indeg)--;
						if (child->indeg == 0) block->zero_indeg_instr.push(child);
					}
				}
				continue;
			}
			ready_partner_ins.erase(partner);
			int last_start = x->last_fa_end_time;
			if (partner->last_fa_end_time > last_start) last_start = partner->last_fa_end_time;
			int fit = FindPairFit(block, function_unit_vector, last_start, x->cycle, x->read_cycle, x->write_cycle);
			Place(block, x, function_unit_vector[0], fit);
			Place(block, partner, function_unit_vector[1], fit);
			current_pos = fit + x->cycle;
			continue;
		}
		if (function_unit_vector.size() != 0){
			pr = FindFit(block, function_unit_vector, x->last_fa_end_time, x->cycle, x->read_cycle, x->write_cycle);
			//if (!strncmp(x->func_unit, "M", 1) || !strncmp(x->func_unit, "SMAC", 4)) ReallocMAC(x, pr.first);
			if (strstr(x->func_unit, "MAC") != NULL) ReallocMAC(x, pr.first);
			Place(block, x, pr.first, pr.second);
			current_pos = pr.second + x->cycle;
		}
		else 
			Place(block, x, 0, 0);
	}
}

// Print the scheduling results into transition file
void PrintSchedulingTransistion(const char* transition_file) {
	FILE* fp = NULL;
	fp = fopen(transition_file, "w");
	if (fp == NULL) {
		PrintError("The transition file \"%s\" can\'t open.\n", transition_file);
		Error();
	}



	for (int i = 0; i < linear_program.total_block_num; i++) {
		//if (linear_program.block_pointer_list[i]->type == BlockType::LoopStart) {
		//	if (linear_program.block_pointer_list[i]->ins_pointer_list[0]->input[1] != "") {	//modified by DC 23/3/12
		//		int unrolling_factor = atoi(linear_program.block_pointer_list[i]->ins_pointer_list[0]->input[1].c_str());
		//		fprintf(fp, "In block %d, the loop is unrolled with a factor of %d...\n", i + 1, unrolling_factor);	
		//	}
		//}
		if (linear_program.block_pointer_list[i]->type == BlockType::Instruction) {
			fprintf(fp, "; cycle(%d)\n", linear_program.block_pointer_list[i]->queue_length);
		}
		
		if (linear_program.block_pointer_list[i]->type != BlockType::Instruction) {
			for (int j = 0; j < linear_program.block_pointer_list[i]->block_ins_num; j++) {
				fprintf(fp, "%s\n", linear_program.block_pointer_list[i]->ins_pointer_list[j]->ins_string);
			}
			continue;
		}

		Instruction nop = InstrAddSnop();
		linear_program.block_pointer_list[i]->block_ins[linear_program.block_pointer_list[i]->block_ins_num] = nop;
		(linear_program.block_pointer_list[i]->ins_pointer_list).push_back(&linear_program.block_pointer_list[i]->block_ins[linear_program.block_pointer_list[i]->block_ins_num]);
		linear_program.block_pointer_list[i]->block_ins_num += 1;

		Instruction* x;
		bool SNOP, first_instr;
		int snop = 0;

		if (linear_program.block_pointer_list[i]->label != "") {
			fprintf(fp, "%s\n", linear_program.block_pointer_list[i]->label.c_str());
		}
		for (int j = 0; j < linear_program.block_pointer_list[i]->queue_length; j++) {
			SNOP = 1;
			first_instr = 1;
			for (int k = 0; k < function_units_index.size(); k++) {
				if (linear_program.block_pointer_list[i]->ins_schedule_queue[j][k] == NULL || (j > 0 && linear_program.block_pointer_list[i]->ins_schedule_queue[j][k] == linear_program.block_pointer_list[i]->ins_schedule_queue[j-1][k])) continue;

				SNOP = 0;
				x = linear_program.block_pointer_list[i]->ins_schedule_queue[j][k];
				//unplace(linear_program.block_pointer_list[i], x, k, j);

				if (first_instr) {
					first_instr = 0;
					if (snop) {
						fprintf(fp, "\t\t\tSNOP\t\t%d\n", snop);
						snop = 0;
					}
				}
				else fputc('|', fp);

				if (x->cond_field[0] != '\0') {
					if (x->cond_flag) fprintf(fp, "[!%s]\t\t%s", x->cond_field.c_str(), x->ins_name);
					else fprintf(fp, "[%s]\t\t%s", x->cond_field.c_str(), x->ins_name);
				}
				else
					fprintf(fp, "\t\t\t%s", x->ins_name);

				//			if (!strcmp(x->func_unit, "M1") || !strcmp(x->func_unit, "SMAC1")) fprintf(fp, ".%s", "M1");

				if (x->input[0][0] != '\0')
				{
					fprintf(fp, "\t%s", x->input[0].c_str());
				}
				if (x->input[1][0] != '\0')
				{
					fprintf(fp, ", %s", x->input[1].c_str());
				}
				if (x->input[2][0] != '\0')
				{
					fprintf(fp, ", %s", x->input[2].c_str());
				}
				if (x->output[0] != '\0')
				{
					fprintf(fp, ", %s", x->output.c_str());
				}
				if (x->note != "") {
					fprintf(fp, "\t\t\t\t\t %s", x->note.c_str());
				}

				fputc('\n', fp);
			}
			if (SNOP) {
				snop++;
				linear_program.block_pointer_list[i]->ins_schedule_queue[j][function_units_index["SBR"]] = &linear_program.block_pointer_list[i]->block_ins[linear_program.block_pointer_list[i]->block_ins_num - 1];
			}
		}
		if (snop > 0) {
			fprintf(fp, "\t\t\tSNOP\t\t%d\n", snop);
		}
	}
	fclose(fp);
	PrintInfo("Transition file \"%s\" is successfully generated. \n\n", transition_file);
}
