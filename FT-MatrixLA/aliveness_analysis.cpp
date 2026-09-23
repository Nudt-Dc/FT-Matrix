#include "utils.h"
#include "aliveness_analysis.h"
#include "software_pipeline.h"

map<int, int> fix_destination;
int flag_keep_alive;

static int block_num, cycle_num, unit_num;
static Block* current_block;
static vector<int> loop_start;


int CheckAliveVarOmission(const string* var, const int& cycle, const int& type) {
	vector<string*> &alive_var_vector = linear_program.block_pointer_list[block_num]->alive_chart[cycle][type];
	for (int i = 0; i < alive_var_vector.size(); ++i) {
		if (alive_var_vector[i] == var) {
			return 0;
		}
	}
	return 1;
}


void AddAliveVar(string* var, const int& cycle, int type) {
	if (CheckAliveVarOmission(var, cycle, type)) {
		linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].push_back(var);
		if (!var_map[*var].partner.empty()) {
			string* var2 = &var_map[var_map[*var].partner].name;
			if (!var_map.count(*var2)) {
				var_map[*var].partner = "";
				return;
			}
			type += 8;
			if (CheckAliveVarOmission(var, cycle, type)) {
				linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].push_back(var);
				linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].push_back(var2);
			}
		}
	}
}


void DeleteAliveVar(const string* var, const int& cycle, int type) {
	string* var2 = NULL;
	if (!var_map[*var].partner.empty()) var2 = &var_map[var_map[*var].partner].name;
	for (auto iter = linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].begin(); iter != linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].end();) {
		if (var2 != NULL && *iter == var2) {
			var2 = NULL;
		}
		if (*iter == var) {
			iter = linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].erase(iter);
			continue;
		}
		iter++;
	}
	if (var2 != NULL) {
		type += 8;
		for (auto iter = linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].begin(); iter != linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].end();) {
			if (*iter == var || *iter == var2) {
				iter = linear_program.block_pointer_list[block_num]->alive_chart[cycle][type].erase(iter);
				continue;
			}
			iter++;
		}
	}
}


void InitAliveChart() {
	int i;
	loop_start[block_num] = -1;
	cycle_num = current_block->queue_length;
	unit_num = 0;
	current_block->alive_chart.resize(cycle_num + 1, vector<vector<string*>>(kRegType));
	// make the variable need to return alive in the last block
	if (block_num == linear_program.block_pointer_list.size() - 1 && linear_program.var_out.size()) {
		current_block->alive_chart[cycle_num][0].push_back(&var_map[linear_program.var_out[0]].name);
		if (linear_program.var_out.size() == 2) {
			current_block->alive_chart[cycle_num][0].push_back(&var_map[linear_program.var_out[1]].name);
			current_block->alive_chart[cycle_num][8].push_back(&var_map[linear_program.var_out[0]].name);
			current_block->alive_chart[cycle_num][8].push_back(&var_map[linear_program.var_out[1]].name);
		}
	}
	// get the alive variables in the last cycle in the current block
	// block in order which means block have no SBR instruction
	if (current_block->sbr.ins_name[0] == '\0') {
		if (block_num != linear_program.block_pointer_list.size() - 1) {
			current_block->exit_block.push_back(block_num + 1);
			linear_program.block_pointer_list[block_num + 1]->entry_block.push_back(block_num);
			current_block->alive_chart[cycle_num] = linear_program.block_pointer_list[block_num + 1]->alive_chart[0];
		}
	}
	// block with a SBR instrucion
	else {
		for (i = 0; i < linear_program.block_pointer_list.size(); ++i) {
			if (linear_program.block_pointer_list[i]->label == current_block->sbr.input[0]) break;
		}
		if (i == linear_program.block_pointer_list.size()) {
			PrintError("The label \"%s\" used in the instruction of \"SBR\" in block \"%s\" has not been declared.\n", current_block->sbr.input[0].c_str(), current_block->label.c_str());
			Error();
		}
		if (i <= block_num) {
			//loop_start[block_num].push_back(i);
			loop_start[block_num] = i;
		}
		else {
			current_block->alive_chart[cycle_num] = linear_program.block_pointer_list[i]->alive_chart[0];
		}
		linear_program.block_pointer_list[i]->entry_block.push_back(block_num);
		current_block->exit_block.push_back(i);
		if (current_block->soft_pipeline) {
			current_block->sbr.ins_name[0] = '\0';
		}
		else if (!current_block->sbr.cond_field.empty()) {
			linear_program.block_pointer_list[block_num + 1]->entry_block.push_back(block_num);
			for (int type = 0; type < kRegType; ++type) {
				for (i = linear_program.block_pointer_list[block_num + 1]->alive_chart[0][type].size() - 1; i >= 0; --i) {
					AddAliveVar(linear_program.block_pointer_list[block_num + 1]->alive_chart[0][type][i], cycle_num, type);
				}
			}
			AddAliveVar(&var_map[current_block->sbr.cond_field].name, cycle_num, int(var_map[current_block->sbr.cond_field].reg_type));
		};
	}
	cycle_num--;
}


void UpdateVarAliveness(const string& var, const int& read_mode) {
	string* variable_name = &(var_map[var].name);
	int reg_type = int(var_map[var].reg_type);
	// it is the first time when the variable appear in this block
	if (!current_block->var_condition.count(variable_name)) {
		current_block->var_condition[variable_name][2] = read_mode;
		current_block->var_condition[variable_name][3] = read_mode;
		current_block->var_condition[variable_name][4] = cycle_num;
		current_block->var_condition[variable_name][read_mode] = cycle_num;
		if (read_mode) {
			current_block->var_condition[variable_name][0] = -1;
			AddAliveVar(variable_name, cycle_num, reg_type);
		}
		else {
			current_block->var_condition[variable_name][1] = -1;
			if (CheckAliveVarOmission(variable_name, cycle_num + 1, reg_type)) {
				for (int i = cycle_num + 1; i < current_block->alive_chart.size(); ++i) {
					AddAliveVar(variable_name, i, reg_type);
				}
			}
			else DeleteAliveVar(variable_name, cycle_num, reg_type);
		}
		return;
	}
	// the variable uesd to appear is to be read this cycle
	if (read_mode) {
		AddAliveVar(variable_name, cycle_num, reg_type);
	}
	// the variable uesd to appear is to be assigned this cycle
	else {
		// it was used to read last time
		if (current_block->var_condition[variable_name][2]) {
			if (cycle_num + current_block->ins_schedule_queue[cycle_num][unit_num]->cycle > current_block->var_condition[variable_name][1]) {
				for (int i = cycle_num; i < current_block->alive_chart.size(); ++i) {
					if (i <= cycle_num + current_block->ins_schedule_queue[cycle_num][unit_num]->cycle) {
						if (!CheckAliveVarOmission(variable_name, i, reg_type)) break;
					}
					AddAliveVar(variable_name, i, reg_type);
				}
				//return;
			}
			else DeleteAliveVar(variable_name, cycle_num, reg_type);
		}
		// it was use to assign last time
		else {
			if (CheckAliveVarOmission(variable_name, cycle_num + 1, reg_type)) {
				for (int i = cycle_num + 1; i <= current_block->var_condition[variable_name][0]; ++i) {
					AddAliveVar(variable_name, i, reg_type);
				}
			}
			DeleteAliveVar(variable_name, cycle_num, reg_type);
		}
	}
	current_block->var_condition[variable_name][2] = read_mode;
	current_block->var_condition[variable_name][read_mode] = cycle_num;
}


void SeparateVariable(const string &operand, int read_mode) {
	if (IsSpecialOperand(operand) != -1) return;

	string var = "";
	int i = 1;
	// memory access
	if (operand[0] == '*') {
		while (i < operand.size() && (operand[i] == '+' || operand[i] == '-')) ++i;
		for (; i < operand.size() && operand[i] != '+' && operand[i] != '-' && operand[i] != '['; ++i) var += operand[i];
		UpdateVarAliveness(var, read_mode);
		while (i < operand.size() && (operand[i] == '+' || operand[i] == '-')) ++i;
		if (i < operand.size() && operand[i] == '[') {
			i++;
			if (operand[i] >= '0' && operand[i] <= '9' || operand[i] == '-') return;
			var = "";
			for (; operand[i] != ']'; ++i) var += operand[i];
			UpdateVarAliveness(var, read_mode);
		}
		return;
	}
	// other form
	i = 0;
	for (; i < operand.size() && operand[i] != ':'; ++i) var += operand[i];
	UpdateVarAliveness(var, read_mode);
	if (i < operand.size() && operand[i] == ':') {
		i++;
		var = "";
		for (; i < operand.size(); ++i) var += operand[i];
		UpdateVarAliveness(var, read_mode);
	}
}


void CompleteFixVariables() {
	int reg_type, var_index;
	int i, j, jj;
	string* var_address;
	string var_name;
	for (i = 1; i < linear_program.block_pointer_list.size(); ++i) {
		// complete variable aliveness in loop
		if (loop_start[i] != -1) {
			vector<vector<string*>>& fix_var_chart = linear_program.block_pointer_list[loop_start[i]]->alive_chart[0];
			for (reg_type = 0; reg_type < kRegType; ++reg_type) {
				for (var_index = 0; var_index < fix_var_chart[reg_type].size(); ++var_index) {
					var_address = fix_var_chart[reg_type][var_index];
					for (block_num = i; block_num >= loop_start[i]; --block_num) {
						current_block = linear_program.block_pointer_list[block_num];
						if (!CheckAliveVarOmission(var_address, current_block->alive_chart.size() - 1, reg_type)) break;
						if (current_block->var_condition.count(var_address)) {
							for (cycle_num = current_block->var_condition[var_address][4] + 1; cycle_num < current_block->alive_chart.size(); ++cycle_num) {
								current_block->alive_chart[cycle_num][reg_type].push_back(var_address);
							}
							break;
						}
						for (cycle_num = 0; cycle_num < current_block->alive_chart.size(); ++cycle_num) {
							current_block->alive_chart[cycle_num][reg_type].push_back(var_address);
						}
					}
				}
			}
		}
		// get branches/blocks whose block_exits have the same block
		// get fix variables
		if (linear_program.block_pointer_list[i]->entry_block.size() > 1) {
			vector<vector<string*>>& fix_var_chart = linear_program.block_pointer_list[i]->alive_chart.back();
			for (reg_type = 0; reg_type < kRegType; ++reg_type) {
				for (int k = 0; k < fix_var_chart[reg_type].size(); ++k) {
					var_name = *fix_var_chart[reg_type][k];
					VarStorage store_info;
					for (j = 0; j < linear_program.block_pointer_list[i]->entry_block.size(); j++) {
						if (linear_program.block_pointer_list[linear_program.block_pointer_list[i]->entry_block[j]]->fix_var.count(var_name)) {
							store_info = linear_program.block_pointer_list[linear_program.block_pointer_list[i]->entry_block[j]]->fix_var[var_name];
							break;
						}
					}
					fix_destination[*linear_program.block_pointer_list[i]->entry_block.rbegin()] = i;
					for (j = *linear_program.block_pointer_list[i]->entry_block.rbegin() + 1; j <= linear_program.block_pointer_list[i]->entry_block[0]; j++) {
						linear_program.block_pointer_list[j]->fix_var[var_name] = store_info;
						fix_destination[j] = i;
					}
				}
			}
		}
	}
	// get maxiums of each type of variables alive
	for (block_num = linear_program.block_pointer_list.size() - 1; block_num >= 0; --block_num) {
		for (cycle_num = 0; cycle_num < linear_program.block_pointer_list[block_num]->alive_chart.size(); ++cycle_num) {
			for (reg_type = 0; reg_type < kRegType; ++reg_type) {
				if (linear_program.block_pointer_list[block_num]->alive_chart[cycle_num][reg_type].size() > linear_program.block_pointer_list[block_num]->max_var_alive[reg_type]) {
					linear_program.block_pointer_list[block_num]->max_var_alive[reg_type] = linear_program.block_pointer_list[block_num]->alive_chart[cycle_num][reg_type].size();
				}
				if (linear_program.block_pointer_list[block_num]->alive_chart[cycle_num][reg_type].size() > linear_program.max_var_alive[reg_type]) {
					linear_program.max_var_alive[reg_type] = linear_program.block_pointer_list[block_num]->alive_chart[cycle_num][reg_type].size();
				}
			}
		}
	}
}


void FixCycleVar() {
	for (block_num = 0; block_num < linear_program.block_pointer_list.size(); ++block_num) {
		map<string*, bool> var_exist[kRegType];
		for (auto iter = linear_program.block_pointer_list[block_num]->fix_var.begin(); iter != linear_program.block_pointer_list[block_num]->fix_var.end(); ++iter) {
			string* var = &var_map[iter->first].name;
			int reg_type = int(var_map[iter->first].reg_type);
			var_exist[reg_type][var] = false;
		}
		for (auto& alive_line : linear_program.block_pointer_list[block_num]->alive_chart) {
			for (int i = 0; i < kRegType; ++i) {
				for (string* var : alive_line[i]) {
					if (var_exist[i].count(var)) {
						var_exist[i][var] = true;
					}
				}
				for (auto& iter : var_exist[i]) {
					if (!iter.second) {
						alive_line[i].push_back(iter.first);
					}
					else iter.second = false;
				}
			}
		}
	}
}


void DebugPrintAliveChart() {
	printf("\nDebug Information: Alive Variables\n");
	int reg_type, cycle, i;
	for (block_num = 0; block_num < linear_program.block_pointer_list.size(); ++block_num) {
		printf("BLOCK %d (%s)\n", block_num, linear_program.block_pointer_list[block_num]->label.c_str());
		for (reg_type = 0; reg_type < kRegType; ++reg_type) {
			printf(" - Max Reg%d Alive: %d\n", reg_type, linear_program.block_pointer_list[block_num]->max_var_alive[reg_type]);
		}
		printf("\n");
		for (cycle = 0; cycle < linear_program.block_pointer_list[block_num]->alive_chart.size(); ++cycle) {
			printf("[Block %d, Cycle %d]\n", block_num, cycle);
			//if (cycle < linear_program.block_pointer_list[block_num]->alive_chart.size() - 1 
			//	&& linear_program.block_pointer_list[block_num]->ins_schedule_queue[cycle][4] != NULL
			//	&& !strcmp(linear_program.block_pointer_list[block_num]->ins_schedule_queue[cycle][4]->ins_name, "SNOP")) {
			//	printf(" - SNOP PASS\n");
			//	continue;
			//}
			for (reg_type = 0; reg_type < kRegType; ++reg_type) {
				printf(" - Reg%d: ", reg_type);
				for (i = 0; i < linear_program.block_pointer_list[block_num]->alive_chart[cycle][reg_type].size(); ++i)
					printf("%s ", linear_program.block_pointer_list[block_num]->alive_chart[cycle][reg_type][i]->c_str());
				printf("\n");
			}
		}
		printf("\n\n");
	}
}


void DebugPrintFixVariables() {
	printf("\nDebug Information: Fix Variables\n");
	for (block_num = 0; block_num < linear_program.block_pointer_list.size(); ++block_num) {
		printf("Block %d:", block_num);
		for (auto iter = linear_program.block_pointer_list[block_num]->fix_var.begin(); iter != linear_program.block_pointer_list[block_num]->fix_var.end(); ++iter) {
			printf("%s, ", iter->first.c_str());
		}
		printf("\n");
	}
	printf("\n\n");
}


void PrintAliveChart() {
	int reg_type, cycle, i = 0, n = var_map.size();
	unordered_map<string*, int> var_index;
	FILE* fpout = fopen("aliveness.csv", "w");
	for (auto& it : var_map) {
		fprintf(fpout, ",");
		fprintf(fpout, it.first.c_str());
		var_index[&it.second.name] = i++;
	}
	i = 0;
	for (block_num = 0; block_num < linear_program.block_pointer_list.size(); ++block_num) {
		if (!linear_program.block_pointer_list[block_num]->ins_schedule_queue.size()) continue;
		current_block = linear_program.block_pointer_list[block_num];
		fprintf(fpout, "\n");
		for (cycle = 0; cycle < linear_program.block_pointer_list[block_num]->queue_length; ++cycle) {
			vector<int> alive_list(n);
			fprintf(fpout, "\n");
			fprintf(fpout, to_string(i++).c_str());
			for (reg_type = 0; reg_type < kRegType; ++reg_type) {
				for (int j = 0; j < current_block->alive_chart[cycle][reg_type].size(); ++j)
					alive_list[var_index[current_block->alive_chart[cycle][reg_type][j]]] = 1;
			}
			Instruction* current_ins;
			for (unit_num = 0; unit_num < kFuncUnitAmount; ++unit_num) {
				current_ins = current_block->ins_schedule_queue[cycle][unit_num];
				if (current_ins == NULL) continue;
				for (string operand : current_ins->input) {
					for (string var : SeperateVar(operand, 1, 1, 0)) {
						alive_list[var_index[&(var_map[var].name)]] = 3;
					}
				}
				if (!current_ins->cond_field.empty()) alive_list[var_index[&(var_map[current_ins->cond_field].name)]] = 3;
				if (!current_ins->output.empty()) {
					if (current_ins->ins_name[1] == 'S' && current_ins->ins_name[2] == 'T') {
						for (string var : SeperateVar(current_ins->output, 1, 1, 0)) {
							alive_list[var_index[&(var_map[var].name)]] = 3;
						}
					}
					else if (!current_ins->cond_field.empty()) {
						for (string var : SeperateVar(current_ins->output, 1, 1, 0)) {
							alive_list[var_index[&(var_map[var].name)]] = 3;
						}
					}
					else {
						for (string var : SeperateVar(current_ins->output, 1, 1, 0)) {
							alive_list[var_index[&(var_map[var].name)]] = 2;
						}
					}
				}
			}
			for (int b : alive_list) {
				fprintf(fpout, ",");
				switch (b) {
				case 1:
					fprintf(fpout, "|");
					break;
				case 2:
					fprintf(fpout, "o");
					break;
				case 3:
					fprintf(fpout, "x");
					break;
				}
			}
		}
		//i--;
	}
	fclose(fpout);
}

void AnalyseAliveness(const int fix_mode) {
	PrintInfo("Analysis of variable aliveness starts...\n");

	fix_destination.clear();
	loop_start.clear();


	int snop_end, output_mode[kFuncUnitAmount];
	Instruction* current_ins;

	loop_start.resize(linear_program.block_pointer_list.size());
	flag_keep_alive = 0;

	for (block_num = linear_program.block_pointer_list.size() - 1; block_num >= 0; block_num--) {
		current_block = linear_program.block_pointer_list[block_num];
		if (current_block->type == BlockType::Close) {
			flag_keep_alive = 0;
		}
		else if (current_block->type == BlockType::Open) {
			flag_keep_alive = 1;
		}
		InitAliveChart();
		snop_end = -1;
		for (; cycle_num >= 0; --cycle_num) {
			//// skip cycles with SNOP
			//if (current_block->ins_schedule_queue[cycle_num][4] != NULL
			//	&& !strcmp(current_block->ins_schedule_queue[cycle_num][4]->ins_name, "SNOP")) {
			//	if (snop_end == -1) snop_end = cycle_num + 1;
			//	current_block->alive_chart[cycle_num] = current_block->alive_chart[cycle_num + 1];
			//	continue;
			//}
			//if (snop_end != -1) {
			//	current_block->alive_chart[cycle_num + 1] = current_block->alive_chart[snop_end];
			//	snop_end = -1;
			//}
			// get the alive variable list from the last cycle
			current_block->alive_chart[cycle_num] = current_block->alive_chart[cycle_num + 1];
			// make the variables which are really assigned alive
			for (unit_num = 0; unit_num < kFuncUnitAmount; ++unit_num) {
				current_ins = current_block->ins_schedule_queue[cycle_num][unit_num];
				output_mode[unit_num] = 0;
				if (current_ins == NULL) continue;
				// detect the usage mode of the output
				if (!current_ins->output.empty()) {
					if (flag_keep_alive) {
						output_mode[unit_num] = 1;
					}
					else if (current_ins->ins_name[1] == 'S' && current_ins->ins_name[2] == 'T') {
						output_mode[unit_num] = 1;
					}
					else if (cycle_num != 0 && current_ins == current_block->ins_schedule_queue[cycle_num - 1][unit_num]) continue;
					else if (!current_ins->cond_field.empty()) {
						output_mode[unit_num] = 1;
					}
					else SeparateVariable(current_ins->output, 0);
				}
			}
			for (unit_num = 0; unit_num < kFuncUnitAmount; ++unit_num) {
				current_ins = linear_program.block_pointer_list[block_num]->ins_schedule_queue[cycle_num][unit_num];
				if (current_ins == NULL) continue;
				if (!current_ins->cond_field.empty()) SeparateVariable(current_ins->cond_field);
				if (!current_ins->input[0].empty()) SeparateVariable(current_ins->input[0]);
				if (!current_ins->input[1].empty()) SeparateVariable(current_ins->input[1]);
				if (!current_ins->input[2].empty()) SeparateVariable(current_ins->input[2]);
				if (output_mode[unit_num]) SeparateVariable(current_ins->output);
			}
		}
	}

	CompleteFixVariables();

	if (fix_mode) FixCycleVar();

	//PrintAliveChart();

	//DebugPrintAliveChart();
	//DebugPrintFixVariables();

	PrintInfo("Analysis of variable aliveness done.\n\n\n");
}