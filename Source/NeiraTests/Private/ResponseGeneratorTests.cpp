#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FResponseGenerator.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

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
    Input.IntentID = EIntentID::GetDefinition;
    Input.ContextKey = TEXT("session_main");
    Input.SemanticCore = TEXT("синтаксис — раздел лингвистики о построении предложений");

    const FResponseGenerationOutput Out = Generator.Generate(Input, Profile);

    const FString Expected =
        TEXT("[profile=personality_profile_v1; tone=calm; len=short; initiative=low; format=v1.intent_1.ctx_session_main.tone_calm.len_short.init_low]\n")
        TEXT("Тон: спокойный.\n")
        TEXT("Инициатива: низкая.\n")
        TEXT("Ответ: определение — синтаксис — раздел лингвистики о построении предложений.\n")
        TEXT("Ограничение: факты не выдумываю.");

    TestEqual(TEXT("Snapshot/contract должен быть стабильным"), Out.ResponseText, Expected);

    TestTrue(TEXT("Обязателен блок profile"), Out.ResponseText.Contains(TEXT("[profile=personality_profile_v1")));
    TestTrue(TEXT("Обязателен блок tone"), Out.ResponseText.Contains(TEXT("Тон:")));
    TestTrue(TEXT("Обязателен блок answer"), Out.ResponseText.Contains(TEXT("Ответ:")));
    TestTrue(TEXT("Обязателен блок hallucination guard"), Out.ResponseText.Contains(TEXT("факты не выдумываю")));

    return true;
}

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
    Input.IntentID = EIntentID::FindMeaning;
    Input.ContextKey = TEXT("dialog_42");
    Input.SemanticCore = TEXT("лексема задаёт смысловое ядро запроса");
    Input.bHasUncertainty = true;
    Input.UncertaintyReason = TEXT("контекст предыдущего шага неполный");

    const FResponseGenerationOutput First = Generator.Generate(Input, Profile);
    const FResponseGenerationOutput Second = Generator.Generate(Input, Profile);

    TestEqual(TEXT("FormatID должен быть детерминирован"), First.FormatID, Second.FormatID);
    TestEqual(TEXT("Ответ должен быть детерминирован для одинакового intent+context"),
              First.ResponseText,
              Second.ResponseText);
    TestTrue(TEXT("При неопределённости нужен явный блок"),
             First.ResponseText.Contains(TEXT("Неопределённость:")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FResponseGenerator_SemanticCore_InvariantAcrossTone,
    "Neira.ResponseGenerator.SemanticCore.InvariantAcrossTone",
    NEIRA_TEST_FLAGS)
bool FResponseGenerator_SemanticCore_InvariantAcrossTone::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;

    const FString CoreMeaning = TEXT("порог confidence ограничивает запуск действия");

    FResponseGenerationInput Input;
    Input.IntentID = EIntentID::AnswerAbility;
    Input.ContextKey = TEXT("ability_gate");
    Input.SemanticCore = CoreMeaning;

    const FResponseGenerationOutput CalmOut = Generator.Generate(
        Input,
        FResponsePersonalityProfile::MakeV1(EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low));

    const FResponseGenerationOutput BusinessOut = Generator.Generate(
        Input,
        FResponsePersonalityProfile::MakeV1(EResponseTone::Business, EResponseLength::Short, EResponseInitiative::Low));

    const FString CoreLine = FString(TEXT("Ответ: возможность — ")) + CoreMeaning + TEXT(".");

    TestTrue(TEXT("Смысловое ядро должно сохраняться в calm"), CalmOut.ResponseText.Contains(CoreLine));
    TestTrue(TEXT("Смысловое ядро должно сохраняться в business"), BusinessOut.ResponseText.Contains(CoreLine));
    TestTrue(TEXT("Тон должен менять стилевую часть"), CalmOut.ResponseText != BusinessOut.ResponseText);

    return true;
}
