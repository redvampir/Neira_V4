// SyntaxParserTests.cpp
// Тесты для FSyntaxParser (v0.3)
//
// Запуск: Unreal Automation Tool → фильтр "Neira.SyntaxParser"

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FSyntaxParser.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
    const FSemanticFrame::FAmbiguousDecisionTrace* FindTraceByToken(
        const FSemanticFrame& Frame,
        const FString& TokenLower)
    {
        for (const FSemanticFrame::FAmbiguousDecisionTrace& Trace : Frame.AmbiguityTrace)
        {
            if (Trace.Token.ToLower() == TokenLower)
            {
                return &Trace;
            }
        }
        return nullptr;
    }
}

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

// ===========================================================================
// Ambiguity trace — объяснимое разрешение конфликтов POS
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_AmbiguousToken_HasTwoCandidates,
    "Neira.SyntaxParser.AmbiguityTrace_HasTwoCandidates",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_AmbiguousToken_HasTwoCandidates::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("расскажи что такое морфология"), EPhraseType::Request);

    const FSemanticFrame::FAmbiguousDecisionTrace* Trace = FindTraceByToken(F, TEXT("что"));
    TestNotNull(TEXT("Trace для 'что' присутствует"), Trace);
    if (Trace == nullptr)
        return false;

    TestTrue(TEXT("У trace минимум 2 кандидата"), Trace->CandidatePOS.Num() >= 2);
    TestEqual(TEXT("Выбран POS для 'что' — Conjunction"), Trace->SelectedPOS, FString(TEXT("Conjunction")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_AmbiguousToken_ReasonAndConfidenceFilled,
    "Neira.SyntaxParser.AmbiguityTrace_ReasonAndConfidenceFilled",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_AmbiguousToken_ReasonAndConfidenceFilled::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("расскажи что такое морфология"), EPhraseType::Request);

    const FSemanticFrame::FAmbiguousDecisionTrace* Trace = FindTraceByToken(F, TEXT("что"));
    TestNotNull(TEXT("Trace для 'что' присутствует"), Trace);
    if (Trace == nullptr)
        return false;

    TestFalse(TEXT("Reason заполнен"), Trace->Reason.IsEmpty());
    TestFalse(TEXT("Anchor заполнен"), Trace->Anchor.IsEmpty());
    TestTrue(TEXT("Confidence > 0"), Trace->Confidence > 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_AmbiguousToken_DeterministicAcrossRuns,
    "Neira.SyntaxParser.AmbiguityTrace_DeterministicAcrossRuns",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_AmbiguousToken_DeterministicAcrossRuns::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FString FirstSelected;
    FString FirstReason;

    for (int32 Run = 0; Run < 5; ++Run)
    {
        FSemanticFrame F = Parser.Parse(TEXT("расскажи что такое морфология"), EPhraseType::Request);
        const FSemanticFrame::FAmbiguousDecisionTrace* Trace = FindTraceByToken(F, TEXT("что"));
        TestNotNull(TEXT("Trace для 'что' присутствует на каждом запуске"), Trace);
        if (Trace == nullptr)
            return false;

        if (Run == 0)
        {
            FirstSelected = Trace->SelectedPOS;
            FirstReason = Trace->Reason;
        }
        else
        {
            TestEqual(TEXT("Выбранный POS стабилен"), Trace->SelectedPOS, FirstSelected);
            TestEqual(TEXT("Причина выбора стабильна"), Trace->Reason, FirstReason);
        }
    }

    return true;
}

// ===========================================================================
// Доменные пакеты regression-v0.4: action / diagnostics / memory
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_DomainPackages_Frames,
    "Neira.SyntaxParser.DomainPackages.Frames",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_DomainPackages_Frames::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;

    {
        FSemanticFrame F = Parser.Parse(TEXT("проверь окно"), EPhraseType::Command);
        TestEqual(TEXT("action_commands: Predicate=проверить"), F.Predicate, FString(TEXT("проверить")));
        TestEqual(TEXT("action_commands: Object=окно"), F.Object, FString(TEXT("окно")));
    }

    {
        FSemanticFrame F = Parser.Parse(TEXT("расскажи что такое морфология"), EPhraseType::Request);
        TestEqual(TEXT("text_diagnostics: Predicate=рассказать"), F.Predicate, FString(TEXT("рассказать")));
        TestEqual(TEXT("text_diagnostics: Object=морфология"), F.Object, FString(TEXT("морфология")));
        TestTrue(TEXT("text_diagnostics: nested clause detected"), F.bHasNestedClause);
    }

    {
        FSemanticFrame F = Parser.Parse(TEXT("что означает слово память"), EPhraseType::Question);
        TestEqual(TEXT("memory_knowledge: Predicate=означать"), F.Predicate, FString(TEXT("означать")));
        TestEqual(TEXT("memory_knowledge: Object=слово"), F.Object, FString(TEXT("слово")));
        TestFalse(TEXT("memory_knowledge: no negation"), F.bIsNegated);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSyntaxParser_DomainPackages_AmbiguousBoundaryTrace,
    "Neira.SyntaxParser.DomainPackages.AmbiguousBoundaryTrace",
    NEIRA_TEST_FLAGS)
bool FSyntaxParser_DomainPackages_AmbiguousBoundaryTrace::RunTest(const FString& Parameters)
{
    FSyntaxParser Parser;
    FSemanticFrame F = Parser.Parse(TEXT("как открыть окно"), EPhraseType::Question);

    const FSemanticFrame::FAmbiguousDecisionTrace* Trace = FindTraceByToken(F, TEXT("как"));
    TestNotNull(TEXT("Ambiguity trace для 'как' присутствует"), Trace);
    if (Trace == nullptr)
        return false;

    TestEqual(TEXT("Для 'как' выбран POS Conjunction"), Trace->SelectedPOS, FString(TEXT("Conjunction")));
    TestTrue(TEXT("Confidence > 0"), Trace->Confidence > 0.0f);
    return true;
}

#undef NEIRA_TEST_FLAGS
