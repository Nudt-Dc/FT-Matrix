#pragma once
#include <stdio.h>
#include <cstdlib>
#include <stdarg.h>
#include "data_structure.h"
#include <Windows.h>

#define PrintDebug(format, ...)      SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 3);\
                                            printf("[DEBUG][%-20s:%-4d]: " format, __func__,__LINE__,##__VA_ARGS__);\
												SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15)

#define PrintInfo(format, ...)       SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);\
                                            printf("[INFO ][%-20s:%-4d]: " format, __func__,__LINE__,##__VA_ARGS__);\
												SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15)

#define PrintWarn(format, ...)       SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13);\
                                            printf("[WARN ][%-20s:%-4d]: " format, __func__,__LINE__,##__VA_ARGS__);\
												SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15)

#define PrintError(format, ...)       SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);\
                                            printf("[ERROR][%-20s:%-4d]: " format, __func__,__LINE__,##__VA_ARGS__);\
												SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15)
// print a dashed line and exit
void Error();

// Formalize the process or error message
// content_level: -1 means it was a error message
// src_line     : -1 means could not locate the error line
void MessageFormalize(const int& content_level, const string& content, int src_line = -1, string src = "");

bool IsEmptyLine(char* line);


int GetFirstWord(char* dst, char* src);
void PrintVariables();
void PrintGlobalData();
int FindChar(string& source, char c);
int FindChar(char* source, char c);

// copy the content from instruction src to instruction dst
void CopyInstruction(Instruction* dst, Instruction* src);

// Determine whether the operand is a block label, a immediate or a control register
// Return 0 when it is a scalar control register
// Return 1 when it is a vector control register
// Return 2 when it is an immediate number
// Return 3 when it is a block label
// Others return -1
int IsSpecialOperand(const string& operand);

// Split the function units of the instructions 
vector<int> FunctionUnitSplit(string function_unit);

// Generate output file
void GenerateOutputFile(const string& output_file);

bool IsSubString(const char* str, const char* substr);

bool IsCtrlInstrction(const Instruction* operand);

// Calculate the ILP for each block
void CalculateILP();