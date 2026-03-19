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
    Store.Confirm(ID, TEXT("второе подтверждение"));  // v0.2: нужно MinConfirmCount=2

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

// ---------------------------------------------------------------------------
// v0.3: EventLog — транзакционный журнал
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_EventLog_Store_AppendsEvent,
    "Neira.HypothesisStore.EventLog_Store_AppendsEvent",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_EventLog_Store_AppendsEvent::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("тестовая гипотеза");
    Store.Store(H);

    const auto& Log = Store.GetEventLog();
    TestEqual(TEXT("После Store → лог содержит 1 запись"), Log.Num(), 1);
    TestEqual(TEXT("MethodName = Store"), Log[0].MethodName, FString(TEXT("Store")));
    TestEqual(TEXT("ToState = Pending"), Log[0].ToState, EKnowledgeState::Pending);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_EventLog_FullPath_FourEntries,
    "Neira.HypothesisStore.EventLog_ConfirmVerify_ContainsFourEvents",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_EventLog_FullPath_FourEntries::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("полный путь");
    int32 ID = Store.Store(H);
    Store.Confirm(ID, TEXT("первое"));
    Store.Confirm(ID, TEXT("второе"));
    Store.Verify(ID, TEXT("верификация"));

    const auto& Log = Store.GetEventLog();
    TestEqual(TEXT("Store + Confirm×2 + Verify → 4 записи"), Log.Num(), 4);
    TestEqual(TEXT("Запись 0: Store"),       Log[0].MethodName, FString(TEXT("Store")));
    TestEqual(TEXT("Запись 1: Confirm"),     Log[1].MethodName, FString(TEXT("Confirm")));
    TestEqual(TEXT("Запись 2: Confirm"),     Log[2].MethodName, FString(TEXT("Confirm")));
    TestEqual(TEXT("Запись 3: Verify"),      Log[3].MethodName, FString(TEXT("Verify")));
    TestEqual(TEXT("Запись 3 ToState: VerifiedKnowledge"),
        Log[3].ToState, EKnowledgeState::VerifiedKnowledge);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_EventLog_FailedVerify_NotLogged,
    "Neira.HypothesisStore.EventLog_FailedVerify_NotLogged",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_EventLog_FailedVerify_NotLogged::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("неудачная верификация");
    int32 ID = Store.Store(H);

    int32 CountBefore = Store.GetEventLog().Num();  // = 1 (Store)
    Store.Verify(ID, TEXT("попытка из Pending — должна провалиться"));
    int32 CountAfter  = Store.GetEventLog().Num();

    TestEqual(TEXT("Неудачный Verify не пишется в лог"),
        CountAfter, CountBefore);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_EventLog_MarkConflicted_Appends,
    "Neira.HypothesisStore.EventLog_MarkConflicted_AppendsEvent",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_EventLog_MarkConflicted_Appends::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("конфликт");
    int32 ID = Store.Store(H);
    Store.MarkConflicted(ID, TEXT("противоречие"));

    const auto& Log = Store.GetEventLog();
    TestEqual(TEXT("Store + MarkConflicted → 2 записи"), Log.Num(), 2);
    TestEqual(TEXT("Запись 1: MarkConflicted"),
        Log[1].MethodName, FString(TEXT("MarkConflicted")));
    TestEqual(TEXT("ToState = Conflicted"),
        Log[1].ToState, EKnowledgeState::Conflicted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_EventLog_FromTo_MatchTransitions,
    "Neira.HypothesisStore.EventLog_FromTo_MatchActualTransitions",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_EventLog_FromTo_MatchTransitions::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("переходы");
    int32 ID = Store.Store(H);
    Store.Confirm(ID, TEXT("первое подтверждение"));

    const auto& Log = Store.GetEventLog();
    // Confirm: Pending → Confirmed
    TestEqual(TEXT("Confirm.FromState = Pending"),
        Log[1].FromState, EKnowledgeState::Pending);
    TestEqual(TEXT("Confirm.ToState = Confirmed"),
        Log[1].ToState, EKnowledgeState::Confirmed);
    return true;
}

// ---------------------------------------------------------------------------
// Negative тесты (критические пробелы)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_Confirm_InvalidId_ReturnsFalse,
    "Neira.HypothesisStore.Confirm_InvalidId_ReturnsFalse",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_Confirm_InvalidId_ReturnsFalse::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    bool bOk = Store.Confirm(999, TEXT("несуществующий ID"));
    TestFalse(TEXT("Confirm(999) → false"), bOk);
    TestEqual(TEXT("EventLog не вырос"), Store.GetEventLog().Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStore_MarkConflicted_AlreadyConflicted_StillOk,
    "Neira.HypothesisStore.MarkConflicted_AlreadyConflicted_StillOk",
    NEIRA_TEST_FLAGS)
bool FHypothesisStore_MarkConflicted_AlreadyConflicted_StillOk::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("двойной конфликт");
    int32 ID = Store.Store(H);
    Store.MarkConflicted(ID, TEXT("первый"));
    bool bOk = Store.MarkConflicted(ID, TEXT("второй — не должен падать"));
    TestTrue(TEXT("Повторный MarkConflicted → true (не падает)"), bOk);
    TestEqual(TEXT("Статус всё ещё Conflicted"),
        Store.Find(ID)->State, EKnowledgeState::Conflicted);
    return true;
}

#undef NEIRA_TEST_FLAGS
