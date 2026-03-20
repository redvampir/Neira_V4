#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

// ---------------------------------------------------------------------------
// Профиль personality_profile_v1 применяется ТОЛЬКО на этапе генерации ответа.
// Он не влияет на NLU и knowledge transitions.
// ---------------------------------------------------------------------------

enum class EResponseTone : uint8
{
    Calm,
    Business,
};

enum class EResponseLength : uint8
{
    Short,
    Medium,
};

enum class EResponseInitiative : uint8
{
    Low,
    Medium,
};

enum class EResponseAddressStyle : uint8
{
    NeutralYou, // Нейтрально на «вы»
    FormalYou,  // Формально на «вы»
    FriendlyYou // Дружелюбно на «ты»
};

struct NEIRACORE_API FResponsePersonalityProfile
{
    FString               ProfileID = TEXT("personality_profile_v1");
    EResponseTone         Tone = EResponseTone::Calm;
    EResponseLength       Length = EResponseLength::Short;
    EResponseInitiative   Initiative = EResponseInitiative::Low;
    EResponseAddressStyle AddressStyle = EResponseAddressStyle::NeutralYou;

    // Запреты v1:
    // 1) Не выдумывать факты.
    // 2) Явно отмечать неопределённость.
    bool bForbidHallucination = true;
    bool bRequireExplicitUncertainty = true;

    static FResponsePersonalityProfile MakeV1(EResponseTone InTone,
                                              EResponseLength InLength,
                                              EResponseInitiative InInitiative,
                                              EResponseAddressStyle InAddressStyle = EResponseAddressStyle::NeutralYou);
};

// Фактологическое решение: формируется NLU/knowledge-пайплайном.
// Personality сюда НЕ входит.
struct NEIRACORE_API FResponseSemanticDecision
{
    EIntentID IntentID = EIntentID::Unknown;
    FString   SemanticCore;

    bool      bHasUncertainty = false;
    FString   UncertaintyReason;

    // Семантически связанные понятия (синонимы, гиперонимы и т.д.),
    // заполняются вызывающим кодом из FSemanticGraph.
    // Отображаются только если список не пустой.
    TArray<FString> RelatedTerms;
    // Метка раздела: "Синонимы", "Родовые понятия", "Связанные понятия" и т.д.
    // По умолчанию "Связанные понятия".
    FString         RelatedTermsLabel;
};

// Рендеринг ответа: сочетает фактологическое решение и personality.
struct NEIRACORE_API FResponseGenerationInput
{
    FString ContextKey;
    FResponseSemanticDecision SemanticDecision;
};

struct NEIRACORE_API FResponseGenerationOutput
{
    FString FormatID;
    FString ResponseText;
};

struct NEIRACORE_API FResponseGenerator
{
    // Deterministic policy:
    // одинаковый (IntentID + ContextKey + Profile) => совместимый формат (стабильный шаблон).
    FResponseGenerationOutput Generate(const FResponseGenerationInput& Input,
                                       const FResponsePersonalityProfile& Profile) const;

private:
    static FString BuildFormatID(const FResponseGenerationInput& Input,
                                 const FResponsePersonalityProfile& Profile);
    static FString BuildIntentBlock(EIntentID IntentID, const FString& SemanticCore);
    static FString ToString(EResponseTone Tone);
    static FString ToString(EResponseLength Length);
    static FString ToString(EResponseInitiative Initiative);
    static FString ToString(EResponseAddressStyle AddressStyle);
};
