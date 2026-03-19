// SyntaxParserTests.cpp
// Тесты для FSyntaxParser (v0.3)
//
// Запуск: Unreal Automation Tool → фильтр "Neira.SyntaxParser"

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FSyntaxParser.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ===========================================================================
// Predicate — глагол извлекается корректно
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_Command_PredicateExtracted,
    "Neira.SyntaxParser.Command_PredicateExtracted",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_Command_PredicateExtracted::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("открой окно"), EPhraseType::Command);
    TestFalse(TEXT("Predicate не пустой"), F.Predicate.IsEmpty());
    TestEqual(TEXT("Predicate → открыть"), F.Predicate, FString(TEXT("открыть")));
    return true;
}

// ===========================================================================
// Object — объект действия
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_Command_ObjectExtracted,
    "Neira.SyntaxParser.Command_ObjectExtracted",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_Command_ObjectExtracted::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("открой окно"), EPhraseType::Command);
    TestEqual(TEXT("Object → окно"), F.Object, FString(TEXT("окно")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_Question_ObjectExtracted,
    "Neira.SyntaxParser.Question_ObjectExtracted",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_Question_ObjectExtracted::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    // «кот — это животное» — Statement, предикат «это»/«является»
    FSemanticFrame F = Parser.Parse(TEXT("найди определение слова"), EPhraseType::Command);
    TestEqual(TEXT("Object → определение"), F.Object, FString(TEXT("определение")));
    return true;
}

// ===========================================================================
// Subject — субъект до глагола
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_Statement_SubjectExtracted,
    "Neira.SyntaxParser.Statement_SubjectExtracted",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_Statement_SubjectExtracted::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    // «Москва является столицей» — субъект перед глаголом
    FSemanticFrame F = Parser.Parse(TEXT("Москва является столицей"), EPhraseType::Statement);
    TestEqual(TEXT("Subject → москва"), F.Subject, FString(TEXT("москва")));
    TestFalse(TEXT("Predicate не пустой"), F.Predicate.IsEmpty());
    return true;
}

// ===========================================================================
// Recipient — адресат через предлог «для»
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_Recipient_ExtractedWithДля,
    "Neira.SyntaxParser.Recipient_Для_Extracted",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_Recipient_ExtractedWithДля::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("найди определение для меня"), EPhraseType::Command);
    TestEqual(TEXT("Recipient → я"), F.Recipient, FString(TEXT("я")));
    return true;
}

// ===========================================================================
// bIsAbilityCheck — «ты можешь»
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_AbilityCheck_Detected,
    "Neira.SyntaxParser.AbilityCheck_TyMozhesh_Detected",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_AbilityCheck_Detected::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("ты можешь объяснять слова?"), EPhraseType::Question);
    TestTrue(TEXT("bIsAbilityCheck = true"), F.bIsAbilityCheck);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_RegularCommand_NotAbilityCheck,
    "Neira.SyntaxParser.RegularCommand_NotAbilityCheck",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_RegularCommand_NotAbilityCheck::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("открой окно"), EPhraseType::Command);
    TestFalse(TEXT("bIsAbilityCheck = false для команды"), F.bIsAbilityCheck);
    return true;
}

// ===========================================================================
// bHasNestedClause — вложенная клауза
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_NestedClause_Что_Detected,
    "Neira.SyntaxParser.NestedClause_Что_Detected",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_NestedClause_Что_Detected::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(
        TEXT("расскажи что такое морфология"), EPhraseType::Request);
    TestTrue(TEXT("bHasNestedClause = true при 'что'"), F.bHasNestedClause);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_NoNested_SimpleCommand,
    "Neira.SyntaxParser.NoNestedClause_SimpleCommand",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_NoNested_SimpleCommand::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("открой окно"), EPhraseType::Command);
    TestFalse(TEXT("bHasNestedClause = false для простой команды"), F.bHasNestedClause);
    return true;
}

// ===========================================================================
// bIsNegated — отрицание
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_Negation_Не_Detected,
    "Neira.SyntaxParser.Negation_Не_Detected",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_Negation_Не_Detected::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("не открой окно"), EPhraseType::Command);
    TestTrue(TEXT("bIsNegated = true при 'не'"), F.bIsNegated);
    return true;
}

// ===========================================================================
// Пустой ввод
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_EmptyPhrase_EmptyFrame,
    "Neira.SyntaxParser.Empty_ReturnsEmptyFrame",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_EmptyPhrase_EmptyFrame::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT(""), EPhraseType::Unknown);
    TestTrue(TEXT("Пустая фраза → IsEmpty()"), F.IsEmpty());
    return true;
}

#undef NEIRA_TEST_FLAGS
