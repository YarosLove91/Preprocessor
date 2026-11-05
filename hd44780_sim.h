// =============================================================================
//  hd44780_sim.h — симуляция ЖК-дисплея HD44780 (16x2) и «регистров AVR»
//  для учебных примеров 4 и 5 по теме «Препроцессор».
//
//  Основано на реальной библиотеке hd44780.h / Defenitions.h (проект SmartLCD,
//  МК AVR). Здесь железо заменено переменными и консолью, но все
//  препроцессорные приёмы работают точно так же.
// =============================================================================

#ifndef HD44780_SIM_H_
#define HD44780_SIM_H_
// INCLUDE GUARD (страж включения): тройка #ifndef / #define / #endif защищает
// файл от повторного включения. Без неё компилятор получил бы файл дважды и
// сообщил об ошибках повторного определения.
// Эксперимент: закомментируйте guard и включите заголовок два раза в
// example_4.cpp — компиляция упадёт.

#include <cstdint>
#include <iostream>
#include <string>

using std::cout;
using std::endl;

// -----------------------------------------------------------------------------
//  «Регистры микроконтроллера»
//  В настоящем AVR PORTB/DDRB/PINB — регистры, отображённые в память.
//  Здесь это обычные переменные — макросам препроцессора всё равно,
//  с чем работать: подстановка происходит до компиляции.
// -----------------------------------------------------------------------------
inline uint8_t PORTB = 0;   // данные порта B
inline uint8_t PORTD = 0;   // данные порта D
inline uint8_t DDRB  = 0;   // направление порта B (1 = выход)
inline uint8_t DDRD  = 0;   // направление порта D
inline uint8_t PINB  = 0;   // вход порта B
inline uint8_t PIND  = 0;   // вход порта D

// -----------------------------------------------------------------------------
//  Побитовые макросы (по аналогии с Defenitions.h проекта SmartLCD)
// -----------------------------------------------------------------------------
#define _Bit(bit)            (1u << (bit))
#define SetBit(reg, bit)     ((reg) |= _Bit(bit))
#define ClearBit(reg, bit)   ((reg) &= ~_Bit(bit))
#define InvBit(reg, bit)     ((reg) ^= _Bit(bit))
#define BitIsSet(reg, bit)   (((reg) & _Bit(bit)) != 0)
#define BitIsClear(reg, bit) (((reg) & _Bit(bit)) == 0)

// Правильный «макрос-оператор» — идиома do{...}while(0).
// Такой макрос можно безопасно использовать в if/else: он ведёт себя
// как один оператор. Наивный вариант  #define M(reg,bit,val) if(val)...;else...;
// сломал бы конструкцию if (x) M(...); else ...  (см. example_5, раздел 5).
#define SetBitVal(reg, bit, val)                     \
  do {                                             \
    if ((val) & 1) { SetBit((reg), (bit)); }     \
    else           { ClearBit((reg), (bit)); }   \
  } while (0)

// -----------------------------------------------------------------------------
//  Симуляция дисплея HD44780 (16 символов x 2 строки)
//
//  «Подключение» (как в hd44780.h):
//    шина данных       -> PORTD (в 4-битном режиме используются линии D4-D7)
//    линия RS          -> PORTB, бит 0   (данные/команда)
//    линия E (строб)   -> PORTB, бит 2   (дисплей защёлкивает данные по спаду E)
//
//  Каждый байт передаётся через sendByte(): в 4-битном режиме байт уходит
//  двумя полубайтами (нибблами) — передач в два раза больше, зато проводов
//  меньше. Счётчик transfers() позволяет увидеть эту разницу.
// -----------------------------------------------------------------------------
  class Hd44780Sim {
    public:
      enum { COLS = 16, ROWS = 2, RS_PIN = 0, E_PIN = 2 };

      Hd44780Sim() { init(4, ROWS); }

      // bitMode: 4 или 8; lines: 1 или 2
      void init(int bitMode, int lines) {
        bitMode_ = (bitMode == 8) ? 8 : 4;
        lines_   = (lines >= 2) ? ROWS : 1;
        DDRD = 0xFF;                          // порт данных — на вывод
        SetBit(DDRB, RS_PIN);
        SetBit(DDRB, E_PIN);
        clear();
        transfers_ = 0;                       // передачи инициализации не считаем
      }

      void clear() {
        for (int r = 0; r < ROWS; ++r)
          for (int c = 0; c < COLS; ++c)
            screen_[r][c] = ' ';
        col_ = 0;
        row_ = 0;
        command(0x01);                        // HD44780: CLEAR DISPLAY
      }

      // Команды настоящего HD44780:
      //   0x01 — очистка, 0x02 — курсор домой, 0x80|addr — установить адрес DDRAM
      void command(uint8_t cmd) {
        sendByte(cmd, false);
        if (cmd == 0x02) {
            col_ = 0; row_ = 0;               // HOME
        } else if (cmd & 0x80) {              // SET DDRAM ADDRESS
          uint8_t addr = static_cast<uint8_t>(cmd & 0x7F);
          if (addr >= 0x40) { row_ = 1; col_ = static_cast<int>(addr - 0x40); }
          else              { row_ = 0; col_ = static_cast<int>(addr); }
        }
      }

      // Установить курсор: col — позиция в строке, row — строка
      void setCursor(int col, int row) {
        command(static_cast<uint8_t>(0x80 | (row ? 0x40 : 0x00) | col));
      }

      void write(char ch) {
        sendByte(static_cast<uint8_t>(ch), true);   // передача символа по шине
        if (row_ < lines_ && col_ < COLS)
          screen_[row_][col_] = ch;
        if (++col_ == COLS) {
          col_ = 0;
          if (row_ + 1 < lines_) ++row_;
        }
      }

      void print(const std::string& s) {
        for (char c : s) write(c);
      }

      int transfers() const { return transfers_; }
      int bitMode()   const { return bitMode_; }
      void cursorOnOff(bool on) { cursorOn_ = on; }

      // Нарисовать «экран» в консоли
      void show() const {
        cout << "  +" << std::string(COLS, '-') << "+" << endl;
          for (int r = 0; r < lines_; ++r) {
            cout << "  |";
            for (int c = 0; c < COLS; ++c) {
              bool cur = cursorOn_ && (r == row_) && (c == col_);
              cout << (cur ? '_' : screen_[r][c]);
            }
          cout << "|" << endl;
        }
        cout << "  +" << std::string(COLS, '-') << "+" << endl;
      }

  private:
    void sendByte(uint8_t b, bool isData) {
      SetBitVal(PORTB, RS_PIN, isData ? 1 : 0);   // RS: данные или команда
      if (bitMode_ == 4) {                        // старший ниббл, затем младший
          putNibble(static_cast<uint8_t>(b >> 4));
          putNibble(static_cast<uint8_t>(b & 0x0F));
      } else {
          putNibble(b);                           // 8 бит — весь байт сразу
      }
    }

    void putNibble(uint8_t n) {
      if (bitMode_ == 4)
        PORTD = static_cast<uint8_t>((PORTD & 0x0F) | (n << 4)); // линии D4-D7
      else
        PORTD = n;                                               // линии D0-D7
      SetBit(PORTB, E_PIN);       // E = 1
      ++transfers_;
      ClearBit(PORTB, E_PIN);     // E = 0 — дисплей защёлкнул данные
    }

    char screen_[ROWS][COLS];
    int  col_ = 0;
    int  row_ = 0;
    int  bitMode_ = 4;
    int  lines_ = ROWS;
    int  transfers_ = 0;
    bool cursorOn_ = true;
};

  inline Hd44780Sim lcd;    // «подключённый» дисплей

  // Печать 8-битного регистра в двоичном виде — для наглядности
  inline void printReg(const char* name, uint8_t value) {
      cout << "  " << name << " = 0b";
      for (int i = 7; i >= 0; --i)
          cout << static_cast<char>('0' + ((value >> i) & 1));
      cout << "  (0x" << std::hex << static_cast<int>(value)
          << std::dec << ")" << endl;
  }

#endif // HD44780_SIM_H_
