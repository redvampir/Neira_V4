CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wno-unused-parameter \
            -I. \
            -I../NeiraCore/Public \
            -I../NeiraTests/Private

CORE_SRC := ../NeiraCore/Private/FPhraseClassifier.cpp \
            ../NeiraCore/Private/FIntentExtractor.cpp \
            ../NeiraCore/Private/FActionRegistry.cpp \
            ../NeiraCore/Private/FHypothesisStore.cpp \
            ../NeiraCore/Private/FMorphAnalyzer.cpp \
            ../NeiraCore/Private/FSyntaxParser.cpp \
            ../NeiraCore/Private/FMemoryPressurePolicy.cpp

TEST_SRC := ../NeiraTests/Private/PhraseClassifierTests.cpp \
            ../NeiraTests/Private/IntentExtractorTests.cpp \
            ../NeiraTests/Private/ActionRegistryTests.cpp \
            ../NeiraTests/Private/HypothesisStoreTests.cpp \
            ../NeiraTests/Private/MorphAnalyzerTests.cpp \
            ../NeiraTests/Private/SyntaxParserTests.cpp \
            ../NeiraTests/Private/MemoryPressurePolicyTests.cpp \
            ../NeiraTests/Private/ThresholdRegressionGateTests.cpp

ALL_SRC  := main.cpp $(CORE_SRC) $(TEST_SRC)

.PHONY: all run regression-gate clean

all: neira_tests

neira_tests: $(ALL_SRC) ue_compat.h test_runner.h
	$(CXX) $(CXXFLAGS) $(ALL_SRC) -o neira_tests

run: neira_tests
	./neira_tests

regression-gate: neira_tests
	bash -o pipefail -c "./neira_tests | tee regression_gate.log"
	grep -q "Neira.RegressionGate" regression_gate.log

clean:
	rm -f neira_tests *.o
