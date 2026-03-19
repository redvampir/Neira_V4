#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FMemoryPressurePolicy.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMemoryPressurePolicy_Transitions_MediumHighCritical,
    "Neira.MemoryPressurePolicy.Transitions_Medium_High_Critical",
    NEIRA_TEST_FLAGS)
bool FMemoryPressurePolicy_Transitions_MediumHighCritical::RunTest(const FString& Parameters)
{
    FMemoryPressurePolicy Policy;
    FMemoryContextState State;
    State.HotMemory = { TEXT("hot-1"), TEXT("hot-2") };
    State.WarmMemory = { TEXT("warm-a"), TEXT("warm-b"), TEXT("warm-a"), TEXT("warm-c") };
    State.ColdMemory = { TEXT("cold::anchor/topic-a"), TEXT("cold::topic-b") };
    State.Anchors = { TEXT("anchor/topic-a") };
    State.AnchorContextPairs = { TEXT("anchor/topic-a::topic-a restored context") };

    FMemoryPressurePolicyOptions Options;
    Options.MaxWarmItemsOnMedium = 2;
    Options.MinimalHotItemsOnCritical = 1;

    FMemoryPolicyApplyResult MediumResult = Policy.Apply(EMemoryPressureLevel::Medium, State, Options);
    TestEqual(TEXT("Medium: статус с деградацией из-за compact WARM"),
        MediumResult.Status, EMemoryPolicyApplyStatus::AppliedWithDegradation);
    TestEqual(TEXT("Medium: WARM compacted до 2"), State.WarmMemory.Num(), 2);
    TestEqual(TEXT("Medium: HOT сохранён"), State.HotMemory.Num(), 2);

    FMemoryPolicyApplyResult HighResult = Policy.Apply(EMemoryPressureLevel::High, State, Options);
    TestEqual(TEXT("High: статус с деградацией из-за summary WARM"),
        HighResult.Status, EMemoryPolicyApplyStatus::AppliedWithDegradation);
    TestEqual(TEXT("High: WARM очищен после summary"), State.WarmMemory.Num(), 0);
    TestFalse(TEXT("High: summary не пустой"), State.WarmSummary.IsEmpty());
    TestEqual(TEXT("High: HOT сохранён"), State.HotMemory.Num(), 2);

    FMemoryPolicyApplyResult CriticalResult = Policy.Apply(EMemoryPressureLevel::Critical, State, Options);
    TestEqual(TEXT("Critical: применён с деградацией HOT"),
        CriticalResult.Status, EMemoryPolicyApplyStatus::AppliedWithDegradation);
    TestEqual(TEXT("Critical: HOT сокращён до минимума"), State.HotMemory.Num(), 1);
    TestEqual(TEXT("Critical: COLD сохранён"), State.ColdMemory.Num(), 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMemoryPressurePolicy_Anchors_NoMandatoryLoss,
    "Neira.MemoryPressurePolicy.Anchors_NoMandatoryLoss",
    NEIRA_TEST_FLAGS)
bool FMemoryPressurePolicy_Anchors_NoMandatoryLoss::RunTest(const FString& Parameters)
{
    FMemoryPressurePolicy Policy;
    FMemoryContextState State;
    State.HotMemory = { TEXT("hot-1") };
    State.WarmMemory = { TEXT("warm-1") };
    State.ColdMemory = { TEXT("cold-1") };

    FMemoryPressurePolicyOptions Options;
    FMemoryPolicyApplyResult Result = Policy.Apply(EMemoryPressureLevel::High, State, Options);

    TestEqual(TEXT("High без anchors отклоняется"), Result.Status, EMemoryPolicyApplyStatus::Rejected);
    TestEqual(TEXT("Причина — обязательные anchors отсутствуют"),
        Result.Reason, EMemoryPolicyDegradationReason::MissingRequiredAnchors);
    TestEqual(TEXT("Состояние не мутировано: WARM на месте"), State.WarmMemory.Num(), 1);
    TestEqual(TEXT("Состояние не мутировано: HOT на месте"), State.HotMemory.Num(), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMemoryPressurePolicy_RestoreContext_FromAnchor,
    "Neira.MemoryPressurePolicy.RestoreContext_FromAnchor_AfterDegradation",
    NEIRA_TEST_FLAGS)
bool FMemoryPressurePolicy_RestoreContext_FromAnchor::RunTest(const FString& Parameters)
{
    FMemoryPressurePolicy Policy;
    FMemoryContextState State;
    State.HotMemory = { TEXT("hot-1"), TEXT("hot-2") };
    State.WarmMemory = { TEXT("warm-1"), TEXT("warm-2") };
    State.ColdMemory = { TEXT("dialog::anchor/project-x::full history") };
    State.Anchors = { TEXT("anchor/project-x") };

    FMemoryPressurePolicyOptions Options;
    Options.MinimalHotItemsOnCritical = 1;

    const FMemoryPolicyApplyResult CriticalResult = Policy.Apply(EMemoryPressureLevel::Critical, State, Options);
    TestEqual(TEXT("Critical применён"), CriticalResult.Status, EMemoryPolicyApplyStatus::AppliedWithDegradation);

    FString Restored;
    EMemoryPolicyDegradationReason RestoreReason = EMemoryPolicyDegradationReason::None;
    const bool bRestored = Policy.RestoreContextFromAnchor(
        TEXT("anchor/project-x"),
        State,
        Restored,
        RestoreReason);

    TestTrue(TEXT("Контекст восстановлен по anchor после деградации"), bRestored);
    TestTrue(TEXT("Восстановленный контекст содержит anchor"), Restored.Contains(TEXT("anchor/project-x"), false));

    return true;
}
