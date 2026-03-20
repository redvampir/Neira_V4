// FIntentExtractor.cpp
// v0.4 — интеграция с FSyntaxParser + улучшенные conversational-heuristics.
//
// Алгоритм (два пути):
//
//   Путь 1: Frame-анализ (приоритет).
//     FSyntaxParser.Parse() строит FSemanticFrame. Затем ExtractFromFrame()
//     пробует разрешить Intent по структурным признакам фрейма и по
//     phrase-level эвристикам для практических диалоговых сценариев:
//       1. memory query patterns → RetrieveMemory
//       2. Frame.bIsAbilityCheck → AnswerAbility
//       3. PhraseType == Statement → StoreFact
//       4. command memory-store patterns → StoreFact
//       5. Predicate ∈ {найти} + Object ∈ {значение, определение, смысл} → FindMeaning
//       6. Predicate ∈ {рассказать, объяснить, сказать} + topic → GetWordFact
//       7. definition-question patterns / Question + valid Object → GetDefinition
//
//   Путь 2: Fallback — строковые шаблоны v0.1.
//     Если путь 1 вернул Unknown → ExtractFromPatterns() ищет маркер-подстроку.
//     Это backward-compatibility: старые тесты продолжают проходить.

#include "FIntentExtractor.h"
#include "FMorphAnalyzer.h"

namespace
{
    struct FIntentPattern
    {
        FString   Marker;
        EIntentID IntentID;
        float     Confidence;
        bool      bExtractAfterMarker;
    };

    const TArray<FIntentPattern> Patterns = {
        { TEXT("что такое "),          EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("кто такой "),          EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("кто такая "),          EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("что означает слово "), EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("что означает "),       EIntentID::GetDefinition, 0.9f, true  },
        { TEXT("что значит "),         EIntentID::GetDefinition, 0.9f, true  },

        { TEXT("найди значение "),     EIntentID::FindMeaning,   0.9f, true  },
        { TEXT("найди определение "),  EIntentID::FindMeaning,   0.9f, true  },

        { TEXT("расскажи про "),       EIntentID::GetWordFact,   0.7f, true  },
        { TEXT("расскажи о "),         EIntentID::GetWordFact,   0.7f, true  },

        { TEXT("ты можешь "),          EIntentID::AnswerAbility, 0.8f, false },
        { TEXT("можешь ли ты "),       EIntentID::AnswerAbility, 0.8f, false },
        { TEXT("умеешь ли "),          EIntentID::AnswerAbility, 0.8f, false },
    };

    FString CleanEntity(const FString& Raw)
    {
        FString Result = Raw.TrimStartAndEnd();

        while (!Result.IsEmpty())
        {
            const char Tail = Result.Last();
            if (Tail == '?' || Tail == '!' || Tail == '.' || Tail == ',' || Tail == ':')
            {
                Result = Result.LeftChop(1).TrimEnd();
                continue;
            }
            break;
        }

        return Result;
    }

    bool TryExtractTailAfterMarker(const FString& Phrase,
                                   const TArray<FString>& Markers,
                                   FString& OutTail)
    {
        const FString Lower = Phrase.ToLower();

        for (const FString& Marker : Markers)
        {
            const int32 MarkerPos = Lower.Find(Marker);
            if (MarkerPos == INDEX_NONE)
                continue;

            const int32 TailStart = MarkerPos + Marker.Len();
            OutTail = CleanEntity(Phrase.Mid(TailStart));
            if (!OutTail.IsEmpty())
                return true;
        }

        return false;
    }

    bool IsFindPredicate(const FString& LemmaLower)
    {
        return LemmaLower == TEXT("найти")
            || LemmaLower == TEXT("найди")
            || LemmaLower == TEXT("искать");
    }

    bool IsTellPredicate(const FString& LemmaLower)
    {
        return LemmaLower == TEXT("рассказать")
            || LemmaLower == TEXT("объяснить")
            || LemmaLower == TEXT("сказать")
            || LemmaLower == TEXT("расскажи")
            || LemmaLower == TEXT("объясни")
            || LemmaLower == TEXT("скажи");
    }

    bool IsMemoryStorePredicate(const FString& LemmaLower)
    {
        return LemmaLower == TEXT("запомнить")
            || LemmaLower == TEXT("запомни")
            || LemmaLower == TEXT("сохранить")
            || LemmaLower == TEXT("сохрани")
            || LemmaLower == TEXT("записать")
            || LemmaLower == TEXT("запиши");
    }

    bool IsDefinitionObject(const FString& ObjectLower)
    {
        return ObjectLower == TEXT("значение")
            || ObjectLower == TEXT("определение")
            || ObjectLower == TEXT("смысл")
            || ObjectLower == TEXT("значения")
            || ObjectLower == TEXT("определения");
    }

    bool IsMetaWord(const FString& ObjectLower)
    {
        return ObjectLower == TEXT("слово")
            || ObjectLower == TEXT("слова")
            || ObjectLower == TEXT("термин")
            || ObjectLower == TEXT("понятие")
            || ObjectLower == TEXT("выражение");
    }

    bool IsMeaningMarkerToken(const FString& TokenLower)
    {
        return TokenLower == TEXT("значение")
            || TokenLower == TEXT("определение")
            || TokenLower == TEXT("смысл");
    }

    bool IsTermMetaToken(const FString& TokenLower)
    {
        return TokenLower == TEXT("слово")
            || TokenLower == TEXT("слова")
            || TokenLower == TEXT("термин")
            || TokenLower == TEXT("термина")
            || TokenLower == TEXT("понятие")
            || TokenLower == TEXT("выражение");
    }

    bool IsQuestionPlaceholderToken(const FString& TokenLower)
    {
        return TokenLower == TEXT("что")
            || TokenLower == TEXT("кто")
            || TokenLower == TEXT("как")
            || TokenLower == TEXT("какой")
            || TokenLower == TEXT("какая")
            || TokenLower == TEXT("какие")
            || TokenLower == TEXT("я")
            || TokenLower == TEXT("ты")
            || TokenLower == TEXT("вы");
    }

    FString StripLeadingPrefixes(const FString& RawValue, const TArray<FString>& Prefixes)
    {
        FString Result = CleanEntity(RawValue);

        bool bChanged = true;
        while (bChanged)
        {
            bChanged = false;
            const FString Lower = Result.ToLower();
            for (const FString& Prefix : Prefixes)
            {
                if (Lower.StartsWith(Prefix, false))
                {
                    Result = CleanEntity(Result.Mid(Prefix.Len()));
                    bChanged = true;
                    break;
                }
            }
        }

        return Result;
    }

    FString StripLeadingTermMetaToken(const FString& RawTerm)
    {
        const TArray<FString> Prefixes = {
            TEXT("слово "), TEXT("слова "), TEXT("термин "), TEXT("термина "),
            TEXT("понятие "), TEXT("выражение ")
        };
        return StripLeadingPrefixes(RawTerm, Prefixes);
    }

    FString StripLeadingDefinitionPrompt(const FString& RawValue)
    {
        const TArray<FString> Prefixes = {
            TEXT("что такое "), TEXT("кто такой "), TEXT("кто такая "),
            TEXT("что означает слово "), TEXT("что означает "),
            TEXT("что значит "), TEXT("что значит слово ")
        };
        return StripLeadingPrefixes(RawValue, Prefixes);
    }

    bool TryExtractAbilityAction(const FString& Phrase, FString& OutAction)
    {
        const TArray<FString> Markers = {
            TEXT("ты можешь "),
            TEXT("вы можете "),
            TEXT("можешь ли ты "),
            TEXT("можете ли вы "),
            TEXT("ты умеешь "),
            TEXT("вы умеете "),
            TEXT("умеешь ли ты "),
            TEXT("умеете ли вы ")
        };

        if (!TryExtractTailAfterMarker(Phrase, Markers, OutAction))
            return false;

        OutAction = StripLeadingDefinitionPrompt(OutAction);
        return !OutAction.IsEmpty();
    }

    bool TryExtractStoreClaim(const FString& Phrase, FString& OutClaim)
    {
        const TArray<FString> ExplicitFactMarkers = {
            TEXT("запомни, что "),
            TEXT("запомни что "),
            TEXT("сохрани, что "),
            TEXT("сохрани что "),
            TEXT("запиши, что "),
            TEXT("запиши что ")
        };

        if (TryExtractTailAfterMarker(Phrase, ExplicitFactMarkers, OutClaim))
        {
            if (OutClaim.ToLower().StartsWith(TEXT("что "), false))
            {
                OutClaim = CleanEntity(OutClaim.Mid(4));
            }

            return !OutClaim.IsEmpty();
        }

        const TArray<FString> GenericStoreMarkers = {
            TEXT("запомни "),
            TEXT("сохрани "),
            TEXT("запиши ")
        };

        if (!TryExtractTailAfterMarker(Phrase, GenericStoreMarkers, OutClaim))
            return false;

        const FString ClaimLower = OutClaim.ToLower();
        if (ClaimLower.Contains(TEXT(" - "))
            || ClaimLower.Contains(TEXT(" — "))
            || ClaimLower.Contains(TEXT(" это "))
            || ClaimLower.Contains(TEXT(" является "))
            || ClaimLower.Contains(TEXT(" был "))
            || ClaimLower.Contains(TEXT(" была "))
            || ClaimLower.Contains(TEXT(" было "))
            || ClaimLower.Contains(TEXT(" будут "))
            || ClaimLower.Contains(TEXT(" есть ")))
        {
            return true;
        }

        TArray<FString> Tokens;
        OutClaim.ParseIntoArrayWS(Tokens);
        return Tokens.Num() >= 3;
    }

    bool TryExtractRetrieveMemoryTarget(const FString& Phrase, FString& OutTarget)
    {
        const FString Lower = Phrase.ToLower().TrimStartAndEnd();

        if (Lower.StartsWith(TEXT("что я сказал"), false))
        {
            OutTarget = TEXT("__last_user_utterance__");
            return true;
        }

        if (Lower.StartsWith(TEXT("что ты запомнила"), false) ||
            Lower.StartsWith(TEXT("что ты запомнил"), false) ||
            Lower.StartsWith(TEXT("что ты помнишь"), false))
        {
            OutTarget = TEXT("__last_stored_fact__");
            return true;
        }

        if (Lower.StartsWith(TEXT("вспомни "), false))
        {
            OutTarget = CleanEntity(Phrase.Mid(8));
            return !OutTarget.IsEmpty();
        }

        if (Lower.StartsWith(TEXT("что ты знаешь про "), false))
        {
            OutTarget = CleanEntity(Phrase.Mid(17));
            return !OutTarget.IsEmpty();
        }

        return false;
    }

    bool TryExtractDefinitionTarget(const FString& Phrase, FString& OutTarget)
    {
        const TArray<FString> Markers = {
            TEXT("что такое "),
            TEXT("кто такой "),
            TEXT("кто такая "),
            TEXT("что означает слово "),
            TEXT("что означает "),
            TEXT("что значит слово "),
            TEXT("что значит "),
            TEXT("как называется ")
        };

        if (!TryExtractTailAfterMarker(Phrase, Markers, OutTarget))
            return false;

        OutTarget = StripLeadingTermMetaToken(OutTarget);
        return !OutTarget.IsEmpty();
    }

    bool TryExtractTopicAfterTellPredicate(const FString& Phrase, FString& OutTopic)
    {
        const TArray<FString> Markers = {
            TEXT("расскажи мне про "),
            TEXT("расскажи мне о "),
            TEXT("расскажи про "),
            TEXT("расскажи о "),
            TEXT("объясни мне "),
            TEXT("объясни "),
            TEXT("скажи мне про "),
            TEXT("скажи про "),
            TEXT("скажи ")
        };

        if (!TryExtractTailAfterMarker(Phrase, Markers, OutTopic))
            return false;

        OutTopic = StripLeadingDefinitionPrompt(OutTopic);
        OutTopic = StripLeadingTermMetaToken(OutTopic);
        return !OutTopic.IsEmpty();
    }

    bool TryExtractTermByExplicitPattern(const FString& Phrase, FString& OutTerm)
    {
        const FString Lower = Phrase.ToLower();
        const TArray<FString> Markers = {
            TEXT("найди значение "),
            TEXT("найди определение "),
            TEXT("найти значение "),
            TEXT("найти определение "),
            TEXT("значение слова "),
            TEXT("определение термина ")
        };

        for (const FString& Marker : Markers)
        {
            const int32 MarkerPos = Lower.Find(Marker);
            if (MarkerPos == INDEX_NONE)
                continue;

            const int32 EntityStart = MarkerPos + Marker.Len();
            OutTerm = StripLeadingTermMetaToken(Phrase.Mid(EntityStart));
            if (!OutTerm.IsEmpty() && !IsTermMetaToken(OutTerm.ToLower()))
                return true;
        }

        return false;
    }

    bool TryExtractFirstNounAfterMeaningMeta(const FString& Phrase, FString& OutTerm)
    {
        FMorphAnalyzer Analyzer;
        const TArray<FMorphResult> Tokens = Analyzer.AnalyzePhrase(Phrase);
        bool bMetaSeen = false;

        for (const FMorphResult& Token : Tokens)
        {
            const FString Lower = Token.Lemma.IsEmpty()
                                      ? Token.OriginalWord.ToLower()
                                      : Token.Lemma.ToLower();

            if (IsMeaningMarkerToken(Lower) || IsTermMetaToken(Lower))
            {
                bMetaSeen = true;
                continue;
            }

            if (bMetaSeen && Token.PartOfSpeech == EPosTag::Noun)
            {
                OutTerm = StripLeadingTermMetaToken(Token.OriginalWord);
                if (!OutTerm.IsEmpty() && !IsTermMetaToken(OutTerm.ToLower()))
                    return true;
            }
        }

        return false;
    }

    bool TryExtractMeaningTerm(const FString& Phrase, FString& OutTerm)
    {
        return TryExtractTermByExplicitPattern(Phrase, OutTerm)
            || TryExtractFirstNounAfterMeaningMeta(Phrase, OutTerm);
    }
}

FIntentResult FIntentExtractor::ExtractFromFrame(const FSemanticFrame& Frame,
                                                 const FString& OriginalPhrase,
                                                 EPhraseType PhraseType) const
{
    FIntentResult Result;
    const FString PredLower = Frame.Predicate.ToLower();
    const FString ObjectLower = Frame.Object.ToLower();

    FString MemoryTarget;
    if (TryExtractRetrieveMemoryTarget(OriginalPhrase, MemoryTarget))
    {
        Result.IntentID = EIntentID::RetrieveMemory;
        Result.EntityTarget = MemoryTarget;
        Result.Confidence = 0.95f;
        Result.DecisionTrace = TEXT("PhrasePattern:RetrieveMemory");
        Result.FailReason = EActionFailReason::None;
        return Result;
    }

    if (Frame.bIsAbilityCheck)
    {
        FString AbilityAction = Frame.Object;
        if (AbilityAction.IsEmpty())
        {
            TryExtractAbilityAction(OriginalPhrase, AbilityAction);
        }

        Result.IntentID = EIntentID::AnswerAbility;
        Result.EntityTarget = AbilityAction;
        Result.Confidence = AbilityAction.IsEmpty() ? 0.80f : 0.90f;
        Result.DecisionTrace = AbilityAction.IsEmpty()
            ? TEXT("Frame.AbilityCheck")
            : TEXT("Frame.AbilityCheck+ExtractedTail");
        Result.FailReason = EActionFailReason::None;
        return Result;
    }

    if (PhraseType == EPhraseType::Statement)
    {
        Result.IntentID = EIntentID::StoreFact;
        Result.EntityTarget = OriginalPhrase.TrimStartAndEnd();
        Result.Confidence = 0.8f;
        Result.DecisionTrace = TEXT("PhraseType:Statement");
        Result.FailReason = EActionFailReason::None;
        return Result;
    }

    if (PhraseType == EPhraseType::Command && IsMemoryStorePredicate(PredLower))
    {
        FString Claim;
        if (TryExtractStoreClaim(OriginalPhrase, Claim))
        {
            Result.IntentID = EIntentID::StoreFact;
            Result.EntityTarget = Claim;
            Result.Confidence = 0.9f;
            Result.DecisionTrace = TEXT("Frame.Predicate:MemoryStore");
            Result.FailReason = EActionFailReason::None;
            return Result;
        }
    }

    if (IsFindPredicate(PredLower) && IsDefinitionObject(ObjectLower))
    {
        FString Term;
        if (!TryExtractMeaningTerm(OriginalPhrase, Term))
        {
            Result.IntentID = EIntentID::Unknown;
            Result.Confidence = 0.0f;
            Result.DecisionTrace = TEXT("Frame.PartialParse:MeaningTermMissing");
            Result.FailReason = EActionFailReason::PartialParse;
            Result.DiagnosticNote = TEXT("Найден meta-object для FindMeaning, но целевой термин не извлечён.");
            return Result;
        }

        Result.IntentID = EIntentID::FindMeaning;
        Result.EntityTarget = Term;
        Result.Confidence = 0.9f;
        Result.DecisionTrace = TEXT("Frame.Predicate:найти+DefinitionObject+ExtractedTerm");
        Result.FailReason = EActionFailReason::None;
        return Result;
    }

    if (IsTellPredicate(PredLower))
    {
        FString Topic = Frame.Object;
        if (TryExtractTopicAfterTellPredicate(OriginalPhrase, Topic))
        {
            Result.IntentID = EIntentID::GetWordFact;
            Result.EntityTarget = Topic;
            Result.Confidence = 0.8f;
            Result.DecisionTrace = TEXT("Frame.Predicate:рассказать/объяснить+ExtractedTopic");
            Result.FailReason = EActionFailReason::None;
            return Result;
        }

        if (!Frame.Object.IsEmpty())
        {
            Result.IntentID = EIntentID::GetWordFact;
            Result.EntityTarget = Frame.Object;
            Result.Confidence = 0.75f;
            Result.DecisionTrace = TEXT("Frame.Predicate:рассказать/объяснить");
            Result.FailReason = EActionFailReason::None;
            return Result;
        }
    }

    if (PhraseType == EPhraseType::Question)
    {
        FString DefinitionTarget;
        if (TryExtractDefinitionTarget(OriginalPhrase, DefinitionTarget))
        {
            Result.IntentID = EIntentID::GetDefinition;
            Result.EntityTarget = DefinitionTarget;
            Result.Confidence = 0.9f;
            Result.DecisionTrace = TEXT("Frame.Question+DefinitionTarget");
            Result.FailReason = EActionFailReason::None;
            return Result;
        }

        if (!Frame.Object.IsEmpty()
            && !IsMetaWord(ObjectLower)
            && !IsQuestionPlaceholderToken(ObjectLower))
        {
            Result.IntentID = EIntentID::GetDefinition;
            Result.EntityTarget = Frame.Object;
            Result.Confidence = 0.85f;
            Result.DecisionTrace = TEXT("Frame.Question+Object");
            Result.FailReason = EActionFailReason::None;
            return Result;
        }
    }

    Result.IntentID = EIntentID::Unknown;
    Result.Confidence = 0.0f;
    Result.DecisionTrace = TEXT("Frame:NoIntent");
    Result.FailReason = Frame.IsEmpty()
        ? EActionFailReason::UnknownIntent
        : EActionFailReason::PartialParse;
    Result.DiagnosticNote = Frame.IsEmpty()
        ? TEXT("FSyntaxParser не извлёк опорные элементы (Predicate/Object).")
        : TEXT("FSyntaxParser извлёк частичный фрейм, но правил Intent недостаточно.");
    return Result;
}

FIntentResult FIntentExtractor::ExtractFromPatterns(const FString& Phrase) const
{
    FIntentResult Result;
    const FString Lower = Phrase.ToLower().TrimStartAndEnd();

    for (const FIntentPattern& P : Patterns)
    {
        const int32 MarkerPos = Lower.Find(P.Marker);
        if (MarkerPos == INDEX_NONE)
            continue;

        Result.IntentID = P.IntentID;
        Result.Confidence = P.Confidence;
        Result.DecisionTrace = FString::Printf(TEXT("Pattern:%s"), *P.Marker.TrimStartAndEnd());
        Result.FailReason = EActionFailReason::None;

        if (P.bExtractAfterMarker)
        {
            const int32 EntityStart = MarkerPos + P.Marker.Len();
            FString Raw = Phrase.Mid(EntityStart);
            FString Term = StripLeadingTermMetaToken(CleanEntity(Raw));
            if (Term.IsEmpty() || IsTermMetaToken(Term.ToLower()))
                continue;
            Result.EntityTarget = Term;
        }

        return Result;
    }

    Result.IntentID = EIntentID::Unknown;
    Result.Confidence = 0.4f;
    Result.EntityTarget = TEXT("");
    Result.DecisionTrace = TEXT("Fallback:Unknown");
    Result.FailReason = EActionFailReason::UnknownIntent;
    Result.DiagnosticNote = TEXT("Fallback-паттерны не распознали намерение.");
    return Result;
}

FIntentResult FIntentExtractor::Extract(const FString& Phrase, EPhraseType PhraseType) const
{
    const FString Trimmed = Phrase.TrimStartAndEnd();

    if (Trimmed.IsEmpty())
    {
        FIntentResult Empty;
        Empty.IntentID = EIntentID::Unknown;
        Empty.Confidence = 0.0f;
        Empty.DecisionTrace = TEXT("EmptyInput");
        Empty.FailReason = EActionFailReason::EmptyInput;
        Empty.DiagnosticNote = TEXT("Пустой ввод: intent не может быть извлечён.");
        return Empty;
    }

    const FSemanticFrame Frame = SyntaxParser.Parse(Trimmed, PhraseType);
    FIntentResult FrameResult = ExtractFromFrame(Frame, Trimmed, PhraseType);
    if (FrameResult.IntentID != EIntentID::Unknown)
        return FrameResult;

    FIntentResult PatternResult = ExtractFromPatterns(Trimmed);
    if (PatternResult.IntentID != EIntentID::Unknown)
        return PatternResult;

    if (FrameResult.FailReason == EActionFailReason::PartialParse)
    {
        if (!FrameResult.DecisionTrace.IsEmpty())
            FrameResult.DecisionTrace += TEXT(" -> ");
        FrameResult.DecisionTrace += PatternResult.DecisionTrace;
        return FrameResult;
    }

    return PatternResult;
}
