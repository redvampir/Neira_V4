#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

enum class EAgreementCase : uint8
{
    Nominative,
    Prepositional,
    Instrumental,
};

struct NEIRACORE_API FEntityTargetForms
{
    FString Nominative;
    FString Prepositional;
    FString Instrumental;
    bool    bUsedFallback = false;
};

// Лёгкий realizer-согласователь для EntityTarget.
// Ограничение: эвристические правила только для однословных русских форм.
struct NEIRACORE_API FMorphAgreement
{
    static FEntityTargetForms BuildEntityTargetForms(const FString& RawEntityTarget);

private:
    static FString NormalizeInput(const FString& RawEntityTarget);
    static bool TryInflectSingleWord(const FString& Word, EAgreementCase Case, FString& Out);
    static bool IsLikelySingleWord(const FString& Value);
};

