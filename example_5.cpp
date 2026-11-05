// =============================================================================
//  Препроцессор языка Си. Часть 5.
//  Операторы ## (склеивание лексем) и # (преобразование в строку)
//  в настоящей embedded-библиотеке.
//
//  Основано на библиотеке HD44780 для AVR (проект SmartLCD): там имена
//  регистров (PORTD, DDRB) и выводов (PB2) НЕ вписаны в код вручную — их
//  собирает препроцессор из конфигурационных макросов. Поменял «распайку»
//  платы — изменил один #define, и вся библиотека переехала на другой порт.
//
//  Эксперименты:
//    1. make run_5   — запуск: видно, во что разворачиваются имена.
//    2. Поменяйте LCD_DATA_PORT_LETTER с D на B и пересоберите — макросы
//       сами «перепаяют» программу на порт B (DDRB/PORTB вместо DDRD/PORTD).
//    3. make ppm_5   — посмотреть build/example_5.ppm: в коде после
//       препроцессора остались только готовые идентификаторы PORTB, DDRD и т.д.
// =============================================================================

#include <iostream>
#include "hd44780_sim.h"

using std::cout;
using std::endl;

// -----------------------------------------------------------------------------
//  Конфигурация «распайки» дисплея (по аналогии с hd44780.h)
// -----------------------------------------------------------------------------
#define LCD_DATA_PORT_LETTER     D    // шина данных    -> порт D
#define LCD_CONTROL_PORT_LETTER  B    // линии управления -> порт B
#define LCD_RS_PIN               0    // линия RS (данные/команда)
#define LCD_E_PIN                2    // линия E  (строб)

// -----------------------------------------------------------------------------
//  Генерация имён регистров: ## склеивает лексемы.
//  GLUE(PORT, D) -> PORTD
// -----------------------------------------------------------------------------
#define GLUE(a, b)       a##b
#define GLUE3(a, b, c)   a##b##c

#define PORT_REG(x)      GLUE(PORT, x)    // PORT_REG(D)      -> PORTD
#define DDR_REG(x)       GLUE(DDR, x)     // DDR_REG(B)       -> DDRB
#define PIN_NAME(l, n)   GLUE3(P, l, n)   // PIN_NAME(B, 2)   -> PB2

// ВАЖНО: склейка двухступенчатая. В теле PORT_REG параметр x НЕ соседствует
// с ##, поэтому сначала он раскрывается (LCD_DATA_PORT_LETTER -> D), и лишь
// затем работает GLUE. Наивный вариант склеивает ДО раскрытия аргумента:
#define NAIVE_PORT(x)    PORT##x          // NAIVE_PORT(LCD_DATA_PORT_LETTER)
                                          //   -> PORTLCD_DATA_PORT_LETTER (ошибка!)

// -----------------------------------------------------------------------------
//  # — превращает аргумент макроса в строковый литерал.
//  STR_ НЕ раскрывает аргумент, а STR — сначала раскрывает, потом строит строку.
// -----------------------------------------------------------------------------
#define STR_(x)          #x
#define STR(x)           STR_(x)

// Наглядный макрос: печатает само выражение (благодаря #expr) и результат
// его раскрытия (благодаря STR(expr)):
#define PRINT_EXPR(expr) \
  cout << "  " << #expr << endl << "      -> " << STR(expr) << endl

// -----------------------------------------------------------------------------
//  Управляющие линии — ровно как в настоящем hd44780.h (E_SET / E_CLR):
//  имена портов и номера битов собраны препроцессором из конфигурации.
// -----------------------------------------------------------------------------
#define E_SET     SetBit(PORT_REG(LCD_CONTROL_PORT_LETTER), LCD_E_PIN)
#define E_CLR     ClearBit(PORT_REG(LCD_CONTROL_PORT_LETTER), LCD_E_PIN)
#define RS_SET(v) SetBitVal(PORT_REG(LCD_CONTROL_PORT_LETTER), LCD_RS_PIN, v)

// Отладочная печать с файлом и номером строки: __FILE__ и __LINE__ —
// предопределённые макросы, их подставляет сам препроцессор.
#define TRACE(msg) \
  cout << "  [" << __FILE__ << ":" << __LINE__ << "] " << msg << endl

  int main() {
    cout << "1) ## builds register names from configuration:" << endl;
    PRINT_EXPR(PORT_REG(LCD_DATA_PORT_LETTER));
    PRINT_EXPR(DDR_REG(LCD_CONTROL_PORT_LETTER));
    PRINT_EXPR(PIN_NAME(LCD_CONTROL_PORT_LETTER, LCD_E_PIN));

    cout << endl << "2) Why do we need the two-stage scheme (GLUE)?" << endl;
    cout << "  naive  PORT##x : " << STR(NAIVE_PORT(LCD_DATA_PORT_LETTER)) << endl;
    cout << "  via    GLUE    : " << STR(PORT_REG(LCD_DATA_PORT_LETTER)) << endl;
    cout << "  ## prevents argument expansion, so the raw text is glued." << endl;
    cout << "  Identifier PORTLCD_DATA_PORT_LETTER does not exist -" << endl;
    cout << "  a naive macro would break compilation." << endl;

    cout << endl << "3) The built names are real MCU registers:" << endl;
    DDR_REG(LCD_DATA_PORT_LETTER)  = 0xFF;    // на самом деле: DDRD  = 0xFF
    PORT_REG(LCD_DATA_PORT_LETTER) = 0x00;    // на самом деле: PORTD = 0x00
    printReg("DDRD ", DDR_REG(LCD_DATA_PORT_LETTER));
    printReg("PORTD", PORT_REG(LCD_DATA_PORT_LETTER));

    cout << endl << "4) Manual transfer of byte 'A' (0x41) to the display:" << endl;
    RS_SET(1);                                // PORTB |= (1<<0) — режим данных
    printReg("PORTB (RS=1)", PORTB);
    PORT_REG(LCD_DATA_PORT_LETTER) = 'A';     // байт на шину данных
    printReg("PORTD (='A') ", PORTD);
    E_SET;                                    // строб E=1 (PB2)
    printReg("PORTB (E=1)  ", PORTB);
    E_CLR;                                    // спад E — дисплей защёлкнул данные
    printReg("PORTB (E=0)  ", PORTB);

    cout << endl << "5) do{...}while(0) statement macro (from Defenitions.h):" << endl;
    bool rsLevel = false;
    if (rsLevel)
        RS_SET(1);       // наивный макрос «if...; else...;» здесь бы сломался:
    else                 // после раскрытия «else» остался бы без пары «if»
        RS_SET(0);
    printReg("PORTB (RS=0)", PORTB);

    cout << endl << "6) Predefined macros (__FILE__, __LINE__, __DATE__):" << endl;
    TRACE("display is ready");
    TRACE("second trace point");
    cout << "  Build date: " << __DATE__ << " " << __TIME__ << endl;

    cout << endl << "7) All together - the driver prints text on screen:" << endl;
    lcd.init(4, 2);
    lcd.print("GLUE & STR!");
    lcd.show();

    return 0;
  }
