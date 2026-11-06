// =============================================================================
//  Препроцессор языка Си. Часть 7.
//  Передача параметров компиляции: ключи -D, -U, -I,
//  значения по умолчанию через #ifndef и встроенные макросы компилятора.
//
//  Ключ -DNAME задаёт макрос ДО обработки исходного текста — это ровно то же,
//  что вписать #define NAME первой строкой файла. -DNAME=VALUE задаёт значение
//  (число или строку: -DAPP_NAME='"My App"' — внешние кавычки для shell,
//  внутренние — часть значения). Один исходник так собирается в разные
//  «прошивки»: debug/release, с фичами и без.
//
//  Ключ -I добавляет каталог поиска заголовков (в Makefile: -Iinclude),
//  ключ -U отменяет ранее заданное определение:
//      g++ -DFEATURE_LOG -UFEATURE_LOG ...   -> FEATURE_LOG НЕ определён
//      (ключи обрабатываются по порядку, побеждает последний).
//
//  Эксперименты (цели в Makefile):
//    make run_7           — сборка со значениями по умолчанию (#ifndef сработал)
//    make run7_log        — -DFEATURE_LOG: логирование включено
//    make run7_release    — -DNDEBUG -DAPP_VERSION=2 -O2: «продуктовая» сборка
//                           (макрос CHECK() — свой assert — отключится)
//    make run7_custom DEFS='-DLED_COUNT=8 -DFEATURE_LOG'
//    make pp7_custom DEFS='-DFEATURE_LOG'
//                         — увидеть в .ppm, что ключи сделали с кодом
//    g++ -DLED_COUNT=100  — сборка упадёт: параметр проверят ещё #if/#error
// =============================================================================

#include <iostream>

using std::cout;
using std::endl;

// -----------------------------------------------------------------------------
//  1. Параметры со значениями ПО УМОЛЧАНИЮ.
//  #ifndef определит макрос только если он НЕ пришёл из командной строки.
//  Именно так устроены конфиги настоящих библиотек (см. example_4).
// -----------------------------------------------------------------------------
#ifndef APP_NAME
    #define APP_NAME "Preprocessor Demo"    // строковый параметр
#endif

#ifndef APP_VERSION
    #define APP_VERSION 1                   // числовой параметр
#endif

#ifndef LED_COUNT
    #define LED_COUNT 4
#endif

// Валидация параметра ВО ВРЕМЯ КОМПИЛЯЦИИ: значение из -D участвует
// в арифметике #if как обычное число. Попробуйте -DLED_COUNT=100:
#if LED_COUNT < 1 || LED_COUNT > 8
    #error "LED_COUNT must be in range [1..8]"
#endif

// Значение -D используется и в выражениях: маска из LED_COUNT бит
// вычисляется препроцессором как текст ((1u << (4)) - 1u),
// а компилятор сведёт его к константе.
#define LED_MASK ((1u << (LED_COUNT)) - 1u)

// -----------------------------------------------------------------------------
//  2. Флаги фич: макрос-переключатель проверяется через #ifdef,
//  значение ему не нужно — важен сам факт определения (-DFEATURE_LOG).
// -----------------------------------------------------------------------------
#ifdef FEATURE_LOG
    #define LOG(msg) cout << "[LOG] " << msg << endl
#else
    #define LOG(msg)  /* пусто: ноль байт в прошивке */
#endif

// NDEBUG — стандартный макрос «продуктовой» сборки: его устанавливает
// ключ -DNDEBUG (и build-системы: cmake --build ... --config Release).
// Именно NDEBUG отключает assert() из <cassert>.
#ifdef NDEBUG
    #define BUILD_FLAVOUR "release (NDEBUG)"
#else
    #define BUILD_FLAVOUR "debug"
#endif

// Оператор defined() — гибкая проверка, чем #ifdef: работает с &&, ||, !
// прямо внутри #if. Здесь: «логирование включено И сборка отладочная».
#if defined(FEATURE_LOG) && !defined(NDEBUG)
    #define STARTUP_BANNER 1
#endif

// -----------------------------------------------------------------------------
//  3. Встроенные макросы компилятора и платформы — заданы БЕЗ всякого -D.
//  Так пишут переносимый код: одна исходница собирается GCC/Clang/MSVC,
//  под Linux/Windows/macOS, x86_64/ARM.
// -----------------------------------------------------------------------------
// __cplusplus — версия стандарта языка: 201703L = C++17
#if __cplusplus >= 202002L
    #define CPP_STD "C++20 or newer"
#elif __cplusplus >= 201703L
    #define CPP_STD "C++17"
#elif __cplusplus >= 201103L
    #define CPP_STD "C++11"
#else
    #define CPP_STD "C++98"
#endif

// -----------------------------------------------------------------------------
//  4. Практика: собственный assert() — всё сразу: __FILE__, __LINE__,
//  оператор # (текст выражения), идиома do{...}while(0) и зависимость от
//  ключа -DNDEBUG. В «продуктовой» сборке макрос становится пустым —
//  ровно так же устроен assert() из <cassert>.
// -----------------------------------------------------------------------------
#ifdef NDEBUG
    #define CHECK(expr)  /* отключён ключом -DNDEBUG из командной строки */
#else
    #define CHECK(expr)                                                  \
        do {                                                             \
            if (expr) {                                                  \
                cout << "[CHECK OK]     " << #expr << endl;              \
            } else {                                                     \
                cout << "[CHECK FAILED] " << __FILE__ << ":" << __LINE__ \
                     << ": (" << #expr << ")" << endl;                   \
            }                                                            \
        } while (0)
#endif

int main() {
    // Всё напечатанное ниже пришло ИЗ КОМАНДНОЙ СТРОКИ компилятора:
    cout << "==============================================" << endl;
    cout << "  Application : " << APP_NAME << endl;
    cout << "  Version     : " << APP_VERSION << endl;
    cout << "  Build       : " << BUILD_FLAVOUR << endl;
    cout << "  LED_COUNT   : " << LED_COUNT << endl;
    cout << "  LED_MASK    : 0x" << std::hex << LED_MASK << std::dec << endl;
    cout << "==============================================" << endl;

#ifdef STARTUP_BANNER
    cout << "Verbose startup: FEATURE_LOG is set, NDEBUG is not" << endl;
#endif

    for (int i = 0; i < LED_COUNT; ++i)
        LOG("LED " << i << " initialized");

    // CHECK показывает текст выражения (оператор #), а при провале —
    // ещё и место (__FILE__:__LINE__). В release-сборке (make run7_release)
    // исчезает и сам CHECK (пустое раскрытие), и весь блок ниже (#ifndef).
#ifndef NDEBUG
    cout << "CHECK macro (__FILE__, __LINE__ and #), disabled by -DNDEBUG:" << endl;
    CHECK(LED_COUNT >= 1 && LED_COUNT <= 8);
    CHECK(LED_MASK == ((1u << LED_COUNT) - 1u));
    CHECK(APP_VERSION == 99);   // намеренно ложное: демонстрирует FAILED-вывод
    cout << endl;
#endif

    cout << "Compiler built-in macros (no -D needed):" << endl;
    cout << "  __cplusplus = " << __cplusplus << " (" << CPP_STD << ")" << endl;
    cout << "  __DATE__    = " << __DATE__ << endl;
    cout << "  __TIME__    = " << __TIME__ << endl;

#ifdef __GNUC__
    cout << "  __GNUC__    = " << __GNUC__ << '.' << __GNUC_MINOR__
         << '.' << __GNUC_PATCHLEVEL__;
    #ifdef __clang__
        cout << " (clang " << __clang_major__ << '.' << __clang_minor__ << ")";
    #endif
    cout << endl;
#endif

#if defined(__linux__)
    cout << "  Platform    : Linux" << endl;
#elif defined(_WIN32)
    cout << "  Platform    : Windows" << endl;
#elif defined(__APPLE__)
    cout << "  Platform    : macOS" << endl;
#endif

#if defined(__x86_64__)
    cout << "  Arch        : x86_64" << endl;
#elif defined(__aarch64__)
    cout << "  Arch        : aarch64" << endl;
#endif

    // __OPTIMIZE__ компилятор определяет сам при -O1/-O2/-O3:
#ifdef __OPTIMIZE__
    cout << "  Optimization: enabled (-O...)" << endl;
#else
    cout << "  Optimization: disabled (-O0)" << endl;
#endif

    return 0;
}
