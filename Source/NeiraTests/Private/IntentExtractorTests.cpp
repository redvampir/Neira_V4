// IntentExtractorTests.cpp
// Тесты для FIntentExtractor (v0.1)
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

#undef NEIRA_TEST_FLAGS
