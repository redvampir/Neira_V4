#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FPhraseClassifier.h"
#include "FSyntaxParser.h"
#include "FIntentExtractor.h"
#include "FActionRegistry.h"
#include "FMemoryPressurePolicy.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDoDIntegration_EndToEnd_SyntaxTraceFailReasonThreshold,
    "Neira.Integration.DoDv03.EndToEnd_SyntaxTraceFailReasonThreshold",
    NEIRA_TEST_FLAGS)
bool FDoDIntegration_EndToEnd_SyntaxTraceFailReasonThreshold::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    FSyntaxParser Parser;
    FIntentExtractor Extractor;
    FActionRegistry Registry;

    const FString Phrase = TEXT("найди значение слова синтаксис");
    const EPhraseType PhraseType = Classifier.Classify(Phrase);
    TestEqual(TEXT("Классификатор должен распознать команду"), PhraseType, EPhraseType::Command);

    const FSemanticFrame Frame = Parser.Parse(Phrase, PhraseType);
    TestEqual(TEXT("Predicate должен быть распознан"), Frame.Predicate, FString(TEXT("найти")));
    TestEqual(TEXT("Object должен быть распознан как мета-объект"), Frame.Object, FString(TEXT("значение")));

    const FIntentResult Intent = Extractor.Extract(Phrase, PhraseType);
    TestEqual(TEXT("Intent должен быть FindMeaning"), Intent.IntentID, EIntentID::FindMeaning);
    TestEqual(TEXT("EntityTarget должен быть извлечён из фразы"), Intent.EntityTarget, FString(TEXT("синтаксис")));
    TestTrue(TEXT("Trace должен фиксировать frame-путь"),
             Intent.DecisionTrace.Contains(TEXT("Frame.Predicate:найти+DefinitionObject+ExtractedTerm")));
    TestEqual(TEXT("Успешный intent не должен иметь fail-reason"), Intent.FailReason, EActionFailReason::None);

    Registry.Register(EActionID::FindMeaning,
        [](const FActionRequest& Request) -> FActionResult
        {
            FActionResult Result;
            Result.bSuccess = true;
            Result.ResultText = FString(TEXT("ok:")) + Request.EntityTarget;
            Result.FailReason = EActionFailReason::None;
            return Result;
        });

    FActionRequest OkRequest;
    OkRequest.ActionID = EActionID::FindMeaning;
    OkRequest.EntityTarget = Intent.EntityTarget;
    OkRequest.Confidence = Intent.Confidence;

    const FActionResult OkResult = Registry.Execute(OkRequest);
    TestTrue(TEXT("Action должен успешно выполниться при confidence >= threshold"), OkResult.bSuccess);
    TestEqual(TEXT("Успешный action должен вернуть FailReason::None"), OkResult.FailReason, EActionFailReason::None);

    FActionRequest LowConfidenceRequest = OkRequest;
    LowConfidenceRequest.Confidence = FActionRegistry::LowConfidenceThreshold - 0.01f;

    const FActionResult LowConfidenceResult = Registry.Execute(LowConfidenceRequest);
    TestFalse(TEXT("Action должен быть заблокирован threshold gate"), LowConfidenceResult.bSuccess);
    TestEqual(TEXT("Threshold gate должен вернуть LowConfidence"),
              LowConfidenceResult.FailReason, EActionFailReason::LowConfidence);
    TestFalse(TEXT("LowConfidence должен содержать DiagnosticNote"),
              LowConfidenceResult.DiagnosticNote.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDoDIntegration_PartialParseAndMemoryDegradation,
    "Neira.Integration.DoDv03.PartialParseAndMemoryDegradation",
    NEIRA_TEST_FLAGS)
bool FDoDIntegration_PartialParseAndMemoryDegradation::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;

    const FIntentResult Partial = Extractor.Extract(TEXT("открой пожалуйста"), EPhraseType::Command);
    TestEqual(TEXT("Неполный синтаксический разбор должен давать Unknown intent"),
              Partial.IntentID, EIntentID::Unknown);
    TestEqual(TEXT("FailReason должен быть PartialParse"),
              Partial.FailReason, EActionFailReason::PartialParse);
    TestTrue(TEXT("Trace должен фиксировать frame и fallback-ветку"),
             Partial.DecisionTrace.Contains(TEXT("Frame:NoIntent"))
             && Partial.DecisionTrace.Contains(TEXT("Fallback:Unknown")));
    TestFalse(TEXT("PartialParse должен содержать диагностику"), Partial.DiagnosticNote.IsEmpty());

    FMemoryPressurePolicy Policy;
    FMemoryContextState State;
    State.HotMemory = { TEXT("session_topic") };
    State.WarmMemory = { TEXT("fact_a"), TEXT("fact_b") };
    State.ColdMemory = { TEXT("anchor_main::long_context") };
    State.Anchors = { TEXT("anchor_main") };
    State.AnchorContextPairs = { TEXT("anchor_main::long_context") };

    const FMemoryPolicyApplyResult HighResult = Policy.Apply(EMemoryPressureLevel::High, State);
    TestEqual(TEXT("High pressure должен применяться с деградацией"),
              HighResult.Status, EMemoryPolicyApplyStatus::AppliedWithDegradation);
    TestEqual(TEXT("Причина деградации для High — WarmSummarized"),
              HighResult.Reason, EMemoryPolicyDegradationReason::WarmSummarized);
    TestTrue(TEXT("WARM должен быть очищен после summary"), State.WarmMemory.IsEmpty());
    TestFalse(TEXT("WarmSummary должен быть заполнен"), State.WarmSummary.IsEmpty());

    return true;
}
