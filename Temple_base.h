#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <crtdbg.h>
#include <memory>
class Temple_base
{
protected:
    uint16_t r[23];                       // 汎用レジスタ  
    uint16_t t[4];                        // 疑似命令用レジスタ  
    uint16_t sp;                          // スタックポインタ  
    uint16_t ra;                          // 戻りアドレス      
    const uint16_t zero = 0;              // 0レジスタ    
    const uint16_t one = 1;               // 1レジスタ     
    const uint16_t allone = UINT16_MAX;   // -1レジスタ   
    uint16_t Acc;                         // アキュムレータ   
    uint16_t PC;                          // プログラムカウンタ   

    bool flags[3];

    bool& f_zero = flags[0];
    bool& f_neg = flags[1];
    bool& f_carry = flags[2];


    const std::string str;
    const char* ptr;
    const std::unique_ptr <uint16_t[]> Mem; // メモリマップ

    const char* (Temple_base::* const funcs[8])() = { &Temple_base::nor, &Temple_base::add, &Temple_base::ld, &Temple_base::move, &Temple_base::sd, &Temple_base::seti, &Temple_base::jl, &Temple_base::srl };
public:
    Temple_base(const char* filename, uint16_t set_pc = 0, uint16_t set_sp = 10000)
        : str([](const char* filename) {
            std::ifstream ifs(filename);
            if (!ifs) {
                std::cerr << "失敗" << std::endl;
                throw std::runtime_error("ファイルが開けませんでした");
        }
        return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }(filename)), ptr(str.c_str()), sp(set_sp), PC(set_pc), Mem(new uint16_t[(0xffff + 1) / 2]) {}
    virtual ~Temple_base() {
    }
    uint16_t& operator[](const int n) {
        if (n >= 0 && n <= 0xffff)
            return Mem[n/2];
        else
            throw std::runtime_error("不正アクセス");
    }
protected:
    static bool mystrcmp(const char* str, const char* a) {
        while (*a) {
            if ((*str - *a) && (!('a' <= *str && *str <= 'z') || ((*str + 'A' - 'a') - *a)))
                return false;
            ++str;
            ++a;
        }
        return true;
    }
    //virtual void jump_comment(const char*) = 0;
    virtual int set_next() = 0;
    virtual const char* nor() = 0;
    virtual const char* add() = 0;
    virtual const char* ld() = 0;
    virtual const char* move() = 0;
    virtual const char* sd() = 0;
    virtual const char* seti() = 0;
    virtual const char* jl() = 0;
    virtual const char* srl() = 0;
public:
    virtual int run() = 0;
};

