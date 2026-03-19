#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

/**
 * FHypothesis — единица знания в системе.
 *
 * Новая гипотеза всегда создаётся со статусом Pending.
 * Перевод в VerifiedKnowledge возможен только через явную валидацию,
 * прямое присваивание статуса запрещено.
 */
struct NEIRACORE_API FHypothesis
{
    FString         Claim;                              // утверждение
    FString         Source;                             // откуда пришло
    float           Confidence    = 0.0f;               // [0..1]
    EKnowledgeState State         = EKnowledgeState::Pending;
    FString         Reason;                             // причина текущего статуса
};

/**
 * FHypothesisStore
 *
 * Хранилище гипотез и знаний агента.
 * Инвариант: нельзя создать гипотезу сразу в статусе VerifiedKnowledge.
 *
 * Реализация: v0.1
 */
struct NEIRACORE_API FHypothesisStore
{
    /**
     * Сохранить новую гипотезу. Возвращает ID.
     * Статус всегда Pending вне зависимости от переданного значения.
     */
    int32 Store(const FHypothesis& Hypothesis);

    /** Получить гипотезу по ID. Возвращает nullptr если не найдена. */
    const FHypothesis* Find(int32 HypothesisID) const;

    /**
     * Подтвердить гипотезу. Pending → Confirmed.
     * Если уже Confirmed и вызывается повторно — остаётся Confirmed
     * (переход в VerifiedKnowledge только через Verify()).
     */
    bool Confirm(int32 HypothesisID, const FString& Reason);

    /**
     * Верифицировать гипотезу. Confirmed → VerifiedKnowledge.
     * Из Pending напрямую — запрещено, возвращает false.
     */
    bool Verify(int32 HypothesisID, const FString& Reason);

    /**
     * Зафиксировать конфликт. Любой статус → Conflicted.
     */
    bool MarkConflicted(int32 HypothesisID, const FString& Reason);

    int32 Count() const;

private:
    TArray<FHypothesis> Hypotheses;
};
