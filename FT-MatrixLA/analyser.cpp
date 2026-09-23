#include "data_structure.h"
#include "analyser.h"
#include "utils.h"
#include <fstream>
#include <cstring>
#include <iostream> 
Block* current_block;



// global data structure for the 
// program, refer instructions and variables




bool IsCommentLine(char* one_line_code)
{
	bool flag = false;
	int i = 0;
	while (1) {
		if (one_line_code[i] == '\t' || one_line_code[i] == ' ')
			i++;
		else if (one_line_code[i] == ';')
			return true;
		else if (one_line_code[i] == '\0')
			return flag;
		else {
			flag = false;
			return flag;
		}
	}
}

void VarDeclarationCheck(string ins_var, int line_num)
{
	// cases: 
	// 0: imm number
	// 1: address (start with *)
	// 2: special variables reg_ctrl[2];
	// 3: normal
	// 4: label

	// if not case 0, 2, 4
	if (IsSpecialOperand(ins_var) == -1) {
		// if case 1 and 3
		if (var_map.count(ins_var) == 0) {
			PrintError("Error in line %d: Variable %s is not declared.\n", line_num, ins_var.c_str());
			Error();
		}
		else
			var_map[ins_var].use_flag = 1;
	}
}

void VarAssignmentCheck(string ins_var, int line_num)
{
	if (var_map[ins_var].assign_flag == 0)
	{
		PrintError("Error in line %d: Variable %s is used without assignment.\n", line_num, ins_var.c_str());
		Error();
	}
}

void VarRegConfirm(int is_output, char* func_unit, char* refer_var, string ins_var, int line_num, Instruction* ins)
{

	string ins_var_1 = "";
	string ins_var_2 = "";
	RegType current_reg_type = RegType::Default;
	int reg_num = -1;
	int pair_split = FindChar(ins_var, ':');
	int sv_flag = -1;
	int i = 0;

	// input is imm
	if (IsSpecialOperand(ins_var) == 2) {
		if (FindChar(refer_var, 'I') == -1) {
			PrintError("In line %d the imm %d is not a suitable\n", line_num, atoi(ins_var.c_str()));
			Error();
		}
		return;
	}
	// AR, OR
	else if (refer_var[0] == 'A') {
		if (!strcmp(func_unit, "SLDST") && !IsSubString(ins->ins_name, ".LS")) sv_flag = 0;
		else if (!strcmp(func_unit, "VLDST1/VLDST2")) sv_flag = 1;
		// PrintDebug("%d svflag: %d \n", line_num, sv_flag);
		// add var should not be a pair
		if (pair_split != -1) {
			PrintError("In line %d the operand %s should be an address\n", line_num, ins_var.c_str());
			Error();
		}
		// find name of AR and OR
		// AR: ins_var_1  OR: ins_var_2
		while (ins_var[i] == '*' || ins_var[i] == '+' || ins_var[i] == '-') i++;
		for (; ins_var[i] != '+' && ins_var[i] != '-' && ins_var[i] != '[' && ins_var[i] != '\0'; i++)
			ins_var_1 += ins_var[i];
		while (ins_var[i] == '+' || ins_var[i] == '-') i++;
		if (ins_var[i] == '[') {
			i++;
			if (ins_var[i] < '0' || ins_var[i] > '9') {
				for (; ins_var[i] != ']' && i < ins_var.size(); i++) {
					ins_var_2 += ins_var[i];
				}
				if (i == ins_var.size()) {
					PrintError("In line %d the operand lacks \"]\" \n", line_num);
					Error();
				}
			}
		}
		//PrintDebug("%d AR:%s, OR:%s\n", line_num, ins_var_1.c_str(), ins_var_2.c_str());
		current_reg_type = RegType::AR_OR;

		// check and set type of ins_var_1 (AR)
		if (ins_var_1 == "") {
			PrintError("In line %d the operand lacks AR variable \n", line_num);
			Error();
		}
		else {
			VarDeclarationCheck(ins_var_1, line_num);
			if (is_output == 0) {
				VarAssignmentCheck(ins_var_1, line_num);
			}
			else {
				var_map[ins_var_1].assign_flag = 1;
			}
			ins->extracted_var.insert(ins_var_1);
			current_reg_type = RegType::AR_OR;
			if (sv_flag == 1) current_reg_type = RegType::V_AR;
			else if (sv_flag == 0) current_reg_type = RegType::S_AR;

				
			// if already set
			if (var_map[ins_var_1].reg_type != RegType::Default) {
				// equal to current
				if (current_reg_type == var_map[ins_var_1].reg_type)
					var_map[ins_var_1].reg_type = current_reg_type;
				// already set to AR_OR
				else if (var_map[ins_var_1].reg_type == RegType::AR_OR)
					var_map[ins_var_1].reg_type = current_reg_type;
				else if (current_reg_type == RegType::AR_OR && (var_map[ins_var_1].reg_type == RegType::V_AR || var_map[ins_var_1].reg_type == RegType::S_AR)) {
					;
				}
				else if (current_reg_type == RegType::AR_OR && (var_map[ins_var_1].reg_type == RegType::V_OR || var_map[ins_var_1].reg_type == RegType::S_OR)) {
					;
				}
				else if (var_map[ins_var_1].reg_type != current_reg_type){
					PrintError("In line %d, variable %s is used as %s and %s at the same time\n."
						, line_num, ins_var_1.c_str(), kRegTypeStr[int(current_reg_type) + 1].c_str(), kRegTypeStr[int(var_map[ins_var_1].reg_type) + 1].c_str());
					Error();
				}
			}
			// have not been set
			else {
				var_map[ins_var_1].reg_type = current_reg_type;
			}
		}


		// check and set type of ins_var_2 (OR)
		if (ins_var_2 != "") {
			VarDeclarationCheck(ins_var_2, line_num);
			if (is_output == 0) {
				VarAssignmentCheck(ins_var_2, line_num);
			}
			else {
				var_map[ins_var_2].assign_flag = 1;
			}
			ins->extracted_var.insert(ins_var_2);
			current_reg_type = RegType::AR_OR;
			if (sv_flag == 1) current_reg_type = RegType::V_OR;
			else if (sv_flag == 0) current_reg_type = RegType::S_OR;
			//PrintDebug("%d %s %s %s\n", line_num, ins_var_2.c_str(), kRegTypeStr[int(current_reg_type) + 1].c_str(), kRegTypeStr[int(var_map[ins_var_2].reg_type) + 1].c_str());
			// if not set
			if (var_map[ins_var_2].reg_type == RegType::Default)
				var_map[ins_var_2].reg_type = current_reg_type;
			// if already set as AR_OR
			else if (var_map[ins_var_2].reg_type == RegType::AR_OR) {
				//PrintDebug("Here %d \n", line_num);
				var_map[ins_var_2].reg_type = current_reg_type;
			}
			else if (current_reg_type == RegType::AR_OR && (var_map[ins_var_2].reg_type == RegType::V_AR || var_map[ins_var_2].reg_type == RegType::S_AR)) {
				;
			}
			else if (current_reg_type == RegType::AR_OR && (var_map[ins_var_2].reg_type == RegType::V_OR || var_map[ins_var_2].reg_type == RegType::S_OR)) {
				;
			}
			else if (var_map[ins_var_2].reg_type != current_reg_type){
				PrintError("In line %d, variable %s is used as %s and %s at the same time\n."
					, line_num, ins_var_2.c_str(), kRegTypeStr[int(current_reg_type) + 1].c_str(), kRegTypeStr[int(var_map[ins_var_2].reg_type) + 1].c_str());
				Error();
			}
		}

	}
	// CR, CVR
	else if (refer_var[0] == 'C') {
		// if not a special reg
		if (IsSpecialOperand(ins_var) != 0
			&& IsSpecialOperand(ins_var) != 1) {
			printf("%s %d\n", ins_var.c_str(),IsSpecialOperand(ins_var));
			PrintError("In line %d the var %s is not a control reg\n", line_num, ins_var.c_str());
			Error();
		}
		// if not a suitable special reg
		if (IsSpecialOperand(ins_var) == 0
			&& refer_var[1] == 'C') {
			PrintError("In line %d the var %s is not a suitable control reg\n", line_num, ins_var.c_str());
			Error();
		}
		// do not need to set the type of CR and CVR

	}
	// imm
	else if (refer_var[0] == 'I') {
		if (ins_var[0] == '.' || ins_var[0] == '-' || (ins_var[0] >= '0' && ins_var[0] <= '9')) return;
		else {
			PrintError("In line %d the %s should be a imm\n", line_num, ins_var.c_str());
			Error();
		}
	}
	// R, VR
	else {
		int refer_split = FindChar(refer_var, ':');
		sv_flag = (refer_var[0] == 'V') ? 1 : 0;

		// check if the num of vars is right
		if (refer_split != -1) {
			if (pair_split == -1) {
				PrintError("In line %d, %s should be 2 vars\n", line_num, ins_var.c_str());
				Error();
			}
		}
		else if (pair_split != -1) {
			PrintError("In line %d, %s should be 1 vars\n", line_num, ins_var.c_str());
			Error();
		}

		// if ins_var contains two vars
		if (pair_split != -1) {
			// find the two vars
			for (i = 0; i < pair_split; i++)
				ins_var_1 += ins_var[i];
			i++;
			for (; i < ins_var.length(); i++) {
				if (ins_var[i] == ':') {
					PrintError("In line %d, %s should be no more than 2 vars\n", line_num, ins_var.c_str());
					Error();
				}
				ins_var_2 += ins_var[i];
			}
			VarDeclarationCheck(ins_var_1, line_num);
			VarDeclarationCheck(ins_var_2, line_num);
			if (is_output == 0) {
				VarAssignmentCheck(ins_var_1, line_num);
				VarAssignmentCheck(ins_var_2, line_num);
			}
			else {
				var_map[ins_var_1].assign_flag = 1;
				var_map[ins_var_2].assign_flag = 1;
			}
			// verify if ins_var_1 and ins_var_2 are pairs
			if (strcmp(&(var_map[ins_var_1].partner[0]), &ins_var_2[0]) != 0) {
				if (var_map[ins_var_1].partner == "" && var_map[ins_var_2].partner == "") {
					PrintError("In line %d, %s and %s should be declared as a pair of 2 vars\n", line_num, ins_var_1.c_str(), ins_var_2.c_str());
					Error();
				}
				else {
					PrintError("In line %d, %s should be a pair of 2 vars\n", line_num, ins_var.c_str());
					Error();
				}

				
			}
			// verify the possition of pairs
			if (var_map[ins_var_1].pair_pos != 0) {
				PrintError("In line %d, the positions of 2 vars in %s are wrong\n", line_num, ins_var.c_str());
				Error();
			}
			ins->extracted_var.insert(ins_var_1);
			ins->extracted_var.insert(ins_var_2);
			// check and set the reg type of ins_var_1 and ins_var_2
			current_reg_type = RegType::Default;
			sv_flag = (refer_var[0] == 'V') ? 1 : 0;
			if (sv_flag == 1)
				current_reg_type = RegType::VR;
			else
				current_reg_type = RegType::R;
			// first, ins_var_1
			if (var_map[ins_var_1].reg_type == RegType::Default
				|| var_map[ins_var_1].reg_type == current_reg_type)
				var_map[ins_var_1].reg_type = current_reg_type;
			else if (var_map[ins_var_1].reg_type == RegType::VR_COND
				&& current_reg_type == RegType::VR)
				;
			else if (var_map[ins_var_1].reg_type == RegType::R_COND
				&& current_reg_type == RegType::R)
				;
			else {
				printf("ERROR: in line %d, variable %s is used as %s and %s at the same time\n."
					, line_num, ins_var_1.c_str(), kRegTypeStr[int(current_reg_type) + 1].c_str(), kRegTypeStr[int(var_map[ins_var_1].reg_type) + 1].c_str());
				Error();
			}
			// second, ins_var_2
			if (var_map[ins_var_2].reg_type == RegType::Default
				|| var_map[ins_var_2].reg_type == current_reg_type)
				var_map[ins_var_2].reg_type = current_reg_type;
			else if (var_map[ins_var_2].reg_type == RegType::VR_COND
				&& current_reg_type == RegType::VR)
				;
			else if (var_map[ins_var_2].reg_type == RegType::R_COND
				&& current_reg_type == RegType::R)
				;
			else {
				printf("ERROR: in line %d, variable %s is used as %s and %s at the same time\n."
					, line_num, ins_var_2.c_str(), kRegTypeStr[int(current_reg_type) + 1].c_str(), kRegTypeStr[int(var_map[ins_var_2].reg_type) + 1].c_str());
				Error();
			}


		}
		// ins_var is only one var
		else {
			// imm is handled at the begining of this function
			// thus, only need to check R or VR
			// if is a label, return 
			if (ins_var[0] == '.')
				return;
			if (IsSpecialOperand(ins_var) >= 0) {
				printf("ERROR: in line %d, variable %s should not be a special variable \n."
					, line_num, ins_var.c_str());
				Error();
			}
			VarDeclarationCheck(ins_var, line_num);
			if (is_output == 0) {
				VarAssignmentCheck(ins_var, line_num);
			}
			else {
				var_map[ins_var].assign_flag = 1;
			}
			ins->extracted_var.insert(ins_var);
			sv_flag = (refer_var[0] == 'V') ? 1 : 0;
			current_reg_type = RegType::R;
			if (sv_flag == 1)
				current_reg_type = RegType::VR;

			if (var_map[ins_var].reg_type == RegType::Default
				|| var_map[ins_var].reg_type == current_reg_type)
				var_map[ins_var].reg_type = current_reg_type;
			else if (var_map[ins_var].reg_type == RegType::VR_COND
				&& current_reg_type == RegType::VR)
				;
			else if (var_map[ins_var].reg_type == RegType::R_COND
				&& current_reg_type == RegType::R)
				;
			else if (var_map[ins_var].reg_type != current_reg_type) {
				printf("ERROR: in line %d, variable %s is used as %s and %s at the same time\n."
					, line_num, ins_var.c_str(), kRegTypeStr[int(current_reg_type) + 1].c_str(), kRegTypeStr[int(var_map[ins_var].reg_type) + 1].c_str());
				Error();
			}


		}


	}
}


//void LoadReference(const char* refer_filename)
//{
//	FILE* fp;
//	int i, j, end = 0, num_of_entries;
//	char line[256], buf[32];
//	string label;
//
//	fp = fopen(refer_filename, "r");
//
//	if (fp == NULL) {
//		printf("ERROR: The reference file \"%s\" doesn\'t exist.\n", refer_filename);
//		Error();
//	}
//	else printf("Reference file \"%s\" starts loading...\n", refer_filename);
//
//	while (end == 0)
//	{
//		// reset the count variable
//		num_of_entries = 0;
//		if (fscanf(fp, "%s", line) == EOF) break;
//		//printf("%s\n", line);
//		label = "";
//		for (i = 0; line[i] != ','; i++)
//			label += line[i];
//		fscanf(fp, "%s", line);
//		//printf("%s\n", line);
//		// 0. handle the core type of the DSP
//		if (label == "CORE") {
//			for (j = i = 0; line[i] != ','; i++, j++)
//				buf[j] = line[i];
//			buf[j] = '\0';
//			core = atoi(buf);
//			printf("\tDSP CORE : \t %d\n", core);
//			if (fscanf(fp, "%s", line) == EOF) end = 1;
//		}
//		// 1. handle the instructions
//		if (label == "INSTRUCTION") {
//			// skip the title line in ths instruction part
//			fscanf(fp, "%s", line);
//			while (end == 0) {
//				if (line[0] == ',') break;
//				ReferInstruction x;
//				for (j = i = 0; line[i] != ','; i++, j++)
//					x.name[j] = line[i];
//				x.name[j] = '\0';
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					x.func_unit[j] = line[i];
//				x.func_unit[j] = '\0';
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					buf[j] = line[i];
//				buf[j] = '\0';
//				x.cycle = atoi(buf);
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					x.input[0][j] = line[i];
//				x.input[0][j] = '\0';
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					x.input[1][j] = line[i];
//				x.input[1][j] = '\0';
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					x.input[2][j] = line[i];
//				x.input[2][j] = '\0';
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					x.output[j] = line[i];
//				x.output[j] = '\0';
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					buf[j] = line[i];
//				buf[j] = '\0';
//				x.read_cycle = atoi(buf);
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					buf[j] = line[i];
//				buf[j] = '\0';
//				x.write_cycle = atoi(buf);
//				refer_table.table.push_back(x);
//				num_of_entries++;
//				if (fscanf(fp, "%s", line) == EOF) {
//					end = 1;
//					break;
//				}
//				//printf("%s\n", line);
//			}
//			printf("\tReference instructions: %d\n", num_of_entries);
//		}
//		// 2. handle numbers of function units
//		if (label == "FUNC_UNIT") {
//			while (end == 0) {
//				if (line[0] == ',') break;
//				int index;
//				string function_unit_name;
//				for (j = i = 0; line[i] != ','; i++, j++)
//					buf[j] = line[i];
//				buf[j] = '\0';
//				function_unit_name = buf;
//				i++;
//				for (j = 0; line[i] != ','; i++, j++)
//					buf[j] = line[i];
//				buf[j] = '\0';
//				index = atoi(buf);
//				function_units_index[function_unit_name] = index;
//				num_of_entries = index + 1;
//				if (fscanf(fp, "%s", line) == EOF) {
//					end = 1;
//					break;
//				}
//			}
//			printf("\tFunction Units    : %d\n", num_of_entries);
//		}
//
//		// 3. handle R_CTRL
//		if (label == "R_CTRL") {
//			while (end == 0) {
//				if (line[0] == ',') break;
//				string reg = "";
//				for (i = 0; line[i] != ','; i++)
//					reg += line[i];
//				reg_ctrl[0].push_back(reg);
//				num_of_entries++;
//				if (fscanf(fp, "%s", line) == EOF) {
//					end = 1;
//					break;
//				}
//			}
//			printf("\tSMVCGC/SMVCCG Reg : %d\n", num_of_entries);
//		}
//		// 4. handle VR_CTRL
//		if (label == "VR_CTRL") {
//			while (end == 0) {
//				if (line[0] == ',') break;
//				string reg = "";
//				for (i = 0; line[i] != ','; i++)
//					reg += line[i];
//				reg_ctrl[1].push_back(reg);
//				num_of_entries++;
//				if (fscanf(fp, "%s", line) == EOF) {
//					end = 1;
//					break;
//				}
//			}
//			printf("\tVMVCGC/VMVCCG Reg : %d\n", num_of_entries);
//		}
//
//		// 5. handle REG_AMOUNT
//		if (label == "REG_AMOUNT") {
//			while (end == 0) {
//				if (line[0] == ',') break;
//				for (i = 0; line[i] != ','; i++);
//				for (j = 0, ++i; line[i] != ','; i++, j++)
//					buf[j] = line[i];
//				buf[j] = '\0';
//				reg_amount.push_back(atoi(buf));
//				num_of_entries++;
//				if (fscanf(fp, "%s", line) == EOF) {
//					end = 1;
//					break;
//				}
//			}
//			// TODO
//			sv_interval = reg_amount[3];
//			printf("\tReg Type         : %d\n", num_of_entries);
//		}
//		// 6. handle REG_RES
//		if (label == "REG_RES") {
//			while (end == 0) {
//				if (line[0] == ',') break;
//				for (i = 0; line[i] != ','; i++);
//				vector<int> reg_buf;
//				while (1) {
//					if (line[i] == '\0') break;
//					buf[0] = '\0';
//					for (j = 0, ++i; line[i] != ',' && line[i] != '\0'; i++, j++)
//						buf[j] = line[i];
//					buf[j] = '\0';
//					if (buf[0] == '\0') break;
//					else reg_buf.push_back(atoi(buf));
//				}
//				reg_res.push_back(reg_buf);
//				num_of_entries++;
//				if (fscanf(fp, "%s", line) == EOF) {
//					end = 1;
//					break;
//				}
//			}
//			printf("\tReserve Reg Type : %d\n", num_of_entries);
//		}
//		
//		// 7. handle REG_SPEC
//		if (label == "REG_SPEC") {
//			while (end == 0) {
//				if (line[0] == ',') break;
//				string type = "";
//				for (i = 0; line[i] != ','; i++)
//					type += line[i];
//				if (type == "IN") {
//					int start, end;
//					for (j = 0, ++i; line[i] != ','; i++, j++)
//						buf[j] = line[i];
//					buf[j] = '\0';
//					start = atoi(buf);
//					for (j = 0, ++i; line[i] != ','; i++, j++)
//						buf[j] = line[i];
//					buf[j] = '\0';
//					end = atoi(buf);
//					for (int k = start; k <= end; k += 2)
//						reg_in.push_back(k);
//				}
//				if (type == "OUT") {
//					for (j = 0, ++i; line[i] != ','; i++, j++)
//						buf[j] = line[i];
//					buf[j] = '\0';
//					reg_out = atoi(buf);
//				}
//				if (type == "RETURN") {
//					for (j = 0, ++i; line[i] != ','; i++, j++)
//						buf[j] = line[i];
//					buf[j] = '\0';
//					reg_return = atoi(buf);
//				}
//				if (type == "S_STACK") {
//					for (j = 0, ++i; line[i] != ','; i++, j++)
//						buf[j] = line[i];
//					buf[j] = '\0';
//					// TODO
//					scalar_stack_AR = atoi(buf) - sv_interval;
//				}
//				if (type == "V_STACK") {
//					for (j = 0, ++i; line[i] != ','; i++, j++)
//						buf[j] = line[i];
//					buf[j] = '\0';
//					vector_stack_AR = atoi(buf);
//				}
//				num_of_entries++;
//				if (fscanf(fp, "%s", line) == EOF) {
//					end = 1;
//					break;
//				}
//			}
//			printf("\tSpecific Reg type : %d\n", num_of_entries);
//		}
//		if (end) break;
//	}
//	fclose(fp);
//	// check exceptions
//	if (core == -1) {
//		printf("ERROR: The reference file is lack of core information.\n");
//		Error();
//	}
//	if (refer_table.table.size() == 0) {
//		printf("ERROR: The reference file is lack of instruction information.\n");
//		Error();
//	}
//	if (function_units_index.size() == 0) {
//		printf("ERROR: The reference file is lack of function unit information.\n");
//		Error();
//	}
//	if (reg_ctrl[0].size() == 0 || reg_ctrl[1].size() == 0) {
//		printf("ERROR: The reference file is lack of control register information.\n");
//		Error();
//	}
//
//	if (reg_amount.size() == 0) {
//		printf("ERROR: The reference file is lack of register amount information.\n");
//		Error();
//	}
//	if (reg_res.size() == 0) {
//		printf("ERROR: The reference file is lack of reserve register information.\n");
//		Error();
//	}
//	if (reg_in.size() == 0 || reg_out == -1 || reg_return == -1 || scalar_stack_AR == -1 || vector_stack_AR == -1) {
//		printf("ERROR: The reference file is lack of specific register information.\n");
//		Error();
//	}
//	printf("Reference file is sucessfully loaded.\n\n\n");
//}

void LoadLinearProgram(const char* linear_fliename)
{
	ifstream in(linear_fliename);
	if (!in)
	{
		PrintError("The linear assembly file \"%s\" doesn\'t exist.\n", linear_fliename);
		Error();
	}
	else
	{
		PrintInfo("Linear assembly file \"%s\" starts analysing...\n", linear_fliename);
	}

	
	char line[kMaxLineLength];
	line[0] = '\0';
	int end = 0, index_cursor = 0;
	int total_ins_num = 0;
	int line_num = 0;
	// denote the loop_depth (to handle nested loops) 
	int current_depth = 0;
	// a pointer links to the previous line
	streampos old_pos = in.tellg();
	// process the file until the end
	int loop_mark_check = 0;
	while (!in.eof())
	{
		
		current_block = &linear_program.program_block[linear_program.total_block_num];
		current_block->block_index = index_cursor;
		current_block->type = BlockType::Default;
		index_cursor++;
		
		// if file not end
		while (in.getline(line, kMaxLineLength))
		{
			line_num++;
			//printf("== %d: %s\n", line_num, line);
			Instruction *current_ins = &current_block->block_ins[current_block->block_ins_num];
			current_ins->type = InsType::Default;
			//current_ins.ins_string = (char*)malloc(strlen(line) * sizeof(char) + 4);
			//printf("ADD %d\n", &current_ins);
			// 0. if empty line or comment line, skip or others to do.
			if (IsEmptyLine(line) || IsCommentLine(line)) {
				old_pos = in.tellg();
				continue;
			}
			//printf("%s\n", line);

			// int line_type = BlockSplitSymbol(line);

			GenerateInstr(line, current_ins, line_num, current_block->block_ins_num);

			//PrintGlobalData();
			// 1. if block split symbol
			//    store into current block
			//    store the block into linear_program
			if (current_ins->ins_name[0] == '.' || !strcmp(current_ins->func_unit, "SBR") || IsCtrlInstrction(current_ins) ) {
				//printf("%d\n", BlockSplitSymbol(line)); 
				// 0. block starts with a function using SBR function unit
				if (!strcmp(current_ins->func_unit, "SBR\0")) {
					// generate instr
					//current_block->ins_vector.push_back(current_ins);
					//current_block->ins_pointer_list.push_back(&(current_block->block_ins[current_block->block_ins_num]));
					current_block->block_ins_num += 1;
					current_block->loop_depth = current_depth;
					// block_type 2: instructions
					current_block->type = BlockType::Instruction; 
					//linear_program.block_vector.push_back(current_block);
					linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
					linear_program.total_block_num++;
					//printf("==%d\n", current_block->block_type);
					linear_program.total_ins_num += current_block->block_ins_num;
					break;
				}
				// input or output have ctrl register
				else if (IsCtrlInstrction(current_ins)){
					current_block->block_ins_num += 1;
					current_block->loop_depth = current_depth;
					current_block->type = BlockType::Instruction;
					linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
					linear_program.total_block_num++;
					linear_program.total_ins_num += current_block->block_ins_num;
					break;
				}
				// 1. block starts with a label
				else
				{
					// if current_block is empty
					if (current_block->block_ins_num == 0) {
						
						current_block->block_ins_num += 1;
						// strore the block label
						char block_label[32];
						GetFirstWord(block_label, line);
						if (block_label[strlen(block_label) - 1] == ':')
							block_label[strlen(block_label) - 1] = '\0';
						current_block->label = block_label;
						// loop start
						if (!strcmp(current_ins->ins_name, ".loop")){
							loop_mark_check += 1;
							current_depth ++;
							current_block->loop_depth = current_depth;
							current_block->type = BlockType::LoopStart;
							//linear_program.block_vector.push_back(current_block);
							//linear_program.block_pointer_list.push_back(&(linear_program.block_vector[linear_program.total_block_num]));
							linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
						}
						// loop end
						else if (!strcmp(current_ins->ins_name, ".endloop")) {
							loop_mark_check -= 1;
							current_block->loop_depth = current_depth;
							current_depth --;
							current_block->type = BlockType::LoopEnd;
							//linear_program.block_vector.push_back(current_block);
							//linear_program.block_pointer_list.push_back(&(linear_program.block_vector[linear_program.total_block_num]));
							linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
						}
						else if (!strcmp(current_ins->ins_name, ".close")) {
							current_block->type = BlockType::Close;
							linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
						}
						else if (!strcmp(current_ins->ins_name, ".open")) {
							current_block->type = BlockType::Open;
							linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
						}
						// normal label
						else {
							
							current_block->type = BlockType::Label;
							//linear_program.block_vector.push_back(current_block);
							//linear_program.block_pointer_list.push_back(&(linear_program.block_vector[linear_program.total_block_num]));
							linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
							//printf("*** %d\n", &(linear_program.program_block[linear_program.total_block_num]));
						}
						//printf("==%d\n", current_block->block_type);
						linear_program.total_block_num++;
						linear_program.total_ins_num += current_block->block_ins_num;
						
						break;
					}
					// else if current_block is not empty
					else {
						current_block->loop_depth = current_depth;
						// current_block->block_type = 2;
						//linear_program.block_vector.push_back(current_block);
						//linear_program.block_pointer_list.push_back(&(linear_program.block_vector[linear_program.total_block_num]));
						linear_program.block_pointer_list.push_back(&(linear_program.program_block[linear_program.total_block_num]));
						//printf("==%d\n", current_block->block_type);
						linear_program.total_block_num++;
						linear_program.total_ins_num += current_block->block_ins_num;
						in.seekg(old_pos);
						line_num--;
						break;
					}
				}
			}
			// 2. if not block split symbol, a normal instruction
			//    store into current block and move to next line
			else {
				// TODO generat instr
				// printf("%s\n", current_ins.ins_string);
				//current_block->ins_vector.push_back(current_ins);
				//current_block->ins_pointer_list.push_back(&(current_block->ins_vector[current_block->block_ins_num]));

				if (current_block->block_ins_num == kMaxInsNumPerBlock) {
					PrintError("The basic block (line %d) contains too many instructions (max %d)\n", line_num, kMaxInsNumPerBlock);
					Error();
				}

				//current_block->ins_pointer_list.push_back(&(current_block->block_ins[current_block->block_ins_num]));
				current_block->block_ins_num += 1;
				current_block->type = BlockType::Instruction;
				// store the current file position  (end of this line)
				old_pos = in.tellg();
			}
		}
	}
	in.close();
	
	if (loop_mark_check > 0) {
		PrintError("The numbers of .loop and .endloop do not match ( %d more .loop), please check the code. \n", loop_mark_check);
		Error();
	}
	if (loop_mark_check < 0) {
		PrintError("The numbers of .loop and .endloop do not match ( %d more .endloop), please check the code. \n", loop_mark_check);
		Error();
	}


	// Determines the register type of the AR_OR
	for (auto block = linear_program.block_pointer_list.rbegin(); block != linear_program.block_pointer_list.rend(); block++) {
		for (int ins_index = (*block)->block_ins_num - 1; ins_index >= 0; ins_index--) {
			Instruction* ins = &(*block)->block_ins[ins_index];
	
			bool exist_AR_OR = false;
			bool exist_AR = false;
			bool exist_OR = false;

			string varname = "";
			RegType type = RegType::Default;

			for (int i = 0; i < 3; i++) {
				// input isn't null and not contains multiple variables
				if (ins->input[i] != "" && var_map.count(ins->input[i])) {
					if (var_map[ins->input[i]].reg_type == RegType::AR_OR) {
						exist_AR_OR = true;
						varname = ins->input[i];
					}
					else if (var_map[ins->input[i]].reg_type == RegType::S_AR || var_map[ins->input[i]].reg_type == RegType::V_AR) {
						exist_AR = true;
						type = var_map[ins->input[i]].reg_type;
					}
					else if (var_map[ins->input[i]].reg_type == RegType::S_OR || var_map[ins->input[i]].reg_type == RegType::V_OR) {
						exist_OR = true;
						type = var_map[ins->input[i]].reg_type;
					}
				}
			}
			if (var_map.count(ins->output)) {
				if (var_map[ins->output].reg_type == RegType::AR_OR) {
					exist_AR_OR = true;
					varname = ins->output;
				}
				else if (var_map[ins->output].reg_type == RegType::S_AR || var_map[ins->output].reg_type == RegType::V_AR) {
					exist_AR = true;
					type = var_map[ins->output].reg_type;
				}
				else if (var_map[ins->output].reg_type == RegType::S_OR || var_map[ins->output].reg_type == RegType::V_OR) {
					exist_OR = true;
					type = var_map[ins->output].reg_type;
				}
			}
			if (exist_AR && exist_OR) {
				PrintError("The instruction %s can't contain both AR and OR register types. \n", ins->ins_string);
			}

			if (exist_AR_OR) {
				if (exist_AR || exist_OR) {
					var_map[varname].reg_type = type;
				}
			}
		}
	}

	// check if all used vars have determined regs
	int error_flag = 0;
	vector<string> unused_var;
	for (auto v : var_map)
	{
		if (v.second.use_flag == 1) {
			if (v.second.reg_type == RegType::AR_OR || v.second.reg_type == RegType::Default) {
				error_flag = 1;
				PrintError("The variable %s should have a only certain register type instead of %s. \n", v.first.c_str(), kRegTypeStr[int(v.second.reg_type) + 1].c_str());
			}
		}
		else {
			PrintWarn("The variable %s is declared but not used in the program (auto removed). \n", v.first.c_str());
			if(v.second.type == VarType::Input)
				var_map[v.first].reg_type = RegType::R;
			else unused_var.push_back(v.first);
		}

	}
	for (auto v : unused_var)	var_map.erase(v);
	if (error_flag == 1) Error();


	for (int i = 0; i < linear_program.block_pointer_list.size(); i++) {
		for (int j = 0; j < linear_program.block_pointer_list[i]->block_ins_num; j++) {
			linear_program.block_pointer_list[i]->ins_pointer_list.push_back(&(linear_program.block_pointer_list[i]->block_ins[j]));
		}
	}

	LoadCycle();

	//GenReadOrder();	// modified by DC

	PrintInfo("The linear program %s is sucessfully loaded.\n", linear_fliename);
}

// TODO
void GenerateInstr(char* one_line_code, Instruction* ins, int line_num, int block_ins_num)
{
	// set ins_string
	ins->ins_string = (char*)malloc(strlen(one_line_code) * sizeof(char) + 4);
	strcpy(ins->ins_string, one_line_code);
	
	// one_line_code end with \0
	int i, j;
	char first_word[kMaxWordLength], second_word[kMaxWordLength];
	int num_blanks = GetFirstWord(first_word, one_line_code);
	num_blanks += GetFirstWord(second_word, one_line_code + num_blanks + strlen(first_word));
	// case 0: if reference instruction
	if (first_word[0] != '.') {
		// set the instruction type (1: instruction)
		ins->type = InsType::ReferIns;
		// condition register: cond_field and cond_flag
		for (i = 0; one_line_code[i] == '\t' || one_line_code[i] == '|' || one_line_code[i] == ' '; i++);
		if (one_line_code[i] == '[') {
			if (one_line_code[++i] == '!') {
				i++;
				ins->cond_flag = 1;
			}
			for (j = i; one_line_code[i] != ']' && one_line_code[i] != '\0'; i++);
			if (one_line_code[i] == '\0') {
				PrintError("Line %d: Lack of \']\' after condiction variable.\n\t-%s", line_num, one_line_code);
				Error();
			}
			ins->cond_field = one_line_code + j;
			ins->cond_field.resize(i - j);
			ins->extracted_var.insert(ins->cond_field);
			i++;
		}
		//set ins_name and func_unit
		while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
		if (one_line_code[i] == ',') {
			PrintError("Line %d: Please delete \',\' between condiction variable and instruction.\n\t-%s", line_num, one_line_code);
			Error();
		}
		for (j = 0; one_line_code[i] != ' ' && one_line_code[i] != '\t' && one_line_code[i] != '\0'; i++, j++)
			ins->ins_name[j] = one_line_code[i];
		ins->ins_name[j] = '\0';
		if ((j = FindChar(ins->ins_name, '.')) != -1 && !IsSubString(ins->ins_name, ".L")) {
			ins->ins_name[j] = '\0';
		}
		int refer_ins_index = GetReferIns(ins->ins_name);
		if (refer_ins_index == refer_table.table.size()) {
			PrintError("The instruction %s in Line %d is not found in the reference file.\n", ins->ins_name, line_num);
			Error();
		}
		strcpy(ins->func_unit, refer_table.table[refer_ins_index].func_unit);
		
		// set cycles
		ins->cycle = refer_table.table[refer_ins_index].cycle;
		ins->read_cycle = refer_table.table[refer_ins_index].read_cycle;
		ins->write_cycle = refer_table.table[refer_ins_index].write_cycle;
		
		// cond variable recheck
		if (ins->cond_field != "") {
			VarDeclarationCheck(ins->cond_field, line_num);
			RegType current_type = RegType::Default;
			if (ins->ins_name[0] == 'V') current_type = RegType::VR_COND;
			else current_type = RegType::R_COND;
			if (var_map[ins->cond_field].reg_type == RegType::Default)
				var_map[ins->cond_field].reg_type = current_type;
			else if (var_map[ins->cond_field].reg_type != current_type) {
				if (var_map[ins->cond_field].reg_type == RegType::R
					&& current_type == RegType::R_COND) {
					var_map[ins->cond_field].reg_type = current_type;
				}
				else if (var_map[ins->cond_field].reg_type == RegType::VR
					&& current_type == RegType::VR_COND) {
					var_map[ins->cond_field].reg_type = current_type;
				}
				else
				{
					PrintError("In line %d, variable is used as %s and %s at the same time\n."
						, line_num, kRegTypeStr[int(current_type) + 1].c_str(), kRegTypeStr[int(var_map[ins->cond_field].reg_type) + 1].c_str());
					Error();
				}

			}
		}

		
		// set input 
		while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
		if (one_line_code[i] == ',') {
			PrintError("Line %d: Please delete \',\' between instrunction and the first operand.\n\t-%s", line_num, one_line_code);
			Error();
		}
		// input 0 
		if (strcmp(refer_table.table[refer_ins_index].input[0], "")) {
			if (one_line_code[i] == ';' || one_line_code[i] == '\0') {
				PrintError("Line %d: Instruction lacks of operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			for (j = i; one_line_code[i] != ',' && one_line_code[i] != '\0' && one_line_code[i] != '\t' && one_line_code[i] != ' '; i++);
			ins->input[0] = one_line_code + j;
			ins->input[0].resize(i - j);
			VarRegConfirm(0, ins->func_unit, refer_table.table[refer_ins_index].input[0], ins->input[0], line_num, ins);
			while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
		}

		// input 1
		if (strcmp(refer_table.table[refer_ins_index].input[1], "")) {
			if (one_line_code[i] == ';' || one_line_code[i] == '\0') {
				PrintError("Line %d: Instruction lacks of operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			else if (one_line_code[i] != ',') {
				PrintError("Line %d: Lacks of \',\' between operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			else i++;
			while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
			if (one_line_code[i] == ';' || one_line_code[i] == '\0') {
				PrintError("Line %d: Instruction lacks of operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			for (j = i; one_line_code[i] != ',' && one_line_code[i] != '\0' && one_line_code[i] != '\t' && one_line_code[i] != ' '; i++);
			ins->input[1] = one_line_code + j;
			ins->input[1].resize(i - j);
			VarRegConfirm(0, ins->func_unit, refer_table.table[refer_ins_index].input[1], ins->input[1], line_num, ins);
			while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
		}

		// input 2
		if (strcmp(refer_table.table[refer_ins_index].input[2], "")) {
			if (one_line_code[i] == ';' || one_line_code[i] == '\0') {
				PrintError("Line %d: Instruction lacks of operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			else if (one_line_code[i] != ',') {
				PrintError("Line %d: Lacks of \',\' between operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			else i++;
			while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
			if (one_line_code[i] == ';' || one_line_code[i] == '\0') {
				PrintError("Line %d: Instruction lacks of operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			for (j = i; one_line_code[i] != ',' && one_line_code[i] != '\0' && one_line_code[i] != '\t' && one_line_code[i] != ' '; i++);
			ins->input[2] = one_line_code + j;
			ins->input[2].resize(i - j);
			VarRegConfirm(0, ins->func_unit, refer_table.table[refer_ins_index].input[2], ins->input[2], line_num, ins);
			while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;

		}

		// output
		if (strcmp(refer_table.table[refer_ins_index].output, "")) {
			if (one_line_code[i] == ';' || one_line_code[i] == '\0') {
				PrintError("Line %d: Instruction lacks of operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			else if (one_line_code[i] != ',') {
				PrintError("Line %d: Lacks of \',\' between operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			else i++;
			while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
			if (one_line_code[i] == ';' || one_line_code[i] == '\0') {
				printf("ERROR(Line %d): Instruction lacks of operands.\n\t-%s", line_num, one_line_code);
				Error();
			}
			for (j = i; one_line_code[i] != ',' && one_line_code[i] != '\0' && one_line_code[i] != '\t' && one_line_code[i] != ' '; i++);
			ins->output = one_line_code + j;
			ins->output.resize(i - j);
			VarRegConfirm(1, ins->func_unit, refer_table.table[refer_ins_index].output, ins->output, line_num, ins);
			while (one_line_code[i] == '\t' || one_line_code[i] == ' ') i++;
		}
		// input[2] and output must be the same
		if ((core == 32 || core == 34) && !strcmp(refer_table.table[refer_ins_index].output, refer_table.table[refer_ins_index].input[2]) && ins->input[2] != ins->output) {
			PrintError("Line %d: input[2] and output must be the same.\n\t-%s\n", line_num, one_line_code);
			Error();
		}

		// other parts in the one_line_code
		if (one_line_code[i] == ',') {
			PrintError("Line %d: Operands are more than expected.\n\t-%s", line_num, one_line_code);
			Error();
		}
		else if (one_line_code[i] != ';' && one_line_code[i] != '\0') {
			PrintError("Line %d: Unexpected statement after the operand.\n\t-%s", line_num, one_line_code);
			Error();
		}
		// 
		if (block_ins_num) {
			string ins_name(ins->ins_name);
			regex vlddwm("VLDDW\[0,1]M\[2, 4]");
			regex vstdwm("VSTDW\[0,1]M\(16|32)");
			if (regex_match(ins_name, vlddwm) || regex_match(ins_name, vstdwm)) {
				Instruction* pre_ins = &current_block->block_ins[block_ins_num - 1];
				string pre_ins_name(pre_ins->ins_name);
				if (regex_match(pre_ins_name, vlddwm) || regex_match(pre_ins_name, vstdwm)) {
					if (pre_ins->partner == NULL) {
						if (pre_ins_name.substr(6, 2) == ins_name.substr(6, 2) && pre_ins_name[5] != ins_name[5]) {
							pre_ins->partner = ins;
							ins->partner = pre_ins;
						}
					}
				}
			}
		}
	}
	// case 1: if a label (starts with '.')
	else
	{	// set the instruction type (0: label)
		ins->type = InsType::Label;
		//  extract the first word (label)
		strcpy(ins->ins_name, first_word);
		const char* split_symbol = ",";
		// handle .input
		if (!strcmp(first_word, ".input")) {
			char* input_var;
			input_var = strtok(one_line_code + 6, split_symbol);
			int input_max_amount = reg_in.size();
			while (input_var != NULL)
			{
				input_max_amount--;
				//printf("%s == \n", input_var);
				// remove spaces in the front
				while (isspace((unsigned char)*input_var)) input_var += 1;
				for (i = 0; i < strlen(input_var); i++) {
					if (input_var[i] == ';' || input_var[i] == ' ' || input_var[i] == '\t')
						input_var[i] = '\0';
				}
				if (input_var[0] == '\0') {
					input_var = strtok(NULL, split_symbol);
					continue;
				}

				
				// store into var_set
				// if input_var is a var pair
				int pair_split = FindChar(input_var, ':');
				if (pair_split != -1) {
					Variable input_x_0, input_x_1;
					input_x_1.name = input_var + pair_split + 1;
					input_var[pair_split] = '\0';
					input_x_0.name = input_var;
					input_x_0.type = VarType::Input;
					input_x_1.type = VarType::Input;
					input_x_0.pair_pos = 0;
					input_x_1.pair_pos = -1;
					input_x_0.assign_flag = 1;
					input_x_1.assign_flag = 1;
					input_x_0.partner = input_x_1.name;
					input_x_1.partner = input_x_0.name;
					var_map.insert(pair<string, Variable>(input_x_0.name, input_x_0));
					var_map.insert(pair<string, Variable>(input_x_1.name, input_x_1));
					linear_program.var_in.push_back(input_x_0.name);
					linear_program.var_in.push_back(input_x_1.name);

				}
				// input_var is a single var
				else {
					Variable input_x;
					input_x.name = input_var;
					input_x.type = VarType::Input;
					input_x.assign_flag = 1;
					var_map.insert(pair<string, Variable>(input_x.name, input_x));
					linear_program.var_in.push_back(input_x.name);
				}
				input_var = strtok(NULL, split_symbol);
				//printf("%s = \n", input_var);
			}
			//PrintDebug("input finished.\n");
			if (input_max_amount < 0) {
				PrintError("Line %d: .Too many input variables.\n\t-%s\n", line_num, one_line_code);
				Error();
			}
		}
		
		// handle .output
		else if (!strcmp(first_word, ".output")) {
			char* output_var;
			output_var = strtok(one_line_code + 7, split_symbol);
			int count = 0;
			while (output_var != NULL)
			{
				count++;
				// remove spaces in the front
				while (isspace((unsigned char)*output_var)) output_var += 1;
				for (i = 0; i < strlen(output_var); i++) {
					if (output_var[i] == ';' || output_var[i] == ' ' || output_var[i] == '\t')
						output_var[i] = '\0';
				}
				if (output_var[0] == '\0') {
					output_var = strtok(NULL, split_symbol);
					continue;
				}
				//printf("%s,", input_var);
				// store into var_set
				int pair_split = FindChar(output_var, ':');
				if (pair_split != -1) {
					Variable output_x_0, output_x_1;
					output_x_1.name = output_var + pair_split + 1;
					output_var[pair_split] = '\0';
					output_x_0.name = output_var;
					output_x_0.type = VarType::Output;
					output_x_1.type = VarType::Output;
					output_x_0.pair_pos = 0;
					output_x_1.pair_pos = -1;
					output_x_0.partner = output_x_1.name;
					output_x_1.partner = output_x_0.name;
					output_x_0.reg_type = RegType::R;
					output_x_1.reg_type = RegType::R;
					var_map.insert(pair<string, Variable>(output_x_0.name, output_x_0));
					var_map.insert(pair<string, Variable>(output_x_1.name, output_x_1));
					linear_program.var_out.push_back(output_x_0.name);
					linear_program.var_out.push_back(output_x_1.name);
				}
				else {
					Variable output_x;
					output_x.name = output_var;
					output_x.type = VarType::Output;
					output_x.reg_type = RegType::R;
					var_map.insert(pair<string, Variable>(output_x.name, output_x));
					linear_program.var_out.push_back(output_x.name);
				}

				output_var = strtok(NULL, split_symbol);

			}
			if (count > 1) {
				PrintError("Line %d: .Too many output variables.\n\t-%s\n", line_num, one_line_code);
				Error();
			}
		}
		// handle .gen_var
		else if (!strcmp(first_word, ".gen_var")) {
			char* gen_var;
			gen_var = strtok(one_line_code + 8, split_symbol);
			while (gen_var != NULL)
			{
				// remove spaces in the front
				while (isspace((unsigned char)*gen_var)) gen_var += 1;
				for (i = 0; i < strlen(gen_var); i++) {
					if (gen_var[i] == ';' || gen_var[i] == ' ' || gen_var[i] == '\t')
						gen_var[i] = '\0';
				}
				if (gen_var[0] == '\0') {
					gen_var = strtok(NULL, split_symbol);
					continue;
				}
				//printf("%s,", input_var);
				// store into var_set
				int pair_split = FindChar(gen_var, ':');
				if (pair_split != -1) {
					Variable gen_x_0, gen_x_1;
					gen_x_1.name = gen_var + pair_split + 1;
					gen_var[pair_split] = '\0';
					gen_x_0.name = gen_var;
					gen_x_0.type = VarType::General;
					gen_x_1.type = VarType::General;
					gen_x_0.pair_pos = 0;
					gen_x_1.pair_pos = -1;
					gen_x_0.partner = gen_x_1.name;
					gen_x_1.partner = gen_x_0.name;

					var_map.insert(pair<string, Variable>(gen_x_0.name, gen_x_0));
					var_map.insert(pair<string, Variable>(gen_x_1.name, gen_x_1));
				}
				else {
					Variable gen_x;
					gen_x.name = gen_var;
					gen_x.type = VarType::General;
					var_map.insert(pair<string, Variable>(gen_x.name, gen_x));
				}
				gen_var = strtok(NULL, split_symbol);

			}
		}
		// handle .add_var
		else if (!strcmp(first_word, ".add_var")) {
			char* add_var;
			add_var = strtok(one_line_code + 8, split_symbol);
			while (add_var != NULL)
			{
				// remove spaces in the front
				while (isspace((unsigned char)*add_var)) add_var += 1;
				for (i = 0; i < strlen(add_var); i++) {
					if (add_var[i] == ';' || add_var[i] == ' ' || add_var[i] == '\t')
						add_var[i] = '\0';
				}
				if (add_var[0] == '\0') {
					add_var = strtok(NULL, split_symbol);
					continue;
				}
				// store into var_set
				Variable add_x;
				add_x.name = add_var;
				add_x.type = VarType::Address;
				var_map.insert(pair<string, Variable>(add_x.name, add_x));
				add_var = strtok(NULL, split_symbol);

			}
		}
		// handle .loop
		else if (!strcmp(second_word, ".loop")) {
			strcpy(ins->ins_name, second_word);
			ins->input[0] = first_word;
			// unroll(x)
			char loop_config_str[20];
			char unroll_factor[20];
			char trip_num[20];
			int tmplen = num_blanks + strlen(first_word) + strlen(second_word);
			tmplen += GetFirstWord(loop_config_str, one_line_code + num_blanks + strlen(first_word) + strlen(second_word));
			if (block_ins_num == 0) {
				while (loop_config_str[0] != '\0') {
					//if (strstr(loop_config_str, "UNROLL") || strstr(loop_config_str, "unroll")) 
					if (strncmp(loop_config_str, ".UNROLL", 7) == 0 || strncmp(loop_config_str, ".unroll", 7) == 0)
					{
						j = 0;
						if (strlen(loop_config_str) < 9) {
							PrintWarn("loop configure option %s in Line %d is too short for unroll\n", loop_config_str, line_num);
							tmplen += strlen(loop_config_str);
							tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
							continue;
						}
						if (loop_config_str[7] != '(') {
							PrintWarn("Wrong format in loop configure option %s in Line %d: after .unroll(.UNROLL) should be a ( \n", loop_config_str, line_num);
							tmplen += strlen(loop_config_str);
							tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
							continue;
						}
						else
						{	
							int error_flag = 0;
							for (i = 8; i < strlen(loop_config_str) - 1; i++) {
								if (loop_config_str[i] >= '0' && loop_config_str[i] <= '9') {
									unroll_factor[j] = loop_config_str[i];
									j++;
								}
								else {
									error_flag = 1;
									break;
								}
							}
							if (error_flag == 1) {
								PrintWarn("Wrong format in loop configure option %s in Line %d: not a legal number in ()\n", loop_config_str, line_num);
								tmplen += strlen(loop_config_str);
								tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
								continue;
							}
							unroll_factor[j] = '\0';
							if (loop_config_str[i] != ')') {
								PrintWarn("Wrong format in loop configure option %s in Line %d: in the end, should be a )\n", loop_config_str, line_num);
								tmplen += strlen(loop_config_str);
								tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
								continue;
							}
							ins->input[1] = unroll_factor;
						}


					}
					//else if (strstr(loop_config_str, "PIPELINE") || strstr(loop_config_str, "pipeline")) 
					else if (strncmp(loop_config_str, ".PIPELINE", 9) == 0 || strncmp(loop_config_str, ".pipeline", 9) == 0)
					{
						if (strlen(loop_config_str) == 9)
							ins->output = "PIPELINE";
						else {
							PrintWarn("pipeline loop configure option should be a single .pipeline, not %s in line %d \n", loop_config_str, line_num);
							tmplen += strlen(loop_config_str);
							tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
							continue;
						}
					}
					//else if (strstr(loop_config_str, "TRIP") || strstr(loop_config_str, "trip")) 
					else if (strncmp(loop_config_str, ".TRIP", 5) == 0 || strncmp(loop_config_str, ".trip", 5) == 0)
					{
						j = 0;
						if (strlen(loop_config_str) < 7) {
							PrintWarn("loop configure option %s in Line %d is too short for trip \n", loop_config_str, line_num);
							tmplen += strlen(loop_config_str);
							tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
							continue;
						}
						if (loop_config_str[5] != '(') {
							PrintWarn("Wrong format in loop configure option %s in Line %d: after .trip(.TRIP) should be a ( \n", loop_config_str, line_num);
							tmplen += strlen(loop_config_str);
							tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
							continue;
						}
						else
						{
							int error_flag = 0;
							for (i = 6; i < strlen(loop_config_str) - 1; i++) {
								if (loop_config_str[i] >= '0' && loop_config_str[i] <= '9') {
									trip_num[j] = loop_config_str[i];
									j++;
								}
								else {
									//PrintWarn("Wrong format in loop configure option %s in Line %d\n", loop_config_str, line_num);
									error_flag = 1;
									break;
								}
							}
							trip_num[j] = '\0';
							if (error_flag == 1) {
								PrintWarn("Wrong format in loop configure option %s in Line %d: not a legal number in ()\n", loop_config_str, line_num);
								tmplen += strlen(loop_config_str);
								tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
								continue;
							}
							if (loop_config_str[i] != ')') {
								PrintWarn("Wrong format in loop configure option %s in Line %d: in the end, should be a )\n", loop_config_str, line_num);
								tmplen += strlen(loop_config_str);
								tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
								continue;
							}
							else {
								ins->input[2] = trip_num;
							}
							
						}
					}
					else {
						PrintWarn("Unknown loop configure option %s in Line %d: support unroll, pipeline and trip now.\n", loop_config_str, line_num);
						tmplen += strlen(loop_config_str);
						tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
						continue;
					}
					tmplen += strlen(loop_config_str);
					tmplen += GetFirstWord(loop_config_str, one_line_code + tmplen);
				}
			}
		}
		// handle .endloop
		else if (!strcmp(first_word, ".endloop")) {
			strcpy(ins->ins_name, first_word);
		}
		else if (!strcmp(first_word, ".global")) {
			strcpy(ins->ins_name, first_word);
			ins->input[0] = second_word;
			string blanks("\t ");
			ins->input[0].erase(0, ins->input[0].find_first_not_of(blanks));
			ins->input[0].erase(ins->input[0].find_last_not_of(blanks) + 1);
		}
		else if (!strcmp(first_word, ".size")) {
			strcpy(ins->ins_name, first_word);
		}
		else if (!strcmp(second_word, ".close")) {
			strcpy(ins->ins_name, second_word);
		}
		else if (!strcmp(first_word, ".open")) {
			strcpy(ins->ins_name, first_word);
		}
		// handle customized label
		// start with . and end with :
		else if (first_word[strlen(first_word) - 1] == ':'){
			first_word[strlen(first_word) - 1] = '\0';
			strcpy(ins->ins_name, first_word);
		}
		// other label
		else {
			PrintError("Unsuported label in line %d, please check the code and format.\n", line_num);
			Error();
		}
	}
}


void LoadReference(const string& refer_option) {
	int i = 0, j, k, num_of_entries;
	char buf[32];
	string label, line;

	SelectReferenceSrc(refer_option);
	if (!refer_src.size()) {
		PrintError("The reference information is invalid.\n");
		Error();
	}
	else PrintInfo("Reference source \"%s\" starts loading...\n", refer_option.c_str());

	while (i < refer_src.size())
	{
		label = "";
		line = refer_src[i++];
		for (int j = 0; line[j] != ','; ++j)
			label += line[j];
		//printf("%s\n", label.c_str());
		num_of_entries = 0;
		// 0. handle the core
		if (label == "CORE") {
			line = refer_src[i];
			for (j = 0; line[j] != ','; j++)
				buf[j] = line[j];
			buf[j] = '\0';
			core = atoi(buf);
			PrintInfo("DSP CORE: %d\n", core);
			i += 2;
		}
		// 1. handle the instructions
		else if (label == "INSTRUCTION") {
			i++;
			// skip the title line in ths instruction part
			while (i < refer_src.size()) {
				line = refer_src[i++];
				if (line[0] == ',') break;
				ReferInstruction x;
				for (j = 0, k = 0; line[j] != ','; ++j, ++k)
					x.name[k] = line[j];
				x.name[k] = '\0';
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					x.func_unit[k] = line[j];;
				x.func_unit[k] = '\0';
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					buf[k] = line[j];
				buf[k] = '\0';
				x.cycle = atoi(buf);
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					x.input[0][k] = line[j];
				x.input[0][k] = '\0';
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					x.input[1][k] = line[j];
				x.input[1][k] = '\0';
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					x.input[2][k] = line[j];
				x.input[2][k] = '\0';
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					x.output[k] = line[j];
				x.output[k] = '\0';
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					buf[k] = line[j];
				buf[k] = '\0';
				x.read_cycle = atoi(buf);
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					buf[k] = line[j];
				buf[k] = '\0';
				x.write_cycle = atoi(buf);
				refer_table.table.push_back(x);
				num_of_entries++;
			}
			PrintInfo("Reference instructions: %d\n", num_of_entries);
		}
		// 2. handle numbers of function units
		else if (label == "FUNC_UNIT") {
			while (i < refer_src.size()) {
				line = refer_src[i++];
				if (line[0] == ',') break;
				int index;
				string function_unit_name = "";
				for (j = 0; line[j] != ','; ++j)
					function_unit_name += line[j];
				for (k = 0, j++; line[j] != ','; ++j, ++k)
					buf[k] = line[j];
				buf[k] = '\0';
				index = atoi(buf);
				function_units_index[function_unit_name] = index;
				num_of_entries = index + 1;
			}
			PrintInfo("Function Units: %d\n", num_of_entries);
		}
		// 3. handle R_CTRL
		else if (label == "R_CTRL") {
			while (i < refer_src.size()) {
				line = refer_src[i++];
				if (line[0] == ',') break;
				string reg = "";
				for (j = 0; line[j] != ','; ++j)
					reg += line[j];
				reg_ctrl[0].push_back(reg);
				num_of_entries++;
			}
			PrintInfo("SMVCGC/SMVCCG Reg: %d\n", num_of_entries);
		}
		// 4. handle VR_CTRL
		else if (label == "VR_CTRL") {
			while (i < refer_src.size()) {
				line = refer_src[i++];
				if (line[0] == ',') break;
				string reg = "";
				for (j = 0; line[j] != ','; ++j)
					reg += line[j];
				reg_ctrl[1].push_back(reg);
				num_of_entries++;
			}
			PrintInfo("VMVCGC/VMVCCG Reg: %d\n", num_of_entries);
		}
		// 5. handle REG_AMOUNT
		else if (label == "REG_AMOUNT") {
			while (i < refer_src.size()) {
				line = refer_src[i++];
				if (line[0] == ',') break;
				for (j = 0; line[j] != ','; ++j);
				for (k = 0, ++j; line[j] != ','; ++j, ++k)
					buf[k] = line[j];
				buf[k] = '\0';
				reg_amount.push_back(atoi(buf));
				num_of_entries++;
			}
			sv_interval = reg_amount[3];
			PrintInfo("Reg Type: %d\n", num_of_entries);
		}
		// 6. handle REG_RES
		else if (label == "REG_RES") {
			while (i < refer_src.size()) {
				line = refer_src[i++];
				if (line[0] == ',') break;
				for (j = 0; line[j] != ','; ++j);
				vector<int> reg_buf;
				for (j++; j < line.size() && line[j] != ','; ++j) {
					for (k = 0; line[j] != ',' && line[j] != '\0'; ++j, ++k)
						buf[k] = line[j];
					buf[k] = '\0';
					reg_buf.push_back(atoi(buf));
				}
				reg_res.push_back(reg_buf);
				num_of_entries++;
			}
			PrintInfo("Reserve Reg Type: %d\n", num_of_entries);
		}
		// 7. handle REG_SPEC
		else if (label == "REG_SPEC") {
			while (i < refer_src.size()) {
				line = refer_src[i++];
				if (line[0] == ',') break;
				string type = "";
				for (j = 0; line[j] != ','; ++j)
					type += line[j];
				if (type == "IN") {
					int start, end;
					for (k = 0, ++j; line[j] != ','; ++j, ++k)
						buf[k] = line[j];
					buf[k] = '\0';
					start = atoi(buf);
					for (k = 0, ++j; line[j] != ','; ++j, ++k)
						buf[k] = line[j];
					buf[k] = '\0';
					end = atoi(buf);
					for (k = start; k <= end; k += 2)
						reg_in.push_back(k);
				}
				else if (type == "OUT") {
					for (k = 0, ++j; line[j] != ','; ++j, ++k)
						buf[k] = line[j];
					buf[k] = '\0';
					reg_out = atoi(buf);
				}
				else if (type == "RETURN") {
					for (k = 0, ++j; line[j] != ','; ++j, ++k)
						buf[k] = line[j];
					buf[k] = '\0';
					reg_return = atoi(buf);
				}
				else if (type == "S_STACK") {
					for (k = 0, ++j; line[j] != ','; ++j, ++k)
						buf[k] = line[j];
					buf[k] = '\0';
					scalar_stack_AR = atoi(buf) - sv_interval;
				}
				else if (type == "V_STACK") {
					for (k = 0, ++j; line[j] != ','; ++j, ++k)
						buf[k] = line[j];
					buf[k] = '\0';
					vector_stack_AR = atoi(buf);
				}
				num_of_entries++;
			}
			PrintInfo("Specific Reg type: %d\n", num_of_entries);
		}
	}
	// check exceptions
	if (core == -1) {
		PrintError("The reference file is lack of core information.\n");
		Error();
	}
	if (refer_table.table.size() == 0) {
		PrintError("The reference file is lack of instruction information.\n");
		Error();
	}
	if (function_units_index.size() == 0) {
		PrintError("The reference file is lack of function unit information.\n");
		Error();
	}
	if (reg_ctrl[0].size() == 0 || reg_ctrl[1].size() == 0) {
		PrintError("The reference file is lack of control register information.\n");
		Error();
	}
	if (reg_amount.size() == 0) {
		PrintError("The reference file is lack of register amount information.\n");
		Error();
	}
	if (reg_res.size() == 0) {
		PrintError("The reference file is lack of reserve register information.\n");
		Error();
	}
	if (reg_in.size() == 0 || reg_out == -1 || reg_return == -1 || scalar_stack_AR == -1 || vector_stack_AR == -1) {
		PrintError("The reference file is lack of specific register information.\n");
		Error();
	}
	PrintInfo("Reference file is sucessfully loaded.\n\n\n");
}

void CsvInternalization(const string& csv_file, const string& output) {
	FILE* fpin = fopen(csv_file.c_str(), "r");

	if (fpin == NULL) {
		PrintError("The reference file \"%s\" doesn\'t exist.\n", csv_file.c_str());
		exit(EXIT_FAILURE);
	}

	char line[256];
	FILE* fpout = fopen(output.c_str(), "w");
	fprintf(fpout, "#pragma once\n#include \"analyser.h\"\n\nusing namespace std;\n\nvector<string> reference = {\n");

	if (fscanf(fpin, "%s", line) != EOF) {
		fprintf(fpout, "            \"%s\"", line);
		while (fscanf(fpin, "%s", line) != EOF)
		{
			fprintf(fpout, ",\n            \"%s\"", line);
		}
	}

	fprintf(fpout, "\n};\n");
	fclose(fpin);
	fclose(fpout);
}

string RegTypeToString(RegType type) {
	if (type == RegType::Default)
		return "Default";
	else if (type == RegType::R)
		return "R";
	else if (type == RegType::VR)
		return "VR";
	else if (type == RegType::S_AR)
		return "S_AR";
	else if (type == RegType::V_AR)
		return "V_AR";
	else if (type == RegType::S_OR)
		return "S_OR";
	else if (type == RegType::V_OR)
		return "V_OR";
	else if (type == RegType::R_COND)
		return "R_COND";
	else if (type == RegType::VR_COND)
		return "VR_COND";
	else return "AR_OR";
}


void LoadCycle() {
	for (int i = 0; i < linear_program.total_block_num; i++) {
		if (linear_program.block_pointer_list[i]->type != BlockType::Instruction) continue;
		for (int j = 0; j < linear_program.program_block[i].block_ins_num; j++) {
			Instruction* ins = linear_program.block_pointer_list[i]->ins_pointer_list[j];
			if (ins->cycle != -1) continue;
			//printf("%s\n", ins->ins_name);
			int refer_index = GetReferIns(ins->ins_name);
			for (int index = refer_index + 1; index < refer_table.table.size(); index++) {
				ReferInstruction refer_ins = refer_table.table[index];
				if (strcmp(ins->ins_name, refer_ins.name))
					break;
				//printf("%s\n", refer_ins.name);
				bool flag = true;
				for (int k = 0; k < 3; k++) {
					if (strcmp(refer_ins.input[k], refer_table.table[refer_index].input[k])) {
						string reg_input;
						// CR CVR
						if (var_map[ins->input[k]].reg_type == RegType::Default) {
							reg_input = ins->input[k];
							while (reg_input[reg_input.length() - 1] <= '9' && reg_input[reg_input.length() - 1] >= '0') {
								reg_input.resize(reg_input.length() - 1);
							}
						}
						else
							reg_input = RegTypeToString(var_map[ins->input[k]].reg_type);
						reg_input = "/" + reg_input + "/";
						if (!IsSubString(refer_ins.input[k], reg_input.c_str())) {
							flag = false;
						}
					}
				}
				if (strcmp(refer_ins.output, refer_table.table[refer_index].output)) {
					string reg_output;
					// CR CVR
					if (var_map[ins->output].reg_type == RegType::Default) {
						reg_output = ins->output;
						while (reg_output[reg_output.length() - 1] <= '9' && reg_output[reg_output.length() - 1] >= '0') {
							reg_output.resize(reg_output.length() - 1);
						}
					}
					else
						reg_output = RegTypeToString(var_map[ins->output].reg_type);
					reg_output = "/" + reg_output + "/";
					if (!IsSubString(refer_ins.output, reg_output.c_str())) {
						flag = false;
					}
				}
				if (flag) {
					ins->cycle = refer_table.table[index].cycle;
					ins->read_cycle = refer_table.table[index].read_cycle;
					ins->write_cycle = refer_table.table[index].write_cycle;
					//printf("%d\n", ins->cycle);
					break;
				}

			}

			if (ins->cycle == -1) {
				string errorInfo = "";
				for (int k = 0; k < 3; k++) {
					if (ins->input[k] == "") break;
					RegType regType = var_map[ins->input[k]].reg_type;
					if (regType == RegType::Default) {
						errorInfo += ins->input[k] + ", ";
					}
					else errorInfo += RegTypeToString(regType) + ", ";
				}
				RegType regType = var_map[ins->output].reg_type;
				if (regType == RegType::Default) {
					errorInfo += ins->output;
				}
				else errorInfo += RegTypeToString(regType);
				PrintError("\"%s\" doesn't find the instructon of the corresponding register. %s    %s\n", ins->ins_string, ins->ins_name, errorInfo.c_str());
				Error();
			}
		}
	}
}

//void GenReadOrder() {
//	for (int i = 0; i < linear_program.block_pointer_list.size(); i++) {
//		for (int j = 0; j < linear_program.block_pointer_list[i]->ins_pointer_list.size(); j++) {
//			linear_program.block_pointer_list[i]->ins_pointer_list[j]->read_order = j;
//		}
//	}
//}