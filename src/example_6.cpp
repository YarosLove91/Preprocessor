// =============================================================================
//  Препроцессор языка Си. Часть 6.
//  Протопотоки (protothreads): многозадачность, целиком собранная из макросов.
//
//  Библиотека Protothread.h — C++-порт классических protothreads Адама Данкельса
//  (https://github.com/benhoyt/protothreads-cpp, лицензия MIT). На таких
//  «потоках» работает ОС Contiki для сенсорных микроконтроллеров.
//
//  В чём магия (все детали — в комментариях Protothread.h):
//    * единственный «контекст» потока — это НОМЕР СТРОКИ исходника (__LINE__),
//      сохранённый в поле _ptLine: ни стека, ни регистров, ~2 байта на поток;
//    * PT_BEGIN() открывает  switch (_ptLine) { case 0: ...,
//      PT_END()   закрывает  default: ; } Stop(); return false;
//      — скобки НЕ сбалансированы внутри одного макроса: препроцессору всё
//      равно, он просто подставляет текст, а компилятор видит коррестный код;
//    * PT_WAIT_UNTIL(cond) запоминает _ptLine = __LINE__, создаёт метку
//      case __LINE__: и выходит из функции (return true = «ещё работаю»);
//      при следующем вызове switch прыгает ровно на сохранённую строку —
//      поток «продолжается с места остановки»;
//    * все переменные потока обязаны быть полями класса (stackless):
//      локальные переменные между вызовами Run() НЕ сохраняются.
//
//  Эксперименты:
//    1. make run_6   — четыре «параллельные» задачи в одном обычном потоке.
//    2. make ppm_6   — в build/example_6.ppm виден РАСКРЫТЫЙ код:
//                      switch (_ptLine) { case 0: ... case 78: ... — вместо
//                      макросов стоят настоящие номера строк этого файла.
//                      Переставьте строки местами — номера case пересчитаются
//                      сами, вручную их никто не пишет!
//    3. Ограничения: PT_WAIT_* нельзя использовать внутри собственного
//                      switch (case привяжется к нему), а два PT_WAIT_*
//                      на одной строке дадут ошибку duplicate case.
// =============================================================================

// Макросы протопотоков содержат НАМЕРЕННОЕ падение в case:
//     _ptLine = __LINE__; case __LINE__: ...
// Компилятор честно предупреждает об этом (-Wimplicit-fallthrough).
// Подавляем только это предупреждение: здесь оно — часть трюка, а не ошибка.
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

#include <iostream>
#include <string>
#include "Protothread.h"     // «вся ОС» — один заголовочный файл из макросов
#include "hd44780_sim.hpp"   // дисплей для итогового отчёта (примеры 4-5)

using std::cout;
using std::endl;

// -----------------------------------------------------------------------------
//  Конфигурация — по традиции макросами (см. example_4)
// -----------------------------------------------------------------------------
#define LED1_PERIOD        5    // период мигания LED1, тиков
#define LED1_TOGGLES       8    // сколько раз переключить LED1
#define LED2_PERIOD        11   // период мигания LED2, тиков
#define LED2_TOGGLES       4    // сколько раз переключить LED2
#define BUTTON_PRESS_TICK  30   // на каком тике «нажать кнопку»
#define REPORT_START       20   // когда потоку-отчёту запустить дочерний поток
#define WORKER_STEPS       3    // число шагов дочернего потока
#define WORKER_STEP_TICKS  5    // длительность шага, тиков

// -----------------------------------------------------------------------------
//  «Железо» и общее состояние — как в настоящей прошивке: глобальные переменные
// -----------------------------------------------------------------------------
unsigned long tick = 0;         // счётчик тиков «планировщика»
bool button_pressed = false;    // флаг «кнопки»

// -----------------------------------------------------------------------------
//  Поток 1 и 2: мигалки. Один класс — две задачи с разными параметрами.
//  Обратите внимание: все переменные — ПОЛЯ КЛАССА (потоки stackless).
// -----------------------------------------------------------------------------
class LedFlasher : public Protothread {
public:
    LedFlasher(const char* name, unsigned long period, int count)
        : name_(name), period_(period), count_(count),
          i_(0), next_(0), on_(false), toggles_(0) {}

    virtual bool Run() {
        PT_BEGIN();

        cout << name_ << " started, period = " << period_ << " ticks" << endl;
        for (i_ = 0; i_ < count_; i_++) {
            next_ += period_;
            PT_WAIT_UNTIL(tick >= next_);   // «сон» БЕЗ блокировки планировщика!
            on_ = !on_;
            ++toggles_;
            cout << name_ << " tick=" << tick << " -> " << (on_ ? "ON" : "OFF") << endl;
        }
        cout << name_ << " finished at tick=" << tick << endl;

        PT_END();
    }

    int toggles() const { return toggles_; }

private:
    const char* name_;
    unsigned long period_;
    int count_;
    int i_;
    unsigned long next_;
    bool on_;
    int toggles_;
};

// -----------------------------------------------------------------------------
//  Поток 3: ждёт СОБЫТИЕ (флаг кнопки), а не время — PT_WAIT_UNTIL принимает
//  любое условие.
// -----------------------------------------------------------------------------
class ButtonWatcher : public Protothread {
public:
    virtual bool Run() {
        PT_BEGIN();

        cout << "[BTN ] waiting for button press..." << endl;
        PT_WAIT_UNTIL(button_pressed);
        press_tick_ = tick;
        cout << "[BTN ] tick=" << tick << ": press detected!" << endl;

        PT_WAIT_UNTIL(tick >= press_tick_ + 10);   // «антидребезг»: ждём 10 тиков
        cout << "[BTN ] tick=" << tick << ": done (debounce window passed)" << endl;

        PT_END();
    }

private:
    unsigned long press_tick_ = 0;
};

// -----------------------------------------------------------------------------
//  Дочерний поток: «долгая операция» из трёх шагов.
//  Запускается из потока Report макросом PT_SPAWN — и это тоже просто
//  подстановка текста препроцессором.
// -----------------------------------------------------------------------------
class Worker : public Protothread {
public:
    virtual bool Run() {
        PT_BEGIN();

        next_ = tick;                            // отсчёт от момента запуска
        for (step_ = 1; step_ <= WORKER_STEPS; step_++) {
            next_ += WORKER_STEP_TICKS;
            PT_WAIT_UNTIL(tick >= next_);
            cout << "[WORK] step " << step_ << "/" << WORKER_STEPS
                 << " done, tick=" << tick << endl;
        }

        PT_END();
    }

private:
    int step_ = 0;
    unsigned long next_ = 0;
};

// -----------------------------------------------------------------------------
//  Поток 4: отчёт. Демонстрирует PT_SPAWN — вложенный (дочерний) протопоток.
// -----------------------------------------------------------------------------
class Report : public Protothread {
public:
    virtual bool Run() {
        PT_BEGIN();

        PT_WAIT_UNTIL(tick >= REPORT_START);
        cout << "[RPT ] tick=" << tick << ": spawning child worker (PT_SPAWN)" << endl;
        PT_SPAWN(worker_);   // = Restart() + ждать завершения дочернего потока
        cout << "[RPT ] tick=" << tick << ": child worker finished" << endl;

        PT_END();
    }

private:
    Worker worker_;          // состояние дочернего потока живёт здесь же
};

// =============================================================================
//  «Планировщик»: один обычный цикл, который каждый тик дёргает Run()
//  всех живых потоков. Run() никогда не блокируется: условие не выполнено —
//  поток сразу возвращает true («я ещё работаю, позови позже»).
// =============================================================================
int main() {
    cout << "Protothreads: 4 cooperative tasks in ONE real thread" << endl;
    cout << "(library: github.com/benhoyt/protothreads-cpp, MIT)" << endl;
    cout << "----------------------------------------------" << endl;

    LedFlasher led1("[LED1]", LED1_PERIOD, LED1_TOGGLES);
    LedFlasher led2("[LED2]", LED2_PERIOD, LED2_TOGGLES);
    ButtonWatcher btn;
    Report report;

    Protothread* threads[] = { &led1, &led2, &btn, &report };

    while (tick < 1000) {                       // 1000 — защита от зависания
        bool any_running = false;
        for (Protothread* t : threads) {
            if (t->IsRunning()) {
                t->Run();                       // вернуть true = «ещё работаю»
                if (t->IsRunning()) any_running = true;
            }
        }

        if (tick == BUTTON_PRESS_TICK) {        // «внешнее событие» от железа
            button_pressed = true;
            cout << "[SIM ] tick=" << tick << ": button pressed (event injected)" << endl;
        }

        if (!any_running) break;                // все потоки завершились
        ++tick;
    }

    cout << "----------------------------------------------" << endl;
    cout << "All protothreads finished at tick=" << tick << endl;
    cout << "No OS, no stacks, no real threads - just macros!" << endl;

    // Итоговый отчёт на дисплей HD44780 (симуляция из примеров 4-5)
    lcd.init(4, 2);
    lcd.print("Protothreads OK");
    lcd.command(static_cast<uint8_t>(0x80 | 0x40));
    lcd.print("L1:" + std::to_string(led1.toggles())
            + " L2:" + std::to_string(led2.toggles()));
    lcd.show();

    return 0;
}
