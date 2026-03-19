#include "FMemoryPressurePolicy.h"

FMemoryPolicyApplyResult FMemoryPressurePolicy::Apply(
    EMemoryPressureLevel Level,
    FMemoryContextState& InOutState,
    const FMemoryPressurePolicyOptions& Options) const
{
    FMemoryPolicyApplyResult Result;
    Result.AppliedLevel = Level;
    Result.Status = EMemoryPolicyApplyStatus::Applied;

    const FMemoryContextState Snapshot = InOutState;

    if (Level == EMemoryPressureLevel::Medium)
    {
        const int32 WarmBefore = InOutState.WarmMemory.Num();
        CompactWarmMemory(InOutState, Options.MaxWarmItemsOnMedium);

        if (InOutState.WarmMemory.Num() < WarmBefore)
        {
            Result.Status = EMemoryPolicyApplyStatus::AppliedWithDegradation;
            Result.Reason = EMemoryPolicyDegradationReason::WarmCompacted;
            Result.Details = TEXT("WARM compacted to fit medium pressure constraints");
        }

        return Result;
    }

    if (InOutState.Anchors.IsEmpty())
    {
        InOutState = Snapshot;
        Result.Status = EMemoryPolicyApplyStatus::Rejected;
        Result.Reason = EMemoryPolicyDegradationReason::MissingRequiredAnchors;
        Result.Details = TEXT("Anchors are required for High/Critical pressure levels");
        return Result;
    }

    if (Level == EMemoryPressureLevel::High)
    {
        if (!InOutState.WarmMemory.IsEmpty())
        {
            InOutState.WarmSummary = SummarizeWarmMemory(InOutState.WarmMemory);
            InOutState.WarmMemory.Reset();
            Result.Status = EMemoryPolicyApplyStatus::AppliedWithDegradation;
            Result.Reason = EMemoryPolicyDegradationReason::WarmSummarized;
            Result.Details = TEXT("WARM replaced by summary while preserving HOT and anchors");
        }

        return Result;
    }

    // Critical
    if (InOutState.HotMemory.Num() < Options.MinimalHotItemsOnCritical)
    {
        InOutState = Snapshot;
        Result.Status = EMemoryPolicyApplyStatus::Rejected;
        Result.Reason = EMemoryPolicyDegradationReason::MissingHotForCritical;
        Result.Details = TEXT("Not enough HOT memory to keep minimal critical context");
        return Result;
    }

    TArray<FString> MinimalHot;
    for (int32 Index = 0; Index < Options.MinimalHotItemsOnCritical; ++Index)
    {
        MinimalHot.Add(InOutState.HotMemory[Index]);
    }

    InOutState.HotMemory = MoveTemp(MinimalHot);
    InOutState.WarmMemory.Reset();
    InOutState.WarmSummary = TEXT("");

    Result.Status = EMemoryPolicyApplyStatus::AppliedWithDegradation;
    Result.Reason = EMemoryPolicyDegradationReason::HotReducedToMinimal;
    Result.Details = TEXT("Critical pressure keeps COLD + anchors and minimal HOT");
    return Result;
}

bool FMemoryPressurePolicy::RestoreContextFromAnchor(
    const FString& Anchor,
    const FMemoryContextState& State,
    FString& OutContext,
    EMemoryPolicyDegradationReason& OutReason) const
{
    OutReason = EMemoryPolicyDegradationReason::None;

    if (!State.Anchors.Contains(Anchor))
    {
        OutReason = EMemoryPolicyDegradationReason::MissingRequiredAnchors;
        return false;
    }

    for (const FString& Pair : State.AnchorContextPairs)
    {
        const FString Prefix = Anchor + TEXT("::");
        if (Pair.StartsWith(Prefix, false))
        {
            OutContext = Pair.Mid(Prefix.Len());
            return true;
        }
    }

    for (const FString& ColdEntry : State.ColdMemory)
    {
        if (ColdEntry.Contains(Anchor, false))
        {
            OutContext = ColdEntry;
            return true;
        }
    }

    OutReason = EMemoryPolicyDegradationReason::AnchorContextMissing;
    return false;
}

void FMemoryPressurePolicy::CompactWarmMemory(FMemoryContextState& State, int32 MaxItems) const
{
    if (MaxItems < 0)
        MaxItems = 0;

    TArray<FString> Unique;
    for (const FString& Item : State.WarmMemory)
    {
        if (!Unique.Contains(Item))
        {
            Unique.Add(Item);
        }
    }

    TArray<FString> Compacted;
    for (int32 Index = 0; Index < Unique.Num() && Index < MaxItems; ++Index)
    {
        Compacted.Add(Unique[Index]);
    }

    State.WarmMemory = MoveTemp(Compacted);
}

FString FMemoryPressurePolicy::SummarizeWarmMemory(const TArray<FString>& WarmMemory) const
{
    FString Summary;

    for (int32 Index = 0; Index < WarmMemory.Num(); ++Index)
    {
        if (Index > 0)
        {
            Summary += TEXT(" | ");
        }

        Summary += WarmMemory[Index];
    }

    return Summary;
}
