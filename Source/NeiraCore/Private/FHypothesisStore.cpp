// FHypothesisStore.cpp
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
    FHypothesis Stored    = Hypothesis;
    Stored.State          = EKnowledgeState::Pending;  // инвариант
    Stored.ConfirmCount   = 0;                          // инвариант v0.2
    Stored.Reason         = TEXT("создана");

    int32 ID = Hypotheses.Num();
    Hypotheses.Add(MoveTemp(Stored));

    FHypothesisEvent Ev;
    Ev.HypothesisID = ID;
    Ev.FromState    = EKnowledgeState::Pending;  // начальное состояние
    Ev.ToState      = EKnowledgeState::Pending;
    Ev.MethodName   = TEXT("Store");
    Ev.Reason       = TEXT("создана");
    EventLog.Add(MoveTemp(Ev));

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

    FHypothesisEvent Ev;
    Ev.HypothesisID = HypothesisID;
    Ev.FromState    = PrevState;
    Ev.ToState      = EKnowledgeState::Confirmed;
    Ev.MethodName   = TEXT("Confirm");
    Ev.Reason       = Reason;
    EventLog.Add(MoveTemp(Ev));

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

    FHypothesisEvent Ev;
    Ev.HypothesisID = HypothesisID;
    Ev.FromState    = EKnowledgeState::Confirmed;
    Ev.ToState      = EKnowledgeState::VerifiedKnowledge;
    Ev.MethodName   = TEXT("Verify");
    Ev.Reason       = Reason;
    EventLog.Add(MoveTemp(Ev));

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

    FHypothesisEvent Ev;
    Ev.HypothesisID = HypothesisID;
    Ev.FromState    = PrevState;
    Ev.ToState      = EKnowledgeState::Conflicted;
    Ev.MethodName   = TEXT("MarkConflicted");
    Ev.Reason       = Reason;
    EventLog.Add(MoveTemp(Ev));

    return true;
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
