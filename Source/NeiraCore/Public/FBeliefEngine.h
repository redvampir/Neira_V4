#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"
#include "FIntentExtractor.h"
#include "FHypothesisStore.h"

/**
 * EBeliefAction — тип решения, принятого FBeliefEngine.
 */
enum class EBeliefAction : uint8
{
    Created,    // Новая гипотеза создана в Store
    Confirmed,  // Существующая гипотеза подтверждена (Confirm вызван)
    Verified,   // Гипотеза верифицирована (VerifiedKnowledge)
    Rejected,   // AppliedConfidence ниже порога — ничего не сделано
    NoMatch,    // IntentID не связан с накоплением знаний / Claim не найден
};

/**
 * FBeliefDecision — результат одного шага рассуждения.
 *
 * Содержит принятое действие, ID затронутой гипотезы,
 * фактическую уверенность после весовой коррекции и причину.
 */
struct NEIRACORE_API FBeliefDecision
{
    EBeliefAction  Action             = EBeliefAction::NoMatch;
    int32          HypothesisID       = -1;
    float          AppliedConfidence  = 0.0f;   // Confidence * SourceWeight
    FString        Reason;
};

/**
 * FBeliefEngine — модуль рассуждений v0.4.
 *
 * Связывает результат FIntentExtractor с FHypothesisStore:
 * - StoreFact  → создаёт гипотезу с весовой коррекцией уверенности.
 * - Запросные интенты (GetDefinition, GetWordFact, FindMeaning, AnswerAbility)
 *   → ищут гипотезу по Claim и подтверждают её (Confirm или Verify).
 *
 * Весовая схема источников:
 *   DeveloperReview    1.00
 *   ExternalValidation 0.95
 *   Dictionary         0.90
 *   UserConfirm        0.85
 *   AutoInference      0.60
 *   Unknown            0.50
 *
 * Порог создания: AppliedConfidence >= 0.30f.
 *
 * Реализация: v0.4
 */
struct NEIRACORE_API FBeliefEngine
{
    /**
     * Обработать намерение и обновить хранилище знаний.
     *
     * @param Intent   Результат FIntentExtractor::Extract().
     * @param Store    Хранилище гипотез (изменяется in-place).
     * @param Source   Тип источника — влияет на вес уверенности.
     * @return         Решение: что было сделано и с какой гипотезой.
     */
    FBeliefDecision Process(const FIntentResult&  Intent,
                            FHypothesisStore&     Store,
                            EHypothesisSource     Source) const;

private:
    static float GetSourceWeight(EHypothesisSource Source);
};
