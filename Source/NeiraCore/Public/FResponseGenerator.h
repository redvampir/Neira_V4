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

struct NEIRACORE_API FResponsePersonalityProfile
{
    FString            ProfileID = TEXT("personality_profile_v1");
    EResponseTone      Tone = EResponseTone::Calm;
    EResponseLength    Length = EResponseLength::Short;
    EResponseInitiative Initiative = EResponseInitiative::Low;

    // Запреты v1:
    // 1) Не выдумывать факты.
    // 2) Явно отмечать неопределённость.
    bool bForbidHallucination = true;
    bool bRequireExplicitUncertainty = true;

    static FResponsePersonalityProfile MakeV1(EResponseTone InTone,
                                              EResponseLength InLength,
                                              EResponseInitiative InInitiative);
};

struct NEIRACORE_API FResponseGenerationInput
{
    EIntentID IntentID = EIntentID::Unknown;
    FString   ContextKey;
    FString   SemanticCore;

    bool      bHasUncertainty = false;
    FString   UncertaintyReason;
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
};
