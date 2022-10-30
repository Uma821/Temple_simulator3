#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <list>
#include "Temple_assembly_3.h"
#include "Temple_machine_3.h"
int main(int argc, char** argv) {
    enum Type
    {
        Asm, Exe
    };
    std::list<char*> oExe;
    std::list<char*> oAsm;
    std::list<std::pair<int, int>> InMem;
    std::list<int> OutMem;
    std::pair<char*, int> Program;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-oExe") || !strcmp(argv[i], "-oexe") || !strcmp(argv[i], "-oExE")) {
            ++i;
            oExe.push_back(argv[i]);
            continue;
        }
        else if (!strcmp(argv[i], "-oAsm") || !strcmp(argv[i], "-oasm")) {
            ++i;
            oExe.push_back(argv[i]);
            continue;
        }
        else if (int mem, immed; sscanf(argv[i], "mem[%d] = %d", &mem, &immed) == 2) {
            InMem.push_back(std::make_pair(mem, immed));
            continue;
        }
        else if (int mem; sscanf(argv[i], "mem[%d]", &mem) == 1) {
            OutMem.push_back(mem);
            continue;
        }
        else if (!strcmp(argv[i], "-iExe")) {
            Program.first = argv[i];
            Program.second = Exe;
        }
        else if (!strcmp(argv[i], "-iAsm")) {
            Program.first = argv[i];
            Program.second = Asm;
        }
        else {
            Program.first = argv[i];
            Program.second = Asm;
        }
    }


    //std::variant<Temple_assembly, Temple_machine> temple(Temple_assembly(Program.first)), machine(Temple_machine(Program.first));
    if (Program.second == Asm) {
        try {
            Temple_assembly temple(Program.first);
            for (const auto& file_name : oAsm)
                temple.output_assembly(file_name);
            for (const auto& file_name : oExe)
                temple.output_assembly(file_name);

            for (const auto& [mem, immed] : InMem)
                temple[mem] = immed;
            temple.run();
            for (const auto& mem : OutMem)
                printf("Mem[%d] = %d\n", mem, temple[mem]);
            return temple.retval();
        }
        catch (std::exception e) {
            std::cout << e.what() << std::endl;
        }
    }
    else {
        try {
            Temple_machine temple(Program.first);
            for (const auto& file_name : oAsm)
                temple.output_assembly(file_name);
            for (const auto& file_name : oExe)
                temple.output_assembly(file_name);

            for (const auto& [mem, immed] : InMem)
                temple[mem] = immed;
            temple.run();
            for (const auto& mem : OutMem)
                printf("Mem[%d] = %d\n", mem, temple[mem]);
            return temple.retval();
        }
        catch (std::exception e) {
            std::cout << e.what() << std::endl;
        }
    }
    _CrtDumpMemoryLeaks();
}