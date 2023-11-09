#include "data_structure.h"
using namespace std;

Program linear_program;
ReferTable refer_table;
unordered_map<string, Variable> var_map;
int core = -1;
unordered_map<string, int> function_units_index;
vector<string> reg_ctrl[2];
vector<int> reg_amount;
vector<vector<int>> reg_res;
vector<int> reg_in;
int reg_out = -1;
int reg_return = -1;
int scalar_stack_AR = -1;
int vector_stack_AR = -1;
int sv_interval = -1;

// return refer_index of ins_name
int GetReferIns(const char* ins_name)
{
	int i = 0;
	for (; i < refer_table.table.size(); i++)
		if (!strcmp(ins_name, refer_table.table[i].name))
			return i;
	return i;
}

// free the space in linear_program
void CleanLinearProgram()
{
	for (int i = 0; i < linear_program.total_block_num; i++) {
		for (int j = 0; j < linear_program.program_block[i].block_ins_num; j++) {
			// free(linear_program.program_block[i].block_ins[j].ins_string);
			// printf("%s\n", linear_program.block_vector[i].ins_vector[j].ins_string);

			linear_program.program_block[i].block_ins[j].type = InsType::Default;
			linear_program.program_block[i].block_ins[j].ins_string = NULL;
			linear_program.program_block[i].block_ins[j].ins_name[0] = '\0';
			linear_program.program_block[i].block_ins[j].func_unit[0] = '\0';
			linear_program.program_block[i].block_ins[j].cond_field = "";
			linear_program.program_block[i].block_ins[j].cond_flag = 0;
			linear_program.program_block[i].block_ins[j].input[0] = "";
			linear_program.program_block[i].block_ins[j].input[1] = "";
			linear_program.program_block[i].block_ins[j].input[2] = "";
			linear_program.program_block[i].block_ins[j].output = "";
			linear_program.program_block[i].block_ins[j].note = "";
			linear_program.program_block[i].block_ins[j].cycle = linear_program.program_block[i].block_ins[j].read_cycle = linear_program.program_block[i].block_ins[j].write_cycle = 0;
			linear_program.program_block[i].block_ins[j].indeg = linear_program.program_block[i].block_ins[j].last_fa_end_time = 0; // modified by CZY
			linear_program.program_block[i].block_ins[j].site = -1;
			linear_program.program_block[i].block_ins[j].state = 0;
			linear_program.program_block[i].block_ins[j].partner = NULL;
			linear_program.program_block[i].block_ins[j].chl.clear();
			linear_program.program_block[i].block_ins[j].ex_loop.clear();
		}

		linear_program.program_block[i].type = BlockType::Default;
		linear_program.program_block[i].block_index = -1;
		linear_program.program_block[i].block_ins_num = 0;
		linear_program.program_block[i].loop_depth = 0;
		linear_program.program_block[i].label = "";
		for (int k = 0; k < kRegType; ++k) {
			linear_program.program_block[i].max_var_alive[k] = 0;
		}
		linear_program.program_block[i].queue_length = 0; // modified by CZY
		linear_program.program_block[i].LT_queue_length = 0;
		linear_program.program_block[i].soft_pipe_cycle = 0;
		linear_program.program_block[i].soft_pipeline = 0;
		linear_program.program_block[i].ins_pointer_list.clear();

		linear_program.program_block[i].ins_schedule_queue.clear();
		linear_program.program_block[i].ins_schedule_queue_write.clear();
		linear_program.program_block[i].module_queue_r.clear();
		linear_program.program_block[i].module_queue_w.clear();
		linear_program.program_block[i].LT_queue_r.clear();
		linear_program.program_block[i].LT_queue_w.clear();

		while (!linear_program.program_block[i].zero_indeg_instr.empty()) {
			linear_program.program_block[i].zero_indeg_instr.pop();
		}
		while (!linear_program.program_block[i].soft_instr_queue.empty()) {
			linear_program.program_block[i].soft_instr_queue.pop();
		}



		linear_program.program_block[i].reg_written.clear();
		linear_program.program_block[i].reg_root.clear();
		linear_program.program_block[i].reg_offset.clear();
		linear_program.program_block[i].mem_written.clear();
		linear_program.program_block[i].mem_read.clear();

		linear_program.program_block[i].alive_chart.clear();
		linear_program.program_block[i].var_condition.clear();
		linear_program.program_block[i].fix_var.clear();

		linear_program.program_block[i].entry_block.clear();
		linear_program.program_block[i].exit_block.clear();

		linear_program.program_block[i].sbr.type = InsType::Default;
		linear_program.program_block[i].sbr.ins_string = NULL;
		linear_program.program_block[i].sbr.ins_name[0] = '\0';
		linear_program.program_block[i].sbr.func_unit[0] = '\0';
		linear_program.program_block[i].sbr.cond_field = "";
		linear_program.program_block[i].sbr.cond_flag = 0;
		linear_program.program_block[i].sbr.input[0] = "";
		linear_program.program_block[i].sbr.input[1] = "";
		linear_program.program_block[i].sbr.input[2] = "";
		linear_program.program_block[i].sbr.output = "";
		linear_program.program_block[i].sbr.note = "";
		linear_program.program_block[i].sbr.cycle = linear_program.program_block[i].sbr.read_cycle = linear_program.program_block[i].sbr.write_cycle = 0;
		linear_program.program_block[i].sbr.indeg = linear_program.program_block[i].sbr.last_fa_end_time = 0; // modified by CZY
		linear_program.program_block[i].sbr.site = -1;
		linear_program.program_block[i].sbr.state = 0;
		linear_program.program_block[i].sbr.partner = NULL;
		linear_program.program_block[i].sbr.chl.clear();
		linear_program.program_block[i].sbr.ex_loop.clear();


		for (int j = 0; j < 8; j++) {
			linear_program.program_block[i].reg_used[j].clear();
			linear_program.program_block[i].reg_fix[j].clear();
		}
		for (int j = 0; j < kRegType; j++) {
			linear_program.program_block[i].reg_available[j].clear();
			linear_program.program_block[i].reg_idle[j].clear();
		}
		linear_program.program_block[i].var_storage.clear();
	}
	linear_program.block_pointer_list.clear();
	linear_program.var_in.clear();
	linear_program.var_out.clear();
	linear_program.total_block_num = 0;
	linear_program.total_ins_num = 0;
	for (int i = 0; i < kRegType; ++i) {
		linear_program.max_var_alive[i] = 0;
	}

	var_map.clear();
	core = -1;
	function_units_index.clear();
	reg_ctrl[0].clear();
	reg_ctrl[1].clear();
	reg_amount.clear();
	reg_res.clear();
	reg_in.clear();
	reg_out = -1;
	reg_return = -1;
	scalar_stack_AR = -1;
	vector_stack_AR = -1;
	sv_interval = -1;
}
