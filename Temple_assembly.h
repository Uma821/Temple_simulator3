#pragma once
#include <sstream>
#include "Temple_base.h"

class Temple_assembly :public Temple_base {
public:
    Temple_assembly(const char* filename, uint16_t set_pc = 0, uint16_t set_sp = 10000):
        str([](const char* filename) {
        std::ifstream ifs(filename);
        if (!ifs) {
            std::cerr << "���s" << std::endl;
            throw std::runtime_error("�t�@�C�����J���܂���ł���");
        }
        return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            }(filename)){

    }
    //    : Temple_base::Temple_base(filename, set_pc, set_sp) {}
private:
    void jump_comment(const char*& ptr) {
        while (!('a' <= *ptr && *ptr <= 'z') && !('A' <= *ptr && *ptr <= 'Z') && !('0' <= *ptr && *ptr <= '9') && !(*ptr == '$')) {
            if (*ptr == '/' && *(ptr + 1) == '/')
                while (*ptr != '\n') ++ptr;
            ++ptr;
            if (*ptr == '\0')
                throw std::out_of_range("�v���O�����I��");
        }
    }
    int set_next() {
        uint16_t count_PC = 0;
        const char* count_ptr = str.c_str();
        int opecode = -1;
        for (; count_PC <= PC;) {
            jump_comment(count_ptr);

            if (mystrcmp(count_ptr, "nor")) {
                count_ptr += 3;
                ++count_PC;
                opecode = 0b000;
            }
            else if (mystrcmp(count_ptr, "add")) {
                count_ptr += 3;
                ++count_PC;
                opecode = 0b001;
            }
            else if (mystrcmp(count_ptr, "ld")) {
                count_ptr += 2;
                ++count_PC;
                opecode = 0b010;
            }
            else if (mystrcmp(count_ptr, "move")) {
                count_ptr += 4;
                ++count_PC;
                opecode = 0b011;
            }
            else if (mystrcmp(count_ptr, "sd")) {
                count_ptr += 2;
                ++count_PC;
                opecode = 0b100;
            }
            else if (mystrcmp(count_ptr, "seti")) {
                count_ptr += 4;
                count_PC += 3;
                opecode = 0b101;
            }
            else if (mystrcmp(count_ptr, "jl")) {
                count_ptr += 2;
                count_PC += 2;
                opecode = 0b110;
            }
            else if (mystrcmp(count_ptr, "srl")) {
                count_ptr += 3;
                ++count_PC;
                opecode = 0b111;
            }
            else
                ++count_ptr;
            if (*count_ptr == '\0') {
                ptr = count_ptr;
                throw std::out_of_range("�v���O�����I��");
            }
        }
        ptr = count_ptr;
        PC = count_PC;
        return opecode;
    }
    const char* nor() override {
        jump_comment(ptr);
        if (*ptr == '$') {
            ++ptr;
            if (mystrcmp(ptr, "allone")) {
                Acc = ~(Acc | allone);
                ptr += 6;
            }
            else if (mystrcmp(ptr, "one")) {
                Acc = ~(Acc | one);
                ptr += 3;
            }
            else if (*ptr == 'r') {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                Acc = ~(Acc | r[n]);
            }
            else if (*ptr == 't') {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                Acc = ~(Acc | t[n]);
            }
            else
                throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());

        }
        else
            throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());

        f_zero = !Acc;
        f_neg = (Acc & 1 << 15) ? true : false;

        return ptr;
    }

    const char* add() override {
        jump_comment(ptr);
        if (*ptr == '$') {
            static auto f_add = [&](const uint16_t& a) {
                unsigned int tmp = 0;
                uint16_t return_Acc = 0;
                for (int n = 0; n < 16; ++n) {
                    tmp += ((1 << n) & Acc) + ((1 << n) & a);
                    return_Acc += (1 << n) & tmp;
                    tmp &= ~(1 << n);
                }
                Acc = return_Acc;
                this->f_carry = !!tmp;
            };
            ++ptr;
            if (mystrcmp(ptr, "allone")) {
                f_add(allone);
                ptr += 6;
            }
            else if (mystrcmp(ptr, "one")) {
                f_add(one);
                ptr += 3;
            }
            else if (mystrcmp(ptr, "zero")) {
                f_add(zero);
                ptr += 4;
            }
            else if (*ptr == 'r') {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                f_add(r[n]);
            }
            else if (*ptr == 't') {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                f_add(t[n]);
            }
            else
                throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        }
        f_zero = !Acc;
        f_neg = (Acc & 1 << 15) ? true : false;

        return ptr;
    }
    const char* ld() override {
        jump_comment(ptr);
        if (mystrcmp(ptr, "$")) {
            ++ptr;
            if (mystrcmp(ptr, "r")) {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                Acc = Mem[r[n]/2];
            }
            else if (mystrcmp(ptr, "t")) {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                Acc = Mem[t[n]/2];
            }
            else if (mystrcmp(ptr, "zero")) {
                ptr += 4;
                Acc = Mem[zero/2];
            }
            else if (mystrcmp(ptr, "one")) {
                ptr += 3;
                Acc = Mem[one/2];
            }
            else if (mystrcmp(ptr, "allone")) {
                ptr += 6;
                Acc = Mem[allone/2];
            }
            else
                throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        }
        else
            throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        return ptr;
    }
    const char* move() override {
        jump_comment(ptr);
        if (mystrcmp(ptr, "$")) {
            ++ptr;
            if (mystrcmp(ptr, "r")) {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                r[n] = Acc;
            }
            else if (mystrcmp(ptr, "t")) {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                t[n] = Acc;
            }
            else if (mystrcmp(ptr, "sp")) {
                sp = Acc;
            }
            else if (mystrcmp(ptr, "ra")) {
                ra = Acc;
            }
            else if (mystrcmp(ptr, "zero")) {
                throw std::runtime_error("�萔���W�X�^$zero�͎g�p�ł��܂���B");
            }
            else if (mystrcmp(ptr, "one")) {
                throw std::runtime_error("�萔���W�X�^$one�͎g�p�ł��܂���B");
            }
            else if (mystrcmp(ptr, "allone")) {
                throw std::runtime_error("�萔���W�X�^$Allone�͎g�p�ł��܂���B");
            }
            else
                throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        }
        else
            throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        return ptr;
    }
    const char* sd() override {
        jump_comment(ptr);
        if (mystrcmp(ptr, "$")) {
            ++ptr;
            if (mystrcmp(ptr, "r")) {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                Mem[r[n]/2] = Acc;
            }
            else if (mystrcmp(ptr, "t")) {
                ++ptr;
                int n = 0;
                while ('0' <= *ptr && *ptr <= '9') {
                    n *= 10;
                    n += *ptr++ - '0';
                }
                Mem[t[n]/2] = Acc;
            }
            else if (mystrcmp(ptr, "zero")) {
                throw std::runtime_error("�萔���W�X�^$zero�ɂ͑���ł��܂���B");
            }
            else if (mystrcmp(ptr, "one")) {
                throw std::runtime_error("�萔���W�X�^$one�ɂ͑���ł��܂���B");
            }
            else if (mystrcmp(ptr, "allone")) {
                throw std::runtime_error("�萔���W�X�^$Allone�ɂ͑���ł��܂���B");
            }
            else
                throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        }
        else
            throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        return ptr;
    }
    uint16_t count_PC(const char* start, const char* const end) {
        // start����end�܂łɕω������v���O�����J�E���^�̒l��Ԃ�
        uint16_t result_PC = 0;
        static const uint16_t F_type = 1;
        static const uint16_t J_type = 2;
        static const uint16_t I_type = 3;
        jump_comment(start);

        while (end >= start) {


            if (mystrcmp(start, "nor")) {
                start += 3;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "add")) {
                start += 3;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "ld")) {
                start += 2;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "move")) {
                start += 4;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "sd")) {
                start += 2;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "seti")) {
                start += 4;
                result_PC += I_type;
            }
            else if (mystrcmp(start, "jl")) {
                start += 2;
                result_PC += J_type;
            }
            else if (mystrcmp(start, "srl")) {
                start += 3;
                result_PC += F_type;
            }
            else
                ++start;
            jump_comment(start);
        }
        return result_PC;
    }
    const char* seti() override {
        jump_comment(ptr);
        if ('0' <= *ptr && *ptr <= '9') {
            int n = 0;
            while ('0' <= *ptr && *ptr <= '9') {
                n *= 10;
                n += *ptr++ - '0';
            }
            Acc = n;
        }
        else {//���x���w�肳�ꂽ�Ƃ�
            const char* start = ptr;
            while (*ptr != ':')
                ++ptr;
            if (start == ptr)
                throw std::runtime_error("���x���w��ŃG���[");
            else {
                char* label = new char[ptr - start + 2];
                label[ptr - start + 1] = '\0';
                label[ptr - start] = ':';
                for (const int i = ptr - start; ptr - start;) {
                    label[i - (ptr - start)] = *start++;
                }
                const char* adr = strstr(str.c_str(), label);//���͏I����Ă��Ȃ��BSETI���߂̃��x���̈ʒu��ǂ݂����Ă���\��������
            retry:
                if (adr) {
                    const char* check = adr;
                    while (1) {
                        if (check == str.c_str() || mystrcmp(check, "nor") || mystrcmp(check, "add") || mystrcmp(check, "ld") || mystrcmp(check, "move") || mystrcmp(check, "sd") || mystrcmp(check, "jl") || mystrcmp(check, "srl"))
                            break; //�����߂���SETI���߈ȊO�Ȃ���Ȃ�
                        if (mystrcmp(check, "seti")) {
                            check += 4;
                            jump_comment(check); //seti���ߑ������܂Ŕ��
                            if (mystrcmp(check, label)) { //������������_��,���l���ɋL����Ă��郉�x�����Ă�ł���
                                adr = strstr(adr + strlen(label), label);
                                goto retry;
                            }
                            else
                                break;
                        }
                        --check;

                    }
                    Acc = count_PC(str.c_str(), adr);
                }
                else
                    throw std::runtime_error("���x���w��ŃG���[");
                delete[] label;
            }
        }
        return ptr;
    }
    const char* jl() override {
        static auto f_add1 = [&]() {
            jump_comment(ptr);
            if (*ptr == '$') {
                ++ptr;
                if (*ptr == 'r' || *ptr == 'R') {
                    ++ptr;
                    if (*ptr == 'a' || *ptr == 'A') {
                        ++ptr;
                        return ra;
                    }
                    else {
                        int n = 0;
                        while ('0' <= *ptr && *ptr <= '9') {
                            n *= 10;
                            n += *ptr++ - '0';
                        }
                        return r[n];
                    }
                }
                else if (*ptr == 't' || *ptr == 'T') {
                    ++ptr;
                    int n = 0;
                    while ('0' <= *ptr && *ptr <= '9') {
                        n *= 10;
                        n += *ptr++ - '0';
                    }
                    return t[n];
                }
                else if (mystrcmp(ptr, "allone")) {
                    ptr += 6;
                    return allone;
                }
                else if (mystrcmp(ptr, "one")) {
                    ptr += 3;
                    return one;
                }
                else if (mystrcmp(ptr, "zero")) {
                    ptr += 4;
                    return zero;
                }
                else
                    throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
            }
            throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        };
        static auto f_add2 = [&]()-> uint16_t& {
            jump_comment(ptr);
            if (*ptr == '$') {
                ++ptr;
                if (*ptr == 'r' || *ptr == 'R') {
                    ++ptr;
                    if (*ptr == 'a' || *ptr == 'A') {
                        ++ptr;
                        return ra;
                    }
                    else {
                        int n = 0;
                        while ('0' <= *ptr && *ptr <= '9') {
                            n *= 10;
                            n += *ptr++ - '0';
                        }
                        return r[n];
                    }
                }
                else if (*ptr == 't' || *ptr == 'T') {
                    ++ptr;
                    int n = 0;
                    while ('0' <= *ptr && *ptr <= '9') {
                        n *= 10;
                        n += *ptr++ - '0';
                    }
                    return t[n];
                }
                else if (mystrcmp(ptr, "allone")) {
                    throw std::runtime_error("$Allone�ɂ͑���ł��܂���B");
                }
                else if (mystrcmp(ptr, "one")) {
                    throw std::runtime_error("$one�ɂ͑���ł��܂���B");
                }
                else if (mystrcmp(ptr, "zero")) {
                    throw std::runtime_error("$zero�ɂ͑���ł��܂���B");
                }
                else
                    throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
            }
            throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
        };
        uint16_t my = f_add1();
        jump_comment(ptr);
        bool jumpflag = false;
        for (int i = 0; i < 3; ++i) {
            switch (*ptr) {
            case '0':
                break;
            case '1':
                if (flags[i])
                    jumpflag = true;
                break;
            default:
                throw std::runtime_error((std::ostringstream() << "�����s�\�ȕ���" << *ptr << "������܂��B").str().c_str());
                break;
            }
            ++ptr;
        }
        if (*(ptr - 1) == '1' && *(ptr - 2) == '1' && *(ptr - 3) == '1')//����������
            jumpflag = true;
        f_add2() = PC;
        if (jumpflag)
            PC = my;
        return ptr;
    }
    const char* srl() override {
        jump_comment(ptr);
        Acc >>= 1;
        f_zero = !Acc;
        return ptr;
    }
public:
#define breakpoint 62
    int run() noexcept {
        try {
            int opecode;
            for (;;) {
                jump_comment(ptr);
                if (PC == allone)
                    break;
                if (PC == breakpoint)
                    []() {}();
                opecode = set_next();
                (this->*(funcs[opecode]))();
            }
        }
        catch (std::out_of_range) {
            return EXIT_SUCCESS;
        }
        catch (std::exception e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
#undef breakpoint
};
