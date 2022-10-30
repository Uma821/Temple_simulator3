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

class Temple_assembly {
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

	enum func_opecode { _nor, _add, _ld, _move, _sd, _seti, _jl, _srl };

	enum field { F = 1, J, I };
	const field func_types[8] = { F, F, F, F, F, I, J, F };

	const std::vector <std::string> split_file;
	const std::vector <std::string> assembly_code;
	const std::vector <uint8_t> machine_code;
	std::unique_ptr <uint16_t[]> Mem; // メモリマップ

	void (Temple_assembly::* const funcs[8])() = { &Temple_assembly::nor, &Temple_assembly::add, &Temple_assembly::ld, &Temple_assembly::move, &Temple_assembly::sd, &Temple_assembly::seti, &Temple_assembly::jl, &Temple_assembly::srl };
public:
	Temple_assembly(const char* filename, uint16_t set_pc = 0, uint16_t set_sp = 10000) : zero(), one(1), allone(UINT16_MAX), flags(),
		split_file([](const char* filename) {

		if (std::ifstream ifs = std::ifstream(filename)) {
			std::string file_buf((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

			for (size_t first; (first = file_buf.find("//")) != std::string::npos;)
				file_buf.erase(first, file_buf.find("\n", first) - first);
			for (size_t first; (first = file_buf.find("/*")) != std::string::npos;)
				file_buf.erase(first, file_buf.find("*/", first) - first);

			std::vector<std::string> sorce;
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
			return sorce;
		}
		else {
			std::cerr << "失敗" << std::endl;
			throw std::runtime_error("入力用ファイルが開けませんでした");
		}
			}(filename)),
		assembly_code([this](const std::vector <std::string>& split_file) {

				std::vector<std::string> assembly_code;
				for (auto itr = split_file.begin(); itr < split_file.end(); ++itr) {
					if (_stricmp((*itr).c_str(), "nor") == 0 || _stricmp((*itr).c_str(), "add") == 0 || _stricmp((*itr).c_str(), "ld") == 0 || _stricmp((*itr).c_str(), "move") == 0 || _stricmp((*itr).c_str(), "sd") == 0) {
						assembly_code.push_back(*itr);
						if (int rgtr = register_int(*++itr); rgtr < 0 || rgtr>31) {
							std::cerr << "error: 判別できない文字列" << *itr << std::endl;
							exit(EXIT_FAILURE);
						}
						else
							assembly_code.push_back("$m" + std::to_string(rgtr));
					}
					else if (_stricmp((*itr).c_str(), "seti") == 0) {
						assembly_code.push_back(*itr);
						assembly_code.push_back(*++itr);
					}
					else if (int mx, my, cond; _stricmp((*itr).c_str(), "jl") == 0) {
						assembly_code.push_back(*itr);
						if (my = register_int(*++itr); my < 0 || my > 31) {
							std::cerr << "error: 判別できない文字列" << *itr << std::endl;
							exit(EXIT_FAILURE);
						}
						try {
							cond = std::stoi(*++itr, nullptr, 2);
							if (cond < 0 || cond > 7)
								throw std::out_of_range("条件は0b000~0b111までで入力");
						}
						catch (...) {
							std::cerr << "error: 判別できない条件" << *itr << std::endl;
							exit(EXIT_FAILURE);
						}
						if (mx = register_int(*++itr); mx < 0 || mx > 31) {
							std::cerr << "error: 判別できない文字列" << *itr << std::endl;
							exit(EXIT_FAILURE);
						}
						assembly_code.push_back("$m" + std::to_string(my));
						assembly_code.push_back(std::bitset<3>(cond).to_string());
						assembly_code.push_back("$m" + std::to_string(mx));
					}
					else if (_stricmp((*itr).c_str(), "srl") == 0)
						assembly_code.push_back(*itr);
					else if (_stricmp((*itr).c_str(), "sub") == 0) { // ここから疑似命令
						int t0_rgtr = register_int("$t0");
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("nor");
						assembly_code.push_back("$m" + std::to_string(register_int("$allone")));
						assembly_code.push_back("nor");
						if (int rgtr = register_int(*++itr); rgtr < 0 || rgtr>31) {
							std::cerr << "error: 判別できない文字列" << *itr << std::endl;
							exit(EXIT_FAILURE);
						}
						else
							assembly_code.push_back("$m" + std::to_string(rgtr));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(register_int("$one")));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
					}
					else if (_stricmp((*itr).c_str(), "push") == 0) {
						int sp_rgtr = register_int("$sp");
						assembly_code.push_back("seti");
						assembly_code.push_back(std::to_string(uint16_t(-2)));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
						assembly_code.push_back("nor");
						assembly_code.push_back("$m" + std::to_string(register_int("$allone")));
						assembly_code.push_back("add");
						if (int rgtr = register_int(*++itr); rgtr < 0 || rgtr>31) {
							std::cerr << "error: 判別できない文字列" << *itr << std::endl;
							exit(EXIT_FAILURE);
						}
						else
							assembly_code.push_back("$m" + std::to_string(rgtr));
						assembly_code.push_back("sd");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
					}
					else if (_stricmp((*itr).c_str(), "pop") == 0) {
						int sp_rgtr = register_int("$sp");
						assembly_code.push_back("ld");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
						assembly_code.push_back("move");
						if (int rgtr = register_int(*++itr); rgtr < 0 || rgtr>31) {
							std::cerr << "error: 判別できない文字列" << *itr << std::endl;
							exit(EXIT_FAILURE);
						}
						else
							assembly_code.push_back("$m" + std::to_string(rgtr));
						assembly_code.push_back("seti");
						assembly_code.push_back(std::to_string(2));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
					}
					else if (_stricmp((*itr).c_str(), "nop") == 0) {
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(register_int("$one")));
					}
					else if (_stricmp((*itr).c_str(), "nopi") == 0) {
						int one_rgtr = register_int("$one");
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(one_rgtr));
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(one_rgtr));
					}
					else if (_stricmp((*itr).c_str(), "jmp") == 0) {
						int t1_rgtr = register_int("$t1");
						assembly_code.push_back("seti");
						assembly_code.push_back(*++itr);
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(t1_rgtr));
						assembly_code.push_back("jl");
						assembly_code.push_back("$m" + std::to_string(t1_rgtr));
						assembly_code.push_back("111");
						assembly_code.push_back("$m" + std::to_string(t1_rgtr - 1)); // t0
					}
					else if (_stricmp((*itr).c_str(), "load") == 0) {
						int t0_rgtr = register_int("$t0");
						assembly_code.push_back("seti");
						assembly_code.push_back(*++itr);
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("ld");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
					}
					else if (_stricmp((*itr).c_str(), "save") == 0) {
						int t0_rgtr = register_int("$t0");
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("seti");
						assembly_code.push_back(*++itr);
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr + 1));
						assembly_code.push_back("nor");
						assembly_code.push_back("$m" + std::to_string(register_int("$allone")));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("sd");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr + 1));
					}
					else if (_stricmp((*itr).c_str(), "halt") == 0) {
						int t0_rgtr = register_int("$t0"), zero_rgtr = register_int("$zero");
						assembly_code.push_back("jl");
						assembly_code.push_back("$m" + std::to_string(zero_rgtr));
						assembly_code.push_back("000");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("jl");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("111");
						assembly_code.push_back("$m" + std::to_string(zero_rgtr));
					}
					else if (_stricmp((*itr).c_str(), "call") == 0) { // call 命令   記述 call ラベル    命令操作内容 CALL命令の次のアドレスをPUSH、呼び出す関数のアドレスへJMPする
						int t0_rgtr = register_int("$t0");
						assembly_code.push_back("jl");
						assembly_code.push_back("$m" + std::to_string(register_int("$zero")));
						assembly_code.push_back("000");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr)); // 次のseti命令のアドレスを入手①

						int sp_rgtr = register_int("$sp");
						assembly_code.push_back("seti");
						assembly_code.push_back(std::to_string(uint16_t(-2)));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr)); // spを-2して次のところを指す

						assembly_code.push_back("seti"); // ①からcall命令の最後までのアドレス増加分を
						assembly_code.push_back(std::to_string(func_types[_seti] + func_types[_add] + func_types[_move] + func_types[_seti] + func_types[_add] + func_types[_sd] + func_types[_seti] + func_types[_move] + func_types[_jl]));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr)); // 加算して
						assembly_code.push_back("sd");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr)); // スタックに保存

						assembly_code.push_back("seti");
						assembly_code.push_back(*++itr);
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("jl");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("111");
						assembly_code.push_back("$m" + std::to_string(register_int("$ra")));
					}
					else if (_stricmp((*itr).c_str(), "ret") == 0) { // ret 命令   記述 ret    命令操作内容 POPして戻り先のアドレスを取得、POPしたアドレスへJMPする
						int sp_rgtr = register_int("$sp"), t0_rgtr = register_int("$t0");
						assembly_code.push_back("ld");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr)); // push してた分の戻ってくるアドレスを $t0 に保存

						assembly_code.push_back("seti");
						assembly_code.push_back(std::to_string(2));
						assembly_code.push_back("add");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr));
						assembly_code.push_back("move");
						assembly_code.push_back("$m" + std::to_string(sp_rgtr)); // spを+2して戻す

						assembly_code.push_back("jl");
						assembly_code.push_back("$m" + std::to_string(t0_rgtr));
						assembly_code.push_back("111");
						assembly_code.push_back("$m" + std::to_string(register_int("$ra")));
					}
					else
						assembly_code.push_back(*itr);
				}
				return assembly_code;

			}(split_file)),
				machine_code([](const std::vector <std::string>& sorce) {

				std::vector<uint8_t> contents;
				std::map<std::string, std::pair<std::vector<long long>, int>> labels;
				for (auto itr = sorce.begin(); itr < sorce.end(); ++itr) {
					if (_stricmp((*itr).c_str(), "nor") == 0)
						contents.push_back(0b000 << 5 | std::stoi((*++itr).substr(2)));
					else if(_stricmp((*itr).c_str(), "add") == 0)
						contents.push_back(0b001 << 5 | std::stoi((*++itr).substr(2)));
					else if(_stricmp((*itr).c_str(), "ld") == 0)
						contents.push_back(0b010 << 5 | std::stoi((*++itr).substr(2)));
					else if(_stricmp((*itr).c_str(), "move") == 0)
						contents.push_back(0b011 << 5 | std::stoi((*++itr).substr(2)));
					else if(_stricmp((*itr).c_str(), "sd") == 0)
						contents.push_back(0b100 << 5 | std::stoi((*++itr).substr(2)));
					else if (int num; _stricmp((*itr).c_str(), "seti") == 0) {
						try {
							num = std::stoi(*++itr);
						}
						catch (...) {
							num = 0;
							if ((*itr).find(":") == std::string::npos) {
								if (labels.find(*itr + ":") == labels.end())
									labels.insert(std::map<std::string, std::pair<std::vector<long long>, int>>::value_type(*itr + ":", std::make_pair(std::vector<long long>{contents.end() - contents.begin()}, -1)));
								else
									labels[*itr + ":"].first.push_back(contents.end() - contents.begin());
							}
							else {
								std::cerr << "error: ラベル指定に\":\"は使用できません" << *itr << std::endl;
								exit(EXIT_FAILURE);
							}
						}
						contents.push_back(0b101 << 5);
						contents.push_back(num & 0xFF);
						contents.push_back(num >> 8);
					}
					else if (_stricmp((*itr).c_str(), "jl") == 0) {
						int my = std::stoi((*++itr).substr(2));
						int cond = std::stoi(*++itr, nullptr, 2);
						contents.push_back(0b110 << 5 | my);
						contents.push_back(cond << 5 | std::stoi((*++itr).substr(2)));
					}
					else if (_stricmp((*itr).c_str(), "srl") == 0)
						contents.push_back(0b111 << 5);
					else {
						for (auto iitr = (*itr).begin(); iitr < (*itr).end(); ++iitr) {
							if (_mbclen((const unsigned char*)std::string(iitr, (*itr).end()).c_str()) != 1) {//全角
								std::cerr << "error: 判別できない文字列\"" + *itr + "\"" << std::endl;
								exit(EXIT_FAILURE);
							}
						}
						if ((*itr)[(*itr).size() - 1] != ':') {
							std::cerr << "error: 判別できない文字列\"" + *itr + "\"はラベルではありません" << std::endl;
							exit(EXIT_FAILURE);
						}
						else {
							if (labels.find(*itr) == labels.end())
								labels.insert(std::map<std::string, std::pair<std::vector<long long>, int>>::value_type(*itr, std::make_pair(std::vector<long long>(), contents.size())));
							else if (labels[*itr].second == -1)
								labels[*itr].second = contents.size();
							else {
								std::cerr << "error: 判別できない文字列" << *itr << ": 同一のラベルが複数あります" << std::endl;
								exit(EXIT_FAILURE);
							}
						}
					}
				}
				for (auto& x : labels) {
					if (x.second.second == -1) {
						std::cerr << "error: ラベル" << x.first << "が見つかりません" << std::endl;
						exit(EXIT_FAILURE);
					}
					else {
						for (auto& y : x.second.first) {
							contents[y + 1] = x.second.second & 0xFF;
							contents[y + 2] = x.second.second >> 8;
						}
					}
				}
				return contents;
					}(assembly_code)), sp(set_sp), PC(set_pc), Mem(new uint16_t[(0xffff + 1) / 2]) {

					}

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
#define breakpoint 77
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
					ofs << std::format("  {:4} {:13} //{:4} // {:08b}\n", *itr, *(itr + 1), pc, machine_code[pc]);
					++itr;
				}
				else if (*itr == "seti") {
					ofs << std::format("  {:4} {:13} //{:4} // {:08b} {:08b} {:08b}\n", *itr, *(itr + 1), pc, machine_code[pc], machine_code[pc + 1], machine_code[pc + 2]);
					pc += 2, ++itr;
				}
				else if (*itr == "jl") {
					ofs << std::format("  {:4} {:4} {} {:4} //{:4} // {:08b} {:08b}\n", *itr, *(itr + 1), *(itr + 2), *(itr + 3), pc, machine_code[pc], machine_code[pc + 1]);
					++pc, itr += 3;
				}
				else if (*itr == "srl") {
					ofs << std::format("  {:18} //{:4} // {:08b}\n", *itr, pc, machine_code[pc]);
				}
				else if ((*itr).back() == ':') {
					ofs << std::format("{:18}   //{:4}\n", *itr, pc--);
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
					ofs << std::format("{:08b} //{:4} // {:4} {:13}\n{:08b} \n{:08b} \n", machine_code[pc], pc, *itr, *(itr + 1), machine_code[pc + 1], machine_code[pc + 2]);
					pc += 2, ++itr;
				}
				else if (*itr == "jl") {
					ofs << std::format("{:08b} //{:4} // {:4} {:4} {} {:4} \n{:08b}\n", machine_code[pc], pc, *itr, *(itr + 1), *(itr + 2), *(itr + 3), machine_code[pc + 1]);
					++pc, itr += 3;
				}
				else if (*itr == "srl") {
					ofs << std::format("{:08b} //{:4} // {:18}\n", machine_code[pc], pc, *itr);
				}
				else if ((*itr).back() == ':') {
					ofs << std::format("         //{:4} // {:18}\n", pc--, *itr);
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
