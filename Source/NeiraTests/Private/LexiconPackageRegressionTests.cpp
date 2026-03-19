#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FMorphAnalyzer.h"
#include "FIntentExtractor.h"
#include "FSyntaxParser.h"

#include "../Fixtures/LexiconPackages.h"
#include "../Fixtures/RegressionIntentFrameFixtures.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
    bool IsPackageCategory(const FString& Category)
    {
        return Category == TEXT("commands_actions")
            || Category == TEXT("household_dialog")
            || Category == TEXT("asr_artifacts");
    }

    bool FrameMatchesBaseline(const FSemanticFrame& Frame, const FRegressionFixture& Fx)
    {
        return Frame.Subject == Fx.BaselineSubject
            && Frame.Predicate == Fx.BaselinePredicate
            && Frame.Object == Fx.BaselineObject
            && Frame.bIsAbilityCheck == Fx.BaselineAbilityCheck
            && Frame.bHasNestedClause == Fx.BaselineNestedClause
            && Frame.bIsNegated == Fx.BaselineNegation;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLexiconPackages_RegressionFixtureCount,
    "Neira.LexiconPackages.RegressionFixtureCount.Min20PerPackage",
    NEIRA_TEST_FLAGS)
bool FLexiconPackages_RegressionFixtureCount::RunTest(const FString& Parameters)
{
    int32 CommandsCount = 0;
    int32 DialogCount = 0;
    int32 AsrCount = 0;
    for (const FRegressionFixture& Fx : GetRUENRegressionFixtures())
    {
        if (Fx.Category == TEXT("commands_actions"))
        {
            ++CommandsCount;
        }
        else if (Fx.Category == TEXT("household_dialog"))
        {
            ++DialogCount;
        }
        else if (Fx.Category == TEXT("asr_artifacts"))
        {
            ++AsrCount;
        }
    }

    TestTrue(TEXT("commands_actions >= 20"), CommandsCount >= 20);
    TestTrue(TEXT("household_dialog >= 20"), DialogCount >= 20);
    TestTrue(TEXT("asr_artifacts >= 20"), AsrCount >= 20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLexiconPackages_RegressionCoverage_IntentSyntaxMorph,
    "Neira.LexiconPackages.RegressionCoverage.IntentSyntaxMorph",
    NEIRA_TEST_FLAGS)
bool FLexiconPackages_RegressionCoverage_IntentSyntaxMorph::RunTest(const FString& Parameters)
{
    FMorphAnalyzer Morph;
    FSyntaxParser Parser;
    FIntentExtractor Extractor;

    for (const FRegressionFixture& Fx : GetRUENRegressionFixtures())
    {
        if (!IsPackageCategory(Fx.Category))
        {
            continue;
        }

        const TArray<FMorphResult> Tokens = Morph.AnalyzePhrase(Fx.Phrase);
        bool bHasKnownToken = false;
        for (const FMorphResult& Token : Tokens)
        {
            if (Token.PartOfSpeech != EPosTag::Unknown)
            {
                bHasKnownToken = true;
                break;
            }
        }
        TestTrue(*FString::Printf(TEXT("[%s] Morph должен распознавать хотя бы 1 токен"), *Fx.CaseID), bHasKnownToken);

        const FSemanticFrame Frame = Parser.Parse(Fx.Phrase, Fx.PhraseType);
        const FIntentResult Intent = Extractor.Extract(Fx.Phrase, Fx.PhraseType);

        TestTrue(*FString::Printf(TEXT("[%s] Frame baseline match"), *Fx.CaseID),
                 FrameMatchesBaseline(Frame, Fx));
        TestEqual(*FString::Printf(TEXT("[%s] Intent baseline match"), *Fx.CaseID),
                  Intent.IntentID, Fx.BaselineIntent);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLexiconPackages_NoWordWithoutScenario,
    "Neira.LexiconPackages.Policy.NoWordWithoutScenario",
    NEIRA_TEST_FLAGS)
bool FLexiconPackages_NoWordWithoutScenario::RunTest(const FString& Parameters)
{
    const TArray<FRegressionFixture>& Fixtures = GetRUENRegressionFixtures();
    const TArray<FLexiconScenarioPackage>& Packages = GetLexiconScenarioPackages();

    for (const FLexiconScenarioPackage& Package : Packages)
    {
        for (const FString& Word : Package.RequiredWords)
        {
            bool bFoundScenario = false;
            for (const FRegressionFixture& Fx : Fixtures)
            {
                if (Fx.Category != Package.Name)
                {
                    continue;
                }

                const FString LowerPhrase = Fx.Phrase.ToLower();
                if (LowerPhrase.Contains(Word.ToLower()))
                {
                    bFoundScenario = true;
                    break;
                }
            }

            TestTrue(
                *FString::Printf(TEXT("[%s] Слово '%s' должно иметь хотя бы 1 проверяемый fixture"),
                                 *Package.Name, *Word),
                bFoundScenario);
        }
    }

    return true;
}

#undef NEIRA_TEST_FLAGS
