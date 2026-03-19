#pragma once

#include "CoreMinimal.h"

// Явные уровни давления памяти для управляемой деградации контекста.
enum class EMemoryPressureLevel : uint8
{
    Medium,
    High,
    Critical,
};

// Результат применения политики без silent data loss.
enum class EMemoryPolicyApplyStatus : uint8
{
    Applied,
    AppliedWithDegradation,
    Rejected,
};

enum class EMemoryPolicyDegradationReason : uint8
{
    None,
    WarmCompacted,
    WarmSummarized,
    HotReducedToMinimal,
    MissingRequiredAnchors,
    MissingHotForCritical,
    AnchorContextMissing,
};

struct NEIRACORE_API FMemoryContextState
{
    TArray<FString> HotMemory;
    TArray<FString> WarmMemory;
    TArray<FString> ColdMemory;
    TArray<FString> Anchors;
    TArray<FString> AnchorContextPairs; // формат: "<anchor>::<context>"
    FString WarmSummary;
};

struct NEIRACORE_API FMemoryPressurePolicyOptions
{
    int32 MaxWarmItemsOnMedium = 3;
    int32 MinimalHotItemsOnCritical = 1;
};

struct NEIRACORE_API FMemoryPolicyApplyResult
{
    EMemoryPolicyApplyStatus Status = EMemoryPolicyApplyStatus::Rejected;
    EMemoryPolicyDegradationReason Reason = EMemoryPolicyDegradationReason::None;
    EMemoryPressureLevel AppliedLevel = EMemoryPressureLevel::Medium;
    FString Details;
};

class NEIRACORE_API FMemoryPressurePolicy
{
public:
    FMemoryPolicyApplyResult Apply(
        EMemoryPressureLevel Level,
        FMemoryContextState& InOutState,
        const FMemoryPressurePolicyOptions& Options = FMemoryPressurePolicyOptions()) const;

    bool RestoreContextFromAnchor(
        const FString& Anchor,
        const FMemoryContextState& State,
        FString& OutContext,
        EMemoryPolicyDegradationReason& OutReason) const;

private:
    void CompactWarmMemory(FMemoryContextState& State, int32 MaxItems) const;
    FString SummarizeWarmMemory(const TArray<FString>& WarmMemory) const;
};
