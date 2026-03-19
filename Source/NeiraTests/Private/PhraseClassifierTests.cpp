// PhraseClassifierTests.cpp
// Тесты для FPhraseClassifier (v0.1)
//
// Запуск: Unreal Automation Tool → фильтр "Neira.PhraseClassifier"
// Все тесты должны ПАДАТЬ до реализации FPhraseClassifier::Classify().

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FPhraseClassifier.h"

// Флаги общие для всех тестов модуля
#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ---------------------------------------------------------------------------
// Вопросы
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_WhatIsQuestion,
    "Neira.PhraseClassifier.WhatIs_ReturnsQuestion",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_WhatIsQuestion::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'что такое кот?' → Question"),
        Classifier.Classify(TEXT("что такое кот?")),
        EPhraseType::Question);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_WhoIsQuestion,
    "Neira.PhraseClassifier.WhoIs_ReturnsQuestion",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_WhoIsQuestion::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'кто такой Пушкин?' → Question"),
        Classifier.Classify(TEXT("кто такой Пушкин?")),
        EPhraseType::Question);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_WhatMeansQuestion,
    "Neira.PhraseClassifier.WhatMeans_ReturnsQuestion",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_WhatMeansQuestion::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'что означает слово синтаксис?' → Question"),
        Classifier.Classify(TEXT("что означает слово синтаксис?")),
        EPhraseType::Question);
    return true;
}

// ---------------------------------------------------------------------------
// Команды
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_OpenCommand,
    "Neira.PhraseClassifier.Open_ReturnsCommand",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_OpenCommand::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'открой окно' → Command"),
        Classifier.Classify(TEXT("открой окно")),
        EPhraseType::Command);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_FindCommand,
    "Neira.PhraseClassifier.Find_ReturnsCommand",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_FindCommand::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'найди определение слова' → Command"),
        Classifier.Classify(TEXT("найди определение слова")),
        EPhraseType::Command);
    return true;
}

// ---------------------------------------------------------------------------
// Утверждения
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_SimpleStatement,
    "Neira.PhraseClassifier.SimpleStatement_ReturnsStatement",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_SimpleStatement::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'кот — это животное' → Statement"),
        Classifier.Classify(TEXT("кот — это животное")),
        EPhraseType::Statement);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_IsStatement,
    "Neira.PhraseClassifier.IsStatement_ReturnsStatement",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_IsStatement::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'Москва является столицей России' → Statement"),
        Classifier.Classify(TEXT("Москва является столицей России")),
        EPhraseType::Statement);
    return true;
}

// ---------------------------------------------------------------------------
// Просьбы
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_TellMeRequest,
    "Neira.PhraseClassifier.TellMe_ReturnsRequest",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_TellMeRequest::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'скажи мне что такое морфология' → Request"),
        Classifier.Classify(TEXT("скажи мне что такое морфология")),
        EPhraseType::Request);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_ExplainRequest,
    "Neira.PhraseClassifier.Explain_ReturnsRequest",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_ExplainRequest::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("'объясни мне синтаксис' → Request"),
        Classifier.Classify(TEXT("объясни мне синтаксис")),
        EPhraseType::Request);
    return true;
}

// ---------------------------------------------------------------------------
// Граничные случаи
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_EmptyInput,
    "Neira.PhraseClassifier.EmptyInput_ReturnsUnknown",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_EmptyInput::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("пустая строка → Unknown"),
        Classifier.Classify(TEXT("")),
        EPhraseType::Unknown);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPhraseClassifier_WhitespaceOnly,
    "Neira.PhraseClassifier.WhitespaceOnly_ReturnsUnknown",
    NEIRA_TEST_FLAGS)
bool FPhraseClassifier_WhitespaceOnly::RunTest(const FString& Parameters)
{
    FPhraseClassifier Classifier;
    TestEqual(TEXT("строка из пробелов → Unknown"),
        Classifier.Classify(TEXT("   ")),
        EPhraseType::Unknown);
    return true;
}

#undef NEIRA_TEST_FLAGS
