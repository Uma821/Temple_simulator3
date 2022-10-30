#pragma once
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <mbstring.h>
#include <map>
#include <bitset>
#include <format>

class Temple_machine {
    union {
        uint16_t m[32];
        struct {
            uint16_t r[23];               // 汎用レジスタ  
            uint16_t t[4];                // 疑似命令用レジスタ  
            uint16_t sp;                  // スタックポインタ  
            uint16_t ra;                  // 戻りアドレス      
            const uint16_t zero;          // 0レジスタ    
            const uint16_t one;           // 1レジスタ     
            const uint16_t allone;        // -1レジスタ   
        };
    };
    uint16_t Acc;                         // アキュムレータ   
    uint16_t PC;                          // プログラムカウンタ   

    union {
        uint8_t flags;
        struct {
            uint8_t f_carry : 1;
            uint8_t f_neg : 1;
            uint8_t f_zero : 1;
        };
    };

    enum field { F = 1, J, I };
    const field func_types[8] = { F, F, F, F, F, I, J, F };

    const std::vector <std::string> split_file;
    const std::vector <std::string> assembly_code;
    const std::vector <uint8_t> machine_code;
    std::unique_ptr <uint16_t[]> Mem; // メモリマップ

    void (Temple_machine::* const funcs[8])() = { &Temple_machine::nor, &Temple_machine::add, &Temple_machine::ld, &Temple_machine::move, &Temple_machine::sd, &Temple_machine::seti, &Temple_machine::jl, &Temple_machine::srl };
public:
    Temple_machine(const char* filename, uint16_t set_pc = 0, uint16_t set_sp = 10000) : zero(), one(1), allone(UINT16_MAX), flags(),
        split_file([](const char* filename) {

            if (std::ifstream ifs = std::ifstream(filename)) {
                std::string file_buf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

                while (file_buf.find("//") != std::string::npos) {
                    static std::string set = "//";
                    file_buf.erase(std::find_first_of(file_buf.begin(), file_buf.end(), set.begin(), set.end()),
                        std::find(std::find_first_of(file_buf.begin(), file_buf.end(), set.begin(), set.end()), file_buf.end(), '\n'));
                }
                while (file_buf.find("/*") != std::string::npos) {
                    static std::string set[] = { "/*","*/" };
                    file_buf.erase(std::find_first_of(file_buf.begin(), file_buf.end(), set[0].begin(), set[0].end()),
                        std::find_first_of(std::find_first_of(file_buf.begin(), file_buf.end(), set[0].begin(), set[0].end()), file_buf.end(), set[1].begin(), set[1].end()) + 2);
                }

                std::vector<std::string> sorce;
                {
                    std::stringstream ss(file_buf);
                    for (std::string line_buf; std::getline(ss, line_buf);) {
                        if (line_buf.find("*/") != std::string::npos) {
                            std::cerr << "error: \"*/\" : " << line_buf << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        std::stringstream ss(std::move(line_buf));
                        for (std::string buf; ss >> buf;)
                            sorce.push_back(std::move(buf));
                    }
                }
                return sorce;
            }
            else {
                std::cerr << "失敗" << std::endl;
                throw std::runtime_error("入力用ファイルが開けませんでした");
        }}(filename)), assembly_code([this](const std::vector <std::string>& split_file) {

            std::vector<std::string> assembly_code;
            for (auto itr = split_file.begin(); itr < split_file.end(); ++itr) {
                try {
                    switch (const static char* opecode_kinds[] = { "nor", "add", "ld", "move", "sd" }; int opecode = std::stoi((*itr).substr(0, 3), nullptr, 2)) {
                    case 0: case 1: case 2: case 3: case 4:
                        assembly_code.push_back(opecode_kinds[opecode]);
                        assembly_code.push_back("$m" + std::to_string(std::stoi((*itr).substr(3), nullptr, 2)));
                        break;
                    case 5:
                        assembly_code.push_back("seti");
                        assembly_code.push_back(std::to_string(std::stoi(*(itr+2)+*(itr+1), nullptr, 2)));
                        itr += 2;
                        break;
                    case 6:
                        assembly_code.push_back("jl");
                        assembly_code.push_back("$m" + std::to_string(std::stoi((*itr).substr(3), nullptr, 2)));
                        assembly_code.push_back((*(itr+1)).substr(0, 3));
                        assembly_code.push_back("$m" + std::to_string(std::stoi((*(itr+1)).substr(3), nullptr, 2)));
                        ++itr;
                        break;
                    case 7:
                        assembly_code.push_back("srl");
                        break;
                    }
                }
                catch (...) {
                    std::cerr << "error : 認識できない文字列\"" + * itr + "\"" << std::endl;
                }
            }
            return assembly_code;

        }(split_file)), machine_code([](const std::vector<std::string>& sorce) {
            std::vector <uint8_t> machine_code;
            std::transform(sorce.begin(), sorce.end(), std::back_inserter(machine_code), [](const std::string &str)->uint8_t { return std::stoi(str, nullptr, 2); });
            return machine_code;
        }(split_file)), sp(set_sp), PC(set_pc), Mem(new uint16_t[(0xffff + 1) / 2]) {}

private:
    int register_int(const std::string& str) {
        if (str[0] == '$' && str[1] == 'm') {
            try {
                if (int num = std::stoi(str.substr(2)); num < 0 || num > 31) {
                    std::cerr << "error: 範囲エラー" << str << std::endl;
                    exit(EXIT_FAILURE);
                }
                else
                    return num;
            }
            catch (...) {
                std::cerr << "error: 判別できない文字列" << str << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (_stricmp(str.c_str(), "$ra") == 0)
            return 28;
        else if (str[0] == '$' && str[1] == 'r') {
            try {
                if (int num = std::stoi(str.substr(2)); num < 0 || num > 22) {
                    std::cerr << "error: 範囲エラー" << str << std::endl;
                    exit(EXIT_FAILURE);
                }
                else
                    return num;
            }
            catch (...) {
                std::cerr << "error: 判別できない文字列" << str << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (str[0] == '$' && str[1] == 't') {
            try {
                if (int num = std::stoi(str.substr(2)); num < 0 || num > 3) {
                    std::cerr << "error: 範囲エラー" << str << std::endl;
                    exit(EXIT_FAILURE);
                }
                else
                    return 23 + num;
            }
            catch (...) {
                std::cerr << "error: 判別できない文字列" << str << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (_stricmp(str.c_str(), "$sp") == 0)
            return 27;
        else if (_stricmp(str.c_str(), "$zero") == 0)
            return 29;
        else if (_stricmp(str.c_str(), "$one") == 0)
            return 30;
        else if (_stricmp(str.c_str(), "$allone") == 0)
            return 31;
        else
            return 32;
    }
    void nor() {
        Acc = ~(Acc | m[machine_code[PC] & 0x1F]);
        f_zero = !Acc;
        f_neg = (Acc & 0x8000) ? true : false;
        PC += func_types[machine_code[PC] >> 5];
    }
    void add() {
        int32_t tmp_Acc = Acc + m[machine_code[PC] & 0x1F];
        f_carry = (tmp_Acc & 0x10000) ? true : false;
        Acc = tmp_Acc & 0xFFFF;
        f_zero = !Acc;
        f_neg = (Acc & 0x8000) ? true : false;
        PC += func_types[machine_code[PC] >> 5];
    }
    void ld() {
        Acc = Mem[m[machine_code[PC] & 0x1F] / 2];
        PC += func_types[machine_code[PC] >> 5];
    }
    void move() {
        if (uint8_t rgtr = machine_code[PC] & 0x1F;  0 <= rgtr && rgtr <= 28)
            m[rgtr] = Acc;
        PC += func_types[machine_code[PC] >> 5];
    }
    void sd() {
        if (uint8_t rgtr = machine_code[PC] & 0x1F; 0 <= rgtr && rgtr <= 27)
            Mem[m[rgtr] / 2] = Acc;
        PC += func_types[machine_code[PC] >> 5];
    }
    void seti() {
        Acc = machine_code[PC + 1] | machine_code[PC + 2] << 8;
        PC += func_types[machine_code[PC] >> 5];
    }
    void jl() {
        if (uint8_t rgtr = machine_code[PC + 1] & 0x1F;  0 <= rgtr && rgtr <= 28)
            m[rgtr] = PC + func_types[machine_code[PC] >> 5];
        if (flags & (machine_code[PC + 1] >> 5) || machine_code[PC + 1] >> 5 == 7)
            PC = m[machine_code[PC] & 0x1F];
        else
            PC += func_types[machine_code[PC] >> 5];
    }
    void srl() {
        Acc >>= 1;
        f_zero = !Acc;
        PC += func_types[machine_code[PC] >> 5];
    }
public:
    uint16_t& operator[](const int n) {
        if (n >= 0 && n <= 0xffff)
            return Mem[n / 2];
        else
            throw std::runtime_error("不正アクセス");
    }
#define breakpoint 23
    int run() noexcept {
        while (PC < machine_code.size() && PC != allone) {
            if (PC == breakpoint)
                []() {}();
            (this->*(funcs[machine_code[PC] >> 5]))();
        }
        return EXIT_SUCCESS;
    }
    uint16_t retval() const {
        return r[0];
    }
#undef breakpoint
    void output_assembly(const char* filename) {
        int pc = 0;
        if (std::ofstream ofs = std::ofstream(filename)) {
            for (auto itr = assembly_code.begin(); itr < assembly_code.end(); ++itr, ++pc) {
                if (*itr == "nor" || *itr == "add" || *itr == "ld" || *itr == "move" || *itr == "sd") {
                    ofs << std::format("{:4} {:13} //{:4} // {:08b}\n", *itr, *(itr + 1), pc, machine_code[pc]);
                    ++itr;
                }
                else if (*itr == "seti") {
                    ofs << std::format("{:4} {:13} //{:4} // {:08b} {:08b} {:08b}\n", *itr, *(itr + 1), pc, machine_code[pc], machine_code[pc + 1], machine_code[pc + 2]);
                    pc += 2, ++itr;
                }
                else if (*itr == "jl") {
                    ofs << std::format("{:4} {:4} {} {:4} //{:4} // {:08b} {:08b}\n", *itr, *(itr + 1), *(itr + 2), *(itr + 3), pc, machine_code[pc], machine_code[pc + 1]);
                    ++pc, itr += 3;
                }
                else if (*itr == "srl") {
                    ofs << std::format("{:18} //{:4} // {:08b} \n", *itr, pc, machine_code[pc]);
                }
                else
                    throw std::runtime_error("処理不可能な文字" + *itr + "が含まれています。バグが発生しています。確認してください");
            }
        }
        else {
            std::cerr << "失敗" << std::endl;
            throw std::runtime_error("ファイルが開けませんでした");
        }
    }
    void output_machine(const char* filename) {
        int pc = 0;
        if (std::ofstream ofs = std::ofstream(filename)) {
            for (auto itr = assembly_code.begin(); itr < assembly_code.end(); ++itr, ++pc) {
                if (*itr == "nor" || *itr == "add" || *itr == "ld" || *itr == "move" || *itr == "sd") {
                    ofs << std::format("{:08b} //{:4} // {:4} {:13}\n", machine_code[pc], pc, *itr, *(itr + 1));
                    ++itr;
                }
                else if (*itr == "seti") {
                    ofs << std::format("{:08b} //{:4} // {:4} {:13}\n{:08b} \n{:08b} \n", machine_code[pc], pc, *itr, *(itr + 1), machine_code[pc + 1u], machine_code[pc + 2u]);
                    pc += 2, ++itr;
                }
                else if (*itr == "jl") {
                    ofs << std::format("{:08b} //{:4} // {:4} {:4} {} {:4} \n{:08b}\n", machine_code[pc], pc, *itr, *(itr + 1), *(itr + 2), *(itr + 3), machine_code[pc + 1u]);
                    ++pc, itr += 3;
                }
                else if (*itr == "srl") {
                    ofs << std::format("{:08b} // {:4} // {:18}\n", machine_code[pc], pc, *itr);
                }
                else
                    throw std::runtime_error("処理不可能な文字" + *itr + "が含まれています。バグが発生しています。確認してください");
            }
        }
        else {
            std::cerr << "失敗" << std::endl;
            throw std::runtime_error("ファイルが開けませんでした");
        }
    }
};

