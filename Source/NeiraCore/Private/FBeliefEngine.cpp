// FBeliefEngine.cpp
// v0.4 — модуль рассуждений: IntentResult × EHypothesisSource → FHypothesisStore.
//
// Логика:
//   StoreFact   → создать гипотезу с AppliedConfidence = Confidence * Weight.
//                 Если AppliedConfidence < 0.30 → Rejected.
//   Запросные интенты (GetDefinition, GetWordFact, FindMeaning, AnswerAbility)
//               → найти гипотезу по EntityTarget, Confirm или Verify.
//   Остальные   → NoMatch.

#include "FBeliefEngine.h"

float FBeliefEngine::GetSourceWeight(EHypothesisSource Source)
{
    switch (Source)
    {
    case EHypothesisSource::DeveloperReview:    return 1.00f;
    case EHypothesisSource::ExternalValidation: return 0.95f;
    case EHypothesisSource::Dictionary:         return 0.90f;
    case EHypothesisSource::UserConfirm:        return 0.85f;
    case EHypothesisSource::AutoInference:      return 0.60f;
    default:                                    return 0.50f;
    }
}

FBeliefDecision FBeliefEngine::Process(const FIntentResult& Intent,
                                       FHypothesisStore&    Store,
                                       EHypothesisSource    Source) const
{
    FBeliefDecision Decision;
    const float Weight           = GetSourceWeight(Source);
    const float AppliedConfidence = Intent.Confidence * Weight;
    Decision.AppliedConfidence   = AppliedConfidence;

    if (Intent.IntentID == EIntentID::StoreFact)
    {
        if (AppliedConfidence < 0.30f)
        {
            Decision.Action = EBeliefAction::Rejected;
            Decision.Reason = TEXT("уверенность после взвешивания ниже порога 0.30");
            return Decision;
        }

        FHypothesis H;
        H.Claim      = Intent.EntityTarget;
        H.Confidence = AppliedConfidence;
        H.SourceType = Source;
        H.Source     = TEXT("BeliefEngine");

        Decision.HypothesisID = Store.Store(H);
        Decision.Action       = EBeliefAction::Created;
        Decision.Reason       = TEXT("StoreFact → гипотеза создана");
        return Decision;
    }

    if (Intent.IntentID == EIntentID::GetDefinition  ||
        Intent.IntentID == EIntentID::GetWordFact     ||
        Intent.IntentID == EIntentID::FindMeaning     ||
        Intent.IntentID == EIntentID::AnswerAbility)
    {
        const int32 ID = Store.FindByClaim(Intent.EntityTarget);
        if (ID == -1)
        {
            Decision.Action = EBeliefAction::NoMatch;
            Decision.Reason = TEXT("гипотеза с таким Claim не найдена");
            return Decision;
        }

        if (Store.IsEligibleForVerification(ID))
        {
            Store.Verify(ID, TEXT("BeliefEngine: достаточно подтверждений"));
            Decision.HypothesisID = ID;
            Decision.Action       = EBeliefAction::Verified;
            Decision.Reason       = TEXT("Verify: накоплено достаточно подтверждений");
        }
        else
        {
            Store.Confirm(ID, TEXT("BeliefEngine: подтверждение через запросный интент"));
            Decision.HypothesisID = ID;
            Decision.Action       = EBeliefAction::Confirmed;
            Decision.Reason       = TEXT("Confirm: запросный интент подтверждает гипотезу");
        }
        return Decision;
    }

    Decision.Action = EBeliefAction::NoMatch;
    Decision.Reason = TEXT("IntentID не связан с накоплением знаний");
    return Decision;
}
