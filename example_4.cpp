// =============================================================================
//  Препроцессор языка Си. Часть 4.
//  Условная компиляция: #if / #elif / #else / #endif / #ifdef / #ifndef / #error
//
//  Пример основан на реальной библиотеке ЖК-дисплея HD44780 для AVR
//  (проект SmartLCD, файлы hd44780.h и Defenitions.h). В embedded-библиотеках
//  конфигурация записывается макросами, а препроцессор оставляет в файле
//  только те ветки кода, которые нужны для данной конфигурации — в программу
//  не попадает ничего лишнего.
//
//  Эксперименты:
//    1. make run_4      — запуск с конфигурацией по умолчанию (4-битная шина)
//    2. make run8bit    — тот же исходник, собранный с -DLCD_CONFIG_BIT=8:
//                         шина стала 8-битной БЕЗ единой строчки #define в коде!
//                         Число передач на шине уменьшится вдвое.
//    3. make ppm_4      — только собственный код после препроцессора
//                         (build/example_4.ppm): видно, что от всех #if
//                         осталась лишь одна ветка.
//    4. make rundebug_4 — макрос DEBUG пришёл из командной строки (-DDEBUG),
//                         и отладочный вывод DBG() «ожил».
//    5. Поставьте LCD_CONFIG_BIT 3 — компилятор откажется собирать программу
//                         и напечатает сообщение из #error.
// =============================================================================

#include <iostream>
#include "hd44780_sim.h"   // симуляция дисплея + include guard (см. комментарии там)

using std::cout;
using std::endl;

// -----------------------------------------------------------------------------
//  Конфигурация дисплея (по аналогии с настоящим hd44780.h).
//  Обёртка #ifndef позволяет переопределить значение из командной строки
//  компилятора:  g++ -DLCD_CONFIG_BIT=8 ...  (см. make run8bit)
// -----------------------------------------------------------------------------
#ifndef LCD_CONFIG_BIT
  #define LCD_CONFIG_BIT    4     // ширина шины данных: 4 или 8 бит
#endif

#ifndef LCD_CONFIG_LINES
  #define LCD_CONFIG_LINES  2     // число строк дисплея: 1 или 2
#endif

// Проверка конфигурации ВО ВРЕМЯ КОМПИЛЯЦИИ:
// #error прерывает компиляцию и печатает сообщение.
#if LCD_CONFIG_BIT != 4 && LCD_CONFIG_BIT != 8
  #error "LCD_CONFIG_BIT must be 4 or 8"
#endif

#if LCD_CONFIG_LINES != 1 && LCD_CONFIG_LINES != 2
  #error "LCD_CONFIG_LINES must be 1 or 2"
#endif

// #if может не только вырезать код, но и определять разные макросы.
// Адрес второй строки в памяти DDRAM настоящего HD44780:
#if LCD_CONFIG_LINES >= 2
  #define LCD_ROW2_ADDR   0x40
#else
  #define LCD_ROW2_ADDR   (-1)    // второй строки нет — код ниже отсечён #if
#endif

// Отладочный вывод: компилируется, только если определён макрос DEBUG
// (его передаёт make rundebug_4 через -DDEBUG).
// В «прошивке» без DEBUG макрос раскрывается в пустоту — ноль накладных
// расходов: ни кода, ни строк в памяти.
#ifdef DEBUG
  #define DBG(text)  cout << "[DBG] " << text << endl
#else
  #define DBG(text)  /* пусто */
#endif

  int main() {
    lcd.init(LCD_CONFIG_BIT, LCD_CONFIG_LINES);

    // Текстовое сообщение тоже выбирается на этапе компиляции:
#if LCD_CONFIG_BIT == 4
    cout << "Display connected via 4-bit bus (data lines D4-D7 of PORTD)" << endl;
#else
    cout << "Display connected via 8-bit bus (data lines D0-D7 of PORTD)" << endl;
#endif

    DBG("init done, bus mode = " << lcd.bitMode() << " bit");

    lcd.clear();
    lcd.print("Hello, HD44780!");

#if LCD_CONFIG_LINES >= 2
    // Курсор во вторую строку — командой установки адреса DDRAM (0x80 | addr):
    lcd.command(static_cast<uint8_t>(0x80 | LCD_ROW2_ADDR));
    #if LCD_CONFIG_BIT == 4
        lcd.print("4-bit bus");
    #else
        lcd.print("8-bit bus");
    #endif
    DBG("second row: DDRAM address = 0x" << std::hex << LCD_ROW2_ADDR << std::dec);
#endif

    lcd.show();

    cout << endl << "Bus transfers: " << lcd.transfers();
#if LCD_CONFIG_BIT == 4
    cout << " (each byte = 2 transfers in 4-bit mode)";
#endif
    cout << endl;

    // Состояние «регистров» после работы с дисплеем (см. hd44780_sim.h)
    cout << "MCU registers:" << endl;
    printReg("PORTB", PORTB);
    printReg("PORTD", PORTD);
    printReg("DDRB ", DDRB);
    printReg("DDRD ", DDRD);

    return 0;
}
