//------------------- Software Pipeline--------------------
// Software Pipeline for the most inner loop

#include "data_structure.h"
#include "analyser.h"
#include "utils.h"
#include "build_dependency.h"
#include <string>
#include "software_pipeline.h"
#include "ins_scheduling.h"
#include<iostream>

map<string, int> flag;//
map<string, int> flag_f;
map<string, vector<coor>> tl_map;
map<string, int> ntc;
map < string, vector<pair<int, int>>> double_wr;
int flag_pd = 0;


vector<string> SeperateVar(const string& operand, const int& general, const int& address, const int& ar) {
	vector<string> var_list;
	string var = "";
	int i = 0;
	if (operand.empty()) return var_list;
	if (operand[0] == '.') return var_list;
	if (operand[0] >= '0' && operand[0] <= '9' || operand[0] == '-') return var_list;
	if (operand[0] == '*') {
		i = 1;
		if (address) {
			while (operand[i] == '+' || operand[i] == '-') ++i;
			while (operand[i] != '\0' && operand[i] != '[' && operand[i] != '+' && operand[i] != '-') var += operand[i++];
			var_list.push_back(var);
			if (operand[i] != '\0') {
				while (operand[i] == '[' || operand[i] == '+' || operand[i] == '-') ++i;
				if (operand[i] != '\0' && (operand[i] < '0' || operand[i] > '9') && operand[i] != '-') {
					var = "";
					while (operand[i] != '\0' && operand[i] != ']') var += operand[i++];
					var_list.push_back(var);
				}
			}
		}
		else if (ar) {
			if (operand[i] == '+' || operand[i] == '-') {
				if (operand[i] == operand[i + 1]) {
					i += 2;
					while (operand[i] != '\0' && operand[i] != '[') var += operand[i++];
					var_list.push_back(var);
				}
			}
			else {
				while (operand[i] != '\0' && operand[i] != '[' && operand[i] != '+' && operand[i] != '-') var += operand[i++];
				if (operand[i] == '+' || operand[i] == '-') {
					if (operand[i] == operand[i + 1]) var_list.push_back(var);
				}
			}
		}
	}
	else if (general) {
		while (i < operand.size()) {
			if (operand[i] == ':') {
				var_list.push_back(var);
				var = "";
				i++;
			}
			else var += operand[i++];
		}
		var_list.push_back(var);
	}
	return var_list;
}


vector<string> GetReadVar(const Instruction* ins) {
	vector<string> var_list, tmp;
	if (!ins->cond_field.empty()) var_list.push_back(ins->cond_field);
	tmp = SeperateVar(ins->input[0], 1, 1, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	tmp = SeperateVar(ins->input[1], 1, 1, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	tmp = SeperateVar(ins->input[2], 1, 1, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	tmp = SeperateVar(ins->output, 0, 1, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	return var_list;
}

vector<string> GetWriteVar(const Instruction* ins) {
	vector<string> var_list, tmp;
	tmp = SeperateVar(ins->input[0], 0, 0, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	tmp = SeperateVar(ins->input[1], 0, 0, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	tmp = SeperateVar(ins->input[2], 0, 0, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	tmp = SeperateVar(ins->output, 1, 0, 0);
	var_list.insert(var_list.end(), tmp.begin(), tmp.end());
	return var_list;
}

// Insert the block into the block pointer list
void InsertBlock(int pos) {
	linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num - 1]));
	for (int i = linear_program.total_block_num - 1; i > pos; i--) {
		linear_program.block_pointer_list[i] = linear_program.block_pointer_list[i - 1];
	}
	linear_program.block_pointer_list[pos] = &(linear_program.program_block[linear_program.total_block_num - 1]);
}

// Get the Init Start Interval
int GetInitT0(Block* block) {
	return 1;
	/*
		int T0 = 0, tmp_T;
		Instruction* x;
		unordered_map<string, int> func_cnt;

		for (int i = 0; i < block->block_ins_num; i++) {
			x = block->ins_pointer_list[i];
			func_cnt[x->func_unit] += x->read_cycle;
		}
		pair<int, int> tmp;
		for (unordered_map<string, pair<int, int> >::iterator it = FUNC_NO.begin(); it != FUNC_NO.end(); it++) {
			tmp = it->second;
			tmp_T = (func_cnt[it->first] + tmp.second - tmp.first) / (tmp.second - tmp.first + 1);
			if (tmp_T > T0 && it->first != "") T0 = tmp_T;
		}

		return T0;
	*/
}

// Check the cross-loop restraint
// true: fit_pos ok
bool ExLoopRestraint(Instruction* x, int fit_pos, int T) {
	Instruction* fa;
	pair<int, int> pr;
	for (int i = 0; i < x->ex_loop.size(); i++) {
		fa = x->ex_loop[i].first;
		if (fa->site == -1) continue;
		pr = x->ex_loop[i].second;
		if (fa->site + pr.first * T - fit_pos < pr.second)
			return false;
	}
	return true;
}

// Instruction Unplacing in Module Scheduling 
void MSUnplace(Block* block, Instruction* x, int no_func, int time_pos, int T) {
	for (int i = time_pos; i < time_pos + x->read_cycle; i++) {
		block->module_queue_r[i % T][no_func] = NULL;
		block->LT_queue_r[i][no_func] = NULL;
	}
	for (int i = time_pos + (x->cycle) - (x->write_cycle); i < time_pos + x->cycle; i++) {
		block->module_queue_w[i % T][no_func] = NULL;
		block->LT_queue_w[i][no_func] = NULL;
	}
}

// Module scheduling failure, restore the instruction state
void MSFailure(Block* block, int T, int LT) {
	Instruction* x;
	for (int i = 0; i < kFuncUnitAmount; i++) {
		for (int j = 0; j <= LT; j++) {
			x = block->LT_queue_r[j][i];
			if (x != NULL) {
				//Unplace(block, x, i, j);
				MSUnplace(block, x, i, j, T);
			}
		}
	}

	// necessary: virtual instruction can't be tracked in schedule_queue
	for (int i = 0; i < block->block_ins_num; i++) {
		x = block->ins_pointer_list[i];
		if (x->site != -1) {
			for (int k = 0; k < x->chl.size(); k++)
				(x->chl[k].first)->indeg++;
		}
		x->site = -1;
		x->last_fa_end_time = 0;
	}
}
/*
bool MSPossible(Block* block, Instruction* x, int no_func, int T) {
	int cycle = x->cycle, r_cycle = x->read_cycle, w_cycle = x->write_cycle;
	bool flag;
	int i, j;
	for (i = 0; i < T; i++) {
		flag = 1;
		for (j = i; j < i + r_cycle; j++) {
			if (block->ins_schedule_queue[j][no_func] != NULL) {
				flag = 0;
				break;
			}
		}
		if (flag == 0) { i = j; continue; }
		for (j = i + cycle - w_cycle; j < i + cycle; j++) {
			if (block->ins_schedule_queue_write[j][no_func] != NULL) {
				flag = 0;
				break;
			}
		}
		if (flag == 0) { i += r_cycle - 1; continue; }
		return true;
	}
	return false;
}*/

// Find the proper position for the instruction in module scheduling
int MSFindFit(Block* block, Instruction* x, int no_func, int begin, int T) {
	int fit_pos = -1;
	int cycle = x->cycle, r_cycle = x->read_cycle, w_cycle = x->write_cycle;
	int flag1;
	int i = begin, j;

	//	for (i = begin; i < T; i++) {
	flag1 = 1;
	if (i + r_cycle > T) return -1;
	for (j = i; j < i + r_cycle; j++) {
		if (block->module_queue_r[j % T][no_func] != NULL) {
			flag1 = 0;
			break;
		}
	}
	if (flag1 == 0) { i = j; return -1; }
	if ((i + cycle - w_cycle) % T + w_cycle > T) return -1;
	for (j = i + cycle - w_cycle; j < i + cycle; j++) {
		if (block->module_queue_w[j % T][no_func] != NULL) {
			flag1 = 0;
			break;
		}
	}
	if (flag1 == 0) { i += r_cycle - 1; return -1; }

	//vector<string> x_varname;
	//x_varname = GetReadVar(x);

	//for (int ii = 0; ii < x_varname.size(); ii++){
	//	for (j = 1; j < T; j++) {
	//		int index = begin - j;
	//		if (index < 0) index += T;
	//		int tmp_num = 0;
	//		for (int k = 0; k < kFuncUnitAmount; k++) {
	//			if (block->module_queue_w[index][k] != NULL) {
	//				
	//				vector<string> y_varname;
	//				int weight = -1;
	//				y_varname = GetWriteVar(block->module_queue_w[index][k]);
	//				vector<string>::iterator result = find(y_varname.begin(), y_varname.end(), x_varname[ii]); 
	//				if (result == y_varname.end()) continue;
	//				if (index < T-1 && block->module_queue_w[index][k] == block->module_queue_w[index + 1][k]) { flag = 0; break; }
	//				Instruction* y = block->module_queue_w[index][k];
	//				bool tag = false;
	//				for (int m = 0; m < (y->chl).size(); m++) {
	//					if (x == y->chl[m].first) { tag = true;  weight = y->chl[m].second;  break; }
	//				}
	//				if (tag == false) { flag = 0; break; }
	//				int y_index = -1;
	//				for (int m = 0; m < T; m++) {
	//					if (block->module_queue_r[m][k] == y) {
	//						y_index = m; 
	//						break;
	//					}
	//				}
	//				int y_pos, x_pos;
	//				if (y_index - y->last_fa_end_time % T < 0) y_pos = y->last_fa_end_time + y_index - y->last_fa_end_time % T + T;
	//				else y_pos = y->last_fa_end_time + y_index - y->last_fa_end_time % T;
	//				if (begin - x->last_fa_end_time % T < 0) x_pos = x->last_fa_end_time + begin - x->last_fa_end_time % T + T;
	//				else x_pos = x->last_fa_end_time + begin - x->last_fa_end_time % T;
	//				if (x_pos - y_pos - weight >= T) { flag = 0; break; }
	//				tmp_num++;
	//			}
	//		}
	//		if (tmp_num > 1) flag = 0;
	//		if (flag == 0) break;
	//		if (tmp_num == 1) break;
	//	}
	//	if (flag == 0) break;
	//}
	//if (flag == 0) { return -1; }

	//x_varname = GetWriteVar(x);
	//begin = (begin + x->cycle - 1) % T;
	////if (begin >= T) begin -= T;

	//for (int ii = 0; ii < x_varname.size(); ii++) {
	//	for (j = 0; j < T; j++) {
	//		int index = begin - j;
	//		if (index < 0) index += T;
	//		int tmp_num = 0;
	//		for (int k = 0; k < kFuncUnitAmount; k++) {
	//			if (block->module_queue_w[index][k] != NULL) {

	//				vector<string> y_varname;
	//				int weight = -1;
	//				y_varname = GetWriteVar(block->module_queue_w[index][k]);
	//				vector<string>::iterator result = find(y_varname.begin(), y_varname.end(), x_varname[ii]);
	//				if (result == y_varname.end()) continue;
	//				if (j < x->write_cycle) { flag = 0; break;}
	//				Instruction* y = block->module_queue_w[index][k];
	//				bool tag = false;
	//				for (int m = 0; m < (y->chl).size(); m++) {
	//					if (x == y->chl[m].first) { tag = true;  weight = y->chl[m].second;  break; }
	//				}
	//				if (tag == false) { flag = 0; break; }
	//				/*int y_index = -1;
	//				for (int m = 0; m < T; m++) {
	//					if (block->module_queue_r[m][k] == y) {
	//						y_index = m;
	//						break;
	//					}
	//				}
	//				int y_pos, x_pos;
	//				if (y_index - y->last_fa_end_time % T < 0) y_pos = y->last_fa_end_time + y_index - y->last_fa_end_time % T + T;
	//				else y_pos = y->last_fa_end_time + y_index - y->last_fa_end_time % T;
	//				if (begin - x->last_fa_end_time % T < 0) x_pos = x->last_fa_end_time + begin - x->last_fa_end_time % T + T;
	//				else x_pos = x->last_fa_end_time + begin - x->last_fa_end_time % T;
	//				if (x_pos - y_pos - weight >= T) { flag = 0; break; }*/
	//				tmp_num++;
	//			}
	//		}
	//		if (tmp_num > 1) flag = 0;
	//		if (flag == 0) break;
	//		if (tmp_num == 1) break;
	//	}
	//	if (flag == 0) break;
	//}
	//if (flag == 0) { return -1; }

	fit_pos = i;

	//	}

	return fit_pos;
}

// Find the proper position in the single loop for the instruction
int LTFindFit(Block* block, Instruction* x, int no_func, int begin, int T) {
	//begin += (x->last_fa_end_time - begin + T - 1) / T * T;
	if (begin - x->last_fa_end_time % T < 0) begin = x->last_fa_end_time + begin - x->last_fa_end_time % T + T;
	else begin = x->last_fa_end_time + begin - x->last_fa_end_time % T;

	int fit_pos = -1;
	int cycle = x->cycle, r_cycle = x->read_cycle, w_cycle = x->write_cycle;
	bool flag;
	int i = begin, j;

	//for (i = begin; i < max_num_of_instr - LONGEST_INSTR_DELAY; i += T) {
	flag = 1;
	for (j = i; j < i + r_cycle; j++) {
		if (block->LT_queue_r[j][no_func] != NULL) {
			flag = 0;
			break;
		}
	}
	//if (flag == 0) { continue; }
	if (flag == 0) return -1;
	for (j = i + cycle - w_cycle; j < i + cycle; j++) {
		if (block->LT_queue_w[j][no_func] != NULL) {
			flag = 0;
			break;
		}
	}
	//if (flag == 0) { continue; }
	if (flag == 0) return -1;

	//if (ExLoopRestraint(x, i, T)) {
	//	fit_pos = i;
	//}
	//break;
//}
	fit_pos = i;
	return fit_pos;
}

// Instruction Placing in Module Scheduling 
void MSPlace(Block* block, Instruction* x, int no_func, int fit_pos, int T, int flag) {

	//place this instruction into schedule_queue
	for (int i = fit_pos; i < fit_pos + x->read_cycle; i++) {
		block->LT_queue_r[i][no_func] = x;
		block->module_queue_r[i % T][no_func] = x;
	}
	for (int i = fit_pos + (x->cycle) - (x->write_cycle); i < fit_pos + x->cycle; i++) {
		block->LT_queue_w[i][no_func] = x;
		block->module_queue_w[i % T][no_func] = x;
	}

	if (flag) {
		Instruction* child; int edge_len;
		for (int j = 0; j < x->chl.size(); j++) {
			child = x->chl[j].first;
			edge_len = x->chl[j].second;
			if (fit_pos + edge_len > child->last_fa_end_time)
				child->last_fa_end_time = fit_pos + edge_len;
			child->indeg--;
			if (child->indeg == 0)
				block->soft_instr_queue.push(child);
		}
	}
}

// Prolog Layout in Software Pipeline
void SoftPipeProlog(Block* block) {
	Instruction* x;
	int LT = 0;
	block->sbr = *new Instruction;

	block->module_queue_r.clear();
	block->module_queue_r.resize(block->soft_pipe_cycle, vector<Instruction*>(function_units_index.size()));


	// period rounding
	//LT = (block->LT_queue_length + block->soft_pipe_cycle - 1) / block->soft_pipe_cycle * block->soft_pipe_cycle;

	// output prolog and steady state
	block->queue_length = block->LT_queue_length - block->soft_pipe_cycle;

	for (int j = 0; j < block->queue_length; j++) {
		for (int i = 0; i < kFuncUnitAmount; i++) {
			x = block->LT_queue_r[j][i];
			if (x != NULL) {
				block->module_queue_r[j % block->soft_pipe_cycle][i] = x;

				if (j == 0 || x != block->LT_queue_r[j - 1][i]) {
					//CopyInstruction(&(block->block_ins[block->block_ins_num]),x);
					if (block->block_ins_num >= kMaxInsNumPerBlock) {
						PrintError("The basic block contains too many instructions (max %d)\n", kMaxInsNumPerBlock);
						Error();
					}

					block->block_ins[block->block_ins_num] = *x;
					block->ins_pointer_list.push_back(&(block->block_ins[block->block_ins_num]));
					for (int k = j; k < j + x->read_cycle; k++) {
						block->ins_schedule_queue[k][i] = &(block->block_ins[block->block_ins_num]);
					}
					block->block_ins_num++;
				}
				//block->ins_schedule_queue[j][i] = x;
			}
			else {
				x = block->module_queue_r[j % block->soft_pipe_cycle][i];
				if (x == NULL) continue;


				//CopyInstruction(&(block->block_ins[block->block_ins_num]),x);
				if (j % block->soft_pipe_cycle == 0 || x != block->module_queue_r[j % block->soft_pipe_cycle - 1][i]) {
					if (block->block_ins_num >= kMaxInsNumPerBlock) {
						PrintError("The basic block contains too many instructions (max %d)\n", kMaxInsNumPerBlock);
						Error();
					}
					block->block_ins[block->block_ins_num] = *x;
					block->ins_pointer_list.push_back(&(block->block_ins[block->block_ins_num]));
					for (int k = j; k < j + x->read_cycle; k++) {
						block->ins_schedule_queue[k][i] = &(block->block_ins[block->block_ins_num]);
					}
					block->block_ins_num++;
				}
			}

		}
	}
}

// Epilog Layout in Software Pipeline
void SoftPipeEpilog(Block* block) {
	block->sbr = *new Instruction;
	Instruction* x;
	for (int j = 0; j < block->soft_pipe_cycle; j++) {
		int new_instr = 0;
		for (int i = 0; i < kFuncUnitAmount; i++) {
			x = block->LT_queue_r[j][i];
			if (x != NULL) block->module_queue_r[j][i] = NULL;
		}
	}
	block->queue_length = block->LT_queue_length - block->soft_pipe_cycle;
	// output epilog
	int k = 0;
	int max_write_cycle = 0;
	for (int j = block->soft_pipe_cycle; j < block->LT_queue_length; j++) {
		int new_instr = 0;
		for (int i = 0; i < kFuncUnitAmount; i++) {
			x = block->module_queue_r[j % block->soft_pipe_cycle][i];
			if (x == NULL || !strcmp(x->func_unit, "SBR")) continue;


			if (j % block->soft_pipe_cycle == 0 || x != block->module_queue_r[j % block->soft_pipe_cycle - 1][i]) {

				if (block->block_ins_num >= kMaxInsNumPerBlock) {
					PrintError("The basic block contains too many instructions (max %d)\n", kMaxInsNumPerBlock);
					Error();
				}
				//CopyInstruction(&(block->block_ins[block->block_ins_num]), x);
				block->block_ins[block->block_ins_num] = *x;
				block->ins_pointer_list.push_back(&(block->block_ins[block->block_ins_num]));
				for (int l = k; l < k + x->read_cycle; l++) {
					block->ins_schedule_queue[l][i] = &(block->block_ins[block->block_ins_num]);
				}
				block->block_ins_num++;
				if (x->cycle + k - 1 > max_write_cycle) max_write_cycle = x->cycle + k - 1;
			}
			if (block->LT_queue_r[j][i] != NULL) {
				for (int l = 0; l < x->read_cycle; l++) {
					block->module_queue_r[j % block->soft_pipe_cycle + l][i] = NULL;
				}
			}
		}
		k++;
	}
	if (max_write_cycle + 1 < block->queue_length) block->queue_length = max_write_cycle + 1;
}

// Steady State Layout in Software Pipeline 
void SoftPipeSteady(Block* block) {
	Instruction* x;

	block->queue_length = block->soft_pipe_cycle;

	for (int j = 0; j < block->queue_length; j++) {
		for (int i = 0; i < kFuncUnitAmount; i++) {

			block->ins_schedule_queue[j][i] = block->module_queue_r[j][i];
		}
	}
}

// Output the Instruction Layout in the single loop
void OutputLT(Block* block, int LT) {
	Instruction* x;
	bool SNOP, first_instr;
	int snop = 0;
	PrintDebug("Software Pipeline Output: Instruction Scheduling in a single loop\n");

	printf("LT:%d\tT:%d\n", LT, block->soft_pipe_cycle);
	for (int j = 0; j < LT; j++) {
		SNOP = 1;
		first_instr = 1;
		for (int i = 0; i < kFuncUnitAmount; i++) {
			x = block->LT_queue_r[j][i];
			if (x == NULL || (j > 0 && x == block->LT_queue_r[j - 1][i])) continue;

			SNOP = 0;

			if (first_instr) {
				first_instr = 0;
				if (snop) {
					printf("\t\tSNOP\t\t%d\n", snop);
					snop = 0;
				}
			}
			else printf("|");

			printf("\t%d", i);

			if (x->cond_field.length())
				printf("\t[%s]\t%s", x->cond_field.c_str(), x->ins_name);
			else
				printf("\t\t%s", x->ins_name);
			if (!strcmp(x->func_unit, "M1")) printf(".%s", x->func_unit);

			if (x->input[0].length())
			{
				printf("\t%s", x->input[0].c_str());
			}
			if (x->input[1].length())
			{
				printf(", %s", x->input[1].c_str());
			}
			if (x->input[2].length())
			{
				printf(", %s", x->input[2].c_str());
			}
			if (x->output.length())
			{
				printf(", %s", x->output.c_str());
			}

			printf("\n");
		}
		if (SNOP) snop++;
	}
	if (snop > 0) printf("\t\tSNOP\t\t%d\n", snop);
	PrintDebug("Software Pipeline Output: Instruction Scheduling in a single loop\n\n");
}

// Software Pipeline for the most inner loop
void SoftwarePipeline() {

	flag.clear();
	flag_f.clear();
	tl_map.clear();
	ntc.clear();
	double_wr.clear();
	flag_pd = 0;


	for (int i = 1; i < linear_program.block_pointer_list.size(); i++) {
		if (linear_program.block_pointer_list[i]->type == BlockType::LoopEnd) {

			bool most_inner_loop = false;
			bool pipeline = false;
			// verify if this is the .endloop of a most inner loop
			for (int j = i - 1; j >= 0; j--) {
				if (linear_program.block_pointer_list[j]->type == BlockType::LoopStart) {
					if (linear_program.block_pointer_list[j]->loop_depth == linear_program.block_pointer_list[i]->loop_depth) {
						most_inner_loop = true;
						if (linear_program.block_pointer_list[j]->ins_pointer_list[0]->output == "PIPELINE") {
							// only one block in the inner loop
							if (j != i - 2) {
								PrintWarn("Software Pipeline only allows that the most inner loop has one block!\n\n");
							}
							else pipeline = true;
						}
						break;
					}
				}
				else if (linear_program.block_pointer_list[j]->type == BlockType::LoopEnd) {
					break;
				}
			}

			// pipeline the most inner loop
			if (most_inner_loop == true && pipeline == true) {
				string loop_label = linear_program.block_pointer_list[i - 2]->ins_pointer_list[0]->input[0];
				if (loop_label[loop_label.length() - 1] == ':') loop_label = loop_label.substr(0, loop_label.length() - 1);

				PrintInfo("Software Pipeline starts...\n");
				//BuildExLoopDependency(linear_program.block_pointer_list[i - 1]);
				ModuleSchedule(linear_program.block_pointer_list[i - 1]);// , linear_program.block_pointer_list[i - 2]->ins_pointer_list[0]->input[0], transition_file);

				string trip = linear_program.block_pointer_list[i - 2]->ins_pointer_list[0]->input[2];
				int trip_num = -1;
				if (trip != "") trip_num = atoi(trip.c_str());

				int min_loop_num = ceil(float(linear_program.block_pointer_list[i - 1]->LT_queue_length) / float(linear_program.block_pointer_list[i - 1]->soft_pipe_cycle));
				if (trip_num != -1 && min_loop_num > trip_num) {
					PrintWarn("Cannot find a suitable scheduling plan of Software Pipeline...The minimum number of loops for software pipeline is higher than .trip...\n\n");
				}
				else {
					if (linear_program.block_pointer_list[i - 1]->soft_pipe_cycle == -1) {
						PrintWarn("Cannot find a suitable scheduling plan of Software Pipeline...\n\n");
					}
					else {
						linear_program.block_pointer_list[i - 1]->soft_pipeline = 1;

						PrintInfo("The minimum number of loops for software pipeline is %d\n", min_loop_num);
						//current_block_prolog = *linear_program.block_pointer_list[i - 1];
						//current_block_prolog.loop_depth--;
						//current_block_prolog.label = loop_label + "_prolog";
						linear_program.program_block[linear_program.total_block_num] = *linear_program.block_pointer_list[i - 1];
						linear_program.program_block[linear_program.total_block_num].loop_depth--;
						linear_program.program_block[linear_program.total_block_num].label = loop_label + "_prolog";
						linear_program.total_block_num++;
						InsertBlock(i - 2);
						SoftPipeProlog(&linear_program.program_block[linear_program.total_block_num - 1]);

						//current_block_epilog = *linear_program.block_pointer_list[i];
						//current_block_epilog.loop_depth--;
						//current_block_epilog.label = loop_label + "_epilog";
						linear_program.program_block[linear_program.total_block_num] = *linear_program.block_pointer_list[i];
						linear_program.program_block[linear_program.total_block_num].loop_depth--;
						linear_program.program_block[linear_program.total_block_num].label = loop_label + "_epilog";
						linear_program.total_block_num++;
						InsertBlock(i + 2);
						SoftPipeEpilog(&linear_program.program_block[linear_program.total_block_num - 1]);

						SoftPipeSteady(linear_program.block_pointer_list[i]);


						PrintInfo("Software Pipeline is successfully finished...\n\n");
						i = i + 2;
					}
				}
			}
		}
	}
}

// Build the cross-loop dependency 
void BuildExLoopDependency(Block* block) {
	Instruction* root, * leaf, * lleaf;
	VarList* tmp_vl;
	string varname;


	for (map<string, Instruction*>::iterator it = block->reg_root.begin(); it != block->reg_root.end(); it++) {
		root = it->second;
		varname = it->first;
		tmp_vl = SplitVar(varname);
		// register type
		if (varname[0] != '*') {
			leaf = block->reg_written[varname];
			if (leaf != NULL && leaf != root) {
				//leaf->ex_loop.push_back(make_pair(root, make_pair(1, leaf->cycle - root->cycle + root->write_cycle)));
				for (int j = 0; j < leaf->chl.size(); j++) {
					lleaf = leaf->chl[j].first;
					lleaf->ex_loop.push_back(make_pair(root, make_pair(1, lleaf->read_cycle + root->write_cycle - root->cycle)));
				}
			}
		}
		// addr type

		if (tmp_vl->var.type == VarType::Address) {
			// leaf is where AR lastly change
			leaf = block->reg_written[varname];
			int offset_per_iteration = block->reg_offset[varname];
			int leaf_offset, offset_pos, inum, len = varname.length();

			for (int j = 0; j < leaf->chl.size(); j++) {
				lleaf = leaf->chl[j].first;
				offset_pos = lleaf->input[0].find(varname);
				if (offset_pos != lleaf->input[0].npos) {
					if (offset_pos + len >= lleaf->input[0].length()) {
						inum = 0;
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
					else if (lleaf->input[0][offset_pos + len] == '[') {
						inum = stoll(lleaf->input[0].substr(offset_pos + len));
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
				}
				offset_pos = lleaf->input[1].find(varname);
				if (offset_pos != lleaf->input[1].npos) {
					if (offset_pos + len >= lleaf->input[1].length()) {
						inum = 0;
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
					else if (lleaf->input[1][offset_pos + len] == '[') {
						inum = stoll(lleaf->input[1].substr(offset_pos + len));
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
				}
				offset_pos = lleaf->input[2].find(varname);
				if (offset_pos != lleaf->input[2].npos) {
					if (offset_pos + len >= lleaf->input[2].length()) {
						inum = 0;
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
					else if (lleaf->input[2][offset_pos + len] == '[') {
						inum = stoll(lleaf->input[2].substr(offset_pos + len));
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
				}
				offset_pos = lleaf->output.find(varname);
				if (offset_pos != lleaf->output.npos) {
					if (offset_pos + len >= lleaf->output.length()) {
						inum = 0;
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
					else if (lleaf->output[offset_pos + len] == '[') {
						inum = stoll(lleaf->output.substr(offset_pos + len));
						lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
					}
				}
			}
		}
	}
	/*for (int i = 0; i < block->block_ins_num; i++) {
		// root instruction
		if (block->ins_pointer_list[i]->indeg == 0) {
			root = block->ins_pointer_list[i];
			tmp_vl = SplitVar(root->output);
			while (tmp_vl != NULL) {
				// register type
				if (tmp_vl->var.name[0] != '*') {
					leaf = block->reg_written[tmp_vl->var.name];
					if (leaf != NULL && leaf != root) {
						leaf->ex_loop.push_back(make_pair(root, make_pair(1, leaf->cycle - root->cycle + root->write_cycle)));
						for (int j = 0; j < leaf->chl.size(); j++) {
							lleaf = leaf->chl[j].first;
							lleaf->ex_loop.push_back(make_pair(root, make_pair(1, lleaf->read_cycle + root->write_cycle - root->cycle)));
						}
					}
				}
				// addr type
				if (tmp_vl->var.type == VarType::Address) {
					// leaf is where AR lastly change
					leaf = block->reg_written[tmp_vl->var.name];
					int offset_per_iteration = block->reg_offset[tmp_vl->var.name];
					int leaf_offset, offset_pos, inum, len = tmp_vl->var.name.length();

					for (int j = 0; j < leaf->chl.size(); j++) {
						lleaf = leaf->chl[j].first;
						offset_pos = lleaf->input[0].find(tmp_vl->var.name);
						if (offset_pos != lleaf->input[0].npos) {
							if (offset_pos + len >= lleaf->input[0].length()) {
								inum = 0;
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
							else if (lleaf->input[0][offset_pos + len] == '[') {
								inum = stoll(lleaf->input[0].substr(offset_pos + len));
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
						}
						offset_pos = lleaf->input[1].find(tmp_vl->var.name);
						if (offset_pos != lleaf->input[1].npos) {
							if (offset_pos + len >= lleaf->input[1].length()) {
								inum = 0;
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
							else if (lleaf->input[1][offset_pos + len] == '[') {
								inum = stoll(lleaf->input[1].substr(offset_pos + len));
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
						}
						offset_pos = lleaf->input[2].find(tmp_vl->var.name);
						if (offset_pos != lleaf->input[2].npos) {
							if (offset_pos + len >= lleaf->input[2].length()) {
								inum = 0;
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
							else if (lleaf->input[2][offset_pos + len] == '[') {
								inum = stoll(lleaf->input[2].substr(offset_pos + len));
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
						}
						offset_pos = lleaf->output.find(tmp_vl->var.name);
						if (offset_pos != lleaf->output.npos) {
							if (offset_pos + len >= lleaf->output.length()) {
								inum = 0;
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
							else if (lleaf->output[offset_pos + len] == '[') {
								inum = stoll(lleaf->output.substr(offset_pos + len));
								lleaf->ex_loop.push_back(make_pair(root, make_pair(1 + inum / offset_per_iteration, lleaf->read_cycle + root->write_cycle - root->cycle)));
							}
						}
					}
				}
				tmp_vl = tmp_vl->next;
			}
		}
	}*/
}

// Select the instructions whose indge = 0 and push them into soft_instr_queue
void LoadZeroInstrIntoQueue(Block* block) {
	// clear queue
	while (block->soft_instr_queue.size()) block->soft_instr_queue.pop();

	for (int i = 0; i < block->block_ins_num; i++) {
		if (block->ins_pointer_list[i]->indeg == 0) {
			block->soft_instr_queue.push(block->ins_pointer_list[i]);
		}
	}
}

// Module Scheduling Algorithm
void ModuleSchedule(Block* block) {
	/*for (int i = 0; i < block->ins_pointer_list.size(); i++) {
		GetVarInfoByInstr(block->ins_pointer_list[i], 10);
	}

	printf("\n\n\n");*/
	int T = GetInitT0(block);
	// T_MAX won't excceed the length of complete serial flow
	int T_MAX = block->block_ins_num * LONGEST_INSTR_DELAY;
	Instruction* x, * sbr = NULL;
	pair<int, int> pr;
	int i, fit_pos = 0;
	int LT = 0;
	vector<int> function_unit_vector;
	int max_cycle = CycleTotolNumOfBlock(block);
	block->ins_schedule_queue.clear();
	block->ins_schedule_queue_write.clear();
	block->ins_schedule_queue.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
	block->ins_schedule_queue_write.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
	block->queue_length = 0;

	for (; T < T_MAX; T++) {
		LT = 0;
		sbr = NULL;


		block->module_queue_r.clear();
		block->module_queue_w.clear();
		block->LT_queue_r.clear();
		block->LT_queue_w.clear();
		block->LT_queue_r.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
		block->LT_queue_w.resize(max_num_of_instr, vector<Instruction*>(function_units_index.size()));
		block->module_queue_r.resize(T, vector<Instruction*>(function_units_index.size()));
		block->module_queue_w.resize(T, vector<Instruction*>(function_units_index.size()));
		tl_map.clear();//跨循环依赖初始化
		flag.clear();  //跨循环依赖初始化
		flag_f.clear();//跨循环依赖初始化

		for (int i = 0; i < block->ins_pointer_list.size(); i++) {
			block->ins_pointer_list[i]->site = -1;
		}
		LoadZeroInstrIntoQueue(block);
		pair<int, int> best_fit;
		while (block->soft_instr_queue.size())
		{
			vector<poslist> var_info0, var_info1;
			x = block->soft_instr_queue.front();
			block->soft_instr_queue.pop();

			if (!strcmp(x->ins_name, "SBR")) {
				sbr = x;
				continue;
			}

			function_unit_vector = FunctionUnitSplit(string(x->func_unit));

			best_fit = make_pair(-1, 0x3f3f3f3f);
			for (i = function_unit_vector.size() - 1; i >= 0; i--)
			{
				/*fit_pos = MSFindFit(block, x, function_unit_vector[i], 0, T);
				if (fit_pos == -1) continue;
				fit_pos = LTFindFit(block, x, function_unit_vector[i], fit_pos, T);
				if (fit_pos != -1 && fit_pos < best_fit.second) {
					best_fit = make_pair(function_unit_vector[i], fit_pos);
					if (x->cycle >= 7) break;
					//LT = fit_pos > LT ? fit_pos : LT;
					//MSPlace(block, x, function_unit_vector[i], fit_pos, T);
					//x->last_fa_end_time = -1; // necessary, this mark whether x is distributed or not.
					//if (strstr(x->func_unit, "MAC") != NULL) ReallocMAC(x, function_unit_vector[i]);
					//break;
				}*/
				int init_pos = x->last_fa_end_time % T;
				int T_pos = -1;
				//bool flag = false;
				for (int j = 0; j < T; j++) {
					//if (init_pos + j >= 2 * T) break;
					//printf("%d  %d\n\n", x->last_fa_end_time, j);
					T_pos = (init_pos + j) % T;
					T_pos = MSFindFit(block, x, function_unit_vector[i], T_pos, T);
					if (T_pos == -1) continue;
					fit_pos = LTFindFit(block, x, function_unit_vector[i], T_pos, T);
					if (fit_pos == -1) continue;
					var_info1 = GetVarInfoByInstr(x, fit_pos);


					/*if (fit_pos != -1 && fit_pos < best_fit.second) {
						best_fit = make_pair(function_unit_vector[i], fit_pos);
						break;
					}*/

					Instruction* tmp = across_dependency(T, max_cycle, var_info1);
					//if (tmp != NULL && T == 6) printf("%s\n", tmp->ins_string);
					del(T, max_cycle, var_info1);
					if (flag_pd == 0)
					{

						if (fit_pos < best_fit.second)
						{
							//printf("%d   var_size: %d\n", max_cycle, var_info.size());
							if (tmp == NULL) {
								best_fit = make_pair(function_unit_vector[i], fit_pos);
								var_info0 = var_info1;
								break;
							}
							else {
								if (tmp == x) break;

								int old_pos = tmp->site;
								int old_unit = tmp->use_unit;
								// remove ins
								vector<poslist> tmp_var_info = GetVarInfoByInstr(tmp, tmp->site);
								MSUnplace(block, tmp, old_unit, old_pos, T);
								del(T, max_cycle, tmp_var_info);
								if (changeInsPosition(block, x, fit_pos, function_unit_vector[i], tmp, T, max_cycle)) {
									best_fit = make_pair(function_unit_vector[i], fit_pos);
									/*printf("	  %s\n", tmp->ins_string);
									printf("old_fit_pos: %d  fit_pos: %d   unit: %d  %d\n", old_pos, tmp->site, old_unit, tmp->use_unit);*/
									var_info0 = var_info1;
									// remove ins
									//break;
									MSUnplace(block, tmp, tmp->use_unit, tmp->site, T);
									vector<poslist> tmp2_var_info = GetVarInfoByInstr(tmp, tmp->site);
									del(T, max_cycle, tmp2_var_info);
								}
								// restore ins
								across_dependency(T, max_cycle, tmp_var_info);
								MSPlace(block, tmp, old_unit, old_pos, T, 0);
								tmp->site = old_pos;
								tmp->use_unit = old_unit;
							}
						}
					}

				}
				//if (flag) break;
			}
			if (function_unit_vector.size() == 0) {
				//MSPlace(block, x, 0, 0, T);
				//x->last_fa_end_time = -1;
				best_fit = make_pair(0, 0);
			}
			if (best_fit.first == -1) {
				//				puts("fail v v v v ");
				//				OutputLT(block, LT);
				MSFailure(block, T, LT);
				PrintDebug("T = %d, trying ins:%s ***faild.\n", T, x->ins_string);
				break;
			}
			//if (T == 3) printf("%d\n", best_fit.second);
			LT = (best_fit.second + x->cycle - 1) > LT ? (best_fit.second + x->cycle - 1) : LT;
			Instruction* tmp = across_dependency(T, max_cycle, var_info0);
			if (tmp != NULL) {
				// remove ins
				vector<poslist> tmp_var_info = GetVarInfoByInstr(tmp, tmp->site);
				MSUnplace(block, tmp, tmp->use_unit, tmp->site, T);
				del(T, max_cycle, tmp_var_info);
				del(T, max_cycle, var_info0);
				changeInsPosition(block, x, best_fit.second, best_fit.first, tmp, T, max_cycle);
				across_dependency(T, max_cycle, var_info0);
			}
			MSPlace(block, x, best_fit.first, best_fit.second, T);
			x->site = best_fit.second; // necessary, this mark whether x is distributed or not.
			x->use_unit = best_fit.first;

			if (strstr(x->func_unit, "MAC") != NULL) ReallocMAC(x, best_fit.first);
		}
		if (block->soft_instr_queue.empty() && best_fit.first != -1) break; // module schedule succeed
		else while (!block->soft_instr_queue.empty()) block->soft_instr_queue.pop();// flush
	}

	for (int i = 0; i < block->ins_pointer_list.size(); i++) {
		if (strstr(block->ins_pointer_list[i]->func_unit, "MAC") != NULL) ReallocMAC(block->ins_pointer_list[i], block->ins_pointer_list[i]->use_unit);
	}

	if ((LT + 1) % T != 0) LT += T - (LT + 1) % T;

	if (sbr != NULL) {
		int start_time;
		start_time = max(sbr->last_fa_end_time, LT - (sbr->cycle) + 1);
		//start_time = max(start_time, LT - T + 1);
		function_unit_vector = FunctionUnitSplit(string(sbr->func_unit));
		while (1) {
			// It's sure that sbr doesn't have ex_loop restraint, so using find_fit instead of ms_find_fit is ok
			pr = FindFit(block, function_unit_vector, start_time, sbr->cycle, sbr->read_cycle, sbr->write_cycle);
			//LT = sbr->last_fa_end_time > LT ? sbr->last_fa_end_time : LT;
			if (pr.second + sbr->cycle - 1 == LT) break;
			start_time += T;
			LT += T;
			//LT = pr.second + sbr->cycle - 1 > LT ? pr.second + sbr->cycle - 1 : LT;
		}
		MSPlace(block, sbr, pr.first, pr.second, T);
		//if ((LT + 1) % T != 0) LT += T - (LT + 1) % T;
		block->sbr = *sbr;
	}

	block->LT_queue_length = LT + 1;
	if (T >= T_MAX) block->soft_pipe_cycle = -1;
	else block->soft_pipe_cycle = T;

	OutputLT(block, block->LT_queue_length);
}


int CycleTotolNumOfBlock(Block* block) {
	int max_cycle = 0;
	Instruction* ins;
	for (int i = 0; i < block->block_ins_num; i++) {
		ins = block->ins_pointer_list[i];
		if (!strcmp(ins->ins_name, "SBR") || ins->cycle == 0) continue;
		max_cycle += ins->cycle;
	}
	/*printf("max_cycle:  %d\n", max_cycle);*/
	return max_cycle;
}


int getAlterNum(Instruction* ins) {
	int res = 100;
	for (int i = 0; i < ins->chl.size(); i++) {
		Instruction* son = ins->chl[i].first;
		if (son->site != -1) {
			res = min(res, son->site - ins->site - ins->chl[i].second);
		}
	}
	return res;
}


bool changeInsPosition(Block* block, Instruction* current_ins, int site, int use_unit, Instruction* ins, int T, int max_cycle) {
	/*return make_pair(-1, -1);*/
	vector<poslist> current_var_info = GetVarInfoByInstr(current_ins, site);
	vector<poslist> var_info;

	vector<int> function_unit_vector = FunctionUnitSplit(string(ins->func_unit));
	//int old_pos = ins->site;
	//int old_unit = ins->use_unit;
	int alterableNum = getAlterNum(ins);
	//printf("%d\n", alterableNum);
	//MSUnplace(block, ins, old_unit, old_pos, T); // remove ins
	//del(T, max_cycle, var_info);

	for (int i = 1; i <= alterableNum; i++) {
		int pos = (i + ins->site) % T;
		for (int j = 0; j < function_unit_vector.size(); j++) {
			//printf("%d\n", function_unit_vector[j]);
			int T_pos = MSFindFit(block, ins, function_unit_vector[j], pos, T);
			if (T_pos == -1) continue;
			int fit_pos = LTFindFit(block, ins, function_unit_vector[j], T_pos, T);
			if (fit_pos == -1) continue;
			var_info = GetVarInfoByInstr(ins, fit_pos);

			Instruction* res = across_dependency(T, max_cycle, var_info);
			if (res == NULL) { // have not relation
				// insert ins
				MSPlace(block, ins, function_unit_vector[j], fit_pos, T, 0);

				int T_pos1 = MSFindFit(block, current_ins, use_unit, site % T, T);
				if (T_pos1 == -1) {
					del(T, max_cycle, var_info);
					MSUnplace(block, ins, function_unit_vector[j], fit_pos, T);
					continue;
				}
				int fit_pos1 = LTFindFit(block, current_ins, use_unit, T_pos1, T);
				if (fit_pos1 == -1) {
					del(T, max_cycle, var_info);
					MSUnplace(block, ins, function_unit_vector[j], fit_pos, T);
					continue;
				}
				Instruction* res2 = across_dependency(T, max_cycle, current_var_info);
				del(T, max_cycle, current_var_info);
				if (res2 == NULL) { // have not relation
					ins->site = fit_pos; // necessary, this mark whether x is distributed or not.
					ins->use_unit = function_unit_vector[j];
					return true;
				}
				else {
					// remove ins
					MSUnplace(block, ins, function_unit_vector[j], fit_pos, T);
					del(T, max_cycle, var_info);
					break;
				}
			}
			else {
				// remove ins
				//MSUnplace(block, ins, function_unit_vector[j], fit_pos, T);
				del(T, max_cycle, var_info);
				break;
			}
		}
	}
	//// restore ins
	//MSPlace(block, ins, old_unit, old_pos, T);
	//var_info = GetVarInfoByInstr(ins, old_pos);
	//across_dependency(T, max_cycle, var_info);
	return false;
}


void SeparateVariableInfo(vector<poslist>& var_info, const string operand, Instruction* ins, int current_cycle, int isWrite) {
	if (IsSpecialOperand(operand) == 2) return;
	poslist pl1, pl2, pl3;
	string var = "";
	int i = 1;
	// memory access
	if (operand[0] == '*') {
		int flag = 0;
		while (i < operand.size() && (operand[i] == '+' || operand[i] == '-')) {
			flag++;
			++i;
		}
		for (; i < operand.size() && operand[i] != '+' && operand[i] != '-' && operand[i] != '['; ++i) var += operand[i];
		while (i < operand.size() && (operand[i] == '+' || operand[i] == '-')) {
			flag++;
			++i;
		}
		pl1.name = var;
		pl1.cycle = 1;
		pl1.judge = 0;
		pl1.ab_po = current_cycle;
		pl1.ins = ins;
		var_info.push_back(pl1);
		if (flag == 2) {
			pl3.name = var;
			pl3.cycle = 1;
			pl3.judge = 1;
			pl3.ab_po = current_cycle;
			pl3.ins = ins;
			var_info.push_back(pl3);
		}
		if (i < operand.size() && operand[i] == '[') {
			i++;
			if (operand[i] >= '0' && operand[i] <= '9' || operand[i] == '-') return;
			var = "";
			for (; operand[i] != ']'; ++i) var += operand[i];
			pl2.name = var;
			pl2.cycle = 1;
			pl2.judge = 0;
			pl2.ab_po = current_cycle;
			pl2.ins = ins;
			var_info.push_back(pl2);
		}
		return;
	}
	// other form
	i = 0;
	for (; i < operand.size() && operand[i] != ':'; ++i) var += operand[i];
	pl1.name = var;
	pl1.ab_po = current_cycle;
	if (!isWrite) {
		pl1.cycle = ins->read_cycle;
	}
	else {
		pl1.cycle = ins->write_cycle;
		pl1.ab_po += ins->cycle - ins->write_cycle;
	}
	pl1.judge = isWrite;
	pl1.ins = ins;
	var_info.push_back(pl1);
	if (i < operand.size() && operand[i] == ':') {
		i++;
		var = "";
		for (; i < operand.size(); ++i) var += operand[i];
		pl2.name = var;
		pl2.ab_po = current_cycle;
		if (!isWrite) {
			pl2.cycle = ins->read_cycle;
		}
		else {
			pl2.cycle = ins->write_cycle;
			pl2.ab_po += ins->cycle - ins->write_cycle;
		}
		pl2.judge = isWrite;
		pl2.ins = ins;
		var_info.push_back(pl2);
	}
}

vector<poslist> GetVarInfoByInstr(Instruction* ins, int current_cycle) {
	// clear var_posInfo
	vector<poslist> var_info;
	if (!ins->cond_field.empty()) SeparateVariableInfo(var_info, ins->cond_field, ins, current_cycle);
	if (!ins->input[0].empty()) SeparateVariableInfo(var_info, ins->input[0], ins, current_cycle);
	if (!ins->input[1].empty()) SeparateVariableInfo(var_info, ins->input[1], ins, current_cycle);
	if (!ins->input[2].empty()) SeparateVariableInfo(var_info, ins->input[2], ins, current_cycle);
	if (!ins->output.empty()) SeparateVariableInfo(var_info, ins->output, ins, current_cycle, 1);

	/*printf("instruction:    %s  c:%d  rc:%d  wr:%d\n", ins->ins_string, ins->cycle, ins->read_cycle, ins->write_cycle);
	for (int i = 0; i < var_info.size(); i++) {
		printf("name: %s  cycle : %d  ab_po: %d  isWrite: %d\n", var_info[i].name.c_str(), var_info[i].cycle, var_info[i].ab_po, var_info[i].judge);
	}
	printf("\n\n");*/
	return var_info;
}


Instruction* across_dependency(int t_cycle/*稳定态周期*/, int ace,/*一个不小于最终总拍数的数*/ vector<poslist> ins)//跨循环依赖分析
{
	//printf("T == %d \n", t_cycle);
	int i, j, k, up, down;
	int tme = 0;
	k = ins.size();
	Instruction* gtr, * gtr_t;
	for (i = 0; i < k; i++)//变量分析
	{
		if (tl_map.count(ins[i].name) == 0)//初始化，建立一个ace长度的vector数组
		{
			tl_map[ins[i].name].resize(ace * 2);
			if (ins[i].judge == 0)//第一个变量是读
			{
				flag[ins[i].name] = ins[i].ab_po + t_cycle;//将这个变量的第一次读的第二次循环所在的拍数记录
			}
		}
		if (ntc[ins[i].name] == 1)
		{
			flag[ins[i].name] = ins[i].ab_po + t_cycle;
			ntc[ins[i].name] = 0;
		}
		for (j = 0; j < (ace / t_cycle + 1); j++)//遍历每个循环的读或者写,j是循环的数目
		{
			if (ins[i].cycle == 1)//只有一拍读或写的时候
			{
				if (ins[i].judge == 0)//这个变量是读
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.push_back(position(j, 0, ins[i].ins));//存入临时的表
					if (j == 0)//只判断第一次循环的情况，逐个放的情况不会出现r的前一个循环的w依赖，所以只要判断后面一次就行了
					{
						for (up = ins[i].ab_po + j * t_cycle; up >= 0; up--)//以这个读所在的位置为起点向上搜写
						{
							if (tl_map[ins[i].name].at(up).wr.size() == 1 && tl_map[ins[i].name].at(up).wr[0].pos != j)//这个写只有一个，说明没有同时写的操作，这个写不等于本次循环数说明不是同一个循环
							{
								if (up == ins[i].ab_po + j * t_cycle && tl_map[ins[i].name].at(up).wr[0].pd != 2)//同拍只要不是写的第二拍和这个读同拍，就不会有依赖
								{
									continue;
								}
								gtr = tl_map[ins[i].name].at(up).wr[0].ins;
								tme = 1;//有依赖
								break;
							}
							else if (tl_map[ins[i].name].at(up).wr.size() > 1)//同一拍出现两个写，不管是两拍的写还是一拍的写，都有依赖
							{
								gtr = tl_map[ins[i].name].at(up).wr[0].ins;
								tme = 1;
							}
							else if (tl_map[ins[i].name].at(up).wr.size() == 1 && tl_map[ins[i].name].at(up).wr[0].pos == j)//找到这个写是同一个循环的说明没有依赖
							{
								break;
							}
						}
					}
				}
				else//这个变量是写
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.push_back(position(j, 0, ins[i].ins));//存入表中		
					if (j == 0)//只判断一次循环的就行了
					{
						if (tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.size() > 1)//存入之后发现这一拍有两个写在同一拍，直接就是依赖
						{
							gtr = tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr[0].ins;
							tme = 1;
						}
						if (flag.count(ins[i].name) != 0)
						{
							if (ins[i].ab_po >= flag[ins[i].name])
							{
								gtr = tl_map[ins[i].name].at(flag[ins[i].name]).re[0].ins;
								tme = 1;
							}
						}
						else
						{
							int cnt_w = 0;
							for (up = ins[i].ab_po + 1; up >= 0; up--)
							{
								if (tl_map[ins[i].name].at(up).re.size() != 0 && tl_map[ins[i].name].at(up).wr.size() != 0)
								{
									if (tl_map[ins[i].name].at(up).wr[0].pos != j)
									{
										cnt_w++;
										gtr_t = tl_map[ins[i].name].at(up).wr[0].ins;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w == 0)
									{
										break;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w != 0)
									{
										gtr = gtr_t;
										tme = 1;
										break;
									}
									for (auto op : tl_map[ins[i].name].at(up).re)
									{
										if (op.pos == j)
										{
											goto loop;
										}
									}
								}
								else if (tl_map[ins[i].name].at(up).re.size() != 0 && tl_map[ins[i].name].at(up).wr.size() == 0)
								{
									for (auto op : tl_map[ins[i].name].at(up).re)
									{
										if (op.pos == j)
										{
											goto loop;
										}
									}
								}
								else if (tl_map[ins[i].name].at(up).re.size() == 0 && tl_map[ins[i].name].at(up).wr.size() != 0)
								{
									if (tl_map[ins[i].name].at(up).wr[0].pos != j)
									{
										cnt_w++;
										gtr_t = tl_map[ins[i].name].at(up).wr[0].ins;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w == 0)
									{
										break;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w != 0)
									{
										gtr = gtr_t;
										tme = 1;
										break;
									}
								}
							}



						loop:

							for (down = ins[i].ab_po + j * t_cycle; down < ace; down++)//向下遍历
							{
								if (tl_map[ins[i].name].at(down).re.size() != 0 && flag.count(ins[i].name) == 0)//找到的第一个是读，或者是读和写都有
								{
									if (down == ins[i].ab_po + j * t_cycle)//在第一拍就搜到了读
									{
										for (int tem = 0; tem < tl_map[ins[i].name].at(down).re.size(); tem++)//遍历这个读的pd
											if (tl_map[ins[i].name].at(down).re[tem].pd == 1)//只要有一个pd=1，就说明有  两拍的读 的第一拍和这个写同拍。有依赖
											{
												tme = 1;
												gtr = tl_map[ins[i].name].at(down).re[tem].ins;
											}
										continue;
									}
									for (auto io : tl_map[ins[i].name].at(down).re)
									{
										if (io.pos != j)
										{
											gtr = io.ins;
											tme = 1;
										}
									}
									break;
								}
								else if (tl_map[ins[i].name].at(down).wr.size() > 1)
								{
									gtr = tl_map[ins[i].name].at(down).wr[0].ins;
									tme = 1;
								}
								else if (tl_map[ins[i].name].at(down).re.size() == 0 && tl_map[ins[i].name].at(down).wr.size() != 0 && down != ins[i].ab_po + j * t_cycle)//找到的第一个是写，没有依赖
								{
									break;
								}
							}
						}
					}
				}
			}
			if (ins[i].cycle == 2)
			{
				if (ins[i].judge == 0)
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.push_back(position(j, 1, ins[i].ins));
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).re.push_back(position(j, 2, ins[i].ins));
					if (j == 0)//只判断第一次循环的情况，逐个放的情况不会出现r的前一个循环的w依赖，所以只要判断后面一次就行了
					{
						for (up = ins[i].ab_po + j * t_cycle; up >= 0; up--)//以这个读的第一拍所在的位置为起点向上搜写
						{
							if (tl_map[ins[i].name].at(up).wr.size() == 1 && tl_map[ins[i].name].at(up).wr[0].pos != j)//这个写只有一个，说明没有同时写的操作，这个写不等于本次循环数说明不是同一个循环
							{
								gtr = tl_map[ins[i].name].at(up).wr[0].ins;
								tme = 1;//有依赖
								break;
							}
							else if (tl_map[ins[i].name].at(up).wr.size() > 1)//同一拍出现两个写，不管是两拍的写还是一拍的写，都有依赖
							{
								gtr = tl_map[ins[i].name].at(up).wr[0].ins;
								tme = 1;
								break;
							}
							else if (tl_map[ins[i].name].at(up).wr.size() == 1 && tl_map[ins[i].name].at(up).wr[0].pos == j)//找到这个写是同一个循环的说明没有依赖
							{
								break;
							}
						}
					}
				}
				else
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.push_back(position(j, 1, ins[i].ins));
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).wr.push_back(position(j, 2, ins[i].ins));

					if (j == 0)//只判断一次循环的就行了
					{
						if (tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.size() > 1 || tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).wr.size() > 1)//存入之后发现这一拍有两个写在同一拍，直接就是依赖
						{
							gtr = tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr[0].ins;
							tme = 1;
						}
						if (flag.count(ins[i].name) != 0)
						{
							if (ins[i].ab_po >= flag[ins[i].name])
							{
								gtr = tl_map[ins[i].name].at(flag[ins[i].name]).re[0].ins;
								tme = 1;
							}
						}
						else
						{
							int cnt_w = 0;
							for (up = ins[i].ab_po + 1; up >= 0; up--)
							{
								if (tl_map[ins[i].name].at(up).re.size() != 0 && tl_map[ins[i].name].at(up).wr.size() != 0)
								{
									if (tl_map[ins[i].name].at(up).wr[0].pos != j)
									{
										cnt_w++;
										gtr_t = tl_map[ins[i].name].at(up).wr[0].ins;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w == 0)
									{
										break;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w != 0)
									{
										gtr = gtr_t;
										tme = 1;
										break;
									}
									for (auto op : tl_map[ins[i].name].at(up).re)
									{
										if (op.pos == j)
										{
											goto loop1;
										}
									}
								}
								else if (tl_map[ins[i].name].at(up).re.size() != 0 && tl_map[ins[i].name].at(up).wr.size() == 0)
								{
									for (auto op : tl_map[ins[i].name].at(up).re)
									{
										if (op.pos == j)
										{
											goto loop1;
										}
									}
								}
								else if (tl_map[ins[i].name].at(up).re.size() == 0 && tl_map[ins[i].name].at(up).wr.size() != 0)
								{
									if (tl_map[ins[i].name].at(up).wr[0].pos != j)
									{
										cnt_w++;
										gtr_t = tl_map[ins[i].name].at(up).wr[0].ins;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w == 0)
									{
										break;
									}
									else if (tl_map[ins[i].name].at(up).wr[0].pos == j && cnt_w != 0)
									{
										gtr = gtr_t;
										tme = 1;
										break;
									}
								}
							}

						loop1:



							for (down = ins[i].ab_po + j * t_cycle; down < ace; down++)//向下遍历
							{
								if (tl_map[ins[i].name].at(down).re.size() != 0 && flag.count(ins[i].name) == 0)//找到的第一个是读，或者是读和写都有
								{
									if (down == ins[i].ab_po + j * t_cycle)//在第一拍就搜到了读
									{
										for (int tem = 0; tem < tl_map[ins[i].name].at(down).re.size(); tem++)//遍历这个读的pd
											if (tl_map[ins[i].name].at(down).re[tem].pd == 1)//只要有一个pd=1，就说明有  两拍的读 的第一拍和这个写同拍。有依赖
											{
												gtr = tl_map[ins[i].name].at(down).re[tem].ins;
												tme = 1;
											}
										continue;
									}
									for (auto io : tl_map[ins[i].name].at(down).re)
									{
										if (io.pos != j)
										{
											gtr = io.ins;
											tme = 1;
										}
									}
									break;
								}
								else if (tl_map[ins[i].name].at(down).wr.size() > 1)
								{
									gtr = tl_map[ins[i].name].at(down).wr[0].ins;
									tme = 1;
								}
								else if (tl_map[ins[i].name].at(down).re.size() == 0 && tl_map[ins[i].name].at(down).wr.size() != 0 && (down != ins[i].ab_po + j * t_cycle || down != ins[i].ab_po + j * t_cycle + 1))//找到的第一个是写，没有依赖
								{
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	if (tme == 0)
		return NULL;
	else
	{
		return gtr;
	}
}
void del(int t_cycle/*稳定态周期*/, int ace,/*一个不小于最终总拍数的数*/vector<poslist> ins)
{
	int i, j, k;
	k = ins.size();
	for (i = 0; i < k; i++)
	{
		if (flag.count(ins[i].name) != 0)
		{
			for (auto ic : tl_map[ins[i].name].at(flag[ins[i].name]).re)
			{
				if (ic.ins == ins[i].ins)
				{
					flag.erase(flag.find(ins[i].name));
					ntc[ins[i].name] = 1;
					break;
				}
			}
		}
		for (j = 0; j < (ace / t_cycle + 1); j++)
		{
			if (ins[i].cycle == 1)
			{
				if (ins[i].judge == 0)
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.erase(find(tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.begin(), tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.end(), position(j, 0, ins[i].ins)));

				}
				else
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.erase(find(tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.begin(), tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.end(), position(j, 0, ins[i].ins)));
				}
			}
			if (ins[i].cycle == 2)
			{
				if (ins[i].judge == 0)
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.erase(find(tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.begin(), tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).re.end(), position(j, 1, ins[i].ins)));
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).re.erase(find(tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).re.begin(), tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).re.end(), position(j, 2, ins[i].ins)));
				}
				else
				{
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.erase(find(tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.begin(), tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle).wr.end(), position(j, 1, ins[i].ins)));
					tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).wr.erase(find(tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).wr.begin(), tl_map[ins[i].name].at(ins[i].ab_po + j * t_cycle + 1).wr.end(), position(j, 2, ins[i].ins)));
				}
			}
		}
	}
}