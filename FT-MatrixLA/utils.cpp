#include "utils.h"




void Error()
{
	printf("------------------------------------------------------------------------------\n");
	exit(EXIT_FAILURE);
}

void MessageFormalize(const int& content_level, const string& content, int src_line, string src) {
	int wrap_width = 80, level_interval = 4;
	int i = 0, j = 0, first_line = 1, line_start = 1;
	string message = "";
	// print process message
	if (content_level != -1) {
		while (i < content.size()) {
			for (j = content_level * level_interval; j > 0; --j) {
				message += ' ';
			}
			j = content_level * level_interval + 2;
			for (; j < wrap_width && i < content.size(); ++j, ++i) {
				if (content[i] == '\n') {
					i++;
					break;
				}
				message += content[i];
			}
			message += '\n';
		}
		printf("%s", message.c_str());
		return;
	}
	// print error message
	message = "\n";
	for (j = 2; j < wrap_width && i < content.size(); ++i, ++j) {
		message += content[i];
	}
	message += '\n';
	for (j = 8; i < content.size(); j = 8) {
		message += "       ";
		while (content[i] == ' ' && i < content.size()) i++;
		for (; j < wrap_width && i < content.size(); ++j, ++i) {
			message += content[i];
		}
		message += '\n';
	}
	printf("%s", message.c_str());
	// print error source line
	if (src_line != -1)  {
		message = "[Line " + to_string(src_line) + "]: ";
		i = 0;
		while ((src[i] == ' ' || src[i] == '\t') && i < src.size()) i++;
		for (j = message.size() + 2; j < wrap_width && i < src.size(); ++i, ++j) {
			message += src[i];
		}
		for (j = 2; i < src.size(); j = 2) {
			message += '\n';
			while ((src[i] == ' ' || src[i] == '\t') && i < src.size()) i++;
			if (src[i] == '\n') break;
			for (; j < wrap_width && i < src.size(); ++j, ++i) {
				if (src[i] == '\n') break;
				if (src[i] == '\t') message += ' ';
				else message += src[i];
			}
		}
		printf("%s", message.c_str());
	}
	printf("------------------------------------------------------------------------------\n");
	exit(EXIT_FAILURE);
}

bool IsEmptyLine(char* line)
{
	bool flag = true;
	int i = 0;
	while (1) {
		if (line[i] == '\t' || line[i] == ' ')
			i++;
		else if (line[i] == '\0')
			return flag;
		else {
			flag = false;
			return flag;
		}
	}
}



int GetFirstWord(char* dst, char* src)
{
	int i = 0, j;
	int num_blanks = 0;
	//for (i = 0; source[i] == '\t' || source[i] == '|' || source[i] == ' '; i++);
	// skip spaces
	while (1) {
		if (src[i] != '\t' && src[i] != ' ' && src[i] != '|') break;
		num_blanks++;
		i++;
	}
	// find split symbol
	for (j = 0; src[i] != ' ' && src[i] != '\t' && src[i] != '\0' && src[i] != ','; i++, j++)
		dst[j] = src[i];


	dst[j] = '\0';
	//printf("== %d\n", begin_blanks);
	return num_blanks;
}

void PrintVariables() {
	PrintInfo("------ Declared variables ------\n");
	// variables
	for (auto i : var_map)
	{
		PrintDebug("var: %s, flag: %d, type: %d, reg_type: %s, partner: %s, pos: %d \n",
			i.first.c_str(), i.second.use_flag, i.second.type, kRegTypeStr[int(i.second.reg_type) + 1].c_str(), i.second.partner.c_str(), i.second.pair_pos);
	}

}

void PrintGlobalData()
{
	PrintDebug("------ Print Global Data Structures with pointers ------\n");
	
	
	for (int i = 0; i < linear_program.block_pointer_list.size(); i++) {
		PrintDebug("[block %d], index %d, type %d, label %s, depth %d, num of instructions %d, %d\n",
			i, linear_program.block_pointer_list[i]->block_index,
			linear_program.block_pointer_list[i]->type,
			linear_program.block_pointer_list[i]->label.c_str(),
			linear_program.block_pointer_list[i]->loop_depth,
			linear_program.block_pointer_list[i]->block_ins_num,
			linear_program.block_pointer_list[i]->ins_pointer_list.size());

		for (int j = 0; j < linear_program.block_pointer_list[i]->ins_pointer_list.size(); j++) {
			PrintDebug("%s == %s, %s, %s, %s, %s, %s \n", linear_program.block_pointer_list[i]->ins_pointer_list[j]->ins_string,
				linear_program.block_pointer_list[i]->ins_pointer_list[j]->ins_name,
				linear_program.block_pointer_list[i]->ins_pointer_list[j]->func_unit,
				linear_program.block_pointer_list[i]->ins_pointer_list[j]->input[0].c_str(),
				linear_program.block_pointer_list[i]->ins_pointer_list[j]->input[1].c_str(),
				linear_program.block_pointer_list[i]->ins_pointer_list[j]->input[2].c_str(),
				linear_program.block_pointer_list[i]->ins_pointer_list[j]->output.c_str());
		}
		
	}

}


int FindChar(char* source, char c) {
	for (int i = 0, len = strlen(source); i < len; i++)
		if (source[i] == c)
			return i;
	return -1;
}

void CopyInstruction(Instruction* dst, Instruction* src)
{
	dst->type = src->type;
	dst->ins_string = (char*)malloc(strlen(src->ins_string) * sizeof(char) + 4);
	strcpy(dst->ins_string, src->ins_string);
	strcpy(dst->ins_name, src->ins_name);
	strcpy(dst->func_unit, src->func_unit);
	dst->cond_field = src->cond_field;
	for (int i = 0; i < sizeof(src->input) / sizeof(src->input[0]); i++)
		dst->input[i] = src->input[i];
	dst->output = src->output;
	dst->note = src->note;
	dst->cond_flag = src->cond_flag;
	dst->cycle = src->cycle;
	dst->write_cycle = src->write_cycle;
	dst->read_cycle = src->read_cycle;
	dst->extracted_var = src->extracted_var;
}

int FindChar(string& source, char c) {
	int res = source.find(c);
	return res == source.npos ? -1 : res;
}


// -1: default
//  0: R_CTRL
//  1: VR_CTRL
//  2: imm number
int IsSpecialOperand(const string& operand) {
	if (operand[0] == '.') return 3;
	if (operand[0] >= '0' && operand[0] <= '9' || operand[0] == '-') return 2;
	for (int type = 0; type < 2; ++type) {
		for (int i = 0; i < reg_ctrl[type].size(); ++i) {
			
			if (reg_ctrl[type][i] == operand) return type;
		}
	}
	return -1;
}


// Split the function units of the instructions 
vector<int> FunctionUnitSplit(string function_unit) {
	int pos = 0;
	int i = 0;
	vector <int> func_vector;
	while (pos < function_unit.length()) {
		char unit[32];
		for (i = 0; function_unit[pos] != '/' && function_unit[pos] != '\0'; i++, pos++)
			unit[i] = function_unit[pos];
		unit[i] = '\0';
		pos++;
		string unit_str = unit;
		int unit_index = function_units_index[unit_str];
		func_vector.push_back(unit_index);
	}
	return func_vector;
}


void GenerateOutputFile(const string& output_file) {
	FILE* fpout = fopen(output_file.c_str(), "w");
	if (!fpout) {
		PrintError("create output file \"%s\" fail\n", output_file.c_str());
		exit(1);
	}
	int interval0 = 8, interval1 = 16, interval2 = 32;
	int block, cycle, unit;
	int snop_num = 0, first_ins = 1;
	int i;
	string line_str, str_buf;

	fprintf(fpout, ".global ");
	fprintf(fpout, block_label[0].c_str());
	fprintf(fpout, "\n\n");

	for (block = 0; block < block_label.size(); ++block) {
		if (!block_label[block].empty()) {
			fprintf(fpout, "%s:\n", block_label[block].c_str());
		}
		if (linear_program.block_pointer_list[block]->type == BlockType::LoopStart) {	//modified by DC 23/3/12
			if (linear_program.block_pointer_list[block]->ins_pointer_list[0]->input[1] != "") {
				int unrolling_factor = atoi(linear_program.block_pointer_list[block]->ins_pointer_list[0]->input[1].c_str());
				if (unrolling_factor) {
					fprintf(fpout, "; unroll (%d)\n", unrolling_factor);
				}
			}
		}
		if (block == 0) fprintf(fpout, "\n");
		if (!ins_output[block].size()) continue;
		fprintf(fpout, "; cycle(%d)\n", ins_output[block].size());
		for (cycle = 0; cycle < ins_output[block].size(); ++cycle) {
			if (ins_output[block][cycle][4] != NULL) {
				if (!strcmp(ins_output[block][cycle][4]->ins_name, "SNOP")) {
					snop_num++;
					continue;
				}
			}
			first_ins = 1;
			if (snop_num) {
				line_str = "";
				for (i = 0; i < interval0; ++i) line_str += " ";
				line_str += "SNOP";
				for (i = 4; i < interval1; ++i) line_str += " ";
				line_str += to_string(snop_num);
				fprintf(fpout, "%s\n", line_str.c_str());
				snop_num = 0;
			}
			for (unit = 0; unit < kFuncUnitAmount; ++unit) {
				if (ins_output[block][cycle][unit] == NULL) continue;
				line_str = "";
				str_buf = "";
				if (first_ins) first_ins = 0;
				else str_buf += '|';
				if (!ins_output[block][cycle][unit]->cond_field.empty()) {
					str_buf += "[";
					if (ins_output[block][cycle][unit]->cond_flag) str_buf += "!";
					str_buf += ins_output[block][cycle][unit]->cond_field;
					str_buf += "]";
				}
				line_str += str_buf;
				for (i = str_buf.size(); i < interval0; ++i) line_str += " ";
				str_buf = ins_output[block][cycle][unit]->ins_name;
				line_str += str_buf;
				for (i = str_buf.size(); i < interval1; ++i) line_str += " ";
				str_buf = "";
				if (!ins_output[block][cycle][unit]->input[0].empty()) {
					str_buf += ins_output[block][cycle][unit]->input[0];
				}
				if (!ins_output[block][cycle][unit]->input[1].empty()) {
					str_buf += ", ";
					str_buf += ins_output[block][cycle][unit]->input[1];
				}
				if (!ins_output[block][cycle][unit]->input[2].empty()) {
					str_buf += ", ";
					str_buf += ins_output[block][cycle][unit]->input[2];
				}
				if (!ins_output[block][cycle][unit]->output.empty()) {
					str_buf += ", ";
					str_buf += ins_output[block][cycle][unit]->output;
				}
				line_str += str_buf;
				if (!ins_output[block][cycle][unit]->note.empty()) {
					for (i = str_buf.size(); i < interval2; ++i) line_str += " ";
					line_str += ins_output[block][cycle][unit]->note;
				}
				fprintf(fpout, "%s\n", line_str.c_str());
			}
		}
		if (snop_num) {
			line_str = "";
			for (i = 0; i < interval0; ++i) line_str += " ";
			line_str += "SNOP";
			for (i = 4; i < interval1; ++i) line_str += " ";
			line_str += to_string(snop_num);
			fprintf(fpout, "%s\n", line_str.c_str());
			snop_num = 0;
		}
		fprintf(fpout, "\n");
	}
	fprintf(fpout, ".size ");
	fprintf(fpout, block_label[0].c_str());
	fprintf(fpout, ", .-");
	fprintf(fpout, block_label[0].c_str());
	fclose(fpout);
}

bool IsSubString(const char* str, const char* substr) {
	int len = strlen(str);
	int substr_len = strlen(substr);
	if (len < substr_len)
		return false;
	for (int i = 0; i < len - substr_len + 1; i++) {
		bool flag = true;
		for (int j = 0; j < substr_len; j++) {
			if (str[i + j] != substr[j]) {
				flag = false;
				break;
			}
		}
		if (flag) return flag;
	}
	return false;
}

bool IsCtrlInstrction(const Instruction* ins) {
	if (ins->ins_name[0] == '\0') return false;
	for (int k = 0; k < 3; k++) {
		if (ins->input[k] == "") continue;
		for (int type = 0; type < 2; ++type) {
			for (int i = 0; i < reg_ctrl[type].size(); ++i) {

				if (reg_ctrl[type][i] == ins->input[k]) return true;
			}
		}
	}
	for (int type = 0; type < 2; ++type) {
		for (int i = 0; i < reg_ctrl[type].size(); ++i) {

			if (reg_ctrl[type][i] == ins->output) return true;
		}
	}
	return false;
}

void CalculateILP() {	// modified by DC 23/3/13
	int cycle = 0;
	int func = 0;
	double ILP = 0;
	for (int i = 0; i < linear_program.block_pointer_list.size(); i++) {
		if (linear_program.block_pointer_list[i]->type == BlockType::LoopStart) {
			int unrolling_factor = atoi(linear_program.block_pointer_list[i]->ins_pointer_list[0]->input[1].c_str());
			if (unrolling_factor) {
				printf("unroll (%d): ", unrolling_factor);
			}
		}
		if (linear_program.block_pointer_list[i]->type != BlockType::Instruction) continue;

		for (int j = 0; j < linear_program.block_pointer_list[i]->ins_schedule_queue.size(); j++) {
			int flag = 0;
			for (int k = 0; k < linear_program.block_pointer_list[i]->ins_schedule_queue[j].size(); k++) {
				if (linear_program.block_pointer_list[i]->ins_schedule_queue[j][k] == NULL) continue;
				if (linear_program.block_pointer_list[i]->ins_schedule_queue[j][k]->type != InsType::ReferIns ||
					string(linear_program.block_pointer_list[i]->ins_schedule_queue[j][k]->ins_name) == "SNOP") continue;
				flag += 1;
				func += 1;
			}
			if (flag != 0) cycle++;
		}
	}
	ILP = func / double(cycle);
	printf("ILP = %lf\n", ILP);
}