CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wno-unused-parameter \
            -I. \
            -I../NeiraCore/Public \
            -I../NeiraTests/Private
LDFLAGS  := -lsqlite3

CORE_SRC := ../NeiraCore/Private/FPhraseClassifier.cpp \
            ../NeiraCore/Private/FIntentExtractor.cpp \
            ../NeiraCore/Private/FActionRegistry.cpp \
            ../NeiraCore/Private/FHypothesisStore.cpp \
            ../NeiraCore/Private/FMorphAnalyzer.cpp \
            ../NeiraCore/Private/FSyntaxParser.cpp \
            ../NeiraCore/Private/FBeliefEngine.cpp \
            ../NeiraCore/Private/FMemoryPressurePolicy.cpp \
            ../NeiraCore/Private/FOfflineVoiceAdapters.cpp \
            ../NeiraCore/Private/FVoiceSessionOrchestrator.cpp \
            ../NeiraCore/Private/FSentencePlanner.cpp \
            ../NeiraCore/Private/FResponseGenerator.cpp \
            ../NeiraCore/Private/FSemanticGraph.cpp \
            ../NeiraCore/Private/FDialoguePipeline.cpp

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
            ../NeiraTests/Private/SentencePlannerTests.cpp \
            ../NeiraTests/Private/ResponseGeneratorTests.cpp \
            ../NeiraTests/Private/VoiceFixtureMetricsTests.cpp \
            ../NeiraTests/Private/SemanticGraphTests.cpp \
            ../NeiraTests/Private/SemanticIntegrationTests.cpp \
            ../NeiraTests/Private/DialoguePipelineTests.cpp

ALL_SRC  := main.cpp $(CORE_SRC) $(TEST_SRC)

.PHONY: all run regression-gate clean

all: neira_tests

neira_tests: $(ALL_SRC) ue_compat.h test_runner.h
	$(CXX) $(CXXFLAGS) $(ALL_SRC) -o neira_tests $(LDFLAGS)

run: neira_tests
	./neira_tests

regression-gate: neira_tests
	# Merge-blocking gate for threshold/fixtures changes: fail only on metric regressions.
	bash ../../scripts/regression_gate.sh

clean:
	rm -f neira_tests *.o
