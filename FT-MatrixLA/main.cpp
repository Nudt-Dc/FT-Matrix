#include <cstdio>
#include "analyser.h"
#include "utils.h"
#include "data_structure.h"
#include "build_dependency.h"
#include "loop_unrolling.h"
#include "ins_scheduling.h"
#include "aliveness_analysis.h"
#include "register_assignment.h"
#include "software_pipeline.h"
#include "ins_rescheduling.h"
#include <time.h>
using namespace std;

int main(int argc, char* argv[])
{
	string refer_option = "32";
	string input_file = ".\\test.sa";
	string output_file_1 = ".\\test_output_1.asm";
	string output_file = ".\\test_output_2.asm";
	string graph_file = ".\\graph.txt";
	string transition_file = ".\\test_transition_1.txt";
	string transition_file_2 = ".\\test_transition_2.txt";
	string optimize_option = "-O1";
	string compile_option = "";
	reschedule = "CLOSE";


	
	// step 0: deal with compile options
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] != '-') {
			input_file = argv[i++];
			if (input_file.find(".sa") == -1) {
				PrintError("\"%s\" isn\'t the correct format of input file.\n", input_file.c_str());
				Error();
			}
			if (i == argc) {
				PrintError("Lack of output file name.\n");
				Error();
			}
			output_file = argv[i];
			if (output_file.find(".asm") == -1) {
				PrintError("\"%s\" isn\'t the correct format of output file.\n", output_file.c_str());
				Error();
			}
			continue;
		}
		compile_option = argv[i];
		if (compile_option == "-O0" || compile_option == "-O1") {
			optimize_option = compile_option;
			if (optimize_option == "-O0") {
				PrintWarn("Pay attention that \"-O0\" will not support software pipeline.\n");
			}
		}
		/*else if (compile_option == "-G") {
			graph_file = ".\\graph.txt";
			transition_file = ".\\test_transition.txt";
		}*/
		else if (compile_option == "-I") {
			if (i == argc) {
				PrintError("Lack of reference file name, please add it after \"-I\".\n");
				Error();
			}
			refer_option = argv[++i];
			//if (reference_file.find(".csv") == -1) {
			//	PrintError("\"%s\" isn\'t the correct format of reference file.\n", reference_file.c_str());
			//	Error();
			//}
		}
		else {
			PrintError("\"%s\" is taken as an option but could not be recognized, please use option: \"-I [reference_file]\" , \"-O0/-O1\" or \"-G\".\n", compile_option.c_str());
			Error();
		}
	}


	// main procedures
	// step 1: read reference and linear assembly program
	//		   declared in analyser.h
	//LoadReference(reference_file.c_str());
	LoadReference(refer_option);

	LoadLinearProgram(input_file.c_str());


	//PrintVariables();

	// step 2: loop unrolling
	//         declared in loop_unrolling.h
	UnrollingProgram(1);

	// step 3: build dependency and print dependency file graph.txt
	//         declared in build_dependency.h
	// modified by CZY
	BuildDependency();
	PrintDependencyGraph(graph_file.c_str());

	// step 4: software pipeline 
	//         declared in software_pipeline.h
	if (optimize_option != "-O0") SoftwarePipeline();

	// step 5: scheduling  and print transition file trasition.asm
	//         declared in ins_scheduling.h
	//modified by CZY
	InsScheduling(optimize_option.c_str());
	PrintSchedulingTransistion(transition_file.c_str());
	//CalculateILP();	//modified by DC 23/3/13

	// step 6：aliveness analysis
	//         declared in aliveness_analysis.h
	AnalyseAliveness(1);


	// step 7：register assignment
	//         declared in register_assignment.h
	AssignRegister();


	// step 8: generate first scheduled asm file
	GenerateOutputFile(output_file_1);

	// step 9: ins recheduling
	//			declared in ins_rescheduling.h
	//start = clock();
	Rescheduling();

	// step 10: generate output file
	//         declared in utils.h
	GenerateOutputFile(output_file);


	CleanLinearProgram();

	PrintInfo("Assembler file \"%s\" is successfully generated.\n", output_file.c_str());
	printf("------------------------------------------------------------------------------\n");
	//system("pause");
	return 0;
	
}