#include "FMorphAgreement.h"

FEntityTargetForms FMorphAgreement::BuildEntityTargetForms(const FString& RawEntityTarget)
{
    FEntityTargetForms Forms;
    Forms.Nominative = NormalizeInput(RawEntityTarget);
    if (Forms.Nominative.IsEmpty())
    {
        return Forms;
    }

    Forms.Prepositional = Forms.Nominative;
    Forms.Instrumental = Forms.Nominative;

    if (!IsLikelySingleWord(Forms.Nominative))
    {
        Forms.Prepositional = FString::Printf(TEXT("«%s»"), *Forms.Nominative);
        Forms.Instrumental = FString::Printf(TEXT("«%s»"), *Forms.Nominative);
        Forms.bUsedFallback = true;
        return Forms;
    }

    bool bPrep = TryInflectSingleWord(Forms.Nominative, EAgreementCase::Prepositional, Forms.Prepositional);
    bool bIns = TryInflectSingleWord(Forms.Nominative, EAgreementCase::Instrumental, Forms.Instrumental);
    Forms.bUsedFallback = !(bPrep && bIns);

    if (!bPrep)
        Forms.Prepositional = FString::Printf(TEXT("«%s»"), *Forms.Nominative);
    if (!bIns)
        Forms.Instrumental = FString::Printf(TEXT("«%s»"), *Forms.Nominative);

    return Forms;
}

FString FMorphAgreement::NormalizeInput(const FString& RawEntityTarget)
{
    return RawEntityTarget.TrimStartAndEnd().ToLower();
}

bool FMorphAgreement::IsLikelySingleWord(const FString& Value)
{
    return !Value.Contains(TEXT(" ")) && !Value.Contains(TEXT("-")) && !Value.Contains(TEXT("/"));
}

bool FMorphAgreement::TryInflectSingleWord(const FString& Word, EAgreementCase Case, FString& Out)
{
    if (Word.IsEmpty() || Case == EAgreementCase::Nominative)
    {
        Out = Word;
        return true;
    }

    FString Stem = Word;

    if (Word.EndsWith(TEXT("а")))
    {
        Stem = Word.LeftChop(FString(TEXT("а")).Len());
        Out = (Case == EAgreementCase::Prepositional) ? (Stem + TEXT("е")) : (Stem + TEXT("ой"));
        return true;
    }

    if (Word.EndsWith(TEXT("я")))
    {
        Stem = Word.LeftChop(FString(TEXT("я")).Len());
        Out = (Case == EAgreementCase::Prepositional) ? (Stem + TEXT("е")) : (Stem + TEXT("ей"));
        return true;
    }

    if (Word.EndsWith(TEXT("о")))
    {
        Stem = Word.LeftChop(FString(TEXT("о")).Len());
        Out = (Case == EAgreementCase::Prepositional) ? (Stem + TEXT("е")) : (Stem + TEXT("ом"));
        return true;
    }

    if (Word.EndsWith(TEXT("е")) || Word.EndsWith(TEXT("ё")))
    {
        Stem = Word.LeftChop(Word.EndsWith(TEXT("ё")) ? FString(TEXT("ё")).Len() : FString(TEXT("е")).Len());
        Out = (Case == EAgreementCase::Prepositional) ? (Stem + TEXT("е")) : (Stem + TEXT("ем"));
        return true;
    }

    if (Word.EndsWith(TEXT("й")))
    {
        Stem = Word.LeftChop(FString(TEXT("й")).Len());
        Out = (Case == EAgreementCase::Prepositional) ? (Stem + TEXT("е")) : (Stem + TEXT("ем"));
        return true;
    }

    if (Word.EndsWith(TEXT("ь")))
    {
        Stem = Word.LeftChop(FString(TEXT("ь")).Len());
        Out = (Case == EAgreementCase::Prepositional) ? (Stem + TEXT("и")) : (Stem + TEXT("ем"));
        return true;
    }
    Out = (Case == EAgreementCase::Prepositional) ? (Word + TEXT("е")) : (Word + TEXT("ом"));
    return true;
}
