#include "FResponseGenerator.h"

namespace
{
FString NormalizeContextKey(const FString& InContextKey)
{
    const FString Trimmed = InContextKey.TrimStartAndEnd();
    return Trimmed.IsEmpty() ? TEXT("default") : Trimmed.ToLower();
}

FString NormalizeSemanticCore(const FString& InSemanticCore)
{
    const FString Trimmed = InSemanticCore.TrimStartAndEnd();
    return Trimmed.IsEmpty() ? TEXT("данных недостаточно") : Trimmed;
}

FString ToneLead(EResponseTone Tone)
{
    switch (Tone)
    {
    case EResponseTone::Calm:     return TEXT("Тон: спокойный.");
    case EResponseTone::Business: return TEXT("Тон: деловой.");
    default:                      return TEXT("Тон: спокойный.");
    }
}

FString InitiativeLine(EResponseInitiative Initiative)
{
    switch (Initiative)
    {
    case EResponseInitiative::Low:
        return TEXT("Инициатива: низкая.");
    case EResponseInitiative::Medium:
        return TEXT("Инициатива: средняя. При необходимости предложу следующий шаг.");
    default:
        return TEXT("Инициатива: низкая.");
    }
}

FString AddressStyleLine(EResponseAddressStyle AddressStyle)
{
    switch (AddressStyle)
    {
    case EResponseAddressStyle::NeutralYou:
        return TEXT("Обращение: нейтральное, на «вы».");
    case EResponseAddressStyle::FormalYou:
        return TEXT("Обращение: формальное, на «вы».");
    case EResponseAddressStyle::FriendlyYou:
        return TEXT("Обращение: дружелюбное, на «ты».");
    default:
        return TEXT("Обращение: нейтральное, на «вы».");
    }
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
    Out.FormatID = BuildFormatID(Input, Profile);

    const FString SemanticCore = NormalizeSemanticCore(Input.SemanticDecision.SemanticCore);

    TArray<FString> Lines;
    Lines.Add(FString::Printf(TEXT("[profile=%s; tone=%s; len=%s; initiative=%s; address=%s; format=%s]"),
                              *Profile.ProfileID,
                              *ToString(Profile.Tone),
                              *ToString(Profile.Length),
                              *ToString(Profile.Initiative),
                              *ToString(Profile.AddressStyle),
                              *Out.FormatID));

    Lines.Add(ToneLead(Profile.Tone));
    Lines.Add(AddressStyleLine(Profile.AddressStyle));
    Lines.Add(InitiativeLine(Profile.Initiative));
    Lines.Add(BuildIntentBlock(Input.SemanticDecision.IntentID, SemanticCore));

    if (Input.SemanticDecision.bHasUncertainty && Profile.bRequireExplicitUncertainty)
    {
        const FString Why = Input.SemanticDecision.UncertaintyReason.TrimStartAndEnd().IsEmpty()
                                ? TEXT("часть данных отсутствует")
                                : Input.SemanticDecision.UncertaintyReason.TrimStartAndEnd();
        Lines.Add(FString::Printf(TEXT("Неопределённость: %s."), *Why));
    }

    if (!Input.SemanticDecision.RelatedTerms.IsEmpty())
    {
        // Метка секции
        const FString Label = Input.SemanticDecision.RelatedTermsLabel.TrimStartAndEnd().IsEmpty()
                                  ? TEXT("Связанные понятия")
                                  : Input.SemanticDecision.RelatedTermsLabel.TrimStartAndEnd();

        // Собрать термины через запятую
        FString TermsList;
        for (int32 i = 0; i < Input.SemanticDecision.RelatedTerms.Num(); ++i)
        {
            if (i > 0)
                TermsList += TEXT(", ");
            TermsList += Input.SemanticDecision.RelatedTerms[i];
        }
        Lines.Add(FString::Printf(TEXT("%s: %s."), *Label, *TermsList));
    }

    if (Profile.bForbidHallucination)
    {
        Lines.Add(TEXT("Ограничение: факты не выдумываю."));
    }

    if (Profile.Length == EResponseLength::Medium && Profile.Initiative == EResponseInitiative::Medium)
    {
        Lines.Add(TEXT("Следующий шаг: при необходимости уточните цель, и я продолжу в том же формате."));
    }

    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        if (Index > 0)
        {
            Out.ResponseText += TEXT("\n");
        }
        Out.ResponseText += Lines[Index];
    }
    return Out;
}

FString FResponseGenerator::BuildFormatID(const FResponseGenerationInput& Input,
                                          const FResponsePersonalityProfile& Profile)
{
    return FString::Printf(TEXT("v1.intent_%d.ctx_%s.tone_%s.len_%s.init_%s.addr_%s"),
                           static_cast<int32>(Input.SemanticDecision.IntentID),
                           *NormalizeContextKey(Input.ContextKey),
                           *ToString(Profile.Tone),
                           *ToString(Profile.Length),
                           *ToString(Profile.Initiative),
                           *ToString(Profile.AddressStyle));
}

FString FResponseGenerator::BuildIntentBlock(EIntentID IntentID, const FString& SemanticCore)
{
    switch (IntentID)
    {
    case EIntentID::GetDefinition:
        return FString::Printf(TEXT("Ответ: определение — %s."), *SemanticCore);
    case EIntentID::GetWordFact:
        return FString::Printf(TEXT("Ответ: факт — %s."), *SemanticCore);
    case EIntentID::FindMeaning:
        return FString::Printf(TEXT("Ответ: смысл — %s."), *SemanticCore);
    case EIntentID::AnswerAbility:
        return FString::Printf(TEXT("Ответ: возможность — %s."), *SemanticCore);
    case EIntentID::StoreFact:
        return FString::Printf(TEXT("Ответ: факт сохранения — %s."), *SemanticCore);
    default:
        return FString::Printf(TEXT("Ответ: %s."), *SemanticCore);
    }
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
