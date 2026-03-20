// FHypothesisStore.cpp
// v0.4 — переходы вниз (Downgrade), поиск по утверждению (FindByClaim).
//
// Изменения относительно v0.3:
//   - Downgrade(): VerifiedKnowledge→Conflicted, Confirmed→Pending (ConfirmCount=0).
//   - FindByClaim(): поиск первой гипотезы с совпадающим Claim, возвращает ID или -1.
//
// v0.3 — транзакционный журнал переходов состояний (EventLog).
//
// Изменения относительно v0.2:
//   - Добавлен append-only EventLog (TArray<FHypothesisEvent>).
//   - Store(), Confirm(), Verify(), MarkConflicted() пишут в лог только
//     при успешном переходе состояния.
//   - Добавлены GetEventLog() и ClearEventLog().
//
// Инварианты EventLog:
//   - Только успешные переходы попадают в лог.
//   - Неудачные вызовы (Verify из Pending, Confirm из Conflicted) → не пишутся.
//   - FromState и ToState точно отражают реальный переход.
//
// Инварианты состояний (без изменений из v0.2):
//   - Store() → State=Pending, ConfirmCount=0.
//   - Pending → Verify() → false (обязателен хотя бы один Confirm()).
//   - Confirmed + ConfirmCount < 2 → Verify() → false.
//   - Confirmed + ConfirmCount >= 2 → Verify() → true (→ VerifiedKnowledge).
//   - MarkConflicted() → Conflicted из любого состояния.

#include "FHypothesisStore.h"

int32 FHypothesisStore::Store(const FHypothesis& Hypothesis)
{
    if (!CanStoreHypothesis(Hypothesis))
        return -1;

    FHypothesis Stored    = Hypothesis;
    Stored.State          = EKnowledgeState::Pending;  // инвариант
    Stored.ConfirmCount   = 0;                          // инвариант v0.2
    Stored.Reason         = TEXT("создана");
    Stored.CreatedSeq     = NextSequence();
    Stored.UpdatedSeq     = Stored.CreatedSeq;

    int32 ID = Hypotheses.Num();
    Hypotheses.Add(MoveTemp(Stored));

    AppendEvent(ID,
                EKnowledgeState::Pending,
                EKnowledgeState::Pending,
                TEXT("Store"),
                TEXT("создана"),
                Hypotheses[ID].DataClass);

    return ID;
}

const FHypothesis* FHypothesisStore::Find(int32 HypothesisID) const
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return nullptr;
    return &Hypotheses[HypothesisID];
}

bool FHypothesisStore::Confirm(int32 HypothesisID, const FString& Reason)
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return false;

    FHypothesis& H = Hypotheses[HypothesisID];

    // Conflicted и Deprecated — не подтверждаем
    if (H.State == EKnowledgeState::Conflicted ||
        H.State == EKnowledgeState::Deprecated)
        return false;

    EKnowledgeState PrevState = H.State;
    H.ConfirmCount++;
    H.State  = EKnowledgeState::Confirmed;
    H.Reason = Reason;
    H.UpdatedSeq = NextSequence();

    AppendEvent(HypothesisID, PrevState, EKnowledgeState::Confirmed,
                TEXT("Confirm"), Reason, H.DataClass);

    return true;
}

bool FHypothesisStore::Verify(int32 HypothesisID, const FString& Reason)
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return false;

    FHypothesis& H = Hypotheses[HypothesisID];

    // Требование v0.2: только из Confirmed + накоплено >= MinConfirmCount
    if (H.State != EKnowledgeState::Confirmed)
        return false;

    if (H.ConfirmCount < MinConfirmCount)
        return false;

    H.State  = EKnowledgeState::VerifiedKnowledge;
    H.Reason = Reason;
    H.UpdatedSeq = NextSequence();

    AppendEvent(HypothesisID, EKnowledgeState::Confirmed,
                EKnowledgeState::VerifiedKnowledge,
                TEXT("Verify"), Reason, H.DataClass);

    return true;
}

bool FHypothesisStore::MarkConflicted(int32 HypothesisID, const FString& Reason)
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return false;

    FHypothesis& H = Hypotheses[HypothesisID];
    EKnowledgeState PrevState = H.State;
    H.State  = EKnowledgeState::Conflicted;
    H.Reason = Reason;
    H.UpdatedSeq = NextSequence();

    AppendEvent(HypothesisID, PrevState, EKnowledgeState::Conflicted,
                TEXT("MarkConflicted"), Reason, H.DataClass);

    return true;
}

bool FHypothesisStore::Downgrade(int32 HypothesisID, const FString& Reason)
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return false;

    FHypothesis& H = Hypotheses[HypothesisID];
    EKnowledgeState PrevState = H.State;
    EKnowledgeState NextState;

    if (H.State == EKnowledgeState::VerifiedKnowledge)
    {
        NextState = EKnowledgeState::Conflicted;
    }
    else if (H.State == EKnowledgeState::Confirmed)
    {
        NextState        = EKnowledgeState::Pending;
        H.ConfirmCount   = 0;
    }
    else
    {
        return false;  // Pending, Conflicted, Deprecated — нельзя понизить
    }

    H.State  = NextState;
    H.Reason = Reason;
    H.UpdatedSeq = NextSequence();

    AppendEvent(HypothesisID, PrevState, NextState,
                TEXT("Downgrade"), Reason, H.DataClass);

    return true;
}

int32 FHypothesisStore::FindByClaim(const FString& Claim) const
{
    for (int32 i = 0; i < Hypotheses.Num(); ++i)
    {
        if (Hypotheses[i].Claim == Claim)
            return i;
    }
    return -1;
}

bool FHypothesisStore::IsEligibleForVerification(int32 HypothesisID) const
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return false;

    const FHypothesis& H = Hypotheses[HypothesisID];
    return H.State == EKnowledgeState::Confirmed &&
           H.ConfirmCount >= MinConfirmCount;
}

int32 FHypothesisStore::Count() const
{
    return Hypotheses.Num();
}

const TArray<FHypothesisEvent>& FHypothesisStore::GetEventLog() const
{
    return EventLog;
}

void FHypothesisStore::ClearEventLog()
{
    EventLog.Reset();
}

void FHypothesisStore::SetRetentionPolicy(const FDataRetentionPolicy& InPolicy)
{
    RetentionPolicy = InPolicy;
}

int32 FHypothesisStore::PurgeExpired()
{
    const int64 CurrentSeq = NextSequence();
    int32 Purged = 0;

    for (int32 i = 0; i < Hypotheses.Num(); ++i)
    {
        FHypothesis& H = Hypotheses[i];
        if (H.State == EKnowledgeState::Deprecated)
            continue;

        const int64 MaxAge = (H.DataClass == EDataClassification::PII)
            ? RetentionPolicy.PIIRetentionOps
            : RetentionPolicy.NonPIIRetentionOps;

        if (CurrentSeq - H.UpdatedSeq <= MaxAge)
            continue;

        const EKnowledgeState PrevState = H.State;
        H.Claim = TEXT("");
        H.Source = TEXT("");
        H.Reason = TEXT("retention purge");
        H.Confidence = 0.0f;
        H.ConfirmCount = 0;
        H.State = EKnowledgeState::Deprecated;
        H.UpdatedSeq = CurrentSeq;

        AppendEvent(i, PrevState, EKnowledgeState::Deprecated,
                    TEXT("RetentionPurge"), TEXT("данные очищены по retention"), H.DataClass);
        ++Purged;
    }

    return Purged;
}

bool FHypothesisStore::CanStoreHypothesis(const FHypothesis& Hypothesis)
{
    return Hypothesis.DataClass != EDataClassification::PII || Hypothesis.bPIIAllowed;
}

int64 FHypothesisStore::NextSequence()
{
    return ++SequenceCounter;
}

void FHypothesisStore::AppendEvent(int32 HypothesisID,
                                   EKnowledgeState FromState,
                                   EKnowledgeState ToState,
                                   const FString& MethodName,
                                   const FString& Reason,
                                   EDataClassification DataClass)
{
    FHypothesisEvent Ev;
    Ev.HypothesisID = HypothesisID;
    Ev.FromState    = FromState;
    Ev.ToState      = ToState;
    Ev.MethodName   = MethodName;
    Ev.Reason       = Reason;
    Ev.DataClass    = DataClass;
    EventLog.Add(MoveTemp(Ev));
}
