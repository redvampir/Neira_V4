#include "FResponseGenerator.h"
#include "FSentencePlanner.h"

namespace
{
FString NormalizeContextKey(const FString& InContextKey)
{
    const FString Trimmed = InContextKey.TrimStartAndEnd();
    return Trimmed.IsEmpty() ? TEXT("default") : Trimmed.ToLower();
}
}

FResponsePersonalityProfile FResponsePersonalityProfile::MakeV1(EResponseTone InTone,
                                                                 EResponseLength InLength,
                                                                 EResponseInitiative InInitiative,
                                                                 EResponseAddressStyle InAddressStyle)
{
    FResponsePersonalityProfile Profile;
    Profile.ProfileID = TEXT("personality_profile_v1");
    Profile.Tone = InTone;
    Profile.Length = InLength;
    Profile.Initiative = InInitiative;
    Profile.AddressStyle = InAddressStyle;
    Profile.bForbidHallucination = true;
    Profile.bRequireExplicitUncertainty = true;
    return Profile;
}

FResponseGenerationOutput FResponseGenerator::Generate(const FResponseGenerationInput& Input,
                                                       const FResponsePersonalityProfile& Profile) const
{
    FResponseGenerationOutput Out;

    // Построить FormatID для трассировки (стабилен при одинаковых входах)
    Out.FormatID = BuildFormatID(Input, Profile);

    const FResponseSemanticDecision& SD = Input.SemanticDecision;

    // Выбрать синтаксическую стратегию и построить предложение
    FSentencePlanner Planner;

    const FString Subject = SD.EntityTarget.TrimStartAndEnd();
    const FString Object  = SD.SemanticCore.TrimStartAndEnd().IsEmpty()
                                ? TEXT("данных недостаточно")
                                : SD.SemanticCore.TrimStartAndEnd();

    Out.StrategyID = Planner.GetStrategyId(
        SD.IntentID, SD.ConfidenceLevel, Profile.Tone, Input.SessionResponseCount);

    FString MainSentence = Planner.Plan(
        SD.IntentID, SD.ConfidenceLevel, Profile.Tone,
        Subject, Object, Input.SessionResponseCount);

    // Обновить FormatID с ID стратегии
    Out.FormatID += FString(TEXT(".s_")) + Out.StrategyID;

    TArray<FString> Lines;
    Lines.Add(MainSentence);

    // Блок неопределённости (сохраняем для совместимости с тестами пайплайна)
    if (SD.bHasUncertainty && Profile.bRequireExplicitUncertainty)
    {
        const FString Why = SD.UncertaintyReason.TrimStartAndEnd().IsEmpty()
                                ? TEXT("часть данных отсутствует")
                                : SD.UncertaintyReason.TrimStartAndEnd();
        Lines.Add(FString::Printf(TEXT("Неопределённость: %s."), *Why));
    }

    // Связанные понятия
    if (!SD.RelatedTerms.IsEmpty())
    {
        const FString Label = SD.RelatedTermsLabel.TrimStartAndEnd().IsEmpty()
                                  ? TEXT("Связанные понятия")
                                  : SD.RelatedTermsLabel.TrimStartAndEnd();

        FString TermsList;
        for (int32 i = 0; i < SD.RelatedTerms.Num(); ++i)
        {
            if (i > 0)
                TermsList += TEXT(", ");
            TermsList += SD.RelatedTerms[i];
        }
        Lines.Add(FString::Printf(TEXT("%s: %s."), *Label, *TermsList));
    }

    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        if (Index > 0)
            Out.ResponseText += TEXT("\n");
        Out.ResponseText += Lines[Index];
    }
    return Out;
}

FString FResponseGenerator::BuildFormatID(const FResponseGenerationInput& Input,
                                           const FResponsePersonalityProfile& Profile)
{
    return FString::Printf(TEXT("v2.intent_%d.ctx_%s.conf_%d.rot_%d.tone_%s"),
                           static_cast<int32>(Input.SemanticDecision.IntentID),
                           *NormalizeContextKey(Input.ContextKey),
                           static_cast<int32>(Input.SemanticDecision.ConfidenceLevel),
                           Input.SessionResponseCount,
                           *ToString(Profile.Tone));
}

FString FResponseGenerator::ToString(EResponseTone Tone)
{
    switch (Tone)
    {
    case EResponseTone::Calm:     return TEXT("calm");
    case EResponseTone::Business: return TEXT("business");
    default:                      return TEXT("calm");
    }
}

FString FResponseGenerator::ToString(EResponseLength Length)
{
    switch (Length)
    {
    case EResponseLength::Short:  return TEXT("short");
    case EResponseLength::Medium: return TEXT("medium");
    default:                      return TEXT("short");
    }
}

FString FResponseGenerator::ToString(EResponseInitiative Initiative)
{
    switch (Initiative)
    {
    case EResponseInitiative::Low:    return TEXT("low");
    case EResponseInitiative::Medium: return TEXT("medium");
    default:                          return TEXT("low");
    }
}

FString FResponseGenerator::ToString(EResponseAddressStyle AddressStyle)
{
    switch (AddressStyle)
    {
    case EResponseAddressStyle::NeutralYou: return TEXT("neutral_you");
    case EResponseAddressStyle::FormalYou:  return TEXT("formal_you");
    case EResponseAddressStyle::FriendlyYou:return TEXT("friendly_you");
    default:                                return TEXT("neutral_you");
    }
}
