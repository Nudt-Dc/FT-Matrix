# FT-Matrix Linear Assembler

#### 介绍
面向FT-Matrix自主DSP平台的线性汇编器工具FT-MatrixLA，主要辅助用户对核心级代码及高性能算法库进行开发与优化，能够降低核心代码的开发难度，提高开发效率。
FT-MatrixLA目前主要支持单个函数级代码的开发，与主函数的对接符合编译器对汇编编写的各种规范。

FT-MatrixLA编程相比于手写汇编的优势：

- 省去寄存器分配的工作
- 不需要考虑功能单元分配 
- 不需要考虑指令的节拍数和延迟槽
- 用户可以直接编写串行程序
- 可以自动完成代码中核心循环的优化，包括循环展开和软流水


#### 软件架构
FT-MatrixLA采用C++进行实现，基于VS2019开发。

FT-MatrixLA读入用户线性汇编文件(.sa)，将其转化为高并行性的汇编代码输出(.asm)。主要处理阶段包括：代码解析、指令依赖分析、核心循环优化、指令调度、寄存器分配。


#### 安装教程
FT-MatrixLA 为单个EXE执行文件，无需安装，采用命令行方式直接调用即可

#### 使用说明
命令行方式直接调用EXE执行文件，后面需要两个必要输入参数，即输入线性汇编文件(.sa)和输出汇编文件(.asm)。完整调用格式如下：

FT-MatrixLA.exe test.sa test_output.asm -I 32/64 -Ox

当输入输出缺省时，输入和输出文件缺省值为“.//test.sa”和“.//test_output.asm”。-I用来指定平台型号或外部指令集描述文件，-Ox指定用户对代码的优化选项，目前支持-O0和-O1。当无任何汇编选项时，所有必要参数将采用缺省设置。

程序执行过程中包含多个输出，主要包括对指令集描述文件的统计信息，以及执行各个模块的信息提示。如果用户程序有问题会对应进行反馈，并给出报错提示。当线性汇编器执行成功时，程序会输出“Assembler file is successfully generated.”。 程序会输出 graph.txt 和 test_transition.txt 一个中间文件，内容为用户指令间的依赖关系描述文件和线性汇编转换调度为汇编代码的中间结果（寄存器分配前），可提供给用户调试使用。

#### 源码目录结构

|--	FT-MatrixLA												FT-MatrixLA工程的主要源码目录

        |-- main.cpp										FT-MatrixLA主函数入口文件

        |-- aliveness_analysis.cpp							变量活性分析模块

        |-- aliveness_analysis.h							变量活性分析头文件

        |-- analyser.cpp									用户代码解析模块

        |-- analyser.h										用户代码解析头文件

        |-- build_dependency.cpp							指令依赖分析模块

        |-- build_dependency.h								指令依赖分析头文件

        |-- data_structure.cpp								整个FT-MatrixLA全局数据结构

        |-- data_structure.h								全局数据结构头文件

        |-- ins_scheduling.cpp								指令并行调度模块

        |-- ins_scheduling.h								指令并行调度头文件

        |-- loop_unrolling.cpp								循环展开模块

        |-- loop_unrolling.h								循环展开头文件

        |-- reference.cpp									指令集信息存储模块

        |-- register_assignment.cpp							寄存器分配模块

        |-- register_assignment.h							寄存器分配头文件

        |-- software_pipeline.cpp							循环软件流水模块

        |-- software_pipeline.h								循环软件流水头文件

        |-- utils.cpp										全局功能函数模块

        |-- utils.h											全局功能函数头文件

        |-- resource.h										外部资源头文件

        |-- favicon.ico										可执行文件图标资源文件

        |-- others											其他VS工程相关文件

|--	FT-MatrixLA.sln											FT-Matrix VS工程源文件

|--	README.md												本说明文件

|--	instruction/instruction_32/instruction_64.csv			FT-Matrix DSP不同架构平台下的外部指令集信息文件，目前版本指
令集信息已融入源码，该文件仅供参考

|--	test/test_64.sa											32/64位FT-Matrix平台下的线性汇编测试样例

|--	test_output.asm											测试样例通过线性汇编器的输出结果展示

|--	test_transition.txt										线性汇编器中间transition文件示例输出

|--	graph.txt												线性汇编器中间graph文件示例输出



#### 参与贡献

总体架构设计：陈照云，马奕民，时洋

代码解析：时洋，马奕民

循环展开：时洋

依赖分析：陈照云，孔玺畅

软流水：陈照云，孔玺畅，刘昕睿，杨京

指令调度：陈照云，邓灿

活性分析/寄存器分配：马奕民，杨京

联系人：陈照云 (chenzhaoyun@nudt.edu.cn)