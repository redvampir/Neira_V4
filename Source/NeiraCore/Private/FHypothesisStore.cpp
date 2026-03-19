// FHypothesisStore.cpp
// v0.1 — хранилище гипотез и знаний агента.
//
// Инварианты:
//   - Store() всегда выставляет State = Pending, игнорируя переданное значение.
//   - Verify() из Pending → false (запрещено, нужен промежуточный Confirm).
//   - MarkConflicted() работает из любого состояния.
//   - ID = индекс в массиве (начиная с 0). Возвращается из Store().

#include "FHypothesisStore.h"

int32 FHypothesisStore::Store(const FHypothesis& Hypothesis)
{
    FHypothesis Stored    = Hypothesis;
    Stored.State          = EKnowledgeState::Pending;  // инвариант: всегда Pending
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

    // Из Conflicted или Deprecated — не подтверждаем
    if (H.State == EKnowledgeState::Conflicted ||
        H.State == EKnowledgeState::Deprecated)
        return false;

    H.State  = EKnowledgeState::Confirmed;
    H.Reason = Reason;
    return true;
}

bool FHypothesisStore::Verify(int32 HypothesisID, const FString& Reason)
{
    if (!Hypotheses.IsValidIndex(HypothesisID))
        return false;

    FHypothesis& H = Hypotheses[HypothesisID];

    // Запрет: Pending → VerifiedKnowledge напрямую
    if (H.State != EKnowledgeState::Confirmed)
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

int32 FHypothesisStore::Count() const
{
    return Hypotheses.Num();
}
