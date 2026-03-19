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
            ../NeiraCore/Private/FBeliefEngine.cpp \
            ../NeiraCore/Private/FMemoryPressurePolicy.cpp \
            ../NeiraCore/Private/FVoiceSessionOrchestrator.cpp \
            ../NeiraCore/Private/FResponseGenerator.cpp

TEST_SRC := ../NeiraTests/Private/PhraseClassifierTests.cpp \
            ../NeiraTests/Private/IntentExtractorTests.cpp \
            ../NeiraTests/Private/ActionRegistryTests.cpp \
            ../NeiraTests/Private/HypothesisStoreTests.cpp \
            ../NeiraTests/Private/MorphAnalyzerTests.cpp \
            ../NeiraTests/Private/SyntaxParserTests.cpp \
            ../NeiraTests/Private/BeliefEngineTests.cpp \
            ../NeiraTests/Private/LexiconPackageRegressionTests.cpp \
            ../NeiraTests/Private/ThresholdRegressionGateTests.cpp \
            ../NeiraTests/Private/MemoryPressurePolicyTests.cpp \
            ../NeiraTests/Private/DoDIntegrationTests.cpp \
            ../NeiraTests/Private/VoiceSessionOrchestratorSmokeTests.cpp \
            ../NeiraTests/Private/ResponseGeneratorTests.cpp

ALL_SRC  := main.cpp $(CORE_SRC) $(TEST_SRC)

.PHONY: all run regression-gate clean

all: neira_tests

neira_tests: $(ALL_SRC) ue_compat.h test_runner.h
	$(CXX) $(CXXFLAGS) $(ALL_SRC) -o neira_tests

run: neira_tests
	./neira_tests

regression-gate: neira_tests
	# Merge-blocking gate for threshold/fixtures changes: fail only on metric regressions.
	bash -o pipefail -c "./neira_tests | tee regression_gate.log || true"
	bash -o pipefail -c "grep -Eq '^  (PASS|FAIL)  Neira\\.RegressionGate\\.' regression_gate.log || echo '[regression-gate] WARNING: no executable Neira.RegressionGate.* tests found in binary'"
	! grep -Eq '^  FAIL  Neira\\.RegressionGate\\.' regression_gate.log

clean:
	rm -f neira_tests *.o
