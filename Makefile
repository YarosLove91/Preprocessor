# ==============================================================================
#  Учебный Makefile: препроцессор языка Си/C++
# ------------------------------------------------------------------------------
#  Каждый пример собирается отдельно, а также может быть разобран на всех
#  этапах трансляции: препроцессор -> токены -> синтаксическое дерево (AST)
#  -> ассемблер -> объектный файл -> исполняемый файл.
#
#  Примеры 4 и 5 работают с симуляцией ЖК-дисплея HD44780 (hd44780_sim.h)
#  и дополнительно демонстрируют:
#     - конфигурацию макросами и условную компиляцию (#if/#error),
#     - передачу макросов из командной строки (-DDEBUG, -DLCD_CONFIG_BIT=8),
#     - генерацию имён регистров оператором ## и строки оператором #.
#
#  Пример 6 — протопотоки (Protothread.h): кооперативная многозадачность,
#  целиком собранная из макросов (__LINE__ + switch + небалансные скобки).
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
CXXFLAGS   := -std=c++17 -Wall -Wextra
CLANG      := clang++
CLANGFLAGS := -std=c++17

# --- Каталоги и источники -----------------------------------------------------
BUILD      := build
SRCS       := $(sort $(wildcard example_*.cpp))   # новые примеры подхватятся сами
HDRS       := $(wildcard *.h)                     # общие заголовки (hd44780_sim.h)
NAMES      := $(SRCS:.cpp=)

BINS       := $(addprefix $(BUILD)/,$(NAMES))
OBJS       := $(addsuffix .o,$(BINS))
ASMS       := $(addsuffix .s,$(BINS))
PPS        := $(addsuffix .ii,$(BINS))            # препроцессор (полный вывод)
PPSCLEAN   := $(addsuffix .ppi,$(BINS))           # препроцессор (без маркеров строк)
PPMAIN     := $(addsuffix .ppm,$(BINS))           # только собственный код после CPP
ASTS       := $(addsuffix .ast,$(BINS))           # синтаксическое дерево (clang AST)
TOKENS     := $(addsuffix .tokens,$(BINS))        # поток токенов (clang)
DEBUGBINS  := $(addsuffix -debug,$(BINS))         # сборки с -DDEBUG

.DEFAULT_GOAL := all

# --- Служебное: каталог сборки ------------------------------------------------
$(BUILD):
	@mkdir -p $(BUILD)

# ==============================================================================
#  ШАБЛОННЫЕ ПРАВИЛА (работают для любого example_N)
# ==============================================================================

# Исполняемый файл
$(BUILD)/%: %.cpp $(HDRS) | $(BUILD)
	@echo "[build]     $< -> $@"
	@$(CXX) $(CXXFLAGS) $< -o $@

# Исполняемый файл с отладочным макросом (демонстрация ключа -D)
$(BUILD)/%-debug: %.cpp $(HDRS) | $(BUILD)
	@echo "[build-dbg] $< -> $@  (-DDEBUG)"
	@$(CXX) $(CXXFLAGS) -DDEBUG $< -o $@

# Объектный файл (компиляция без компоновки)
$(BUILD)/%.o: %.cpp $(HDRS) | $(BUILD)
	@echo "[object]    $< -> $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Ассемблерный листинг
$(BUILD)/%.s: %.cpp $(HDRS) | $(BUILD)
	@echo "[asm]       $< -> $@"
	@$(CXX) $(CXXFLAGS) -S $< -o $@

# Код после препроцессора (полный, с маркерами строк "# N \"file\"")
$(BUILD)/%.ii: %.cpp $(HDRS) | $(BUILD)
	@echo "[preproc]   $< -> $@"
	@$(CXX) $(CXXFLAGS) -E $< -o $@

# Код после препроцессора, читаемый (без маркеров строк, -P)
$(BUILD)/%.ppi: %.cpp $(HDRS) | $(BUILD)
	@echo "[preproc-P] $< -> $@"
	@$(CXX) $(CXXFLAGS) -E -P $< -o $@

# Только СОБСТВЕННЫЙ код (example_N.cpp и *.h проекта) после препроцессора:
# системные заголовки отсекаются по маркерам строк — удобно смотреть на
# студенческих примерах, что именно получилось после раскрытия макросов.
$(BUILD)/%.ppm: %.cpp $(HDRS) | $(BUILD)
	@echo "[preproc-m] $< -> $@  (только собственный код)"
	@$(CXX) $(CXXFLAGS) -E $< | awk '/^# [0-9]+ "/ { keep = ($$3 ~ /^"(example_[^"]+\.cpp|[A-Za-z0-9_]+\.h)"$$/ && $$3 !~ /\/usr\//) } !/^#/ && keep' > $@

# Синтаксическое дерево (Clang AST dump)
$(BUILD)/%.ast: %.cpp $(HDRS) | $(BUILD)
	@echo "[ast]       $< -> $@"
	@$(CLANG) $(CLANGFLAGS) -Xclang -ast-dump -fsyntax-only $< > $@ 2>/dev/null

# Поток токенов после лексического анализа (Clang token dump)
$(BUILD)/%.tokens: %.cpp $(HDRS) | $(BUILD)
	@echo "[tokens]    $< -> $@"
	@$(CLANG) $(CLANGFLAGS) -Xclang -dump-tokens -fsyntax-only $< > $@ 2>&1

# ==============================================================================
#  ДЕМО: конфигурация макросом из командной строки (example_4, -D...)
# ==============================================================================
$(BUILD)/example_4-8bit: example_4.cpp $(HDRS) | $(BUILD)
	@echo "[build]     $< -> $@  (-DLCD_CONFIG_BIT=8)"
	@$(CXX) $(CXXFLAGS) -DLCD_CONFIG_BIT=8 $< -o $@

.PHONY: run8bit
run8bit: $(BUILD)/example_4-8bit
	@echo "==================== example_4  (-DLCD_CONFIG_BIT=8) ===================="
	@./$(BUILD)/example_4-8bit

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
	@echo "  Протопотоки (example_6, Protothread.h):"
	@echo "    run_6               4 кооперативные задачи в одном потоке"
	@echo "    ppm_6               увидеть switch/case __LINE__ после препроцессора"
	@echo ""
	@echo "  Примеры:"
	@echo "    make pp_2           во что разворачиваются макросы example_2"
	@echo "    make ppm_4          что осталось от #if-веток example_4"
	@echo "    make run_5          как ## собирает имена регистров PORTD/DDRB"
	@echo ""
