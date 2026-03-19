#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

/**
 * FHypothesis — единица знания в системе.
 *
 * Новая гипотеза всегда создаётся со статусом Pending.
 * Перевод в VerifiedKnowledge возможен только через явную валидацию,
 * прямое присваивание статуса запрещено.
 *
 * v0.2: добавлен ConfirmCount для отслеживания накопленных подтверждений.
 */
struct NEIRACORE_API FHypothesis
{
    FString         Claim;                              // утверждение
    FString         Source;                             // откуда пришло
    float           Confidence    = 0.0f;               // [0..1]
    EKnowledgeState State         = EKnowledgeState::Pending;
    FString         Reason;                             // причина текущего статуса
    int32           ConfirmCount  = 0;                  // v0.2: счётчик подтверждений
};

/**
 * FHypothesisStore
 *
 * Хранилище гипотез и знаний агента.
 *
 * Инварианты:
 *   - Store() всегда выставляет Pending, ConfirmCount = 0.
 *   - Verify() из Pending → false (требуется Confirm() >= MinConfirmCount).
 *   - Verify() из Confirmed, но ConfirmCount < MinConfirmCount → false.
 *   - MarkConflicted() работает из любого состояния.
 *
 * v0.2: правило устойчивого перехода — MinConfirmCount = 2.
 * Гипотеза становится VerifiedKnowledge только после ≥2 подтверждений.
 *
 * Реализация: v0.2
 */
struct NEIRACORE_API FHypothesisStore
{
    /** Минимальное число подтверждений для Verify(). */
    static constexpr int32 MinConfirmCount = 2;

    /**
     * Сохранить новую гипотезу. Возвращает ID.
     * State = Pending, ConfirmCount = 0 независимо от переданных значений.
     */
    int32 Store(const FHypothesis& Hypothesis);

    /** Получить гипотезу по ID. Возвращает nullptr если не найдена. */
    const FHypothesis* Find(int32 HypothesisID) const;

    /**
     * Подтвердить гипотезу.
     *   Pending    → Confirmed (первое подтверждение).
     *   Confirmed  → остаётся Confirmed, ConfirmCount++.
     *   Conflicted/Deprecated → false (нельзя подтверждать).
     */
    bool Confirm(int32 HypothesisID, const FString& Reason);

    /**
     * Верифицировать гипотезу. Confirmed + ConfirmCount >= MinConfirmCount
     * → VerifiedKnowledge.
     * Из Pending или при недостаточном ConfirmCount → false.
     */
    bool Verify(int32 HypothesisID, const FString& Reason);

    /**
     * Зафиксировать конфликт. Любой статус → Conflicted.
     */
    bool MarkConflicted(int32 HypothesisID, const FString& Reason);

    /**
     * Проверить, достаточно ли подтверждений для верификации.
     * Удобно для тестов и внешней логики.
     */
    bool IsEligibleForVerification(int32 HypothesisID) const;

    int32 Count() const;

private:
    TArray<FHypothesis> Hypotheses;
};
