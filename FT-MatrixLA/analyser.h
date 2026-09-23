#pragma once
#include <stdio.h>
#include <cstdlib>
#include <regex>
#include "data_structure.h"

using namespace std;




bool IsCommentLine(char* one_line_code);


// functions used for semantic check
void VarDeclarationCheck(string ins_var, int line_num);
void VarAssignmentCheck(string ins_var, int line_num);

void VarRegConfirm(int is_output, char* func_unit, char* refer_var, string ins_var, int line_num, Instruction* ins);

void VarAssignmentCheck(string ins_var, int line_num);


// load reference file from refer_filename
// and store into refer_table (global variable)
//void LoadReference(const char* refer_filename);
void LoadReference(const string& refer_option);

// load linear assembly file
// and store into linear_program (global variable)
void LoadLinearProgram(const char* linear_fliename);


// generate an instruction from a line in the 
// linear assembly file
void GenerateInstr(char* one_line_code, Instruction* ins, int line_num, int block_ins_num);


void SelectReferenceSrc(const string& refer_option);

void CsvInternalization(const string& csv_file, const string& output);

string RegTypeToString(RegType type);

void LoadCycle();

// generate the read order of instruction	modified by DC
//void GenReadOrder();