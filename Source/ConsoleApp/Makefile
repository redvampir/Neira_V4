# Makefile для сборки Neira Console — консольного интерфейса ядра Neira
# Использование:
#   make all         — собрать NeiraConsole.exe
#   make run         — собрать и запустить
#   make clean       — очистить

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wno-unused-parameter \
            -I. \
            -I../Tests \
            -I../NeiraCore/Public

# Исходники ядра
CORE_SRC := ../NeiraCore/Private/FPhraseClassifier.cpp \
            ../NeiraCore/Private/FIntentExtractor.cpp \
            ../NeiraCore/Private/FActionRegistry.cpp \
            ../NeiraCore/Private/FHypothesisStore.cpp \
            ../NeiraCore/Private/FMorphAnalyzer.cpp \
            ../NeiraCore/Private/FSyntaxParser.cpp \
            ../NeiraCore/Private/FBeliefEngine.cpp \
            ../NeiraCore/Private/FMemoryPressurePolicy.cpp \
            ../NeiraCore/Private/FResponseGenerator.cpp

# Исходник консольного приложения
CONSOLE_SRC := NeiraConsole.cpp

ALL_SRC := $(CORE_SRC) $(CONSOLE_SRC)

.PHONY: all run clean

all: NeiraConsole.exe

NeiraConsole.exe: $(ALL_SRC) ../Tests/ue_compat.h
	$(CXX) $(CXXFLAGS) $(ALL_SRC) -o NeiraConsole.exe

run: NeiraConsole.exe
	./NeiraConsole.exe

clean:
	rm -f NeiraConsole.exe
