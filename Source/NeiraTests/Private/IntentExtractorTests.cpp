// IntentExtractorTests.cpp
// Тесты для FIntentExtractor (v0.1 + v0.3)
//
// Запуск: Unreal Automation Tool → фильтр "Neira.IntentExtractor"
// Все тесты должны ПАДАТЬ до реализации FIntentExtractor::Extract().

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FIntentExtractor.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ---------------------------------------------------------------------------
// Намерение: GET_DEFINITION
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_WhatIs_GetDefinition,
    "Neira.IntentExtractor.WhatIs_ReturnsGetDefinition",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_WhatIs_GetDefinition::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("что такое кот?"), EPhraseType::Question);
    TestEqual(TEXT("Intent → GetDefinition"), Result.IntentID, EIntentID::GetDefinition);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_WhatIs_ExtractsEntity,
    "Neira.IntentExtractor.WhatIs_ExtractsEntity",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_WhatIs_ExtractsEntity::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("что такое кот?"), EPhraseType::Question);
    TestEqual(TEXT("EntityTarget → 'кот'"), Result.EntityTarget, FString(TEXT("кот")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_WhatMeans_GetDefinition,
    "Neira.IntentExtractor.WhatMeans_ReturnsGetDefinition",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_WhatMeans_GetDefinition::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("что означает слово синтаксис?"), EPhraseType::Question);
    TestEqual(TEXT("Intent → GetDefinition"), Result.IntentID, EIntentID::GetDefinition);
    TestEqual(TEXT("EntityTarget → 'синтаксис'"), Result.EntityTarget, FString(TEXT("синтаксис")));
    return true;
}

// ---------------------------------------------------------------------------
// Намерение: STORE_FACT
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_IsStatement_StoreFact,
    "Neira.IntentExtractor.Statement_ReturnsStoreFact",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_IsStatement_StoreFact::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("кот — это животное"), EPhraseType::Statement);
    TestEqual(TEXT("Intent → StoreFact"), Result.IntentID, EIntentID::StoreFact);
    return true;
}

// ---------------------------------------------------------------------------
// Намерение: ANSWER_ABILITY
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_CanYou_AnswerAbility,
    "Neira.IntentExtractor.CanYou_ReturnsAnswerAbility",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_CanYou_AnswerAbility::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("ты можешь объяснять слова?"), EPhraseType::Question);
    TestEqual(TEXT("Intent → AnswerAbility"), Result.IntentID, EIntentID::AnswerAbility);
    return true;
}

// ---------------------------------------------------------------------------
// Уверенность
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_KnownPattern_HighConfidence,
    "Neira.IntentExtractor.KnownPattern_ConfidenceAbove0_7",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_KnownPattern_HighConfidence::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("что такое кот?"), EPhraseType::Question);
    TestTrue(TEXT("Известный паттерн → Confidence > 0.7"), Result.Confidence > 0.7f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_UnknownPhrase_LowConfidence,
    "Neira.IntentExtractor.UnknownPhrase_ConfidenceBelow0_5",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_UnknownPhrase_LowConfidence::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    // Фраза с неизвестным паттерном
    FIntentResult Result = Extractor.Extract(TEXT("бррр вжух"), EPhraseType::Unknown);
    TestTrue(TEXT("Неизвестная фраза → Confidence < 0.5"), Result.Confidence < 0.5f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_UnknownPhrase_UnknownIntent,
    "Neira.IntentExtractor.UnknownPhrase_ReturnsUnknownIntent",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_UnknownPhrase_UnknownIntent::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("бррр вжух"), EPhraseType::Unknown);
    TestEqual(TEXT("Неизвестная фраза → Intent::Unknown"),
        Result.IntentID, EIntentID::Unknown);
    TestEqual(TEXT("Unknown intent → FailReason::UnknownIntent"),
        Result.FailReason, EActionFailReason::UnknownIntent);
    TestFalse(TEXT("Unknown intent → DiagnosticNote не пустой"),
        Result.DiagnosticNote.IsEmpty());
    TestTrue(TEXT("Unknown intent проходит через fallback"),
        Result.DecisionTrace.Contains(TEXT("Fallback:Unknown")));
    return true;
}

// ---------------------------------------------------------------------------
// v0.3: Frame-путь — извлечение через FSyntaxParser
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Frame_AbilityCheck_ReturnsAnswerAbility,
    "Neira.IntentExtractor.Frame_AbilityCheck_ReturnsAnswerAbility",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Frame_AbilityCheck_ReturnsAnswerAbility::RunTest(const FString& Parameters)
{
    // FSyntaxParser определяет bIsAbilityCheck=true для «ты можешь X»
    // ExtractFromFrame step 1 должен поймать это и вернуть AnswerAbility
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("ты можешь объяснять слова?"), EPhraseType::Question);
    TestEqual(TEXT("Frame.AbilityCheck → AnswerAbility"),
        Result.IntentID, EIntentID::AnswerAbility);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Frame_Question_EntityFromObject,
    "Neira.IntentExtractor.Frame_QuestionWithObject_EntityFromFrame",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Frame_Question_EntityFromObject::RunTest(const FString& Parameters)
{
    // «что такое кот?» — FSyntaxParser даёт Object="кот"
    // Frame path step 5: Question + Object → GetDefinition, EntityTarget из Frame.Object
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("что такое кот?"), EPhraseType::Question);
    TestEqual(TEXT("Intent → GetDefinition"), Result.IntentID, EIntentID::GetDefinition);
    TestEqual(TEXT("EntityTarget из Frame.Object = 'кот'"),
        Result.EntityTarget, FString(TEXT("кот")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Frame_FindPredicate_WithoutTerm_FailsByContract,
    "Neira.IntentExtractor.Frame_CommandFind_WithoutTerm_FailsByContract",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Frame_FindPredicate_WithoutTerm_FailsByContract::RunTest(const FString& Parameters)
{
    // Negative-case: термин не извлечён.
    // Допускаем Unknown/PartialParse по контракту, важно не принимать мета-слово как целевую сущность.
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("найди значение слова"), EPhraseType::Command);
    const bool bUnknownByContract =
        Result.IntentID == EIntentID::Unknown
        || Result.FailReason == EActionFailReason::PartialParse;
    TestTrue(TEXT("Команда без термина → Unknown или PartialParse (контракт fail-path)"),
        bUnknownByContract);
    TestTrue(TEXT("DecisionTrace показывает frame-path или fallback-path"),
        Result.DecisionTrace.Contains(TEXT("Frame."))
        || Result.DecisionTrace.Contains(TEXT("Fallback:"))
        || Result.DecisionTrace.Contains(TEXT("Pattern:")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Frame_FindMeaning_ExtractsSyntaxisTerm,
    "Neira.IntentExtractor.Frame_FindMeaning_ExtractsTermAfterMetaWord",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Frame_FindMeaning_ExtractsSyntaxisTerm::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("найди значение слова синтаксис"), EPhraseType::Command);
    TestEqual(TEXT("Intent → FindMeaning"), Result.IntentID, EIntentID::FindMeaning);
    TestEqual(TEXT("EntityTarget → 'синтаксис'"),
        Result.EntityTarget, FString(TEXT("синтаксис")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Frame_FindMeaning_ExtractsMorphologyTerm,
    "Neira.IntentExtractor.Frame_FindMeaning_ExtractsTermAfterTermMetaWord",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Frame_FindMeaning_ExtractsMorphologyTerm::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("найди определение термина морфология"), EPhraseType::Command);
    TestEqual(TEXT("Intent → FindMeaning"), Result.IntentID, EIntentID::FindMeaning);
    TestEqual(TEXT("EntityTarget → 'морфология'"),
        Result.EntityTarget, FString(TEXT("морфология")));
    return true;
}

// ---------------------------------------------------------------------------
// v0.3: Явный блок FindMeaning/GetDefinition + DecisionTrace
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Block_WhatMeansSyntax_GetDefinition,
    "Neira.IntentExtractor.Block.WhatMeansSyntax_ReturnsGetDefinition",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Block_WhatMeansSyntax_GetDefinition::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("что означает слово синтаксис?"), EPhraseType::Question);

    TestEqual(TEXT("Intent → GetDefinition"), Result.IntentID, EIntentID::GetDefinition);
    TestEqual(TEXT("EntityTarget → 'синтаксис'"),
        Result.EntityTarget, FString(TEXT("синтаксис")));
    TestTrue(TEXT("DecisionTrace содержит fallback-path"),
        Result.DecisionTrace.Contains(TEXT("Pattern:"))
        || Result.DecisionTrace.Contains(TEXT("Fallback:")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Block_FindMeaningSyntax_ReturnsFindMeaning,
    "Neira.IntentExtractor.Block.FindMeaningSyntax_ReturnsFindMeaning",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Block_FindMeaningSyntax_ReturnsFindMeaning::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("найди значение слова синтаксис"), EPhraseType::Command);

    TestEqual(TEXT("Intent → FindMeaning"), Result.IntentID, EIntentID::FindMeaning);
    TestEqual(TEXT("EntityTarget → 'синтаксис'"),
        Result.EntityTarget, FString(TEXT("синтаксис")));
    TestTrue(TEXT("DecisionTrace содержит frame-path"),
        Result.DecisionTrace.Contains(TEXT("Frame.")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Block_FindMeaningMemory_ReturnsFindMeaning,
    "Neira.IntentExtractor.Block.FindMeaningMemory_ReturnsFindMeaning",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Block_FindMeaningMemory_ReturnsFindMeaning::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("найди определение термина память"), EPhraseType::Command);

    TestEqual(TEXT("Intent → FindMeaning"), Result.IntentID, EIntentID::FindMeaning);
    TestEqual(TEXT("EntityTarget → 'память'"),
        Result.EntityTarget, FString(TEXT("память")));
    TestTrue(TEXT("DecisionTrace содержит frame-path"),
        Result.DecisionTrace.Contains(TEXT("Frame.")));
    return true;
}

// ---------------------------------------------------------------------------
// v0.3: DecisionTrace — объяснимость решений
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_DecisionTrace_NotEmpty_OnKnownIntent,
    "Neira.IntentExtractor.DecisionTrace_NotEmpty_OnKnownIntent",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_DecisionTrace_NotEmpty_OnKnownIntent::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("что такое кот?"), EPhraseType::Question);
    TestFalse(TEXT("DecisionTrace не пустой для известного Intent"),
        Result.DecisionTrace.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_DecisionTrace_ContainsFrame_WhenFrameResolved,
    "Neira.IntentExtractor.DecisionTrace_ContainsFrame_OnFrameResolved",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_DecisionTrace_ContainsFrame_WhenFrameResolved::RunTest(const FString& Parameters)
{
    // «что такое кот?» должно разрешаться через Frame-путь
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("что такое кот?"), EPhraseType::Question);
    TestTrue(TEXT("DecisionTrace содержит 'Frame.'"),
        Result.DecisionTrace.Contains(TEXT("Frame.")));
    return true;
}

// ---------------------------------------------------------------------------
// Negative тест: пустой ввод
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_Empty_ReturnsUnknown,
    "Neira.IntentExtractor.EmptyInput_ReturnsUnknown",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Empty_ReturnsUnknown::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT(""), EPhraseType::Unknown);
    TestEqual(TEXT("Пустой ввод → Intent::Unknown"),
        Result.IntentID, EIntentID::Unknown);
    TestTrue(TEXT("Пустой ввод → Confidence == 0"), Result.Confidence == 0.0f);
    TestEqual(TEXT("Пустой ввод → FailReason::EmptyInput"),
        Result.FailReason, EActionFailReason::EmptyInput);
    TestFalse(TEXT("Пустой ввод → DiagnosticNote не пустой"),
        Result.DiagnosticNote.IsEmpty());
    return true;
}

// ---------------------------------------------------------------------------
// Negative/edge: синтаксическая ошибка + частичный разбор
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_SyntaxError_PartialParse,
    "Neira.IntentExtractor.SyntaxError_PartialParse_ReturnsReasonAndDiagnostic",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_SyntaxError_PartialParse::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(TEXT("найди значение"), EPhraseType::Command);

    TestEqual(TEXT("Частично разобранная команда без термина → Intent::Unknown"),
        Result.IntentID, EIntentID::Unknown);
    TestEqual(TEXT("Частичный синтаксический разбор → FailReason::PartialParse"),
        Result.FailReason, EActionFailReason::PartialParse);
    TestFalse(TEXT("PartialParse → DiagnosticNote не пустой"),
        Result.DiagnosticNote.IsEmpty());
    TestTrue(TEXT("DecisionTrace сохраняет frame и fallback шаги"),
        Result.DecisionTrace.Contains(TEXT("Frame.PartialParse"))
        && Result.DecisionTrace.Contains(TEXT("Fallback:Unknown")));
    return true;
}

// ---------------------------------------------------------------------------
// Доменные пакеты regression-v0.4: action / diagnostics / memory
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntentExtractor_DomainPackages_TargetIntents,
    "Neira.IntentExtractor.DomainPackages.TargetIntents",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_DomainPackages_TargetIntents::RunTest(const FString& Parameters)
{
    FIntentExtractor Extractor;

    // action_commands: команда действия не должна ошибочно попадать в semantic-intent.
    {
        FIntentResult Result = Extractor.Extract(TEXT("проверь окно"), EPhraseType::Command);
        TestEqual(TEXT("action_commands → Unknown"), Result.IntentID, EIntentID::Unknown);
    }

    // text_diagnostics: сценарий диагностики текста.
    {
        FIntentResult Result = Extractor.Extract(
            TEXT("найди значение слова текста"), EPhraseType::Command);
        TestEqual(TEXT("text_diagnostics → FindMeaning"), Result.IntentID, EIntentID::FindMeaning);
        TestEqual(TEXT("text_diagnostics: EntityTarget='текста'"),
            Result.EntityTarget, FString(TEXT("текста")));
    }

    // memory_knowledge: извлечение термина после meta-word.
    {
        FIntentResult Result = Extractor.Extract(
            TEXT("найди определение термина память"), EPhraseType::Command);
        TestEqual(TEXT("memory_knowledge → FindMeaning"), Result.IntentID, EIntentID::FindMeaning);
        TestEqual(TEXT("memory_knowledge: EntityTarget='память'"),
            Result.EntityTarget, FString(TEXT("память")));
    }

    return true;
}

#undef NEIRA_TEST_FLAGS
