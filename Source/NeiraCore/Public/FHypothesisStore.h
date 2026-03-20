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
    FString            Claim;                              // утверждение
    FString            Source;                             // свободный комментарий об источнике
    float              Confidence    = 0.0f;               // [0..1]
    EKnowledgeState    State         = EKnowledgeState::Pending;
    FString            Reason;                             // причина текущего статуса
    int32              ConfirmCount  = 0;                  // v0.2: счётчик подтверждений
    EHypothesisSource  SourceType    = EHypothesisSource::Unknown; // v0.4: тип источника
    EDataClassification DataClass    = EDataClassification::NonPII; // v0.6: privacy-класс
    bool               bPIIAllowed   = false;              // v0.6: явное разрешение на PII
    int64              CreatedSeq    = 0;                  // v0.6: sequence для retention
    int64              UpdatedSeq    = 0;                  // v0.6: sequence для retention
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
    EDataClassification DataClass = EDataClassification::NonPII; // privacy label event-log записи
};

struct NEIRACORE_API FDataRetentionPolicy
{
    int64 NonPIIRetentionOps = 1000;
    int64 PIIRetentionOps    = 50;
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
 * v0.4: добавлены Downgrade() (переходы вниз) и FindByClaim() (поиск по утверждению).
 * EHypothesisSource указывается при Store() через поле SourceType в FHypothesis.
 *
 * Реализация: v0.4
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
     * Понизить статус гипотезы (v0.4):
     *   VerifiedKnowledge → Conflicted
     *   Confirmed         → Pending (ConfirmCount сбрасывается в 0)
     *   Pending/Conflicted/Deprecated → false
     * Только успешный переход пишется в EventLog.
     */
    bool Downgrade(int32 HypothesisID, const FString& Reason);

    /**
     * Найти гипотезу по тексту утверждения (Claim).
     * Возвращает ID первого совпадения или -1 если не найдено.
     * v0.4: используется FBeliefEngine для поиска перед подтверждением.
     */
    int32 FindByClaim(const FString& Claim) const;

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

    /** Установить retention policy для очистки. */
    void SetRetentionPolicy(const FDataRetentionPolicy& InPolicy);

    /**
     * Очистить устаревшие записи по retention policy.
     * Возвращает число очищенных гипотез.
     */
    int32 PurgeExpired();

    /**
     * v0.6 guard: PII не хранится без явного разрешения.
     * false => запись отклоняется.
     */
    static bool CanStoreHypothesis(const FHypothesis& Hypothesis);

private:
    TArray<FHypothesis>      Hypotheses;
    TArray<FHypothesisEvent> EventLog;
    FDataRetentionPolicy     RetentionPolicy;
    int64                    SequenceCounter = 0;

    int64 NextSequence();
    void AppendEvent(int32 HypothesisID,
                     EKnowledgeState FromState,
                     EKnowledgeState ToState,
                     const FString& MethodName,
                     const FString& Reason,
                     EDataClassification DataClass);
};
