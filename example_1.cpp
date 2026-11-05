// Учебная программа - препроцессор языка Си. Часть 1
// Цель - показать как работают модификаторы типов

#include <iostream>
#include <iomanip>
#include <stdint.h>

using std::cout;
using std::endl;
using std::setw;

// Модификаторы
#define unsigned_int_mod        4294967295U
#define signed_int_mod          2147483647
#define unsigned_int_long_mod   4294967295UL
#define signed_int_long_long    9223372036854775807LL
#define unsigned_int_long_long  4294967295ULL

#define long_int_mod            9223372036854775807L
#define double_mod              28.0
#define float_mod               28.0F
#define long_double_mod         28.0L

int add (int _a, int _b){

    return _a + _b;
}

int main (void){

    cout << setw(10)
         << " unsigned int: " << sizeof(unsigned_int_mod)           << '\n'
         << " signed int: " << sizeof(signed_int_mod)               << '\n'
         << " unsigned int long: " << sizeof(unsigned_int_long_mod) << '\n'
         << " signed int long long : " << sizeof(signed_int_long_long) << '\n'
         << " unsigned int long long : " << sizeof(unsigned_int_long_long) << '\n'
         << " double mod: " << sizeof(double_mod)                   << '\n'
         << " long int: " << sizeof(long_int_mod)                   << '\n'
         << " long int: " << sizeof(long_int_mod)                   << '\n'
         << " float mod: " << sizeof(float_mod)                     << '\n'
         << " long double: " << sizeof(long_double_mod)             << '\n'
         << endl;

    cout << add(0x34u, 0x32ul) << endl;
    return 0;
}
