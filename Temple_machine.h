#pragma once
#include "Temple_base.h"

class Temple_machine :public Temple_base {
public:
    Temple_machine(const char* filename, uint16_t set_pc = 0, uint16_t set_sp = 10000)
        : Temple_base::Temple_base(filename, set_pc, set_sp) {}
private:
    void jump_comment(const char*& ptr) {
        while (!(*ptr == '1') && !(*ptr == '0')) {
            if (*ptr == '/' && *(ptr + 1) == '/')
                while (*ptr != '\n') ++ptr;
            ++ptr;
            if (*ptr == '\0')
                throw std::out_of_range("プログラム終了");
        }
    }
    int set_next() {
        uint16_t count_PC = 0;
        const char* count_ptr = str.c_str();
        int opecode = -1;
        for (; count_PC <= PC;) {
            jump_comment(count_ptr);

            if (mystrcmp(count_ptr, "000")) {
                if(count_PC < PC)
                    count_ptr += 8;
                else
                    count_ptr += 3;
                ++count_PC;
                opecode = 0b000;
            }
            else if (mystrcmp(count_ptr, "001")) {
                if (count_PC < PC)
                    count_ptr += 8;
                else
                    count_ptr += 3;
                ++count_PC;
                opecode = 0b001;
            }
            else if (mystrcmp(count_ptr, "010")) {
                if (count_PC < PC)
                    count_ptr += 8;
                else
                    count_ptr += 3;
                ++count_PC;
                opecode = 0b010;
            }
            else if (mystrcmp(count_ptr, "011")) {
                if (count_PC < PC)
                    count_ptr += 8;
                else
                    count_ptr += 3;
                ++count_PC;
                opecode = 0b011;
            }
            else if (mystrcmp(count_ptr, "100")) {
                if (count_PC < PC)
                    count_ptr += 8;
                else
                    count_ptr += 3;
                ++count_PC;
                opecode = 0b100;
            }
            else if (mystrcmp(count_ptr, "101")) {
                if (count_PC < PC) {
                    count_ptr += 8;
                    jump_comment(count_ptr);
                    count_ptr += 8;
                    jump_comment(count_ptr);
                    count_ptr += 8;
                }
                else
                    count_ptr += 3;

                count_PC += 3;
                opecode = 0b101;
            }
            else if (mystrcmp(count_ptr, "110")) {
                if (count_PC < PC) {
                    count_ptr += 8;
                    jump_comment(count_ptr);
                    count_ptr += 8;
                }
                else
                    count_ptr += 3;
                count_PC += 2;
                opecode = 0b110;
            }
            else if (mystrcmp(count_ptr, "111")) {
                if (count_PC < PC)
                    count_ptr += 8;
                else
                    count_ptr += 3;
                ++count_PC;
                opecode = 0b111;
            }
            else
                ++ptr;
            if (*count_ptr == '\0') {
                ptr = count_ptr;
                throw std::out_of_range("プログラム終了");
            }
        }
        ptr = count_ptr;
        PC = count_PC;
        return opecode;
    }
    const char* nor() override {
        jump_comment(ptr);

        int n = 0;
        for (int i = 0; i < 5; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }

        if(n >= 0 && n <= 22)
            Acc = ~(Acc | r[n]);
        else if (n >= 23 && n <= 26)
            Acc = ~(Acc | t[n - 23]);
        else if (n == 27)
            Acc = ~(Acc | sp);
        else if (n == 28)
            Acc = ~(Acc | ra);
        else if (n == 29)
            Acc = ~(Acc | zero);
        else if (n == 30)
            Acc = ~(Acc | one);
        else if (n == 31)
            Acc = ~(Acc | allone);
        else
            throw std::runtime_error((std::ostringstream() << "処理不可能な文字" << *ptr << "があります。").str().c_str());

        f_zero = !Acc;
        f_neg = (Acc & 1 << 15) ? true : false;

        return ptr;
    }
    const char* add() override {
        jump_comment(ptr);
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

        int n = 0;
        for (int i = 0; i < 5; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }

        if (n >= 0 && n <= 22)
            f_add(r[n]);
        else if (n >= 23 && n <= 26)
            f_add(t[n - 23]);
        else if (n == 27)
            f_add(sp);
        else if (n == 28)
            f_add(ra);
        else if (n == 29)
            f_add(zero);
        else if (n == 30)
            f_add(one);
        else if (n == 31)
            f_add(allone);
        else
            throw std::runtime_error((std::ostringstream() << "処理不可能な文字" << *ptr << "があります。").str().c_str());

        f_zero = !Acc;
        f_neg = (Acc & 1 << 15) ? true : false;

        return ptr;
    }
    const char* ld() override {
        jump_comment(ptr);

        int n = 0;
        for (int i = 0; i < 5; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }

        if (n >= 0 && n <= 22)
            Acc = Mem[r[n]/2];
        else if (n >= 23 && n <= 26)
            Acc = Mem[t[n - 23]/2];
        else if (n == 27)
            Acc = Mem[sp/2];
        else if (n == 28)
            Acc = Mem[ra/2];
        else if (n == 29)
            Acc = Mem[zero/2];
        else if (n == 30)
            Acc = Mem[one/2];
        else if (n == 31)
            Acc = Mem[allone/2];
        else
            throw std::runtime_error((std::ostringstream() << "処理不可能な文字" << *ptr << "があります。").str().c_str());

        return ptr;
    }
    const char* move() override {
        jump_comment(ptr);

        int n = 0;
        for (int i = 0; i < 5; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }

        if (n >= 0 && n <= 22)
            r[n] = Acc;
        else if (n >= 23 && n <= 26)
            t[n - 23] = Acc;
        else if (n == 27)
            sp = Acc;
        else if (n == 28)
            ra = Acc;
        else if (n == 29)
            throw std::runtime_error("定数レジスタ$zeroには代入できません。");
        else if (n == 30)
            throw std::runtime_error("定数レジスタ$oneには代入できません。");
        else if (n == 31)
            throw std::runtime_error("定数レジスタ$Alloneには代入できません。");
        else
            throw std::runtime_error("範囲エラー");
        
        return ptr;
    }
    const char* sd() override {
        jump_comment(ptr);

        int n = 0;
        for (int i = 0; i < 5; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }

        if (n >= 0 && n <= 22)
            Mem[r[n]/2] = Acc;
        else if (n >= 23 && n <= 26)
            Mem[t[n - 23]/2] = Acc;
        else if (n == 27)
            Mem[sp/2] = Acc;
        else if (n == 28)
            Mem[ra/2] = Acc;
        else if (n == 29)
            throw std::runtime_error("定数レジスタ$zeroは使用できません。");
        else if (n == 30)
            throw std::runtime_error("定数レジスタ$oneは使用できません。");
        else if (n == 31)
            throw std::runtime_error("定数レジスタ$Alloneは使用できません。");
        else
            throw std::runtime_error((std::ostringstream() << "処理不可能な文字" << *ptr << "があります。").str().c_str());

        return ptr;
    }

    uint16_t count_PC(const char* start, const char* const end) {
        // startからendまでに変化したプログラムカウンタの値を返す
        uint16_t result_PC = 0;
        static const uint16_t F_type = 1;
        static const uint16_t J_type = 2;
        static const uint16_t I_type = 3;
        jump_comment(start);

        while (end >= start) {

            if (mystrcmp(start, "000")) {//nor
                start += 8;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "001")) {//add
                start += 8;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "010")) {//ld
                start += 8;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "011")) {//move
                start += 8;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "100")) {//sd
                start += 8;
                result_PC += F_type;
            }
            else if (mystrcmp(start, "101")) {//seti
                start += 8;
                jump_comment(start);
                start += 8;
                jump_comment(start);
                start += 8;
                result_PC += I_type;
            }
            else if (mystrcmp(start, "110")) {//jl
                start += 8;
                jump_comment(start);
                start += 8;
                result_PC += J_type;
            }
            else if (mystrcmp(start, "111")) {//srl
                start += 8;
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
        ptr += 5; //読み捨て

        jump_comment(ptr);
        int n = 0;
        for (int i = 0; i < 8; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }
        Acc = n;

        jump_comment(ptr);
        n = 0;
        for (int i = 0; i < 8; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }
        Acc |= n << 8;
        
        return ptr;
    }
    const char* jl() override {
        jump_comment(ptr);

        int n = 0;
        for (int i = 0; i < 5; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }

        uint16_t my;
        if (n >= 0 && n <= 22)
            my = r[n];
        else if (n >= 23 && n <= 26)
            my = t[n - 23];
        else if (n == 27)
            my = sp;
        else if (n == 28)
            my = ra;
        else if (n == 29)
            my = zero;
        else if (n == 30)
            my = one;
        else if (n == 31)
            my = allone;
        else
            throw std::runtime_error("範囲エラー");

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
                throw std::runtime_error((std::ostringstream() << "処理不可能な文字" << *ptr << "があります。").str().c_str());
                break;
            }
            ++ptr;
        }
        if (*(ptr - 1) == '1' && *(ptr - 2) == '1' && *(ptr - 3) == '1')//無条件分岐
            jumpflag = true;

        n = 0;
        for (int i = 0; i < 5; ++i) {
            n <<= 1;
            n |= *ptr++ - '0';
        }

        static auto f_add = [&](int n)-> uint16_t& {

            if (n >= 0 && n <= 22)
                return r[n];
            else if (n >= 23 && n <= 26)
                return t[n - 23];
            else if (n == 27)
                return sp;
            else if (n == 28)
                return ra;
            else if (n == 29)
                throw std::runtime_error("$zeroには代入できません。");
            else if (n == 30)
                throw std::runtime_error("$oneには代入できません。");
            else if (n == 31)
                throw std::runtime_error("$Alloneには代入できません。");
            else
                throw std::runtime_error("範囲エラー");
        };
        f_add(n) = PC;
        if (jumpflag)
            PC = my;
        return ptr;
    }
    const char* srl() override {
        jump_comment(ptr);
        ptr += 5;
        Acc >>= 1;
        f_zero = !Acc;
        return ptr;
    }
public:
#define breakpoint 99
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
