#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FResponseGenerator.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ---------------------------------------------------------------------------
// Snapshot: выход — натуральное русское предложение, не debug-дамп
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResponseGenerator_ProfileV1_SnapshotContract,
    "Neira.ResponseGenerator.ProfileV1.SnapshotContract",
    NEIRA_TEST_FLAGS)
bool FResponseGenerator_ProfileV1_SnapshotContract::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;
    const FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm,
        EResponseLength::Short,
        EResponseInitiative::Low);

    FResponseGenerationInput Input;
    Input.ContextKey                           = TEXT("session_main");
    Input.SemanticDecision.IntentID            = EIntentID::GetDefinition;
    Input.SemanticDecision.EntityTarget        = TEXT("синтаксис");
    Input.SemanticDecision.SemanticCore        = TEXT("раздел лингвистики о построении предложений");
    Input.SemanticDecision.ConfidenceLevel     = EConfidenceLevel::Verified;
    Input.SessionResponseCount                 = 0;

    const FResponseGenerationOutput Out = Generator.Generate(Input, Profile);

    // Ответ — первая стратегия для GetDefinition+Verified+Calm (index 0): "X — это Y."
    const FString Expected = TEXT("синтаксис — это раздел лингвистики о построении предложений.");
    TestEqual(TEXT("Snapshot должен быть стабильным"), Out.ResponseText, Expected);

    // FormatID стабилен и содержит ключевые части
    TestTrue(TEXT("FormatID содержит intent"), Out.FormatID.Contains(TEXT("intent_")));
    TestTrue(TEXT("FormatID содержит strategy"), Out.FormatID.Contains(TEXT(".s_")));
    TestFalse(TEXT("Нет debug-блока [profile=]"), Out.ResponseText.Contains(TEXT("[profile=")));
    TestFalse(TEXT("Нет Тон:"), Out.ResponseText.Contains(TEXT("Тон:")));

    return true;
}

// ---------------------------------------------------------------------------
// Детерминизм: одинаковый input → одинаковый output
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResponseGenerator_DeterministicPolicy_SameIntentContext,
    "Neira.ResponseGenerator.DeterministicPolicy.SameIntentContext",
    NEIRA_TEST_FLAGS)
bool FResponseGenerator_DeterministicPolicy_SameIntentContext::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;
    const FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Business,
        EResponseLength::Medium,
        EResponseInitiative::Medium);

    FResponseGenerationInput Input;
    Input.ContextKey                           = TEXT("dialog_42");
    Input.SemanticDecision.IntentID            = EIntentID::FindMeaning;
    Input.SemanticDecision.EntityTarget        = TEXT("лексема");
    Input.SemanticDecision.SemanticCore        = TEXT("задаёт смысловое ядро запроса");
    Input.SemanticDecision.ConfidenceLevel     = EConfidenceLevel::Inferred;
    Input.SemanticDecision.bHasUncertainty     = true;
    Input.SemanticDecision.UncertaintyReason   = TEXT("контекст предыдущего шага неполный");
    Input.SessionResponseCount                 = 0;

    const FResponseGenerationOutput First  = Generator.Generate(Input, Profile);
    const FResponseGenerationOutput Second = Generator.Generate(Input, Profile);

    TestEqual(TEXT("FormatID детерминирован"), First.FormatID, Second.FormatID);
    TestEqual(TEXT("ResponseText детерминирован"), First.ResponseText, Second.ResponseText);
    TestTrue(TEXT("StrategyID заполнен"), !First.StrategyID.IsEmpty());
    TestTrue(TEXT("При неопределённости — блок Неопределённость:"),
             First.ResponseText.Contains(TEXT("Неопределённость:")));

    return true;
}

// ---------------------------------------------------------------------------
// Смысловое ядро сохраняется в разных тонах
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResponseGenerator_SemanticCore_InvariantAcrossTone,
    "Neira.ResponseGenerator.SemanticCore.InvariantAcrossTone",
    NEIRA_TEST_FLAGS)
bool FResponseGenerator_SemanticCore_InvariantAcrossTone::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;

    FResponseGenerationInput Input;
    Input.ContextKey                       = TEXT("ability_gate");
    Input.SemanticDecision.IntentID        = EIntentID::AnswerAbility;
    Input.SemanticDecision.EntityTarget    = TEXT("синтаксис");
    Input.SemanticDecision.SemanticCore    = TEXT("синтаксис");
    Input.SemanticDecision.ConfidenceLevel = EConfidenceLevel::Verified;
    Input.SessionResponseCount             = 0;

    const FResponseGenerationOutput CalmOut = Generator.Generate(
        Input,
        FResponsePersonalityProfile::MakeV1(EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low));

    const FResponseGenerationOutput BusinessOut = Generator.Generate(
        Input,
        FResponsePersonalityProfile::MakeV1(EResponseTone::Business, EResponseLength::Short, EResponseInitiative::Low));

    // Оба ответа упоминают синтаксис (entity target)
    TestTrue(TEXT("Calm содержит entity target"), CalmOut.ResponseText.Contains(TEXT("синтаксис")));
    TestTrue(TEXT("Business содержит entity target"), BusinessOut.ResponseText.Contains(TEXT("синтаксис")));
    // Тон меняет стратегию → разные StrategyID
    TestTrue(TEXT("Тон меняет стратегию"), CalmOut.StrategyID != BusinessOut.StrategyID);

    return true;
}

// ---------------------------------------------------------------------------
// Ротация: разные счётчики дают разные стратегии (если > 1 кандидата)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResponseGenerator_Rotation_ChangesPhraseVariant,
    "Neira.ResponseGenerator.Rotation.ChangesPhraseVariant",
    NEIRA_TEST_FLAGS)
bool FResponseGenerator_Rotation_ChangesPhraseVariant::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;
    const FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);

    FResponseGenerationInput Input;
    Input.ContextKey                       = TEXT("rotation_test");
    Input.SemanticDecision.IntentID        = EIntentID::GetDefinition;
    Input.SemanticDecision.EntityTarget    = TEXT("кот");
    Input.SemanticDecision.SemanticCore    = TEXT("животное");
    Input.SemanticDecision.ConfidenceLevel = EConfidenceLevel::Verified;

    Input.SessionResponseCount = 0;
    const FResponseGenerationOutput Out0 = Generator.Generate(Input, Profile);

    Input.SessionResponseCount = 1;
    const FResponseGenerationOutput Out1 = Generator.Generate(Input, Profile);

    // Стратегии 0 и 1 — разные (у GetDefinition+Verified+Calm их 5)
    TestTrue(TEXT("Ротация меняет стратегию"), Out0.StrategyID != Out1.StrategyID);
    TestTrue(TEXT("Ротация меняет текст"),     Out0.ResponseText != Out1.ResponseText);

    // Но StrategyID при SessionResponseCount=0 стабилен (детерминизм)
    Input.SessionResponseCount = 0;
    const FResponseGenerationOutput Out0Again = Generator.Generate(Input, Profile);
    TestEqual(TEXT("Ротация детерминирована"), Out0.StrategyID, Out0Again.StrategyID);

    return true;
}
