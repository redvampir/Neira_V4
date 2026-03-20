#pragma once

#include "CoreMinimal.h"
#include "FPhraseClassifier.h"
#include "FIntentExtractor.h"
#include "FBeliefEngine.h"
#include "FHypothesisStore.h"
#include "FResponseGenerator.h"
#include "FSemanticGraph.h"
#include "FVoiceSessionOrchestrator.h"

// ---------------------------------------------------------------------------
// Конфигурация пайплайна
// ---------------------------------------------------------------------------
struct NEIRACORE_API FDialoguePipelineConfig
{
    // Путь к SQLite базе семантического графа.
    // "" → граф не загружается (семантическое обогащение отключено).
    // ":memory:" → пустой in-memory граф (для тестов, заполняется вручную).
    FString SemanticGraphPath = TEXT("");

    // Максимальное число связанных понятий в ответе (синонимы + гиперонимы).
    int32 MaxRelatedTerms = 3;

    // Источник знания по умолчанию — влияет на вес уверенности в BeliefEngine.
    EHypothesisSource DefaultSource = EHypothesisSource::UserConfirm;

    // Профиль личности для генератора ответов.
    FResponsePersonalityProfile Personality;
};

// ---------------------------------------------------------------------------
// FDialoguePipeline
//
// v0.1 — объединяет весь NLU/knowledge пайплайн в один компонент:
//   FPhraseClassifier → FIntentExtractor → FBeliefEngine → FResponseGenerator
//
// Семантический граф (FSemanticGraph) подключён в двух точках:
//   1. FBeliefEngine::Process(graph) — расширение поиска гипотез через синонимы/гиперонимы.
//   2. EnrichRelatedTerms()          — добавление связанных понятий в ответ.
//
// Использование как FTextPipelineHandler:
//   FDialoguePipeline Pipeline(Config);
//   FVoiceSessionOrchestrator Orchestrator(Flags, Pipeline.MakeHandler(), Stt, Tts);
//
// Использование напрямую:
//   FString Answer = Pipeline.ProcessText(TEXT("что такое синтаксис?"));
// ---------------------------------------------------------------------------
struct NEIRACORE_API FDialoguePipeline
{
    explicit FDialoguePipeline(const FDialoguePipelineConfig& Config);

    // Запрещаем копирование (владеет FSemanticGraph / FHypothesisStore)
    FDialoguePipeline(const FDialoguePipeline&)            = delete;
    FDialoguePipeline& operator=(const FDialoguePipeline&) = delete;

    // ---------------------------------------------------------------------------
    // Основной интерфейс
    // ---------------------------------------------------------------------------

    /**
     * Обработать текстовый запрос, вернуть готовый текст ответа.
     * Весь пайплайн: классификация → intent → belief → response.
     */
    FString ProcessText(const FString& Input);

    /**
     * Создать FTextPipelineHandler для передачи в FVoiceSessionOrchestrator.
     * Handler владеет указателем на this — не переживёт уничтожения Pipeline.
     */
    FTextPipelineHandler MakeHandler();

    // ---------------------------------------------------------------------------
    // Диагностика и доступ для тестов
    // ---------------------------------------------------------------------------

    /** Загружен ли семантический граф. */
    bool IsSemanticGraphLoaded() const;

    /** Доступ к семантическому графу для ручного заполнения (тесты, предзагрузка). */
    FSemanticGraph& GetGraph();

    /** Доступ к хранилищу знаний (только чтение). */
    const FHypothesisStore& GetStore() const;

    /** Доступ к хранилищу знаний (изменение, тесты). */
    FHypothesisStore& GetStore();

private:
    FPhraseClassifier    Classifier;
    FIntentExtractor     Extractor;
    FBeliefEngine        Belief;
    FResponseGenerator   Generator;
    FHypothesisStore     Store;
    FSemanticGraph       Graph;
    FDialoguePipelineConfig Cfg;
    // Счётчик ответов — передаётся в FResponseGenerationInput для ротации стратегий.
    int32                SessionResponseCount = 0;

    /**
     * Построить FResponseSemanticDecision из результатов Intent + Belief.
     * Заполняет SemanticCore, bHasUncertainty и вызывает EnrichRelatedTerms.
     */
    FResponseSemanticDecision BuildSemanticDecision(
        const FIntentResult&   Intent,
        const FBeliefDecision& Decision) const;

    /**
     * Обогатить SemanticDecision связанными понятиями из графа.
     * Добавляет синонимы, затем гиперонимы до Cfg.MaxRelatedTerms.
     */
    void EnrichRelatedTerms(FResponseSemanticDecision& Semantic,
                            const FString&             EntityTarget) const;
};
