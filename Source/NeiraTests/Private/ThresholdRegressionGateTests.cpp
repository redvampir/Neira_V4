#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FPhraseClassifier.h"
#include "FIntentExtractor.h"
#include "FSyntaxParser.h"

#include "../Fixtures/RegressionIntentFrameFixtures.h"

#include <fstream>
#include <sstream>
#include <cmath>

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
    struct FThresholdGateConfig
    {
        int32 ConfigVersion = 0;
        float BaselineIntentMatchPct = 100.0f;
        float BaselineFrameMatchPct = 100.0f;
        float AllowedIntentDriftPct = 0.0f;
        float AllowedFrameDriftPct = 0.0f;
        float MinIntentMatchPct = 100.0f;
        float MinFrameMatchPct = 100.0f;
        float MinAbilityIntentMatchPct = 100.0f;
        float MinFallbackUnknownMatchPct = 100.0f;
        bool bThresholdChangeRequiresRegressionPass = false;
    };

    static constexpr uint32 GThresholdConfigLock = 1118650015u;

    FString TrimAscii(const FString& Value)
    {
        return Value.TrimStartAndEnd();
    }

    bool ParseBool(const FString& Value)
    {
        return Value.ToLower() == TEXT("true") || Value == TEXT("1");
    }

    bool ParseFloat(const FString& Value, float& Out)
    {
        try
        {
            Out = std::stof(Value.Data);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool ParseInt(const FString& Value, int32& Out)
    {
        try
        {
            Out = std::stoi(Value.Data);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool ReadConfigText(FString& OutText)
    {
        const char* CandidatePaths[] = {
            "../NeiraTests/Fixtures/RegressionThresholds.cfg",
            "Source/NeiraTests/Fixtures/RegressionThresholds.cfg"
        };

        for (const char* Path : CandidatePaths)
        {
            std::ifstream In(Path);
            if (!In.good())
            {
                continue;
            }

            std::ostringstream Buffer;
            Buffer << In.rdbuf();
            OutText = FString(Buffer.str());
            return true;
        }

        return false;
    }

    bool ParseThresholdConfig(const FString& Text, FThresholdGateConfig& OutConfig)
    {
        std::istringstream Stream(Text.Data);
        std::string Line;

        while (std::getline(Stream, Line))
        {
            FString Raw(Line);
            FString Trimmed = TrimAscii(Raw);
            if (Trimmed.IsEmpty() || Trimmed.StartsWith(TEXT("#")))
            {
                continue;
            }

            const int32 EqPos = Trimmed.Find(TEXT("="));
            if (EqPos == INDEX_NONE)
            {
                continue;
            }

            const FString Key = TrimAscii(Trimmed.Left(EqPos));
            const FString Value = TrimAscii(Trimmed.Mid(EqPos + 1));

            if (Key == TEXT("config_version"))
            {
                ParseInt(Value, OutConfig.ConfigVersion);
            }
            else if (Key == TEXT("baseline_intent_match_pct"))
            {
                ParseFloat(Value, OutConfig.BaselineIntentMatchPct);
            }
            else if (Key == TEXT("baseline_frame_match_pct"))
            {
                ParseFloat(Value, OutConfig.BaselineFrameMatchPct);
            }
            else if (Key == TEXT("allowed_intent_drift_pct"))
            {
                ParseFloat(Value, OutConfig.AllowedIntentDriftPct);
            }
            else if (Key == TEXT("allowed_frame_drift_pct"))
            {
                ParseFloat(Value, OutConfig.AllowedFrameDriftPct);
            }
            else if (Key == TEXT("min_intent_match_pct"))
            {
                ParseFloat(Value, OutConfig.MinIntentMatchPct);
            }
            else if (Key == TEXT("min_frame_match_pct"))
            {
                ParseFloat(Value, OutConfig.MinFrameMatchPct);
            }
            else if (Key == TEXT("min_ability_check_intent_match_pct"))
            {
                ParseFloat(Value, OutConfig.MinAbilityIntentMatchPct);
            }
            else if (Key == TEXT("min_fallback_unknown_match_pct"))
            {
                ParseFloat(Value, OutConfig.MinFallbackUnknownMatchPct);
            }
            else if (Key == TEXT("policy_threshold_change_requires_regression_pass"))
            {
                OutConfig.bThresholdChangeRequiresRegressionPass = ParseBool(Value);
            }
        }

        return OutConfig.ConfigVersion > 0;
    }

    uint32 ComputeConfigLock(const FThresholdGateConfig& C)
    {
        auto Round10 = [](float V) -> uint32
        {
            const int32 R = static_cast<int32>(std::lround(V * 10.0f));
            return static_cast<uint32>(R);
        };

        uint32 H = 2166136261u;
        auto Mix = [&H](uint32 V)
        {
            H ^= V;
            H *= 16777619u;
        };

        Mix(static_cast<uint32>(C.ConfigVersion));
        Mix(Round10(C.BaselineIntentMatchPct));
        Mix(Round10(C.BaselineFrameMatchPct));
        Mix(Round10(C.AllowedIntentDriftPct));
        Mix(Round10(C.AllowedFrameDriftPct));
        Mix(Round10(C.MinIntentMatchPct));
        Mix(Round10(C.MinFrameMatchPct));
        Mix(Round10(C.MinAbilityIntentMatchPct));
        Mix(Round10(C.MinFallbackUnknownMatchPct));
        Mix(C.bThresholdChangeRequiresRegressionPass ? 1u : 0u);
        return H;
    }

    bool FrameMatchesBaseline(const FSemanticFrame& Frame, const FRegressionFixture& Fixture)
    {
        return Frame.Subject == Fixture.BaselineSubject
            && Frame.Predicate == Fixture.BaselinePredicate
            && Frame.Object == Fixture.BaselineObject
            && Frame.bIsAbilityCheck == Fixture.BaselineAbilityCheck
            && Frame.bHasNestedClause == Fixture.BaselineNestedClause
            && Frame.bIsNegated == Fixture.BaselineNegation;
    }

    float SafePercent(int32 Num, int32 Den)
    {
        if (Den <= 0)
        {
            return 0.0f;
        }
        return (100.0f * static_cast<float>(Num)) / static_cast<float>(Den);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FThresholdRegressionGate_ConfigIsLoadable,
    "Neira.RegressionGate.Config.SourceOfTruthIsLoadable",
    NEIRA_TEST_FLAGS)
bool FThresholdRegressionGate_ConfigIsLoadable::RunTest(const FString& Parameters)
{
    FString ConfigText;
    TestTrue(TEXT("RegressionThresholds.cfg должен быть доступен"), ReadConfigText(ConfigText));
    if (ConfigText.IsEmpty())
    {
        return false;
    }

    FThresholdGateConfig Config;
    TestTrue(TEXT("Конфиг threshold gate должен парситься"), ParseThresholdConfig(ConfigText, Config));
    TestTrue(TEXT("Policy: threshold change requires regression pass"),
             Config.bThresholdChangeRequiresRegressionPass);

    const uint32 ComputedLock = ComputeConfigLock(Config);
    TestEqual(TEXT("Lock подпись threshold-конфига не должна изменяться без явного обновления gate"),
              static_cast<int32>(ComputedLock), static_cast<int32>(GThresholdConfigLock));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FThresholdRegressionGate_BaselineVsCurrent,
    "Neira.RegressionGate.BaselineVsCurrent.IntentAndFrameMetrics",
    NEIRA_TEST_FLAGS)
bool FThresholdRegressionGate_BaselineVsCurrent::RunTest(const FString& Parameters)
{
    FString ConfigText;
    TestTrue(TEXT("RegressionThresholds.cfg должен быть доступен"), ReadConfigText(ConfigText));
    if (ConfigText.IsEmpty())
    {
        return false;
    }

    FThresholdGateConfig Config;
    TestTrue(TEXT("Конфиг threshold gate должен парситься"), ParseThresholdConfig(ConfigText, Config));

    const TArray<FRegressionFixture>& Fixtures = GetRUENRegressionFixtures();
    TestTrue(TEXT("Regression fixture-set не должен быть пустым"), Fixtures.Num() > 0);
    if (Fixtures.IsEmpty())
    {
        return false;
    }

    FIntentExtractor Extractor;
    FSyntaxParser Parser;

    int32 IntentMatches = 0;
    int32 FrameMatches = 0;
    bool bNoIntentDriftDetails = true;
    bool bNoFrameDriftDetails = true;

    int32 AbilityTotal = 0;
    int32 AbilityIntentMatches = 0;

    int32 FallbackTotal = 0;
    int32 FallbackUnknownMatches = 0;

    for (const FRegressionFixture& Fx : Fixtures)
    {
        const FIntentResult Intent = Extractor.Extract(Fx.Phrase, Fx.PhraseType);
        const FSemanticFrame Frame = Parser.Parse(Fx.Phrase, Fx.PhraseType);

        const bool bIntentMatch = Intent.IntentID == Fx.BaselineIntent;
        if (bIntentMatch)
        {
            ++IntentMatches;
        }
        else
        {
            bNoIntentDriftDetails = false;
            printf("    Intent drift case: %s\n", *Fx.CaseID);
        }

        const bool bFrameMatch = FrameMatchesBaseline(Frame, Fx);
        if (bFrameMatch)
        {
            ++FrameMatches;
        }
        else
        {
            bNoFrameDriftDetails = false;
            printf("    Frame drift case: %s\n", *Fx.CaseID);
        }

        if (Fx.Category == TEXT("ability_check"))
        {
            ++AbilityTotal;
            if (bIntentMatch)
            {
                ++AbilityIntentMatches;
            }
        }

        if (Fx.Category == TEXT("fallback"))
        {
            ++FallbackTotal;
            if (Intent.IntentID == EIntentID::Unknown)
            {
                ++FallbackUnknownMatches;
            }
        }
    }

    const float IntentMatchPct = SafePercent(IntentMatches, Fixtures.Num());
    const float FrameMatchPct = SafePercent(FrameMatches, Fixtures.Num());
    const float AbilityIntentMatchPct = SafePercent(AbilityIntentMatches, AbilityTotal);
    const float FallbackUnknownMatchPct = SafePercent(FallbackUnknownMatches, FallbackTotal);

    const float IntentDriftPct = Config.BaselineIntentMatchPct - IntentMatchPct;
    const float FrameDriftPct = Config.BaselineFrameMatchPct - FrameMatchPct;

    TestTrue(TEXT("Intent match >= min_intent_match_pct"), IntentMatchPct >= Config.MinIntentMatchPct);
    TestTrue(TEXT("Frame match >= min_frame_match_pct"), FrameMatchPct >= Config.MinFrameMatchPct);

    TestTrue(TEXT("Intent drift within allowed_intent_drift_pct"),
             IntentDriftPct <= Config.AllowedIntentDriftPct);
    TestTrue(TEXT("Frame drift within allowed_frame_drift_pct"),
             FrameDriftPct <= Config.AllowedFrameDriftPct);

    TestTrue(TEXT("Ability-check guard metric"),
             AbilityIntentMatchPct >= Config.MinAbilityIntentMatchPct);
    TestTrue(TEXT("Fallback unknown guard metric"),
             FallbackUnknownMatchPct >= Config.MinFallbackUnknownMatchPct);

    TestTrue(TEXT("Intent drift details: все fixture-case должны совпадать с baseline"),
             bNoIntentDriftDetails);
    TestTrue(TEXT("Frame drift details: все fixture-case должны совпадать с baseline"),
             bNoFrameDriftDetails);

    return true;
}

#undef NEIRA_TEST_FLAGS
