// Препроцессор языка Си
// Часть 2. Макросы

#include <iostream>
#include <stdint.h>

#define SetBit(reg, bit)      (reg |= (1UL<<bit))             // Установка бита в переменной
#define ClearBit(reg, bit)    (reg &= (~(1UL<<bit)))          // Сброс бита в переменной
#define InvBit(reg, bit)      (reg ^= (1UL<<bit))             // Инверсия бита в переменной
#define BitIsSet(reg, bit)    ((reg & (1UL<<bit)) != 0)       // Проверка установленного бита
#define BitIsClear(reg, bit)  ((reg & (1UL<<bit)) == 0)       // Проверка сброшенного бита
#define bit(b) (1UL << (b))

using std::cout;
using std::endl;

#define NL    '\n'    // NL - new line

#define printf(a) (cout << a << NL)
#define printfEND(a) (cout << a << endl)

#define MIN(a,b) (a < b ? a : b)
#define MAX(a,b) (a > b ? a : b)

#define BEGIN {
#define END }
#define MODULE int main()

MODULE
BEGIN
    BEGIN
        uint32_t var {0};
        printf(var);
        SetBit(var, 0);
        printfEND(var);
    END

    BEGIN
        uint32_t var {UINT32_MAX};
        printf(var);
        ClearBit(var, 0);
        printfEND(var);
    END

    BEGIN
        uint32_t var {1};
        printf(var);
        InvBit(var, 0);
        printf(var);
        InvBit(var, 0);
        printf(var);
        InvBit(var, 0);
        InvBit(var, 1);
        printfEND(var);
    END

    BEGIN
        uint32_t var {0};
        SetBit(var, 0);
        if (BitIsSet(var,0)){
            printfEND("Bit is SET !");
        }else{
            printfEND("Bit is not SET !");
        }
        InvBit(var, 0);
        if (BitIsSet(var,0)){
            printfEND("Bit is SET !");
        }else{
            printfEND("Bit is not SET !");
        }
    END

    BEGIN
        uint32_t var {0};
        SetBit(var, 0);
        if (BitIsSet(var,0)){
            printfEND("Bit is SET !");
        }else{
            printfEND("Bit is not SET !");
        }
        InvBit(var, 0);
        if (BitIsSet(var,0)){
            printfEND("Bit is SET !");
        }else{
            printfEND("Bit is not SET !");
        }
    END

    BEGIN
        uint32_t var {0};
        ClearBit(var, 0);
        if (BitIsClear(var,0)){
            printfEND("Bit is SET !");
        }else{
            printfEND("Bit is not SET !");
        }
        InvBit(var, 0);
        if (BitIsClear(var,0)){
            printfEND("Bit is SET !");
        }else{
            printfEND("Bit is not SET !");
        }
    END

    printfEND(MIN(56,25));
    printfEND(MAX(56,25));
END
