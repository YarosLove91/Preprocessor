// Препроцессор Си. Пример # 3
// В данное примере демонстрируется работа механизма "склеивания"
#include <stdio.h>

// При обработке исходного кода препроцессор может выполнять две операции: # и ##.
// Операция # позволяет заключать текст параметра, который следует после операции, в кавычки:
#define print_int(n) printf(#n"=%d \n",n);
// Директива ## позволяет объединять две лексемы:
#define print_sec(a,b,c) printf("%d", a##b##c);
// Здесь объединяются три числа, которые передаются в макрос print. Или аналогичный пример:
#define get_third(a,b,c) a##b##c;

#define NL "\n"

int main(void)
{
    int x = 56;
    print_int(x);       // x=56
    int y = 115;
    print_int(y);       // y=115
    int number = 237;
    print_int(number);  // number=237

    print_sec(2, 16, 32);   // 21632
    printf(NL);
    int var = get_third(2, 4, 8);   // 248
    printf(NL);
    printf("%d \n", var);
    return 0;
}
