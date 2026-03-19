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
 * FHypothesisEvent — запись в append-only журнале переходов состояний.
 *
 * Пишется только при успешном переходе. Неудачные вызовы (Verify из Pending,
 * Confirm из Conflicted и т.д.) в лог не попадают.
 *
 * v0.3: введён для поддержки транзакционной модели памяти.
 * В v0.4 лог будет использоваться для replay при переходах вниз
 * (Verified → Conflicted → Pending).
 */
struct NEIRACORE_API FHypothesisEvent
{
    int32           HypothesisID = -1;
    EKnowledgeState FromState    = EKnowledgeState::Pending;
    EKnowledgeState ToState      = EKnowledgeState::Pending;
    FString         MethodName;  // "Store" | "Confirm" | "Verify" | "MarkConflicted"
    FString         Reason;
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
 * v0.3: добавлен append-only EventLog.
 * Каждый успешный переход состояния записывается в EventLog.
 *
 * Реализация: v0.3
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

    // -----------------------------------------------------------------------
    // Event log API (v0.3)
    // -----------------------------------------------------------------------

    /** Полный append-only журнал успешных переходов состояний. */
    const TArray<FHypothesisEvent>& GetEventLog() const;

    /** Очистить журнал. Используется в тестах и при сбросе хранилища. */
    void ClearEventLog();

private:
    TArray<FHypothesis>      Hypotheses;
    TArray<FHypothesisEvent> EventLog;
};
