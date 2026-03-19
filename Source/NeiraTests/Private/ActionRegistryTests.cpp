// ActionRegistryTests.cpp
// Тесты для FActionRegistry (v0.1)
//
// Запуск: Unreal Automation Tool → фильтр "Neira.ActionRegistry"
// Все тесты должны ПАДАТЬ до реализации FActionRegistry.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FActionRegistry.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ---------------------------------------------------------------------------
// IsSupported
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_RegisterdAction_IsSupported,
    "Neira.ActionRegistry.RegisteredAction_IsSupported",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_RegisterdAction_IsSupported::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest&) {
        return FActionResult{ true, TEXT("определение"), EActionFailReason::None, TEXT("") };
    });
    TestTrue(TEXT("Зарегистрированное действие → IsSupported == true"),
        Registry.IsSupported(EActionID::GetDefinition));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_UnregisteredAction_NotSupported,
    "Neira.ActionRegistry.UnregisteredAction_NotSupported",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_UnregisteredAction_NotSupported::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;
    TestFalse(TEXT("Незарегистрированное действие → IsSupported == false"),
        Registry.IsSupported(EActionID::GetDefinition));
    return true;
}

// ---------------------------------------------------------------------------
// Execute: успешный путь
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_Execute_ReturnsSuccess,
    "Neira.ActionRegistry.Execute_RegisteredHighConfidence_ReturnsSuccess",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_Execute_ReturnsSuccess::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest& Req) {
        FActionResult Res;
        Res.bSuccess   = true;
        Res.ResultText = FString::Printf(TEXT("определение для %s"), *Req.EntityTarget);
        Res.FailReason = EActionFailReason::None;
        return Res;
    });

    FActionRequest Req;
    Req.ActionID     = EActionID::GetDefinition;
    Req.EntityTarget = TEXT("кот");
    Req.Confidence   = 0.9f;

    FActionResult Result = Registry.Execute(Req);
    TestTrue(TEXT("bSuccess == true"), Result.bSuccess);
    TestEqual(TEXT("FailReason == None"), Result.FailReason, EActionFailReason::None);
    return true;
}

// ---------------------------------------------------------------------------
// Execute: NotSupported
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_Execute_Unregistered_NotSupported,
    "Neira.ActionRegistry.Execute_UnregisteredAction_ReturnsNotSupported",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_Execute_Unregistered_NotSupported::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;

    FActionRequest Req;
    Req.ActionID   = EActionID::GetDefinition;
    Req.Confidence = 0.9f;

    FActionResult Result = Registry.Execute(Req);
    TestFalse(TEXT("bSuccess == false"), Result.bSuccess);
    TestEqual(TEXT("FailReason == NotSupported"),
        Result.FailReason, EActionFailReason::NotSupported);
    return true;
}

// ---------------------------------------------------------------------------
// Execute: LowConfidence
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_Execute_LowConfidence_ReturnsLowConfidence,
    "Neira.ActionRegistry.Execute_LowConfidence_ReturnsLowConfidence",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_Execute_LowConfidence_ReturnsLowConfidence::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest&) {
        return FActionResult{ true, TEXT("определение"), EActionFailReason::None, TEXT("") };
    });

    FActionRequest Req;
    Req.ActionID   = EActionID::GetDefinition;
    Req.Confidence = 0.3f;  // ниже порога 0.5

    FActionResult Result = Registry.Execute(Req);
    TestFalse(TEXT("bSuccess == false при низкой уверенности"), Result.bSuccess);
    TestEqual(TEXT("FailReason == LowConfidence"),
        Result.FailReason, EActionFailReason::LowConfidence);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_Execute_BoundaryConfidence_Passes,
    "Neira.ActionRegistry.Execute_ConfidenceAtThreshold_Passes",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_Execute_BoundaryConfidence_Passes::RunTest(const FString& Parameters)
{
    // Confidence ровно на пороге (0.5) должно пропускаться
    FActionRegistry Registry;
    Registry.Register(EActionID::FindMeaning, [](const FActionRequest&) {
        return FActionResult{ true, TEXT("значение"), EActionFailReason::None, TEXT("") };
    });

    FActionRequest Req;
    Req.ActionID   = EActionID::FindMeaning;
    Req.Confidence = FActionRegistry::LowConfidenceThreshold;  // ровно 0.5

    FActionResult Result = Registry.Execute(Req);
    TestTrue(TEXT("Confidence == порог → bSuccess == true"), Result.bSuccess);
    return true;
}

// ---------------------------------------------------------------------------
// Инвариант: пустой ResultText при bSuccess=false — допустим,
// но DiagnosticNote должна быть заполнена реестром при NotSupported/LowConfidence
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_Execute_NotSupported_HasDiagnostic,
    "Neira.ActionRegistry.Execute_NotSupported_DiagnosticNoteNotEmpty",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_Execute_NotSupported_HasDiagnostic::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;

    FActionRequest Req;
    Req.ActionID   = EActionID::StoreFact;
    Req.Confidence = 0.9f;

    FActionResult Result = Registry.Execute(Req);
    TestFalse(TEXT("DiagnosticNote не пустой при NotSupported"),
        Result.DiagnosticNote.IsEmpty());
    return true;
}

// ---------------------------------------------------------------------------
// Negative тесты: граничные значения Confidence
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_Execute_NegativeConfidence,
    "Neira.ActionRegistry.Execute_NegativeConfidence_ReturnsLowConfidence",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_Execute_NegativeConfidence::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest&) {
        return FActionResult{ true, TEXT("ok"), EActionFailReason::None, TEXT("") };
    });

    FActionRequest Req;
    Req.ActionID   = EActionID::GetDefinition;
    Req.Confidence = -0.1f;

    FActionResult Result = Registry.Execute(Req);
    TestFalse(TEXT("Confidence < 0 → bSuccess = false"), Result.bSuccess);
    TestEqual(TEXT("Confidence < 0 → FailReason = LowConfidence"),
        Result.FailReason, EActionFailReason::LowConfidence);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FActionRegistry_Execute_ZeroConfidence,
    "Neira.ActionRegistry.Execute_ZeroConfidence_ReturnsLowConfidence",
    NEIRA_TEST_FLAGS)
bool FActionRegistry_Execute_ZeroConfidence::RunTest(const FString& Parameters)
{
    FActionRegistry Registry;
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest&) {
        return FActionResult{ true, TEXT("ok"), EActionFailReason::None, TEXT("") };
    });

    FActionRequest Req;
    Req.ActionID   = EActionID::GetDefinition;
    Req.Confidence = 0.0f;

    FActionResult Result = Registry.Execute(Req);
    TestFalse(TEXT("Confidence == 0 → bSuccess = false"), Result.bSuccess);
    TestEqual(TEXT("Confidence == 0 → FailReason = LowConfidence"),
        Result.FailReason, EActionFailReason::LowConfidence);
    return true;
}

#undef NEIRA_TEST_FLAGS
