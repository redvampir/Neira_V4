// FBeliefEngine.cpp
// v0.4 — модуль рассуждений: IntentResult × EHypothesisSource → FHypothesisStore.
//
// Логика:
//   StoreFact   → создать гипотезу с AppliedConfidence = Confidence * Weight.
//                 Если AppliedConfidence < 0.30 → Rejected.
//   Запросные интенты (GetDefinition, GetWordFact, FindMeaning, AnswerAbility)
//               → при AppliedConfidence >= 0.50 найти гипотезу по EntityTarget,
//                 затем Confirm или Verify.
//   Остальные   → NoMatch.

#include "FBeliefEngine.h"
#include "FSemanticGraph.h"

namespace
{
constexpr float StoreThreshold = 0.30f;
constexpr float ConfirmThreshold = 0.50f;
constexpr float ThresholdEpsilon = 1e-6f;

bool IsBelowThreshold(float Value, float Threshold)
{
    // NOTE: защищаем граничные случаи вида (0.50f / 0.85f) * 0.85f,
    // где из-за float-округления получается 0.49999997.
    return Value + ThresholdEpsilon < Threshold;
}
}

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

FBeliefDecision FBeliefEngine::Process(const FIntentResult&  Intent,
                                       FHypothesisStore&     Store,
                                       EHypothesisSource     Source,
                                       const FSemanticGraph* Graph) const
{
    FBeliefDecision Decision;
    const float Weight           = GetSourceWeight(Source);
    const float AppliedConfidence = Intent.Confidence * Weight;
    Decision.AppliedConfidence   = AppliedConfidence;

    if (Intent.IntentID == EIntentID::StoreFact)
    {
        if (IsBelowThreshold(AppliedConfidence, StoreThreshold))
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
        H.DataClass  = EDataClassification::NonPII;
        H.bPIIAllowed = false;

        Decision.HypothesisID = Store.Store(H);
        if (Decision.HypothesisID < 0)
        {
            Decision.Action = EBeliefAction::Rejected;
            Decision.Reason = TEXT("privacy policy: запись гипотезы запрещена");
            return Decision;
        }
        Decision.Action       = EBeliefAction::Created;
        Decision.Reason       = TEXT("StoreFact → гипотеза создана");
        return Decision;
    }

    if (Intent.IntentID == EIntentID::GetDefinition  ||
        Intent.IntentID == EIntentID::GetWordFact     ||
        Intent.IntentID == EIntentID::FindMeaning     ||
        Intent.IntentID == EIntentID::AnswerAbility)
    {
        if (IsBelowThreshold(AppliedConfidence, ConfirmThreshold))
        {
            Decision.Action = EBeliefAction::Rejected;
            Decision.Reason = TEXT("уверенность после взвешивания ниже порога 0.50 для подтверждения");
            return Decision;
        }

        int32 ID = Store.FindByClaim(Intent.EntityTarget);

        // Семантическое расширение: если точного совпадения нет,
        // проверяем синонимы и гиперонимы через граф.
        if (ID == -1 && Graph && Graph->IsLoaded())
        {
            // Сначала синонимы (более близкие по смыслу), затем гиперонимы
            auto TryExpand = [&](const TArray<FString>& Candidates,
                                 const FString& RelLabel)
            {
                for (const FString& Alias : Candidates)
                {
                    const int32 AliasID = Store.FindByClaim(Alias);
                    if (AliasID != -1)
                    {
                        ID = AliasID;
                        Decision.MatchedVia = FString::Printf(
                            TEXT("%s:%s"), *RelLabel, *Alias);
                        return;
                    }
                }
            };

            TryExpand(Graph->GetSynonyms(Intent.EntityTarget),  TEXT("synonym"));
            if (ID == -1)
                TryExpand(Graph->GetHypernyms(Intent.EntityTarget), TEXT("hypernym"));
        }

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
