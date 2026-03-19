#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FIntentExtractor.h"
#include "FMorphAnalyzer.h"

#include "../Fixtures/VoiceFixtures.h"

#include <chrono>
#include <algorithm>
#include <cmath>

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
    float SafePercent(int32 Num, int32 Den)
    {
        if (Den <= 0)
        {
            return 0.0f;
        }
        return (100.0f * static_cast<float>(Num)) / static_cast<float>(Den);
    }

    bool IsWordCoveredByVoiceFixtures(const FString& Word, const TArray<FVoiceFixture>& Fixtures)
    {
        const FString LowerWord = Word.ToLower();
        for (const FVoiceFixture& Fx : Fixtures)
        {
            if (Fx.Phrase.ToLower().Contains(LowerWord))
            {
                return true;
            }
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceFixtures_CoverageByCategory,
    "Neira.VoiceFixtures.Coverage.ByCategory",
    NEIRA_TEST_FLAGS)
bool FVoiceFixtures_CoverageByCategory::RunTest(const FString& Parameters)
{
    const TArray<FVoiceFixture>& Fixtures = GetVoiceFixtures();
    TestTrue(TEXT("Voice fixture-set не должен быть пустым"), Fixtures.Num() > 0);

    int32 ShortCommands = 0;
    int32 Questions = 0;
    int32 Conversational = 0;
    int32 AsrErrors = 0;

    for (const FVoiceFixture& Fx : Fixtures)
    {
        if (Fx.Category == TEXT("short_commands"))
        {
            ++ShortCommands;
        }
        else if (Fx.Category == TEXT("questions"))
        {
            ++Questions;
        }
        else if (Fx.Category == TEXT("conversational_variants"))
        {
            ++Conversational;
        }
        else if (Fx.Category == TEXT("typical_asr_errors"))
        {
            ++AsrErrors;
        }
    }

    TestTrue(TEXT("Есть короткие команды"), ShortCommands > 0);
    TestTrue(TEXT("Есть вопросы"), Questions > 0);
    TestTrue(TEXT("Есть разговорные варианты"), Conversational > 0);
    TestTrue(TEXT("Есть типовые ошибки распознавания"), AsrErrors > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceFixtures_MetricsGate,
    "Neira.VoiceFixtures.Metrics.Gate",
    NEIRA_TEST_FLAGS)
bool FVoiceFixtures_MetricsGate::RunTest(const FString& Parameters)
{
    const TArray<FVoiceFixture>& Fixtures = GetVoiceFixtures();
    TestTrue(TEXT("Voice fixture-set не должен быть пустым"), Fixtures.Num() > 0);
    if (Fixtures.IsEmpty())
    {
        return false;
    }

    FIntentExtractor Extractor;

    int32 IntentMatches = 0;
    int32 CommandTotal = 0;
    int32 CommandSuccess = 0;
    int32 FallbackExpected = 0;
    int32 FallbackObserved = 0;

    TArray<double> LatencyMs;
    LatencyMs.Reserve(Fixtures.Num());

    for (const FVoiceFixture& Fx : Fixtures)
    {
        const auto Start = std::chrono::steady_clock::now();
        const FIntentResult Intent = Extractor.Extract(Fx.Phrase, Fx.PhraseType);
        const auto End = std::chrono::steady_clock::now();

        const double ElapsedMs =
            std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(End - Start).count();
        LatencyMs.Add(ElapsedMs);

        const bool bIntentMatch = (Intent.IntentID == Fx.ExpectedIntent);
        if (bIntentMatch)
        {
            ++IntentMatches;
        }

        if (Fx.bIsCommandCase)
        {
            ++CommandTotal;
            if (bIntentMatch)
            {
                ++CommandSuccess;
            }
        }

        if (Fx.bRequiresFallback)
        {
            ++FallbackExpected;
            if (Intent.IntentID == EIntentID::Unknown)
            {
                ++FallbackObserved;
            }
        }
    }

    std::sort(LatencyMs.begin(), LatencyMs.end());
    const float IntentAccuracyPct = SafePercent(IntentMatches, Fixtures.Num());
    const float CommandSuccessRatePct = SafePercent(CommandSuccess, CommandTotal);
    const float FallbackRatePct = SafePercent(FallbackObserved, FallbackExpected);

    double LatencySum = 0.0;
    for (const double SampleMs : LatencyMs)
    {
        LatencySum += SampleMs;
    }
    const double AvgLatencyMs = LatencyMs.IsEmpty() ? 0.0 : (LatencySum / LatencyMs.Num());

    int32 P95Index = static_cast<int32>(std::ceil(static_cast<double>(LatencyMs.Num()) * 0.95)) - 1;
    if (P95Index < 0)
    {
        P95Index = 0;
    }
    if (!LatencyMs.IsEmpty() && P95Index >= LatencyMs.Num())
    {
        P95Index = LatencyMs.Num() - 1;
    }
    const double P95LatencyMs = LatencyMs.IsEmpty() ? 0.0 : LatencyMs[P95Index];

    printf("[voice-metrics] command_success_rate=%.2f intent_accuracy=%.2f fallback_rate=%.2f avg_latency_ms=%.3f p95_latency_ms=%.3f\n",
           CommandSuccessRatePct,
           IntentAccuracyPct,
           FallbackRatePct,
           AvgLatencyMs,
           P95LatencyMs);

    TestTrue(TEXT("Command success rate >= 90%"), CommandSuccessRatePct >= 90.0f);
    TestTrue(TEXT("Intent accuracy >= 90%"), IntentAccuracyPct >= 90.0f);
    TestTrue(TEXT("Fallback rate >= 95%"), FallbackRatePct >= 95.0f);
    TestTrue(TEXT("Average latency <= 5ms"), AvgLatencyMs <= 5.0);
    TestTrue(TEXT("P95 latency <= 10ms"), P95LatencyMs <= 10.0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVoiceLexiconPolicy_NoDictionaryWordWithoutFixture,
    "Neira.VoiceFixtures.Policy.NoDictionaryWordWithoutFixture",
    NEIRA_TEST_FLAGS)
bool FVoiceLexiconPolicy_NoDictionaryWordWithoutFixture::RunTest(const FString& Parameters)
{
    FMorphAnalyzer Morph;
    const TArray<FString>& GovernedWords = GetVoiceLexiconGovernedWords();
    const TArray<FVoiceFixture>& Fixtures = GetVoiceFixtures();

    for (const FString& Word : GovernedWords)
    {
        const TArray<FMorphResult> Tokens = Morph.AnalyzePhrase(Word);
        const bool bWordKnown = !Tokens.IsEmpty() && Tokens[0].PartOfSpeech != EPosTag::Unknown;
        TestTrue(*FString::Printf(TEXT("Слово '%s' обязано быть в словаре"), *Word), bWordKnown);

        const bool bCoveredByFixture = IsWordCoveredByVoiceFixtures(Word, Fixtures);
        TestTrue(*FString::Printf(TEXT("Слово '%s' обязано иметь voice fixture-кейс"), *Word), bCoveredByFixture);
    }

    return true;
}

#undef NEIRA_TEST_FLAGS
