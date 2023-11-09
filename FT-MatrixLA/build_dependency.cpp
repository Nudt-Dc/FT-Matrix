//------------------- Building Dependency for the Linear Program--------------------
// Including read after write, write after write, write after read, mem dependency

#include "data_structure.h"
#include "analyser.h"
#include "build_dependency.h"
#include "utils.h"
#include <fstream>
#include <string>
#include <iostream>
#include <algorithm>
#include "ins_rescheduling.h"

using namespace std;

// Split variable pairs into variables 
VarList* SplitVar(string& var) {
	int pre_pos = 0, colon_pos = -1;
	VarList* node = new VarList;
	VarList* head = node;
	while ((colon_pos = var.find(':', pre_pos)) != var.npos) {
		node->var.name = var.substr(pre_pos, colon_pos - pre_pos);
		//node->var.type = var.type;
		node->next = new VarList;
		node = node->next;
		pre_pos = colon_pos + 1;
	}
	node->var.name = var.substr(pre_pos, var.length() - pre_pos);
	//node->var.type = var.type;
	node->next = NULL;
	return head;
}

// Judging that the variables are equal, including variables pairs a1:a0 and *AR++[OR]
bool RegEqual(string& str_conj, string& str) {
	// str_conj may be variables pairs or memory access, str is a single variable
	string var1 = "", var2 = "";
	int i = 0;

	while (str_conj[i] == '*' || str_conj[i] == '+' || str_conj[i] == '-') {
		++i;
	}
	for (; str_conj[i] != '+' && str_conj[i] != '-' && str_conj[i] != '[' && str_conj[i] != ':' && i < str_conj.size(); ++i) {
		var1 += str_conj[i];
	}
	if (i < str_conj.size()) {
		while (str_conj[i] == '[' || str_conj[i] == '+' || str_conj[i] == '-' || str_conj[i] == ':') {
			++i;
		}
		for (; str_conj[i] != ']' && i < str_conj.size(); ++i) {
			var2 += str_conj[i];
		}
	}
	return (str == var1 || str == var2);
}


// Determine whether the two mem_info are dependent
// Only AR equal, OR not variables, offset not equal -->> false, no depencence 
bool MemRely(const MemInfo& fa, const MemInfo& chl) {
	if (fa.AR == chl.AR) {
		if (isalpha(chl.OR[0]) || isalpha(fa.OR[0])) return true;
		else if (fa.AR_offset == chl.AR_offset) return true;
		else return false;
	}
	else return false;
}

// Build Dependency for linear program
// From main -> build_dependency
void BuildDependency() {
	PrintInfo("Build Dependency starts...\n");
	for (int i = 0; i < linear_program.total_block_num; i++) {
		if (linear_program.block_pointer_list[i]->type != BlockType::Instruction) continue;
		for (int j = 0; j < linear_program.block_pointer_list[i]->block_ins_num; j++) {
			if (linear_program.block_pointer_list[i]->ins_pointer_list[j]->type == InsType::Default) continue;
			BuildInstrDependency(linear_program.block_pointer_list[i], linear_program.block_pointer_list[i]->ins_pointer_list[j]);
		}
	}
	PrintInfo("Build Dependency is successfully finished...\n\n");
}

// Build instruction dependency for each instruction in each block
// Including read_dependency, write_dependency, mem_dependency
void BuildInstrDependency(Block* b, Instruction* x) {
	if (x->cond_field != "") {
		VarList* cond;
		cond = SplitVar(x->cond_field);
		BuildReadDependency(b, x, cond);
		cond->free();
	}

	VarList* input[3];
	if (x->input[0] != "") {
		input[0] = SplitVar(x->input[0]);
		BuildReadDependency(b, x, input[0]);
		input[0]->free();
	}
	if (x->input[1] != "") {
		input[1] = SplitVar(x->input[1]);
		BuildReadDependency(b, x, input[1]);
		input[1]->free();
	}
	if (x->input[2] != "") {
		input[2] = SplitVar(x->input[2]);
		BuildReadDependency(b, x, input[2]);
		input[2]->free();
	}

	if (x->output != "") {
		VarList* output;
		output = SplitVar(x->output);
		BuildWriteDependency(b, x, output);
		output->free();
	}
	return;
}

// Build read dependency
// Read after write
void BuildReadDependency(Block* block, Instruction* x, VarList* vl) {
	if (vl->var.name[0] != '*') {
		while (vl != NULL) {
			string var_name = vl->var.name;
			vector <string>::iterator s = find(reg_ctrl[0].begin(), reg_ctrl[0].end(), var_name);
			if (s != reg_ctrl[0].end() && var_name.substr(0, 3) == "SVR") {
				var_name = "SVR";
			}
			if (var_name[0] == '-' || isdigit(var_name[0])) return;
		
			Instruction* ptr = block->reg_written[var_name];
			if (ptr == NULL) {
				// Construct virtual instructions 
				Instruction vptr;
				vptr.output = var_name;
				vptr.chl.push_back(make_pair(x, vptr.cycle));
				x->indeg++;
				if (block->block_ins_num >= kMaxInsNumPerBlock) {
					PrintError("The basic block contains too many instructions (max %d)\n", kMaxInsNumPerBlock);
					Error();
				}
				block->block_ins[block->block_ins_num] = vptr;
				(block->ins_pointer_list).push_back(&block->block_ins[block->block_ins_num]);
				block->reg_written[var_name] = &block->block_ins[block->block_ins_num];
				block->block_ins_num += 1;
			}
			else {
				ptr->chl.push_back(make_pair(x, ptr->cycle));
				x->indeg++;
			}
			vl = vl->next;
		}
	}
	else {
		BuildMemDependency(block, x, vl, 1);
	}
}

// Build write dependency
// Write after write, Write after read
void BuildWriteDependency(Block* block, Instruction* x, VarList* vl) {
	//--------------------------------------
	int buf;
	//--------------------------------------
	if (vl->var.name[0] != '*') {
		while (vl != NULL) {
			string var_name = vl->var.name;
			vector <string>::iterator s = find(reg_ctrl[0].begin(), reg_ctrl[0].end(), var_name);
			if (s != reg_ctrl[0].end() && var_name.substr(0, 3) == "SVR") {
				var_name = "SVR";
			}
			if (var_name[0] == '-' || isdigit(var_name[0])) return;
			Instruction* ptr = block->reg_written[var_name];
			if (ptr == NULL || ptr->cycle == 0) {
				block->reg_root[var_name] = x;
			}
			if (ptr != NULL) {
				//--------------------------------------
				buf = ptr->cycle - x->cycle + x->write_cycle;
				if (reschedule == "CLOSE") {
					if (buf < 0) buf = 0;
				}
				//if (buf < 0) buf = 0;
				ptr->chl.push_back(make_pair(x, buf));
				//--------------------------------------
				//ptr->chl.push_back(make_pair(x, ptr->cycle - x->cycle + x->w_cycle));
				x->indeg++;
				for (int i = 0; i < (ptr->chl).size(); i++) {
					Instruction* child = (ptr->chl[i]).first;
					if (x == child) {
						continue;
					}
					if (RegEqual(child->cond_field, var_name) || RegEqual(child->input[0], var_name) || RegEqual(child->input[1], var_name) || RegEqual(child->input[2], var_name) || (child->output[0] == '*' && RegEqual(child->output, var_name))) {
						//--------------------------------------
						buf = child->read_cycle - x->cycle + x->write_cycle - 1;
						//--------------------------//
						//ÐÞ¸Äbuf
						if (reschedule == "CLOSE") {
							if (buf <= 0) buf = 1;
						}
						
						child->chl.push_back(make_pair(x, buf));
						//--------------------------------------
						//child->chl.push_back(make_pair(x, child->r_cycle - x->cycle + x->w_cycle - 1));
						x->indeg++;
					}
				}
			}
			block->reg_written[var_name] = x;
			vl = vl->next;
		}
	}
	else {
		BuildMemDependency(block, x, vl, 0);
	}
}

// Build mem dependency
// including reg dependency analysis(AR, OR) and mem dependency (mem_read, mem_written)
void BuildMemDependency(Block* block, Instruction* x, VarList* vl, int load_store) {
	string AR, OR;
	int i, ii, pos, offset, inum;
	bool self_change, pre_change = false;
	//--------------------------------------
	int buf;
	//--------------------------------------

	// judge AR self_change
	pos = max(FindChar(vl->var.name, '-'), FindChar(vl->var.name, '+'));
	if (pos == -1) self_change = false;
	else if ((vl->var.name)[pos + 1] == '+' || (vl->var.name)[pos + 1] == '-') self_change = true;
	else self_change = false;

	// get AR, OR and offset
	if (pos == -1) {
		AR = (vl->var.name).substr(1);
		OR = "";
		offset = block->reg_offset[AR];
	}
	else if (self_change == true) {
		if (pos == 1) {
			i = (vl->var.name).find('[');
			if (i != -1) {
				AR = (vl->var.name).substr(pos + 2, i - pos - 2);
				OR = (vl->var.name).substr(i + 1, (vl->var.name).length() - i - 2);
				pre_change = true;
			}
			else {
				AR = (vl->var.name).substr(pos + 2, vl->var.name.length() - pos - 2);
				OR = "1";
				pre_change = true;
			}
		}
		else {
			AR = (vl->var.name).substr(1, pos - 1);
			i = (vl->var.name).find('[');
			if (i != -1) {
				OR = (vl->var.name).substr(pos + 3, (vl->var.name).length() - pos - 4);
			}
			else
				OR = "1";
			pre_change = false;
		}
	}
	else {
		i = (vl->var.name).find('[');
		if (pos == 1) {
			if (i != -1) {
				AR = (vl->var.name).substr(2, i - 2);
				OR = (vl->var.name).substr(i + 1, (vl->var.name).length() - i - 2);
			}
			else {
				AR = (vl->var.name).substr(2, (vl->var.name).length() - 2);
				OR = "";
			}
		}
		else {
			AR = (vl->var.name).substr(1, pos - 1);
			i = (vl->var.name).find('[');
			if (i != -1) {
				OR = (vl->var.name).substr(i + 1, (vl->var.name).length() - i - 2);
			}
			else {
				OR = "";
			}
		}		
	}

	if (isdigit(OR[0]) || OR[0] == '-') {
		offset = block->reg_offset[AR];
		inum = stoll(OR);
		offset += (vl->var.name)[1] == '+' ? inum : 0;
		offset -= (vl->var.name)[1] == '-' ? inum : 0;
	}
	else offset = 0;

	// fresh reg_offset
	if (self_change && pre_change == true)
		block->reg_offset[AR] = offset;

	/*
	// all mem_read or mem_write Instr should be placed after last AR change Instr
	Instr* last_AR_change = this->up_AR_change[AR];
	if (last_AR_change != NULL) {
//		int last_cycle = last_AR_change->cycle;
//		if (!strcmp(last_AR_change->func_unit, "SLDST") || !strcmp(last_AR_change->func_unit, "VLDST")) last_cycle = 1;
		if (load_store) {
			last_AR_change->chl.push_back(make_pair(x, last_AR_change->cycle));
			x->indeg++;
		}
		else {
			last_AR_change->chl.push_back(make_pair(x, last_AR_change->cycle - x->cycle + x->w_cycle));
			x->indeg++;
		}
	}*/


	if (!isdigit(OR[0])) {
		//this->reg_read[res] = x;
		Instruction* ptr = block->reg_written[OR];
		if (ptr == NULL) {
			Instruction vptr;
			vptr.output = OR;
			vptr.chl.push_back(make_pair(x, vptr.cycle));
			x->indeg++;
			if (block->block_ins_num >= kMaxInsNumPerBlock) {
				PrintError("The basic block contains too many instructions (max %d)\n", kMaxInsNumPerBlock);
				Error();
			}
			block->block_ins[block->block_ins_num] = vptr;
			(block->ins_pointer_list).push_back(&block->block_ins[block->block_ins_num]);
			block->reg_written[OR] = &block->block_ins[block->block_ins_num];
			block->block_ins_num += 1;
		}
		else {
			ptr->chl.push_back(make_pair(x, ptr->cycle));
			x->indeg++;
		}

		//if (self_change)
		//	this->up_AR_change[AR] = x;
	}

	//reg_info resA(AR);
	if (self_change == false) {
		//this->reg_read[res] = x;
		Instruction* ptr = block->reg_written[AR];
		if (ptr == NULL) {
			Instruction vptr;
			vptr.output = AR;
			vptr.chl.push_back(make_pair(x, vptr.cycle));
			x->indeg++;
			if (block->block_ins_num >= kMaxInsNumPerBlock) {
				PrintError("The basic block contains too many instructions (max %d)\n", kMaxInsNumPerBlock);
				Error();
			}
			block->block_ins[block->block_ins_num] = vptr;
			(block->ins_pointer_list).push_back(&block->block_ins[block->block_ins_num]);
			block->reg_written[AR] = &block->block_ins[block->block_ins_num];
			block->block_ins_num += 1;
		}
		else {
			int last_cycle = ptr->cycle;
			if (strstr(ptr->func_unit, "SLDST")!=NULL || strstr(ptr->func_unit, "VLDST")!=NULL) last_cycle = 1;
			ptr->chl.push_back(make_pair(x, last_cycle));
			x->indeg++;
		}
	}
	else {
		Instruction* ptr = block->reg_written[AR];
		if (ptr == NULL || ptr->cycle == 0) {
			block->reg_root[AR] = x;
		}
		if (ptr != NULL) {
			int last_cycle = ptr->cycle;
			if (strstr(ptr->func_unit, "SLDST") != NULL || strstr(ptr->func_unit, "VLDST") != NULL) last_cycle = 1;
			ptr->chl.push_back(make_pair(x, last_cycle));
			x->indeg++;
			for (i = 0; i < (ptr->chl).size(); i++) {
				Instruction* child = (ptr->chl[i]).first;
				if (child == x) continue;
				if (RegEqual(child->input[0], AR) || RegEqual(child->input[1], AR) || RegEqual(child->input[2], AR) || RegEqual(child->output, AR)) {
					//--------------------------------------
					buf = child->read_cycle - 1;
					//------------------------------------//
					//ÐÞ¸Äbuf
					if (reschedule == "CLOSE") {
						if (buf <= 0) buf = 1;
					}
					
					child->chl.push_back(make_pair(x, buf));
					//--------------------------------------
					//child->chl.push_back(make_pair(x, child->r_cycle - 1));
					x->indeg++;
				}
			}
		}
		block->reg_written[AR] = x;
	}

	// pack this Instr
	MemInfo res;
	res.AR = AR;
	res.OR = OR;
	res.AR_offset = offset;
	for (map<MemInfo, Instruction*, MemInfoCMP>::iterator it = block->mem_written.begin(); it != block->mem_written.end();) {
		if (it->second != NULL && MemRely(it->first, res)) {
			((it->second)->chl).push_back(make_pair(x, it->second->cycle));
			x->indeg++;
			(block->mem_written).erase(it++);
		}
		else {
			it++;
		}
	}

	if (load_store) block->mem_read[res] = x;
	//read after write; write after write
	else {
		for (map<MemInfo, Instruction*, MemInfoCMP>::iterator it = block->mem_read.begin(); it != block->mem_read.end();) {
			if (it->second != NULL && MemRely(it->first, res)) {
				//--------------------------------------
				buf = it->second->read_cycle - x->cycle + x->write_cycle;
				//------------------------------------//
				//ÐÞ¸Äbuf
				if (reschedule == "CLOSE") {
					if (buf <= 0) buf = 1;
				}
				
				((it->second)->chl).push_back(make_pair(x, buf));
				//--------------------------------------
				//((it->second)->chl).push_back(make_pair(x, it->second->r_cycle - x->cycle + x->w_cycle - 1));
				(block->mem_read).erase(it++);
				x->indeg++;
			}
			else {
				it++;
			}
		}
		block->mem_written[res] = x;
	}
}

// Print Instruction Dependency File graph.txt
void PrintDependencyGraph(const char* graph_file) {
	FILE* fpgraph = NULL;
	fpgraph = fopen(graph_file, "w");
	if (fpgraph == NULL) {
		PrintError("The graph file \"%s\" can\'t open.\n", graph_file);
		Error();
	}

	for (int i = 0; i < linear_program.total_block_num; i++) {

		Instruction* x;
		int id = 0;

		for (int j = 0, len = linear_program.block_pointer_list[i]->block_ins_num; j < len; j++) {
			x = linear_program.block_pointer_list[i]->ins_pointer_list[j];
			if (x->cycle == 0) continue;
			x->last_fa_end_time = id++;
		}

		for (int j = 0, len = linear_program.block_pointer_list[i]->block_ins_num; j < len; j++) {
			x = linear_program.block_pointer_list[i]->ins_pointer_list[j];
			if (x->cycle == 0) continue;
			fprintf(fpgraph, "%d:%s\t%s\t%s\t%s\t%s\t%s \nchildren: ", x->last_fa_end_time, x->cond_field.c_str(), 
				x->ins_name, x->input[0].c_str(), x->input[1].c_str(),x->input[2].c_str(), x->output.c_str());
			for (int k = 0, len2 = x->chl.size(); k < len2; k++) {
				fprintf(fpgraph, "(%d, %d) ", x->chl[k].first->last_fa_end_time, x->chl[k].second);
			}
			fprintf(fpgraph, "\n");
		}
		for (int j = 0, len = linear_program.block_pointer_list[i]->block_ins_num; j < len; j++) {
			x = linear_program.block_pointer_list[i]->ins_pointer_list[j];
			if (x->cycle == 0) continue;
			x->last_fa_end_time = 0;
		}
		fprintf(fpgraph, "\n");
	}
	fclose(fpgraph);
	PrintInfo("Graph file \"%s\" is successfully generated. \n\n", graph_file);
}