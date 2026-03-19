// BeliefEngineTests.cpp
// Тесты для FBeliefEngine (v0.4)
//
// Запуск: make run в Source/Tests/
// Все тесты должны ПАДАТЬ до реализации FBeliefEngine.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FBeliefEngine.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ---------------------------------------------------------------------------
// StoreFact — создание гипотезы
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_StoreFact_CreatesHypothesis,
    "Neira.BeliefEngine.StoreFact_CreatesHypothesis",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_StoreFact_CreatesHypothesis::RunTest(const FString& Parameters)
{
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::StoreFact;
    Intent.EntityTarget = TEXT("кот — животное");
    Intent.Confidence   = 0.8f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm);

    TestEqual(TEXT("Action == Created"), D.Action, EBeliefAction::Created);
    TestTrue(TEXT("HypothesisID >= 0"), D.HypothesisID >= 0);
    TestEqual(TEXT("Гипотеза сохранена в Store"), Store.Count(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_StoreFact_LowWeight_Rejected,
    "Neira.BeliefEngine.StoreFact_LowWeight_Rejected",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_StoreFact_LowWeight_Rejected::RunTest(const FString& Parameters)
{
    // AutoInference вес 0.60 * Confidence 0.40 = 0.24 < 0.30 → Rejected
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::StoreFact;
    Intent.EntityTarget = TEXT("что-то сомнительное");
    Intent.Confidence   = 0.40f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::AutoInference);

    TestEqual(TEXT("Action == Rejected"), D.Action, EBeliefAction::Rejected);
    TestEqual(TEXT("Store остался пустым"), Store.Count(), 0);
    TestTrue(TEXT("AppliedConfidence < 0.30"), D.AppliedConfidence < 0.30f);
    return true;
}

// ---------------------------------------------------------------------------
// Запросные интенты — подтверждение существующей гипотезы
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_GetDefinition_ConfirmsExisting,
    "Neira.BeliefEngine.GetDefinition_ConfirmsExisting",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_GetDefinition_ConfirmsExisting::RunTest(const FString& Parameters)
{
    FBeliefEngine Engine;
    FHypothesisStore Store;

    // Создать гипотезу вручную
    FHypothesis H;
    H.Claim = TEXT("кот");
    Store.Store(H);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кот");
    Intent.Confidence   = 0.9f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm);

    TestEqual(TEXT("Action == Confirmed"), D.Action, EBeliefAction::Confirmed);
    TestTrue(TEXT("HypothesisID >= 0"), D.HypothesisID >= 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_GetDefinition_LowConfidence_RejectedWithoutConfirm,
    "Neira.BeliefEngine.GetDefinition_LowConfidence_RejectedWithoutConfirm",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_GetDefinition_LowConfidence_RejectedWithoutConfirm::RunTest(const FString& Parameters)
{
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FHypothesis H;
    H.Claim = TEXT("кот");
    const int32 ID = Store.Store(H);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кот");
    Intent.Confidence   = 0.58f; // 0.58 * 0.85 = 0.493 < 0.50

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm);

    TestEqual(TEXT("Action == Rejected"), D.Action, EBeliefAction::Rejected);
    const FHypothesis* Stored = Store.Find(ID);
    TestNotNull(TEXT("Stored hypothesis exists"), Stored);
    if (Stored != nullptr)
    {
        TestEqual(TEXT("ConfirmCount не увеличился"), Stored->ConfirmCount, 0);
        TestEqual(TEXT("State остаётся Pending"), Stored->State, EKnowledgeState::Pending);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_GetDefinition_ConfirmThresholdBoundary_Deterministic,
    "Neira.BeliefEngine.GetDefinition_ConfirmThresholdBoundary_Deterministic",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_GetDefinition_ConfirmThresholdBoundary_Deterministic::RunTest(const FString& Parameters)
{
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FHypothesis H;
    H.Claim = TEXT("кошка");
    const int32 ID = Store.Store(H);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кошка");
    Intent.Confidence   = 0.50f / 0.85f; // AppliedConfidence ровно на пороге 0.50

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm);

    TestEqual(TEXT("Action == Confirmed на границе порога"), D.Action, EBeliefAction::Confirmed);
    const FHypothesis* Stored = Store.Find(ID);
    TestNotNull(TEXT("Stored hypothesis exists"), Stored);
    if (Stored != nullptr)
    {
        TestEqual(TEXT("ConfirmCount увеличился до 1"), Stored->ConfirmCount, 1);
        TestEqual(TEXT("State == Confirmed"), Stored->State, EKnowledgeState::Confirmed);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_GetDefinition_NoMatch_IfNotStored,
    "Neira.BeliefEngine.GetDefinition_NoMatch_IfNotStored",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_GetDefinition_NoMatch_IfNotStored::RunTest(const FString& Parameters)
{
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("несуществующий объект");
    Intent.Confidence   = 0.9f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm);

    TestEqual(TEXT("Action == NoMatch"), D.Action, EBeliefAction::NoMatch);
    TestEqual(TEXT("HypothesisID == -1"), D.HypothesisID, -1);
    return true;
}

// ---------------------------------------------------------------------------
// Весовая схема
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_SourceWeight_DeveloperReview,
    "Neira.BeliefEngine.SourceWeight_DeveloperReview_MaxWeight",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_SourceWeight_DeveloperReview::RunTest(const FString& Parameters)
{
    // DeveloperReview вес 1.00 → AppliedConfidence == Confidence
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::StoreFact;
    Intent.EntityTarget = TEXT("тест");
    Intent.Confidence   = 0.5f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::DeveloperReview);

    TestEqual(TEXT("AppliedConfidence == 0.5"), D.AppliedConfidence, 0.5f);
    TestEqual(TEXT("Action == Created"), D.Action, EBeliefAction::Created);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_SourceWeight_AutoInference_Reduced,
    "Neira.BeliefEngine.SourceWeight_AutoInference_Reduced",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_SourceWeight_AutoInference_Reduced::RunTest(const FString& Parameters)
{
    // AutoInference вес 0.60 * Confidence 1.0 = 0.60
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::StoreFact;
    Intent.EntityTarget = TEXT("тест");
    Intent.Confidence   = 1.0f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::AutoInference);

    TestTrue(TEXT("AppliedConfidence ≈ 0.60"), fabsf(D.AppliedConfidence - 0.60f) < 0.001f);
    return true;
}

// ---------------------------------------------------------------------------
// Авто-верификация
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_AutoVerify_WhenEligible,
    "Neira.BeliefEngine.AutoVerify_WhenEligible",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_AutoVerify_WhenEligible::RunTest(const FString& Parameters)
{
    FBeliefEngine Engine;
    FHypothesisStore Store;

    // Создать гипотезу и дважды подтвердить вручную → IsEligibleForVerification=true
    FHypothesis H;
    H.Claim = TEXT("синтаксис");
    int32 ID = Store.Store(H);
    Store.Confirm(ID, TEXT("confirm1"));
    Store.Confirm(ID, TEXT("confirm2"));

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("синтаксис");
    Intent.Confidence   = 0.9f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm);

    TestEqual(TEXT("Action == Verified"), D.Action, EBeliefAction::Verified);

    const FHypothesis* Stored = Store.Find(ID);
    TestEqual(TEXT("State == VerifiedKnowledge"),
        Stored->State, EKnowledgeState::VerifiedKnowledge);
    return true;
}

// ---------------------------------------------------------------------------
// Нерелевантные интенты
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBeliefEngine_UnrelatedIntent_NoMatch,
    "Neira.BeliefEngine.UnrelatedIntent_ReturnsNoMatch",
    NEIRA_TEST_FLAGS)
bool FBeliefEngine_UnrelatedIntent_NoMatch::RunTest(const FString& Parameters)
{
    FBeliefEngine Engine;
    FHypothesisStore Store;

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::CheckTextErrors;
    Intent.EntityTarget = TEXT("текст");
    Intent.Confidence   = 0.9f;

    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm);

    TestEqual(TEXT("Action == NoMatch"), D.Action, EBeliefAction::NoMatch);
    TestEqual(TEXT("Store не изменился"), Store.Count(), 0);
    return true;
}
