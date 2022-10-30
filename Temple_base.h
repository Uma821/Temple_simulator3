#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <crtdbg.h>
#include <memory>
class Temple_base
{
protected:
    uint16_t r[23];                       // �ėp���W�X�^  
    uint16_t t[4];                        // �^�����ߗp���W�X�^  
    uint16_t sp;                          // �X�^�b�N�|�C���^  
    uint16_t ra;                          // �߂�A�h���X      
    const uint16_t zero = 0;              // 0���W�X�^    
    const uint16_t one = 1;               // 1���W�X�^     
    const uint16_t allone = UINT16_MAX;   // -1���W�X�^   
    uint16_t Acc;                         // �A�L�������[�^   
    uint16_t PC;                          // �v���O�����J�E���^   

    bool flags[3];

    bool& f_zero = flags[0];
    bool& f_neg = flags[1];
    bool& f_carry = flags[2];


    const std::string str;
    const char* ptr;
    const std::unique_ptr <uint16_t[]> Mem; // �������}�b�v

    const char* (Temple_base::* const funcs[8])() = { &Temple_base::nor, &Temple_base::add, &Temple_base::ld, &Temple_base::move, &Temple_base::sd, &Temple_base::seti, &Temple_base::jl, &Temple_base::srl };
public:
    Temple_base(const char* filename, uint16_t set_pc = 0, uint16_t set_sp = 10000)
        : str([](const char* filename) {
            std::ifstream ifs(filename);
            if (!ifs) {
                std::cerr << "���s" << std::endl;
                throw std::runtime_error("�t�@�C�����J���܂���ł���");
        }
        return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }(filename)), ptr(str.c_str()), sp(set_sp), PC(set_pc), Mem(new uint16_t[(0xffff + 1) / 2]) {}
    virtual ~Temple_base() {
    }
    uint16_t& operator[](const int n) {
        if (n >= 0 && n <= 0xffff)
            return Mem[n/2];
        else
            throw std::runtime_error("�s���A�N�Z�X");
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

