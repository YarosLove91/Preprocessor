# ==============================================================================
#  Учебный Makefile: препроцессор языка Си/C++
# ------------------------------------------------------------------------------
#  Каждый пример собирается отдельно, а также может быть разобран на всех
#  этапах трансляции: препроцессор -> токены -> синтаксическое дерево (AST)
#  -> ассемблер -> объектный файл -> исполняемый файл.
#
#  Примеры 4 и 5 работают с симуляцией ЖК-дисплея HD44780 (include/hd44780_sim.hpp)
#  и дополнительно демонстрируют:
#     - конфигурацию макросами и условную компиляцию (#if/#error),
#     - передачу макросов из командной строки (-DDEBUG, -DLCD_CONFIG_BIT=8),
#     - генерацию имён регистров оператором ## и строки оператором #.
#
#  Пример 6 — протопотоки (include/Protothread.h): кооперативная многозадачность,
#  целиком собранная из макросов (__LINE__ + switch + небалансные скобки).
#
#  Пример 7 — параметры компиляции: ключи -D/-U, значения по умолчанию через
#  #ifndef, встроенные макросы компилятора (run7_log, run7_custom DEFS='-D...').
#
#  Быстрый старт:
#     make help            - показать справку по всем целям
#     make                 - собрать все исполняемые примеры
#     make run_4           - собрать и запустить example_4 (4-битная шина)
#     make run8bit         - собрать example_4 с -DLCD_CONFIG_BIT=8 и запустить
#     make rundebug_4      - собрать example_4 с -DDEBUG и запустить
#     make ppm_4           - только СОБСТВЕННЫЙ код после препроцессора
#     make everything      - сгенерировать ВСЕ учебные артефакты
#     make clean           - удалить каталог build/
# ==============================================================================

# --- Инструменты и флаги ------------------------------------------------------
CXX        := g++
CXXFLAGS   := -std=c++17 -Wall -Wextra -Iinclude
CLANG      := clang++
CLANGFLAGS := -std=c++17 -Iinclude

# --- Каталоги и источники -----------------------------------------------------
BUILD      := build
SRCS       := $(sort $(wildcard src/example_*.cpp))          # новые примеры подхватятся сами
HDRS       := $(wildcard include/*.hpp) $(wildcard include/*.h)
NAMES      := $(notdir $(SRCS:.cpp=))

BINS       := $(addprefix $(BUILD)/,$(NAMES))
OBJS       := $(addsuffix .o,$(BINS))
ASMS       := $(addsuffix .s,$(BINS))
PPS        := $(addsuffix .ii,$(BINS))            # препроцессор (полный вывод)
PPSCLEAN   := $(addsuffix .ppi,$(BINS))           # препроцессор (без маркеров строк)
PPMAIN     := $(addsuffix .ppm,$(BINS))           # только собственный код после CPP
ASTS       := $(addsuffix .ast,$(BINS))           # синтаксическое дерево (clang AST)
TOKENS     := $(addsuffix .tokens,$(BINS))        # поток токенов (clang)
DEBUGBINS  := $(addsuffix -debug,$(BINS))         # сборки с -DDEBUG

# Фильтр для .ppm: оставить только строки из файлов проекта (src/, include/),
# отбросив системные заголовки (определяются по маркерам строк "# N \"file\"").
# Важно: # внутри присваивания переменной make — начало комментария, поэтому \#
PPM_FILTER := awk '/^\# [0-9]+ "/ { keep = ($$3 ~ /^"(src|include)\//) } !/^\#/ && keep'

.DEFAULT_GOAL := all

# --- Служебное: каталог сборки ------------------------------------------------
$(BUILD):
	@mkdir -p $(BUILD)

# ==============================================================================
#  ШАБЛОННЫЕ ПРАВИЛА (работают для любого example_N)
# ==============================================================================

# Исполняемый файл
$(BUILD)/%: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[build]     $< -> $@"
	@$(CXX) $(CXXFLAGS) $< -o $@

# Исполняемый файл с отладочным макросом (демонстрация ключа -D)
$(BUILD)/%-debug: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[build-dbg] $< -> $@  (-DDEBUG)"
	@$(CXX) $(CXXFLAGS) -DDEBUG $< -o $@

# Объектный файл (компиляция без компоновки)
$(BUILD)/%.o: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[object]    $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Ассемблерный листинг
$(BUILD)/%.s: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[asm]       $< -> $@"
	@$(CXX) $(CXXFLAGS) -S $< -o $@

# Код после препроцессора (полный, с маркерами строк "# N \"file\"")
$(BUILD)/%.ii: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[preproc]   $< -> $@"
	@$(CXX) $(CXXFLAGS) -E $< -o $@

# Код после препроцессора, читаемый (без маркеров строк, -P)
$(BUILD)/%.ppi: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[preproc-P] $< -> $@"
	@$(CXX) $(CXXFLAGS) -E -P $< -o $@

# Только СОБСТВЕННЫЙ код (src/ и include/ проекта) после препроцессора:
# системные заголовки отсекаются по маркерам строк — удобно смотреть на
# студенческих примерах, что именно получилось после раскрытия макросов.
$(BUILD)/%.ppm: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[preproc-m] $< -> $@  (только собственный код)"
	@$(CXX) $(CXXFLAGS) -E $< | $(PPM_FILTER) > $@

# Синтаксическое дерево (Clang AST dump)
$(BUILD)/%.ast: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[ast]       $< -> $@"
	@$(CLANG) $(CLANGFLAGS) -Xclang -ast-dump -fsyntax-only $< > $@ 2>/dev/null

# Поток токенов после лексического анализа (Clang token dump)
$(BUILD)/%.tokens: src/%.cpp $(HDRS) | $(BUILD)
	@echo "[tokens]    $< -> $@"
	@$(CLANG) $(CLANGFLAGS) -Xclang -dump-tokens -fsyntax-only $< > $@ 2>&1

# ==============================================================================
#  ДЕМО: конфигурация макросом из командной строки (example_4, -D...)
# ==============================================================================
$(BUILD)/example_4-8bit: src/example_4.cpp $(HDRS) | $(BUILD)
	@echo "[build]     $< -> $@  (-DLCD_CONFIG_BIT=8)"
	@$(CXX) $(CXXFLAGS) -DLCD_CONFIG_BIT=8 $< -o $@

.PHONY: run8bit
run8bit: $(BUILD)/example_4-8bit
	@echo "==================== example_4  (-DLCD_CONFIG_BIT=8) ===================="
	@./$(BUILD)/example_4-8bit

# ==============================================================================
#  ДЕМО: передача параметров компиляции (example_7, ключи -D/-U)
#  Цели пересобираются КАЖДЫЙ раз: параметры приходят из командной строки
#  (make run7_custom DEFS='-DLED_COUNT=8 -DFEATURE_LOG')
# ==============================================================================
DEFS ?=

.PHONY: run7_log run7_release run7_custom pp7_custom

run7_log: | $(BUILD)
	@echo "[build]     src/example_7.cpp -> $(BUILD)/example_7-log  (-DFEATURE_LOG)"
	@$(CXX) $(CXXFLAGS) -DFEATURE_LOG src/example_7.cpp -o $(BUILD)/example_7-log
	@./$(BUILD)/example_7-log

run7_release: | $(BUILD)
	@echo "[build]     src/example_7.cpp -> $(BUILD)/example_7-release  (-DNDEBUG -DAPP_VERSION=2 -O2)"
	@$(CXX) $(CXXFLAGS) -DNDEBUG -DAPP_VERSION=2 -DAPP_NAME='"Release Build"' -O2 src/example_7.cpp -o $(BUILD)/example_7-release
	@./$(BUILD)/example_7-release

run7_custom: | $(BUILD)
	@echo "[build]     src/example_7.cpp -> $(BUILD)/example_7-custom  ($(DEFS))"
	@$(CXX) $(CXXFLAGS) $(DEFS) src/example_7.cpp -o $(BUILD)/example_7-custom
	@./$(BUILD)/example_7-custom

# Собственный код после препроцессора с теми же параметрами —
# наглядно видно, что ключи -D сделали с исходником.
pp7_custom: | $(BUILD)
	@echo "[preproc-m] src/example_7.cpp -> $(BUILD)/example_7-custom.ppm  ($(DEFS))"
	@$(CXX) $(CXXFLAGS) $(DEFS) -E src/example_7.cpp | $(PPM_FILTER) > $(BUILD)/example_7-custom.ppm
	@echo "Результат: $(BUILD)/example_7-custom.ppm"

# ==============================================================================
#  ГРУППОВЫЕ ЦЕЛИ
# ==============================================================================
.PHONY: all everything clean help run preprocess preprocess-main preprocess-clean \
        asm obj ast tokens debug rundebug

all: $(BINS)
	@echo "Сборка завершена. Исполняемые файлы в '$(BUILD)/'"

# Все учебные артефакты для всех примеров
everything: all debug preprocess preprocess-main preprocess-clean asm obj ast tokens
	@echo "Все учебные артефакты сгенерированы в '$(BUILD)/'"

preprocess:       $(PPS)
preprocess-main:  $(PPMAIN)
preprocess-clean: $(PPSCLEAN)
asm:              $(ASMS)
obj:              $(OBJS)
ast:              $(ASTS)
tokens:           $(TOKENS)
debug:            $(DEBUGBINS)

run: $(BINS)
	@for b in $(BINS); do \
		echo ""; echo "==================== $$b ===================="; \
		./$$b; \
	done

rundebug: $(DEBUGBINS)
	@for b in $(DEBUGBINS); do \
		echo ""; echo "==================== $$b (-DDEBUG) ===================="; \
		./$$b; \
	done

# ==============================================================================
#  ЦЕЛИ ДЛЯ КАЖДОГО ПРИМЕРА ОТДЕЛЬНО (раздельная сборка и разбор)
#  Генерируются автоматически через $(eval $(call ...)) для всех example_N
#    $(1) = имя примера (example_1), $(2) = короткий индекс (1)
# ==============================================================================
define EXAMPLE_TARGETS
.PHONY: example_$(2) run_$(2) pp_$(2) ppm_$(2) ppc_$(2) asm_$(2) obj_$(2) \
        ast_$(2) tok_$(2) debug_$(2) rundebug_$(2) study_$(2)

example_$(2):  $(BUILD)/$(1)                      ## собрать исполняемый файл
run_$(2):      $(BUILD)/$(1)                      ## собрать и запустить
	@./$(BUILD)/$(1)
pp_$(2):       $(BUILD)/$(1).ii                   ## код ПОСЛЕ препроцессора (полный)
ppm_$(2):      $(BUILD)/$(1).ppm                  ## код ПОСЛЕ препроцессора (только свой)
ppc_$(2):      $(BUILD)/$(1).ppi                  ## код ПОСЛЕ препроцессора (читаемый)
asm_$(2):      $(BUILD)/$(1).s                    ## ассемблерный листинг
obj_$(2):      $(BUILD)/$(1).o                    ## объектный файл
ast_$(2):      $(BUILD)/$(1).ast                  ## синтаксическое дерево (AST)
tok_$(2):      $(BUILD)/$(1).tokens               ## поток токенов
debug_$(2):    $(BUILD)/$(1)-debug                ## собрать с -DDEBUG
rundebug_$(2): $(BUILD)/$(1)-debug                ## собрать с -DDEBUG и запустить
	@./$(BUILD)/$(1)-debug
study_$(2):    $(BUILD)/$(1).ii $(BUILD)/$(1).ppm $(BUILD)/$(1).ppi $(BUILD)/$(1).s \
               $(BUILD)/$(1).o $(BUILD)/$(1).ast $(BUILD)/$(1).tokens $(BUILD)/$(1)
endef

# $(subst example_,,$(name)) извлекает номер: example_4 -> 4
$(foreach name,$(NAMES),$(eval $(call EXAMPLE_TARGETS,$(name),$(subst example_,,$(name)))))

# ==============================================================================
#  ОЧИСТКА И СПРАВКА
# ==============================================================================
clean:
	@rm -rf $(BUILD)
	@echo "Каталог '$(BUILD)/' удалён."

help:
	@echo ""
	@echo "Учебные цели по теме «Препроцессор» (N = номер примера):"
	@echo ""
	@echo "  Общие:"
	@echo "    all                 собрать все исполняемые примеры"
	@echo "    everything          сгенерировать ВСЕ артефакты для всех примеров"
	@echo "    run / rundebug      запустить все примеры (обычные / с -DDEBUG)"
	@echo "    preprocess          код после препроцессора (.ii)  — все примеры"
	@echo "    preprocess-main     только собственный код после препроцессора (.ppm)"
	@echo "    preprocess-clean    то же, что preprocess, но без маркеров строк (.ppi)"
	@echo "    ast                 синтаксическое дерево (.ast)   — все примеры"
	@echo "    tokens              поток токенов (.tokens)        — все примеры"
	@echo "    asm                 ассемблер (.s)                 — все примеры"
	@echo "    obj                 объектные файлы (.o)           — все примеры"
	@echo "    debug               собрать все примеры с -DDEBUG"
	@echo "    clean               удалить каталог build/"
	@echo ""
	@echo "  По одному примеру (замените N на номер):"
	@echo "    example_N           собрать исполняемый файл"
	@echo "    run_N               собрать и запустить"
	@echo "    pp_N                код ПОСЛЕ препроцессора (полный, .ii)"
	@echo "    ppm_N               код ПОСЛЕ препроцессора (только свой, .ppm)"
	@echo "    ppc_N               код ПОСЛЕ препроцессора (читаемый, .ppi)"
	@echo "    ast_N               синтаксическое дерево (.ast)"
	@echo "    tok_N               поток токенов (.tokens)"
	@echo "    asm_N               ассемблерный листинг (.s)"
	@echo "    obj_N               объектный файл (.o)"
	@echo "    debug_N             собрать с макросом DEBUG (-DDEBUG)"
	@echo "    rundebug_N          собрать с -DDEBUG и запустить"
	@echo "    study_N             сразу все артефакты по примеру N"
	@echo ""
	@echo "  Демо конфигурации макросами (example_4, дисплей HD44780):"
	@echo "    run_4               4-битная шина (конфигурация по умолчанию)"
	@echo "    run8bit             то же, но собранное с -DLCD_CONFIG_BIT=8"
	@echo "    rundebug_4          включить отладочный вывод через -DDEBUG"
	@echo ""
	@echo "  Протопотоки (example_6, include/Protothread.h):"
	@echo "    run_6               4 кооперативные задачи в одном потоке"
	@echo "    ppm_6               увидеть switch/case __LINE__ после препроцессора"
	@echo ""
	@echo "  Параметры компиляции (example_7, ключи -D/-U):"
	@echo "    run_7               сборка со значениями по умолчанию (#ifndef)"
	@echo "    run7_log            сборка с -DFEATURE_LOG"
	@echo "    run7_release        сборка с -DNDEBUG -DAPP_VERSION=2 -O2"
	@echo "    run7_custom DEFS='-DLED_COUNT=8 -DFEATURE_LOG'"
	@echo "    pp7_custom DEFS='...'  что сделали -D с кодом (.ppm)"
	@echo ""
	@echo "  Примеры:"
	@echo "    make pp_2           во что разворачиваются макросы example_2"
	@echo "    make ppm_4          что осталось от #if-веток example_4"
	@echo "    make run_5          как ## собирает имена регистров PORTD/DDRB"
	@echo ""
