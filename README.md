# FT-Matrix Compiler

#### Introduction
FT-Matrix is a VLIW architecture. It is implemented on a GPDSP platform. The FT-Matrix compiler is developed with C++. 


#### Compiler Architecture
The input of FT-Matrix compiler is a low-level IR (.sa) and the output is assembly code (.asm). The major components of the compiler include: code analysis, dependency analysis, loop optimization, instruction scheduling, and register allocation.


#### Installation Tutorial
FT-Matrix is an EXE file, which can be directly invoked from the command line without installation.

The FT-Matrix compiler can also be inivoked by double-click on the FT-Matrix.exe file.

#### Directions for Use
The command line directly invokes the EXE executor, followed by two necessary input parameters, namely the input file (.sa) and the output assembly file (.asm). The full call format is as follows:

FT-MatrixLA.exe test.sa test_output.asm -I -Ox

When the input and output files are default, the default values for input and output files are ".//test.sa" and ".//test_output.asm". "-I" is used to specify the platform model or external instruction set description file, and "-Ox" is used to specify the user's optimization options for the code, and "-O0" and "-O1" are currently supported. When there are no assembly options, all necessary parameters are set by default.

The program execution process contains multiple outputs, mainly including statistics on the instruction set description file, and information tips for executing each module. If there is a problem with the user's program, it will give feedback and give an error prompt. When the linear assembler is executed successfully, the program outputs "Assembler file is successfully generated." The program will output graph.txt and test_transition.txt an intermediate file, which contains a dependency description file between user instructions and a linear assembly conversion scheduled as an intermediate result of assembly code (before register allocation), which can be used by the user for debugging.

###  Benchmarks
The benchmarks are stored in the "benchmarks" folder, containing 34 functions. During tests, move the target bench to the same folder of "FT-Matrix.exe" and modify the file name as "test.sa".

#### The Content

|--	The directory of FT-Matrix												

        |-- main.cpp										

        |-- aliveness_analysis.cpp							

        |-- aliveness_analysis.h							

        |-- analyser.cpp									

        |-- analyser.h										

        |-- build_dependency.cpp							

        |-- build_dependency.h								

        |-- data_structure.cpp								

        |-- data_structure.h								

        |-- ins_scheduling.cpp								

        |-- ins_scheduling.h								

        |-- loop_unrolling.cpp								

        |-- loop_unrolling.h								

        |-- reference.cpp									

        |-- register_assignment.cpp							

        |-- register_assignment.h							

        |-- software_pipeline.cpp							

        |-- software_pipeline.h								

        |-- utils.cpp										

        |-- utils.h											

        |-- resource.h										

        |-- favicon.ico										

        |-- others											

	|--	FT-MatrixLA.sln											

	|--	README.md												



	|--	test.sa													

	|--	test_output.asm											

	|--	test_transition.txt										

	|--	graph.txt												
