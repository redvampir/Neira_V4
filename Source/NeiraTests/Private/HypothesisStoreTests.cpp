// HypothesisStoreTests.cpp
// Тесты для FHypothesisStore (v0.1)
//
// Запуск: Unreal Automation Tool → фильтр "Neira.HypothesisStore"
// Все тесты должны ПАДАТЬ до реализации FHypothesisStore.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FHypothesisStore.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ---------------------------------------------------------------------------
// Store — базовые инварианты
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Store_StatusIsPending,
    "Neira.HypothesisStore.Store_NewHypothesis_StatusIsPending",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Store_StatusIsPending::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim      = TEXT("кот — животное");
    H.Source     = TEXT("пользователь");
    H.Confidence = 0.8f;

    int32 ID = Store.Store(H);
    const FHypothesis* Stored = Store.Find(ID);

    TestNotNull(TEXT("Гипотеза найдена по ID"), Stored);
    TestEqual(TEXT("Статус новой гипотезы — Pending"),
        Stored->State, EKnowledgeState::Pending);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Store_IgnoresPassedState,
    "Neira.HypothesisStore.Store_IgnoresNonPendingState",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Store_IgnoresPassedState::RunTest(const FString& Parameters)
{
    // Даже если передали VerifiedKnowledge — должно стать Pending
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim  = TEXT("земля круглая");
    H.Source = TEXT("пользователь");
    H.State  = EKnowledgeState::VerifiedKnowledge;  // попытка обхода

    int32 ID = Store.Store(H);
    const FHypothesis* Stored = Store.Find(ID);

    TestEqual(TEXT("Store игнорирует переданный статус → всегда Pending"),
        Stored->State, EKnowledgeState::Pending);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Store_CountIncreases,
    "Neira.HypothesisStore.Store_CountIncreases",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Store_CountIncreases::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    TestEqual(TEXT("Пустое хранилище → Count == 0"), Store.Count(), 0);

    FHypothesis H;
    H.Claim = TEXT("тест");
    Store.Store(H);
    TestEqual(TEXT("После Store → Count == 1"), Store.Count(), 1);
    return true;
}

// ---------------------------------------------------------------------------
// Confirm — Pending → Confirmed
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Confirm_PendingToConfirmed,
    "Neira.HypothesisStore.Confirm_Pending_BecomesConfirmed",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Confirm_PendingToConfirmed::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("кот — млекопитающее");
    int32 ID = Store.Store(H);

    bool bOk = Store.Confirm(ID, TEXT("подтверждено словарём"));
    TestTrue(TEXT("Confirm вернул true"), bOk);
    TestEqual(TEXT("Статус → Confirmed"),
        Store.Find(ID)->State, EKnowledgeState::Confirmed);
    return true;
}

// ---------------------------------------------------------------------------
// Verify — Confirmed → VerifiedKnowledge; Pending → запрещено
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Verify_ConfirmedToVerified,
    "Neira.HypothesisStore.Verify_Confirmed_BecomesVerified",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Verify_ConfirmedToVerified::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("вода мокрая");
    int32 ID = Store.Store(H);
    Store.Confirm(ID, TEXT("первое подтверждение"));

    bool bOk = Store.Verify(ID, TEXT("многократно подтверждено"));
    TestTrue(TEXT("Verify вернул true"), bOk);
    TestEqual(TEXT("Статус → VerifiedKnowledge"),
        Store.Find(ID)->State, EKnowledgeState::VerifiedKnowledge);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Verify_PendingForbidden,
    "Neira.HypothesisStore.Verify_Pending_ReturnsFalse",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Verify_PendingForbidden::RunTest(const FString& Parameters)
{
    // Нельзя перейти из Pending напрямую в VerifiedKnowledge
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("непроверенная гипотеза");
    int32 ID = Store.Store(H);

    bool bOk = Store.Verify(ID, TEXT("попытка пропустить шаг"));
    TestFalse(TEXT("Verify из Pending → false"), bOk);
    TestEqual(TEXT("Статус остался Pending"),
        Store.Find(ID)->State, EKnowledgeState::Pending);
    return true;
}

// ---------------------------------------------------------------------------
// MarkConflicted — любой статус → Conflicted
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Conflict_FromPending,
    "Neira.HypothesisStore.MarkConflicted_FromPending_BecomesConflicted",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Conflict_FromPending::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("спорное утверждение");
    int32 ID = Store.Store(H);

    Store.MarkConflicted(ID, TEXT("найдено противоречие"));
    TestEqual(TEXT("Статус → Conflicted"),
        Store.Find(ID)->State, EKnowledgeState::Conflicted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Conflict_FromVerified,
    "Neira.HypothesisStore.MarkConflicted_FromVerified_BecomesConflicted",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Conflict_FromVerified::RunTest(const FString& Parameters)
{
    // Даже VerifiedKnowledge может быть оспорено
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("земля плоская");
    int32 ID = Store.Store(H);
    Store.Confirm(ID, TEXT("первое подтверждение"));
    Store.Verify(ID, TEXT("неверно верифицировано"));

    Store.MarkConflicted(ID, TEXT("противоречит другим фактам"));
    TestEqual(TEXT("Статус → Conflicted из VerifiedKnowledge"),
        Store.Find(ID)->State, EKnowledgeState::Conflicted);
    return true;
}

// ---------------------------------------------------------------------------
// Find — несуществующий ID
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Find_InvalidId_ReturnsNull,
    "Neira.HypothesisStore.Find_InvalidId_ReturnsNull",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Find_InvalidId_ReturnsNull::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    const FHypothesis* Result = Store.Find(9999);
    TestNull(TEXT("Несуществующий ID → nullptr"), Result);
    return true;
}

#undef NEIRA_TEST_FLAGS
