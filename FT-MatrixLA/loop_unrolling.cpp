#include "loop_unrolling.h"
#include "data_structure.h"
#include <string>
#include "utils.h"
#include <regex>

// set used to store temporary vars of a loop
set<string> temp_var_set;
string printer = "";


Block back_1, back_2;

void RemoveLoop(int start_pos, int end_pos)
{
	// remove .loop block
	RemoveBlock(start_pos);
	end_pos--;
	// remove SBR instruction
	int sbr_ins_pos = linear_program.block_pointer_list[end_pos - 1]->ins_pointer_list.size() - 1;
	Instruction* sbr_ins = linear_program.block_pointer_list[end_pos - 1]->ins_pointer_list[sbr_ins_pos];
	linear_program.block_pointer_list[end_pos - 1]->ins_pointer_list.pop_back();
	linear_program.block_pointer_list[end_pos - 1]->block_ins_num--;
	linear_program.block_pointer_list[end_pos - 1]->type = BlockType::Instruction;
	//PrintDebug("++\n");
	ResetIns(sbr_ins);
	// if this SBR block becomes an empty block
	if (linear_program.block_pointer_list[end_pos - 1]->block_ins_num == 0) {
		RemoveBlock(end_pos - 1);
		end_pos--;
	}
	// remove .endloop block
	RemoveBlock(end_pos);
	// now, the start_pos is the first block in previous loop
	// and the end_pos is the first block after the previous loop
	//PrintDebug("%d, %d\n", start_pos, end_pos);
	//PrintGlobalData();
	end_pos -= MergeBlock(start_pos);
	MergeBlock(end_pos);
}

void ResetIns(Instruction* x) {
	free(x->ins_string);
	x->type = InsType::Default;
	x->ins_string = NULL;
	x->ins_name[0] = '\0';
	x->func_unit[0] = '\0';
	x->cond_field = "";
	x->cond_flag = 0;
	x->input[0] = "";
	x->input[1] = "";
	x->input[2] = "";
	x->output = "";
	x->note = "";
	x->cycle = 0;
	x->read_cycle = 0;
	x->write_cycle = 0;
	x->indeg = 0;
	x->last_fa_end_time = 0;
	x->extracted_var.clear();
	x->state = 0;  //  modified by DC
}


void ResetBlock(int index)
{
	linear_program.program_block[index].type = BlockType::Default;
	linear_program.program_block[index].label = "";
	linear_program.program_block[index].loop_depth = -1;
	Instruction x = Instruction();
	for (int i = 0; i < linear_program.program_block[index].block_ins_num; i++) {
		if (linear_program.program_block[index].block_ins[i].ins_string != NULL)
			free(linear_program.program_block[index].block_ins[i].ins_string);
		linear_program.program_block[index].block_ins[i] = x;
	}
	linear_program.program_block[index].block_ins_num = 0;
	//(linear_program.program_block[index].ins_pointer_list).swap(vector<Instruction*>() );
	vector <Instruction*>().swap(linear_program.program_block[index].ins_pointer_list);
}

void RemoveBlock(int pos)
{
	// 0. find the physical position of the block
	// if this is the last block in program_block
	int block_index = linear_program.block_pointer_list[pos]->block_index;
	if (block_index == (linear_program.total_block_num - 1)) {
		ResetBlock(block_index);
		linear_program.block_pointer_list.erase(linear_program.block_pointer_list.begin() + pos);
		linear_program.total_block_num--;
	}
	else
	{
		// erase the pointer of this block
		linear_program.block_pointer_list.erase(linear_program.block_pointer_list.begin() + pos);
		ResetBlock(block_index);
		// copy the content in the last block in program_block into this block
		int last_index = linear_program.total_block_num - 1;
		linear_program.program_block[block_index].type = linear_program.program_block[last_index].type;
		linear_program.program_block[block_index].label = linear_program.program_block[last_index].label;
		linear_program.program_block[block_index].block_index = block_index;
		linear_program.program_block[block_index].loop_depth = linear_program.program_block[last_index].loop_depth;
		linear_program.program_block[block_index].block_ins_num = linear_program.program_block[last_index].block_ins_num;
		for (int i = 0; i < linear_program.program_block[last_index].ins_pointer_list.size(); i++) {
			CopyInstruction(&(linear_program.program_block[block_index].block_ins[i]),
				linear_program.program_block[last_index].ins_pointer_list[i]);
			linear_program.program_block[block_index].ins_pointer_list.push_back(&(linear_program.program_block[block_index].block_ins[i]));
		}
		// modifiy pointers
		for (int i = 0; i < linear_program.block_pointer_list.size(); i++) {
			if (linear_program.block_pointer_list[i] == &(linear_program.program_block[last_index])) {
				linear_program.block_pointer_list[i] = &(linear_program.program_block[block_index]);

				break;
			}
		}
		ResetBlock(last_index);
		linear_program.total_block_num--;
	}
}

int MergeBlock(int pos)
{
	// 
	Block* blk = linear_program.block_pointer_list[pos];
	Block* prev_blk = linear_program.block_pointer_list[pos - 1];
	Instruction* last_ins_prev_blk = prev_blk->ins_pointer_list[prev_blk->ins_pointer_list.size() - 1];
	// can be merged into one block
	if (prev_blk->type == BlockType::Instruction
		&& blk->type == BlockType::Instruction
		&& (strcmp(last_ins_prev_blk->func_unit, "SBR\0") == 1)) {
		// merge the block
		// 0. modifify the block label (if the blk contains SBR function instruction)
		prev_blk->label = blk->label;
		// 1. copy instructions from blk to prev_blk
		int ins_index = prev_blk->block_ins_num;
		for (int i = 0; i < blk->ins_pointer_list.size(); i++) {
			//PrintDebug("%d, %d \n", i, ins_index);
			CopyInstruction(&(prev_blk->block_ins[ins_index]), blk->ins_pointer_list[i]);

			prev_blk->ins_pointer_list.push_back(&(prev_blk->block_ins[ins_index]));
			prev_blk->block_ins_num++;
			ins_index++;
		}
		// 2. remove the blk
		RemoveBlock(pos);
		return 1;
	}
	// can not be merged
	else
		return 0;
}


void TemporaryVar(int start_pos, int end_pos)
{
	// 0. initialize the set
	temp_var_set.clear();

	// 1. insert all variables in the loop from bloack start_pos (.loop label)
	//    to end_pos (.endloop) into the temp_var_set
	for (int i = start_pos + 1; i < end_pos; i++) {
		for (int j = 0; j < linear_program.block_pointer_list[i]->block_ins_num; j++) {
			temp_var_set.insert(linear_program.block_pointer_list[i]->ins_pointer_list[j]->extracted_var.begin(),
				linear_program.block_pointer_list[i]->ins_pointer_list[j]->extracted_var.end());
		}
	}
	/*
	for (auto i : temp_var_set)
		printf("%s, ", i.c_str());
	printf("\n");
	*/
	// 2. remove all variables that exist outside the loop

	for (int i = 0; i < start_pos; i++)
		for (int j = 0; j < linear_program.block_pointer_list[i]->block_ins_num; j++) {
			for (auto k : linear_program.block_pointer_list[i]->ins_pointer_list[j]->extracted_var) {
				temp_var_set.erase(k);
			}
		}

	for (int i = end_pos; i < linear_program.total_block_num; i++)
		for (int j = 0; j < linear_program.block_pointer_list[i]->block_ins_num; j++) {
			for (auto k : linear_program.block_pointer_list[i]->ins_pointer_list[j]->extracted_var) {
				temp_var_set.erase(k);
			}
		}

	// *3. filter:
	//    <1> remove vars declared in .input
	//    <2> remove vars that is a temp var, but its partner is not a temp var
	set<string> filter;
	for (auto i : temp_var_set) {
		// <1> remove vars declared in .input
		if (var_map[i].type == VarType::Input)
			filter.insert(i);
		// <2> remove vars that is a temp var, but its partner is not a temp var
		if (var_map[i].partner != "") {
			if (temp_var_set.count(var_map[i].partner) == 0)
				filter.insert(i);
		}
	}

	for (auto i : filter)
		temp_var_set.erase(i);

	for (auto i : temp_var_set) {
		printer += i.c_str();
		printer += ",";
	}
	//PrintDebug("Temporary vars can be renamed in Loop %s: %s \n", linear_program.block_pointer_list[start_pos]->label.c_str(), printer.c_str());
	printer = "";
}

// src  pattern: tempoary var 
string ExactReplace(string src, string pattern, string suffix)
{
	string res = src;
	int search_range = res.length() - pattern.length();
	int i = 0;
	if (search_range < 0) return res;
	for (i = 0; i <= res.length() - pattern.length(); i++) {
		string x = res.substr(i, pattern.length());
		if (x == pattern) {
			// recheck
			int flag = 0;
			if (i == 0) flag = 1;
			else if (res[i - 1] == ' ') flag = 1;
			else if (res[i - 1] == ':') flag = 1;
			else if (res[i - 1] == ',') flag = 1;
			else if (res[i - 1] == '\t') flag = 1;
			else if (res[i - 1] == '*') flag = 1;

			if (flag == 0) continue;

			flag = 0;
			if (i == res.length() - pattern.length()) flag = 1;
			else if (res[i + pattern.length()] == ' ') flag = 1;
			else if (res[i + pattern.length()] == ':') flag = 1;
			else if (res[i + pattern.length()] == ',') flag = 1;
			else if (res[i + pattern.length()] == '\t') flag = 1;
			else if (res[i + pattern.length()] == '\r') flag = 1;
			else if (res[i + pattern.length()] == '+') flag = 1;
			else if (res[i + pattern.length()] == '-') flag = 1;

			if (flag == 0) continue;

			// replace
			res.replace(res.begin() + i, res.begin() + i + pattern.length(), pattern + "_" + suffix);
			i = i + pattern.length() + 1 + suffix.length() - 1;
		}
	}
	return res;
}

void RenameVar(Instruction* ins, Instruction* ins_ori, string suffix)
{
	// 0. build the suffix_str 
	//    (eg: suffix = 0, ssuffix_str = "_0")
	string suffix_str = "_";
	suffix_str.append(suffix);

	// 1. rename the variables in ins's field
	// TODO : vars with parteners ???
	for (auto s : temp_var_set) {
		bool replace_flag = false;
		if (ins_ori->extracted_var.count(s) == 1) {
			replace_flag = true;
			// 1.1. check cond_field
			//ins->cond_field = regex_replace(ins->cond_field, regex(s), s + suffix_str);
			if (ins->cond_field != "")
				ins->cond_field = ExactReplace(ins->cond_field, s, suffix);
			// 1.2. check input
			//ins->input[0] = regex_replace(ins->input[0], regex(s), s + suffix_str);
			//ins->input[1] = regex_replace(ins->input[1], regex(s), s + suffix_str);
			//ins->input[2] = regex_replace(ins->input[2], regex(s), s + suffix_str);

			if (ins->input[0] != "")
				ins->input[0] = ExactReplace(ins->input[0], s, suffix);

			if (ins->input[1] != "")
				ins->input[1] = ExactReplace(ins->input[1], s, suffix);
			if (ins->input[2] != "")
				ins->input[2] = ExactReplace(ins->input[2], s, suffix);
			// 1.3. check output
			if (ins->output != "")
				ins->output = ExactReplace(ins->output, s, suffix);
			//ins->output = regex_replace(ins->output, regex(s), s + suffix_str);
			// 2. modifiy the ins's extracted_var
			string renamed_var = s + suffix_str;
			ins->extracted_var.erase(s);
			ins->extracted_var.insert(renamed_var);
			// 3. rebuild the ins's ins_str
			string renamed_ins_string = ins->ins_string;
			if (renamed_ins_string != "")
				//renamed_ins_string = regex_replace(renamed_ins_string, regex(s), s + suffix_str);
				renamed_ins_string = ExactReplace(renamed_ins_string, s, suffix);
			std::free(ins->ins_string);
			ins->ins_string = (char*)malloc((renamed_ins_string.length()) * sizeof(char) + 4);
			std::strcpy(ins->ins_string, renamed_ins_string.c_str());
			// 4. add new var into var_map
			Variable v;
			v.name = renamed_var;
			v.type = var_map[s].type;
			v.reg_type = var_map[s].reg_type;
			if (var_map[s].partner != "")
				v.partner = var_map[s].partner + suffix_str;
			v.pair_pos = var_map[s].pair_pos;
			if (var_map.count(v.name) == 0) {
				v.use_flag = 1;
				v.assign_flag = 1;
				var_map.insert(pair<string, Variable>(renamed_var, v));
			}


		}

	}



}

void ReBuildInsStr(Instruction* ins)
{
}

bool MergeIns(Block* blk) {
	bool merge_flag = false;
	// 0. backup the original block
	// for back_up: instructions in block_ins is in order
	back_1.block_ins_num = 0;
	for (auto i : blk->ins_pointer_list) {
		CopyInstruction(&(back_1.block_ins[back_1.block_ins_num]), i);
		back_1.ins_pointer_list.push_back(&(back_1.block_ins[back_1.block_ins_num]));
		back_1.block_ins_num++;
	}
	// check if double is possible
	for (int i = 0; i < back_1.block_ins_num - 1; i++) {
		if (DoubleCheck(&back_1, i))
			merge_flag = true;
	}
	if (merge_flag == false) {
		PrintInfo("This loop can not be doubled \n");
		return merge_flag;
	}
	// 1. begin to double the basic block
	//    including instruction reordering and double (VLDW and STDW)
	// for each instruction in the block except the last SBR instruction
	int vstw_pos = 0;
	for (int i = 0; i < back_1.block_ins_num - 1; i++) {
		// 1.1 if VLDW (merge into the first instruction)
		if (DoubleCheck(&back_1, i)) {
			//printf("=== Ins %d can be doubled\n", i);
			if (strcmp(back_1.block_ins[i].ins_name, "VLDW") == 0) {
				vstw_pos++;
				// modifiy output
				string ins_var = blk->block_ins[i].output;
				string ins_var_d = ins_var + "_d";
				//strcpy(blk->block_ins[i].output, ins_var_d + ":" + ins_var);
				blk->block_ins[i].output = ins_var_d + ":" + ins_var;
				// change VLDW to VLDDW
				strcpy(blk->block_ins[i].ins_name, "VLDDW\0");
				string renamed_ins_string = blk->block_ins[i].ins_string;
				renamed_ins_string = regex_replace(renamed_ins_string, regex(ins_var), ins_var_d + ":" + ins_var);
				renamed_ins_string = regex_replace(renamed_ins_string, regex("VLDW"), "VLDDW");
				free(blk->block_ins[i].ins_string);
				blk->block_ins[i].ins_string = (char*)malloc((renamed_ins_string.length()) * sizeof(char) + 4);
				strcpy(blk->block_ins[i].ins_string, renamed_ins_string.c_str());
				// modifiy var_map
				blk->block_ins[i].extracted_var.insert(ins_var_d);
				Variable v;
				v.name = ins_var_d;
				v.type = var_map[ins_var].type;
				v.reg_type = var_map[ins_var].reg_type;
				v.partner = ins_var;
				v.pair_pos = 0;
				var_map[ins_var].partner = v.name;
				var_map[ins_var].pair_pos = -1;
				if (var_map.count(ins_var_d) != 0)
					var_map.erase(ins_var_d);
				v.use_flag = 1;
				var_map.insert(pair<string, Variable>(ins_var_d, v));
				if (temp_var_set.count(ins_var_d) == 0)
					temp_var_set.insert(ins_var_d);
			}
			else if (strcmp(back_1.block_ins[i].ins_name, "VSTW") == 0) {

				// modifiy input[0]
				string ins_var = blk->block_ins[i].input[0];
				string ins_var_d = ins_var + "_d";
				//strcpy(blk->block_ins[i].output, ins_var_d + ":" + ins_var);
				blk->block_ins[i].input[0] = ins_var_d + ":" + ins_var;
				// change VLDW to VLDDW
				strcpy(blk->block_ins[i].ins_name, "VSTDW\0");
				string renamed_ins_string = blk->block_ins[i].ins_string;
				renamed_ins_string = regex_replace(renamed_ins_string, regex(ins_var), ins_var_d + ":" + ins_var);
				renamed_ins_string = regex_replace(renamed_ins_string, regex("VSTW"), "VSTDW");
				free(blk->block_ins[i].ins_string);
				blk->block_ins[i].ins_string = (char*)malloc((renamed_ins_string.length()) * sizeof(char) + 4);
				strcpy(blk->block_ins[i].ins_string, renamed_ins_string.c_str());
				// modifiy var_map
				blk->block_ins[i].extracted_var.insert(ins_var_d);
				Variable v;
				v.name = ins_var_d;
				v.type = var_map[ins_var].type;
				v.reg_type = var_map[ins_var].reg_type;
				v.partner = ins_var;
				v.pair_pos = 0;
				var_map[ins_var].partner = v.name;
				var_map[ins_var].pair_pos = -1;
				if (var_map.count(ins_var_d) != 0)
					var_map.erase(ins_var_d);
				v.use_flag = 1;
				var_map.insert(pair<string, Variable>(ins_var_d, v));
				if (temp_var_set.count(ins_var_d) == 0)
					temp_var_set.insert(ins_var_d);
				// change pointers to the last postion
				blk->ins_pointer_list.erase(blk->ins_pointer_list.begin() + vstw_pos);
				blk->ins_pointer_list.emplace(blk->ins_pointer_list.end() - 1, &(blk->block_ins[i]));
				//PrintGlobalData();
			}

		}
		else {
			vstw_pos++;
			Instruction* x = &(blk->block_ins[blk->block_ins_num]);
			CopyInstruction(x, back_1.ins_pointer_list[i]);
			blk->ins_pointer_list.emplace(blk->ins_pointer_list.end() - 1, x);
			RenameVar(x, back_1.ins_pointer_list[i], "d");

			blk->block_ins_num++;
		}
		// 1.2 if VSTW (merge into the second instrucion)

		// 1.3 other instrucitons
		// TODO: 1.4 if other instruction (try to move it to just behind the first instrucion)
		//  
	}



	// 2. finish loop unrolling
	back_1.type = BlockType::Default;
	back_1.label = "";
	back_1.loop_depth = -1;
	Instruction x = Instruction();
	for (int i = 0; i < back_1.block_ins_num; i++) {
		if (back_1.block_ins[i].ins_string != NULL)
			free(back_1.block_ins[i].ins_string);
		back_1.block_ins[i] = x;
	}
	back_1.block_ins_num = 0;
	//(linear_program.program_block[index].ins_pointer_list).swap(vector<Instruction*>() );
	vector <Instruction*>().swap(back_1.ins_pointer_list);
	return merge_flag;
}

bool DoubleCheck(Block* blk, int index)
{
	bool double_flag = false;
	if (strcmp(blk->ins_pointer_list[index]->ins_name, "VLDW") == 0) {
		string ins_var = blk->ins_pointer_list[index]->output;
		string ins_ar = "";
		string ins_or = "";
		for (auto i : blk->ins_pointer_list[index]->extracted_var) {
			if (var_map[i].reg_type == RegType::S_AR || var_map[i].reg_type == RegType::V_AR)
				ins_ar = i;
			if (var_map[i].reg_type == RegType::S_OR || var_map[i].reg_type == RegType::V_OR)
				ins_or = i;
		}
		if (ins_ar != "") {
			for (int i = 0; i < blk->block_ins_num - 1; i++) {
				if (i != index) {
					if (blk->ins_pointer_list[i]->extracted_var.count(ins_ar) != 0)
						return double_flag;
				}
			}
		}

		if (ins_or != "") {
			for (int i = 0; i < blk->block_ins_num - 1; i++) {
				if (i != index) {
					if (blk->ins_pointer_list[i]->output.find(ins_or) != -1) {
						if (strcmp(blk->ins_pointer_list[i]->ins_name, "VSTW") != 0
							&& strcmp(blk->ins_pointer_list[i]->ins_name, "VSTDW") != 0)
							return double_flag;
					}
				}
			}
		}

		for (int i = 0; i < index; i++) {
			if (blk->ins_pointer_list[i]->extracted_var.count(ins_var) != 0)
				return double_flag;
		}
		if (var_map[ins_var].partner != "" && var_map[ins_var].partner != (ins_var + "_d")) return double_flag;
		if (temp_var_set.count(ins_var) == 0) return double_flag;
		double_flag = true;

	}
	else if (strcmp(blk->ins_pointer_list[index]->ins_name, "VSTW") == 0)
	{
		string ins_var = blk->ins_pointer_list[index]->input[0];
		string ins_ar = "";
		string ins_or = "";
		for (auto i : blk->ins_pointer_list[index]->extracted_var) {
			if (var_map[i].reg_type == RegType::S_AR || var_map[i].reg_type == RegType::V_AR)
				ins_ar = i;
			if (var_map[i].reg_type == RegType::S_OR || var_map[i].reg_type == RegType::V_OR)
				ins_or = i;
		}
		if (ins_ar != "") {
			for (int i = 0; i < index; i++)
				if (blk->ins_pointer_list[i]->extracted_var.count(ins_ar) != 0)
					return double_flag;
			for (int i = index + 1; i < blk->block_ins_num - 1; i++)
				if (blk->ins_pointer_list[i]->extracted_var.count(ins_ar) != 0)
					return double_flag;
		}
		if (ins_or != "") {
			for (int i = 0; i < blk->block_ins_num - 1; i++) {
				if (i != index) {
					if (blk->ins_pointer_list[i]->output.find(ins_or) != -1) {
						if (strcmp(blk->ins_pointer_list[i]->ins_name, "VSTW") != 0
							&& strcmp(blk->ins_pointer_list[i]->ins_name, "VSTDW") != 0)
							return double_flag;
					}
				}
			}
		}

		for (int i = index + 1; i < blk->block_ins_num - 1; i++) {
			if (blk->ins_pointer_list[i]->extracted_var.count(ins_var) != 0)
				return double_flag;
		}
		if (var_map[ins_var].partner != "" && var_map[ins_var].partner != (ins_var + "_d")) return double_flag;
		if (temp_var_set.count(ins_var) == 0) return double_flag;
		double_flag = true;
	}

	return double_flag;
}

Block* CopyBlockByIndex(int src)
{
	int dst = linear_program.total_block_num;
	linear_program.program_block[dst].type = linear_program.block_pointer_list[src]->type;
	linear_program.program_block[dst].loop_depth = linear_program.block_pointer_list[src]->loop_depth;
	linear_program.program_block[dst].block_index = dst;

	for (int i = 0; i < linear_program.block_pointer_list[src]->block_ins_num; i++) {
		Instruction* z = &(linear_program.program_block[dst].block_ins[i]);
		//GenerateInstr(linear_program.block_pointer_list[src]->ins_pointer_list[i]->ins_string, z, -1);
		CopyInstruction(z, linear_program.block_pointer_list[src]->ins_pointer_list[i]);
		linear_program.program_block[dst].ins_pointer_list.push_back(z);
	}
	linear_program.program_block[dst].block_ins_num = linear_program.block_pointer_list[src]->block_ins_num;
	linear_program.total_block_num++;
	linear_program.total_ins_num += linear_program.program_block[dst].block_ins_num;
	return &(linear_program.program_block[linear_program.total_block_num - 1]);
}

int UnrollingLoop(int end_pos, int unrolling_factor, int double_extension)
{
	// the number of newly added blocks
	int added_block_num = 0;
	// the position of .loop label
	int start_pos = end_pos;
	while (start_pos >= 0)
	{
		if ((linear_program.block_pointer_list[start_pos]->loop_depth ==
			linear_program.block_pointer_list[end_pos]->loop_depth)
			&& (linear_program.block_pointer_list[start_pos]->type == BlockType::LoopStart))
			break;
		else
			start_pos--;
	}

	printf("==Unrolling loop from position %d to %d ==\n", start_pos, end_pos);
	int loopbody_block_num = end_pos - start_pos - 1;
	int last_loopbody_ins_num = linear_program.block_pointer_list[end_pos - 1]->block_ins_num;
	// unrolling the loop with unrolling_factor times
	// TODO:
	// 1. loop rename to handle nested loops
	// 2. gen var rename 
	// 3. double memory extension
	for (int i = 0; i < unrolling_factor; i++) {
		for (int j = 1; j <= loopbody_block_num; j++) {
			// the last block in this loop
			if (j == loopbody_block_num) {
				// if only contains SSUBU and SBR
				if (last_loopbody_ins_num == 2) {
					continue;
				}
				// if contains other normal instructions
				else
				{
					// first block of the next loop body
					Block* x = linear_program.block_pointer_list[start_pos + j];
					// last block of the original loop body
					Block* y = linear_program.block_pointer_list[end_pos - 1 + added_block_num];
					for (int k = 0; k < last_loopbody_ins_num - 2; k++) {
						Instruction* z = &(x->block_ins[x->block_ins_num]);
						//GenerateInstr(y->block_ins[k].ins_string, z, -1);
						CopyInstruction(z, &(y->block_ins[k]));
						x->ins_pointer_list.emplace(x->ins_pointer_list.begin() + k, z);
						x->block_ins_num += 1;
					}
				}
			}
			// not the last block in this loop
			else {
				// the pos of the block needed to be copy
				// start_pos + j - 1: the pos of the first block of next loop body
				// j : num of blocks already inserted
				Block* x = CopyBlockByIndex(start_pos + (j - 1) + j);
				linear_program.block_pointer_list.emplace(
					linear_program.block_pointer_list.begin() + (start_pos + 1) + (j - 1), x);
				added_block_num++;
				// 1. special cases for nested loops: rename loops
				// handle .loop, the name of the loop stored in input[0]
				// add suffix _(i+1)
				if (x->type == BlockType::LoopStart) {
					x->ins_pointer_list[0]->input[0].pop_back();
					x->ins_pointer_list[0]->input[0] = x->ins_pointer_list[0]->input[0] + "_" + to_string(i + 1) + ":" + "\0";
				}
				// handle .endloop
				// modifiy the SBR instruction in the previous block
				// change the input[0] of the SBR instruction
				if (x->type == BlockType::LoopEnd) {
					// find the pos of previous block contains SBR instruction
					int sbr_block_pos = start_pos + j - 1;
					int sbr_ins_pos = linear_program.block_pointer_list[sbr_block_pos]->ins_pointer_list.size() - 1;
					linear_program.block_pointer_list[sbr_block_pos]->ins_pointer_list[sbr_ins_pos]->input[0] =
						linear_program.block_pointer_list[sbr_block_pos]->ins_pointer_list[sbr_ins_pos]->input[0] + "_" + to_string(i + 1) + "\0";
					// TODO: modify the ins string
				}
			}
		}
	}


	// modify the loop control instruction (SSUBU)
	// support: SSUBU instruction with imm
	Block* x = linear_program.block_pointer_list[end_pos - 1 + added_block_num];
	int prev_step = atoi(x->ins_pointer_list[x->block_ins_num - 2]->input[0].c_str());
	int new_step = prev_step * (unrolling_factor + 1);
	x->ins_pointer_list[x->block_ins_num - 2]->input[0] = to_string(new_step);
	// modify the ins_string
	string new_loop_ctrl_str = "\t" + string(x->ins_pointer_list[x->block_ins_num - 2]->ins_name)
		+ "\t" + x->ins_pointer_list[x->block_ins_num - 2]->input[0] + ","
		+ x->ins_pointer_list[x->block_ins_num - 2]->input[1] + ","
		+ x->ins_pointer_list[x->block_ins_num - 2]->output + "\0";
	strcpy(x->ins_pointer_list[x->block_ins_num - 2]->ins_string, &new_loop_ctrl_str[0]);

	return added_block_num;
}

int UnrollingMostInnerLoop(int start_pos, int end_pos, int unrolling_factor, int double_extension)
{
	int added_block_num = 0;
	int loopbody_block_num = end_pos - start_pos - 1;
	int last_loopbody_ins_num = linear_program.block_pointer_list[end_pos - 1]->ins_pointer_list.size();

	if (unrolling_factor == 0) return added_block_num;


	TemporaryVar(start_pos, end_pos);
	// no added blocks
	if (loopbody_block_num == 1) {

		// try to merge instructions
		back_2.block_ins_num = 0;
		for (auto i : linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list) {
			CopyInstruction(&(back_2.block_ins[back_2.block_ins_num]), i);
			back_2.ins_pointer_list.push_back(&(back_2.block_ins[back_2.block_ins_num]));
			back_2.block_ins_num++;
		}
		//PrintDebug("Backup finished.\n");
		if (double_extension == 1 && MergeIns(linear_program.block_pointer_list[start_pos + 1])) {
			//PrintDebug("Begin double unrolling.\n");
			// unrolling the loop
			//PrintGlobalData();
			// 2 form
			if (unrolling_factor % 2 == 1) {

				int unroll_factor_d = (unrolling_factor - 1) / 2;
				int block_ins_num_d = linear_program.block_pointer_list[start_pos + 1]->block_ins_num;
				for (int j = 0; j < unroll_factor_d; j++) {
					for (int i = 0; i < block_ins_num_d - 1; i++) {
						Instruction* x = &(linear_program.block_pointer_list[start_pos + 1]->
							block_ins[linear_program.block_pointer_list[start_pos + 1]->block_ins_num]);
						CopyInstruction(x, linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list[i]);

						RenameVar(x, linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list[i], to_string(j));
						linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list.emplace(
							linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list.end() - 1, x);
						linear_program.block_pointer_list[start_pos + 1]->block_ins_num++;
					}
				}
			}
			// 2 + 1 form
			else
			{
				int unroll_factor_d = (unrolling_factor / 2) - 1;
				int block_ins_num_d = linear_program.block_pointer_list[start_pos + 1]->block_ins_num;
				for (int j = 0; j < unroll_factor_d; j++) {
					for (int i = 0; i < block_ins_num_d - 1; i++) {
						Instruction* x = &(linear_program.block_pointer_list[start_pos + 1]->
							block_ins[linear_program.block_pointer_list[start_pos + 1]->block_ins_num]);
						CopyInstruction(x, linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list[i]);
						RenameVar(x, linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list[i], to_string(j));
						linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list.emplace(
							linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list.end() - 1, x);
						linear_program.block_pointer_list[start_pos + 1]->block_ins_num++;
					}
				}
				// add 1 original block's instructions (except the final SBR)
				for (int i = 0; i < back_2.block_ins_num - 1; i++) {
					Instruction* x = &(linear_program.block_pointer_list[start_pos + 1]->
						block_ins[linear_program.block_pointer_list[start_pos + 1]->block_ins_num]);
					CopyInstruction(x, &back_2.block_ins[i]);
					RenameVar(x, &back_2.block_ins[i], to_string(unroll_factor_d));
					linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list.emplace(
						linear_program.block_pointer_list[start_pos + 1]->ins_pointer_list.end() - 1, x);
					linear_program.block_pointer_list[start_pos + 1]->block_ins_num++;
				}
			}

			return added_block_num;
		}


		back_2.type = BlockType::Default;
		back_2.label = "";
		back_2.loop_depth = -1;
		Instruction x = Instruction();
		for (int i = 0; i < back_2.block_ins_num; i++) {
			if (back_2.block_ins[i].ins_string != NULL)
				free(back_2.block_ins[i].ins_string);
			back_2.block_ins[i] = x;
		}
		back_2.block_ins_num = 0;
		//(linear_program.program_block[index].ins_pointer_list).swap(vector<Instruction*>() );
		vector <Instruction*>().swap(back_2.ins_pointer_list);
		// basic process for single block loop
		for (int i = 0; i < unrolling_factor; i++)
		{
			//PrintDebug("Begin unrolling.\n");
			Block* x = linear_program.block_pointer_list[start_pos + 1];
			for (int j = 0; j < last_loopbody_ins_num - 1; j++) {
				Instruction* z = &(x->block_ins[x->block_ins_num]);
				//GenerateInstr(x->block_ins[j].ins_string, z, -1);
				CopyInstruction(z, &(x->block_ins[j]));
				x->ins_pointer_list.emplace(x->ins_pointer_list.begin() + j, z);
				x->block_ins_num += 1;
				// instruction rename
				RenameVar(z, &(x->block_ins[j]), to_string(i));
			}

		}
		return added_block_num;
	}
	// will add new blocks
	else {
		for (int i = 0; i < unrolling_factor; i++) {
			for (int j = 1; j <= loopbody_block_num; j++) {
				// last block
				if (j == loopbody_block_num) {
					if (last_loopbody_ins_num == 1) continue;
					else {
						// first block of the next loop body
						Block* x = linear_program.block_pointer_list[start_pos + j];
						// last block of the original loop body
						Block* y = linear_program.block_pointer_list[end_pos - 1 + added_block_num];
						for (int k = 0; k < last_loopbody_ins_num - 1; k++) {
							Instruction* z = &(x->block_ins[x->block_ins_num]);
							//GenerateInstr(y->block_ins[k].ins_string, z, -1);
							CopyInstruction(z, &(y->block_ins[k]));
							x->ins_pointer_list.emplace(x->ins_pointer_list.begin() + k, z);
							x->block_ins_num += 1;
							// rename vars
							RenameVar(z, &(y->block_ins[k]), to_string(i));
						}

					}
				}
				// not last block
				else {
					// copy the block and stored into linear_program's block array
					Block* x = CopyBlockByIndex(start_pos + (j - 1) + j);
					// rename the copyed instructions
					Block* y = linear_program.block_pointer_list[start_pos + (j - 1) + j];
					for (int k = 0; k < x->block_ins_num; k++) {
						RenameVar(x->ins_pointer_list[k], y->ins_pointer_list[k], to_string(i));
					}
					// insert the new block into linear_program's block pointer list
					linear_program.block_pointer_list.emplace(
						linear_program.block_pointer_list.begin() + (start_pos + 1) + (j - 1), x);
					added_block_num++;
				}
			}
		}
	}
	//printf("added %d blocks\n", added_block_num);
	return added_block_num;
}

void UnrollingProgram(int double_extension)
{
	/*
	for (int i = 0; i < linear_program.block_pointer_list.size(); i++) {

		if (linear_program.block_pointer_list[i]->type == BlockType::LoopEnd) {
			// find unroll_factor
			int unrolling_factor = 0;
			for (int j = i; j >= 0; j--) {
				if (linear_program.block_pointer_list[j]->type == BlockType::LoopStart
					&& linear_program.block_pointer_list[j]->loop_depth == linear_program.block_pointer_list[i]->loop_depth) {
					unrolling_factor = atoi(linear_program.block_pointer_list[j]->ins_pointer_list[0]->input[1].c_str());
					break;
				}


			}
			i += UnrollingLoop(i, unrolling_factor, double_extension);
		}
	}
	*/
	PrintInfo("Loop optimization begins... \n");
	temp_var_set.clear();
	for (int i = 1; i < linear_program.block_pointer_list.size(); i++) {
		if (linear_program.block_pointer_list[i]->type == BlockType::LoopEnd) {

			bool most_inner_loop = false;
			int unrolling_factor = 0;
			int loop_trip = 0;
			int j = i;
			// verify if this is the .endloop of a most inner loop
			for (j = i - 1; j >= 0; j--) {
				if (linear_program.block_pointer_list[j]->type == BlockType::LoopStart) {
					if (linear_program.block_pointer_list[j]->loop_depth == linear_program.block_pointer_list[i]->loop_depth) {
						most_inner_loop = true;
						unrolling_factor = atoi(linear_program.block_pointer_list[j]->ins_pointer_list[0]->input[1].c_str());
						//if (unrolling_factor) {	// modified by DC 23/3/12
						//	printf("; In block %d, loop is unrolled with a factor of %d....\n", j, unrolling_factor);	
						//}
						loop_trip = atoi(linear_program.block_pointer_list[j]->ins_pointer_list[0]->input[2].c_str());
						break;
					}
				}
				else if (linear_program.block_pointer_list[j]->type == BlockType::LoopEnd) {
					break;
				}
			}
			string loop_label = linear_program.block_pointer_list[j]->label;
			// unrolling the most inner loop
			//PrintGlobalData();
			if (most_inner_loop == true && (UnrollChanceCheck(j, i, unrolling_factor, loop_trip, loop_label) == 0)) {
				//PrintInfo("%d, %d\n", j, i);
				i += UnrollingMostInnerLoop(j, i, unrolling_factor, double_extension);
				// if the loop is totally unrolled
				// remove the loop label and merge blocks
				if (loop_trip == (unrolling_factor + 1)) {
					RemoveLoop(j, i);
					PrintInfo("\t Loop %s is removed due to totally unrolling.\n", loop_label.c_str());
				}

			}
		}

	}
	//PrintDebug("Loop body of after optimization:\n");
	//PrintVariables();
	PrintInfo("Loop optimization finished. \n\n");
	//PrintGlobalData();

}

int UnrollChanceCheck(int start_pos, int end_pos, int unrolling_factor, int trip_num, string loop_name)
{
	int loop_ins_num = 0;
	for (int i = start_pos + 1; i < end_pos; i++) {
		loop_ins_num += linear_program.block_pointer_list[i]->block_ins_num;
	}

	if ((trip_num > 0) && (unrolling_factor > 0) && (trip_num < (unrolling_factor + 1))) {
		PrintWarn("%s (trip num %d) can not be unrolled %d times, this loop is not unrolled.\n",
			loop_name.c_str(), trip_num, unrolling_factor);
		return 1;
	}

	if (trip_num % (unrolling_factor + 1) != 0) {
		PrintWarn("%s (trip num %d) can not be unrolled %d times, this loop is not unrolled.\n",
			loop_name.c_str(), trip_num, unrolling_factor);
		return 2;
	}

	if ((loop_ins_num * (unrolling_factor + 1) > kMaxInsNumPerBlock) && (end_pos - start_pos == 2)) {
		PrintWarn("%s (%d instructions) can not be unrolled %d times due to block capacity, this loop is not unrolled.\n",
			loop_name.c_str(), loop_ins_num, unrolling_factor);
		return 3;
	}

	return 0;
}
