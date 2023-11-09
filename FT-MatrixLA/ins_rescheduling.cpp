//------------------- Instruction Rescheduling--------------------
//----------------Reschedule Instructions in the output-----------
//------------------------modified by DC--------------------------

#include "utils.h"
#include "register_assignment.h"
#include "ins_scheduling.h"
#include "ins_rescheduling.h"
#include "data_structure.h"
#include "build_dependency.h"
#include <cstdio>


using namespace std;

string reschedule = "";


void Rescheduling() {
	
	reschedule = "OPEN";

	string transition_file_2 = ".\\test_transition_2.txt";
	string graph_file_2 = ".\\graph_2.txt";
	
	if (reschedule == "CLOSE") return;

	MoveOutputToInput();
	
	BuildDependency();
	PrintDependencyGraph(graph_file_2.c_str());

	// reschedule instructions
	for (int block_num = 0; block_num < linear_program.total_block_num; block_num++) {
		if (linear_program.block_pointer_list[block_num]->type == BlockType::Instruction) {
			O1SchedulingDP(linear_program.block_pointer_list[block_num]);	//modified by DC
		}
	}

	PrintSchedulingTransistion(transition_file_2.c_str());

	GenerateOutput();
}



void MoveOutputToInput() {
	
	Block* current_block;
	Instruction* ins;

	for (int block_num = 0; block_num < linear_program.block_pointer_list.size(); ++block_num) {
		current_block = linear_program.block_pointer_list[block_num];
		Block* block;
		block = new Block;

		if (current_block->type == BlockType::Instruction) {
			for (int cycle = 0; cycle < ins_output[block_num].size(); cycle++) {
				map<string, vector<Instruction*>> reg_read, reg_write;
				vector<Instruction*> ins_buf;
				for (int func = 0; func < ins_output[block_num][cycle].size(); func++) {
					ins = ins_output[block_num][cycle][func];
					if (ins == NULL) {
						continue;
					}
					else if (string(ins->ins_name) == "SNOP") {
						continue;
					}
					if (ins->note != "") {
						ins->type = InsType::ReferIns;
					}
					ins->state = 0;
					ins->chl.clear();
					ins->indeg = 0;
					//ins->extracted_var.clear();
					ins->last_fa_end_time = 0;

					block->ins_pointer_list.push_back(ins);

				}
			}

			for (int i = 0; i < block->ins_pointer_list.size(); i++) {
				block->block_ins[i] = *block->ins_pointer_list[i];
			}
			block->block_ins_num = block->ins_pointer_list.size();
			block->block_index = block_num;
			block->type = current_block->type;
			linear_program.block_pointer_list[block_num] = block;
		}
		else {
			current_block->block_ins_num = current_block->ins_pointer_list.size();
		}
	}
}

void GenerateOutput() {

	// move ins_schedule_queue to ins_output
	for (int i = 0; i < linear_program.total_block_num; i++) {
		if (linear_program.block_pointer_list[i]->type == BlockType::Instruction) {
			ins_output[i].clear();
			for (int j = 0; j < linear_program.block_pointer_list[i]->queue_length; j++) {
				Instruction* current_ins, * last_ins;
				//bool flag = false;
				for (int k = 0; k < linear_program.block_pointer_list[i]->ins_schedule_queue[j].size(); k++) {
					current_ins = linear_program.block_pointer_list[i]->ins_schedule_queue[j][k];
					if (current_ins == NULL)	continue;
					if (string(current_ins->ins_name) == "SNOP")	break;
					if (j > 0) {
						last_ins = linear_program.block_pointer_list[i]->ins_schedule_queue[j - 1][k];
						if (last_ins == current_ins) linear_program.block_pointer_list[i]->ins_schedule_queue[j][k] = NULL;
					}
				}
				ins_output[i].push_back(linear_program.block_pointer_list[i]->ins_schedule_queue[j]);
			}
		}
	}
}

