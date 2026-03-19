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
    FIntentExtractor_Frame_FindPredicate_ReturnsFindMeaning,
    "Neira.IntentExtractor.Frame_CommandFind_ReturnsFindMeaning",
    NEIRA_TEST_FLAGS)
bool FIntentExtractor_Frame_FindPredicate_ReturnsFindMeaning::RunTest(const FString& Parameters)
{
    // «найди значение слова» — Predicate="найти", Object="значение"
    // Frame step 3: IsFindPredicate + IsDefinitionObject → FindMeaning
    FIntentExtractor Extractor;
    FIntentResult Result = Extractor.Extract(
        TEXT("найди значение слова"), EPhraseType::Command);
    TestEqual(TEXT("Frame.найти+значение → FindMeaning"),
        Result.IntentID, EIntentID::FindMeaning);
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
    return true;
}

#undef NEIRA_TEST_FLAGS
