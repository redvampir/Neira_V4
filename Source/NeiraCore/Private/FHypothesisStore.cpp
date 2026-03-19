// FHypothesisStore.cpp
// v0.2 — правила устойчивого перехода гипотез в знания.
//
// Изменения относительно v0.1:
//   - Добавлен ConfirmCount в FHypothesis (см. заголовок).
//   - Confirm() инкрементирует ConfirmCount при каждом вызове.
//   - Verify() требует ConfirmCount >= MinConfirmCount (= 2).
//   - Добавлен IsEligibleForVerification() для внешней проверки.
//
// Инварианты:
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

    H.ConfirmCount++;
    H.State  = EKnowledgeState::Confirmed;
    H.Reason = Reason;
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
    return true;
}

bool FHypothesisStore::MarkConflicted(int32 HypothesisID, const FString& Reason)
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return false;

    FHypothesis& H = Hypotheses[HypothesisID];
    H.State  = EKnowledgeState::Conflicted;
    H.Reason = Reason;
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
