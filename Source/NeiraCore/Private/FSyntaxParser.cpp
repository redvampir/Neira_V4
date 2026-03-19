// FSyntaxParser.cpp
// v0.3 — базовые синтаксические связи: кто → действие → объект → адресат.
//
// Алгоритм одного прохода по токенам (см. подробно в заголовке):
//
//   Шаг 1. FMorphAnalyzer.AnalyzePhrase() — массив FMorphResult.
//   Шаг 2. Найти первый Verb → Predicate. Запомнить его индекс (VerbIdx).
//   Шаг 3. Subject: первый Noun/Pronoun с индексом < VerbIdx.
//           Для команды (PhraseType=Command): субъект опускается (подразумевается «ты»).
//   Шаг 4. Object: первый Noun/Pronoun с индексом > VerbIdx
//           (исключая токены, ставшие Recipient).
//   Шаг 5. Recipient: предлог «для»/«к»/«ко» + следующий Noun/Pronoun.
//   Шаг 6. Флаги:
//     bIsAbilityCheck  — Subject.Contains("ты"/"вы") && Predicate.Contains("мочь"/"уметь")
//     bHasNestedClause — есть Conjunction "что"/"чтобы"/"если"
//     bIsNegated       — есть Particle "не"/"нельзя"
//
// Разрешение омонимии (v0.3):
//   - «что» в позиции Conjunction → Conjunction (не Noun).
//   - Токен в начале Command-фразы с Unknown или Verb POS → Predicate.
//
// Ограничения v0.3:
//   - Падежи не анализируются: порядок слов — главный сигнал.
//   - Координация («и X, и Y») → берётся первый токен.
//   - Вложенные клаузы помечаются, но не рекурсивно разбираются.

#include "FSyntaxParser.h"

namespace
{
    FString PosToString(EPosTag POS)
    {
        switch (POS)
        {
            case EPosTag::Unknown:     return TEXT("Unknown");
            case EPosTag::Noun:        return TEXT("Noun");
            case EPosTag::Verb:        return TEXT("Verb");
            case EPosTag::Adjective:   return TEXT("Adjective");
            case EPosTag::Adverb:      return TEXT("Adverb");
            case EPosTag::Pronoun:     return TEXT("Pronoun");
            case EPosTag::Preposition: return TEXT("Preposition");
            case EPosTag::Conjunction: return TEXT("Conjunction");
            case EPosTag::Particle:    return TEXT("Particle");
            case EPosTag::Numeral:     return TEXT("Numeral");
            default:                   return TEXT("Unknown");
        }
    }

    bool AddUniqueCandidate(TArray<EPosTag>& Candidates, EPosTag POS)
    {
        if (Candidates.Contains(POS))
            return false;
        Candidates.Add(POS);
        return true;
    }

    int32 PosPriority(EPosTag POS)
    {
        switch (POS)
        {
            case EPosTag::Verb:        return 100;
            case EPosTag::Noun:        return 90;
            case EPosTag::Pronoun:     return 80;
            case EPosTag::Conjunction: return 70;
            case EPosTag::Adverb:      return 60;
            case EPosTag::Adjective:   return 50;
            case EPosTag::Preposition: return 40;
            case EPosTag::Particle:    return 30;
            case EPosTag::Numeral:     return 20;
            case EPosTag::Unknown:
            default:                   return 0;
        }
    }

    // Предлоги-маркеры адресата
    bool IsRecipientPrep(const FMorphResult& Token, EPosTag POS)
    {
        const FString L = Token.Lemma.ToLower();
        return (L == TEXT("для") || L == TEXT("к") || L == TEXT("ко"))
               && POS == EPosTag::Preposition;
    }

    // Союзы, маркирующие вложенную клаузу
    bool IsNestedMarker(const FMorphResult& Token, EPosTag POS)
    {
        if (POS != EPosTag::Conjunction)
            return false;
        const FString L = Token.Lemma.ToLower();
        return L == TEXT("что") || L == TEXT("чтобы") || L == TEXT("если");
    }

    // Частицы отрицания
    bool IsNegation(const FMorphResult& Token, EPosTag POS)
    {
        if (POS != EPosTag::Particle)
            return false;
        const FString L = Token.Lemma.ToLower();
        return L == TEXT("не") || L == TEXT("нельзя") || L == TEXT("ни");
    }

    bool IsNounOrPronoun(EPosTag POS)
    {
        return POS == EPosTag::Noun || POS == EPosTag::Pronoun;
    }

    // Привести лемму или OriginalWord к нижнему регистру для сравнения
    FString Lower(const FMorphResult& R)
    {
        return R.Lemma.IsEmpty() ? R.OriginalWord.ToLower() : R.Lemma.ToLower();
    }

    EPosTag ResolveTokenPos(
        const TArray<FMorphResult>& Tokens,
        int32 TokenIndex,
        EPhraseType PhraseType,
        FSemanticFrame& Frame)
    {
        const FMorphResult& Token = Tokens[TokenIndex];
        const FString LowerToken = Lower(Token);

        TArray<EPosTag> Candidates;
        AddUniqueCandidate(Candidates, Token.PartOfSpeech);

        if (LowerToken == TEXT("что"))
        {
            AddUniqueCandidate(Candidates, EPosTag::Conjunction);
            AddUniqueCandidate(Candidates, EPosTag::Noun);
        }
        if (LowerToken == TEXT("как"))
        {
            AddUniqueCandidate(Candidates, EPosTag::Conjunction);
            AddUniqueCandidate(Candidates, EPosTag::Adverb);
        }
        if (LowerToken == TEXT("кто"))
        {
            AddUniqueCandidate(Candidates, EPosTag::Pronoun);
            AddUniqueCandidate(Candidates, EPosTag::Noun);
        }
        if (Token.PartOfSpeech == EPosTag::Unknown && PhraseType == EPhraseType::Command && TokenIndex == 0)
        {
            AddUniqueCandidate(Candidates, EPosTag::Verb);
        }

        if (Candidates.Num() <= 1)
            return Candidates[0];

        EPosTag Chosen = Candidates[0];
        FString Reason = TEXT("HighestPriorityPOS");
        FString Anchor = FString::Printf(TEXT("phrase_type=%d"), static_cast<int32>(PhraseType));
        float Confidence = FMath::Clamp(Token.Confidence * 0.70f, 0.0f, 1.0f);

        if (LowerToken == TEXT("что") || LowerToken == TEXT("чтобы") || LowerToken == TEXT("если"))
        {
            if (Candidates.Contains(EPosTag::Conjunction))
            {
                Chosen = EPosTag::Conjunction;
                Reason = TEXT("NestedClauseMarkerPriority");
                Anchor = TEXT("token=что/чтобы/если");
                Confidence = FMath::Clamp(Token.Confidence * 0.85f + 0.15f, 0.0f, 1.0f);
            }
        }
        else if (TokenIndex == 0 && PhraseType == EPhraseType::Command && Candidates.Contains(EPosTag::Verb))
        {
            Chosen = EPosTag::Verb;
            Reason = TEXT("CommandLeadingVerbPriority");
            Anchor = TEXT("command_first_token");
            Confidence = FMath::Clamp(Token.Confidence * 0.80f + 0.20f, 0.0f, 1.0f);
        }
        else
        {
            int32 BestPriority = TNumericLimits<int32>::Lowest();
            for (EPosTag Candidate : Candidates)
            {
                const int32 CandidatePriority = PosPriority(Candidate);
                if (CandidatePriority > BestPriority)
                {
                    BestPriority = CandidatePriority;
                    Chosen = Candidate;
                }
            }
            Reason = TEXT("StaticPOSPriorityRule");
            Anchor = FString::Printf(TEXT("priority=%d"), PosPriority(Chosen));
        }

        FSemanticFrame::FAmbiguousDecisionTrace Trace;
        Trace.Token = Token.OriginalWord.IsEmpty() ? LowerToken : Token.OriginalWord;
        Trace.TokenIndex = TokenIndex;
        for (EPosTag Candidate : Candidates)
        {
            Trace.CandidatePOS.Add(PosToString(Candidate));
        }
        Trace.SelectedPOS = PosToString(Chosen);
        Trace.Confidence = Confidence;
        Trace.Reason = Reason;
        Trace.Anchor = Anchor;
        Frame.AmbiguityTrace.Add(MoveTemp(Trace));

        return Chosen;
    }
}

FSemanticFrame FSyntaxParser::Parse(const FString& Phrase, EPhraseType PhraseType) const
{
    FSemanticFrame Frame;

    TArray<FMorphResult> Tokens = MorphAnalyzer.AnalyzePhrase(Phrase);
    if (Tokens.IsEmpty())
        return Frame;

    TArray<EPosTag> ResolvedPos;
    ResolvedPos.Reserve(Tokens.Num());
    for (int32 i = 0; i < Tokens.Num(); ++i)
    {
        ResolvedPos.Add(ResolveTokenPos(Tokens, i, PhraseType, Frame));
    }

    // Шаг 1: найти индекс первого глагола (Predicate)
    int32 VerbIdx = INDEX_NONE;
    for (int32 i = 0; i < Tokens.Num(); ++i)
    {
        if (ResolvedPos[i] == EPosTag::Verb)
        {
            VerbIdx   = i;
            Frame.Predicate = Tokens[i].Lemma.IsEmpty()
                              ? Tokens[i].OriginalWord : Tokens[i].Lemma;
            break;
        }
    }

    // Если глагол не найден, но фраза — команда: первый токен → Predicate
    if (VerbIdx == INDEX_NONE && PhraseType == EPhraseType::Command && !Tokens.IsEmpty())
    {
        VerbIdx = 0;
        Frame.Predicate = Tokens[0].OriginalWord.ToLower();
    }

    // Шаг 2: Subject — первый Noun/Pronoun до глагола
    for (int32 i = 0; i < VerbIdx && i < Tokens.Num(); ++i)
    {
        if (IsNounOrPronoun(ResolvedPos[i]))
        {
            Frame.Subject = Tokens[i].Lemma.IsEmpty()
                            ? Tokens[i].OriginalWord : Tokens[i].Lemma;
            break;
        }
    }

    // Шаг 3: Recipient — предлог «для»/«к» + следующий Noun/Pronoun
    TSet<int32> RecipientIndices;
    for (int32 i = VerbIdx + 1; i < Tokens.Num() - 1; ++i)
    {
        if (IsRecipientPrep(Tokens[i], ResolvedPos[i]) && IsNounOrPronoun(ResolvedPos[i + 1]))
        {
            if (Frame.Recipient.IsEmpty())
            {
                Frame.Recipient = Tokens[i + 1].Lemma.IsEmpty()
                                  ? Tokens[i + 1].OriginalWord : Tokens[i + 1].Lemma;
            }
            RecipientIndices.Add(i);
            RecipientIndices.Add(i + 1);
        }
    }

    // Шаг 4: Object — первый Noun/Pronoun после глагола, не вошедший в Recipient
    for (int32 i = VerbIdx + 1; i < Tokens.Num(); ++i)
    {
        if (RecipientIndices.Contains(i))
            continue;
        if (IsNounOrPronoun(ResolvedPos[i]))
        {
            Frame.Object = Tokens[i].Lemma.IsEmpty()
                           ? Tokens[i].OriginalWord : Tokens[i].Lemma;
            break;
        }
    }

    // Шаг 5: Флаги
    for (int32 i = 0; i < Tokens.Num(); ++i)
    {
        if (IsNestedMarker(Tokens[i], ResolvedPos[i]))
            Frame.bHasNestedClause = true;

        if (IsNegation(Tokens[i], ResolvedPos[i]))
            Frame.bIsNegated = true;
    }

    // bIsAbilityCheck: «ты/вы» в субъекте + «мочь/уметь» в предикате
    {
        const FString SubjL = Frame.Subject.ToLower();
        const FString PredL = Frame.Predicate.ToLower();
        if ((SubjL == TEXT("ты") || SubjL == TEXT("вы")) &&
            (PredL == TEXT("мочь") || PredL == TEXT("уметь") ||
             PredL == TEXT("можешь") || PredL == TEXT("можете") ||
             PredL == TEXT("умеешь") || PredL == TEXT("умеете")))
        {
            Frame.bIsAbilityCheck = true;
        }
        // Также: фраза начинается с «ты можешь» без явного Subject (вопрос)
        if (Frame.Subject.IsEmpty() && Tokens.Num() >= 2)
        {
            const FString T0 = Lower(Tokens[0]);
            const FString T1 = Lower(Tokens[1]);
            if ((T0 == TEXT("ты") || T0 == TEXT("вы")) &&
                (T1 == TEXT("мочь") || T1 == TEXT("уметь") ||
                 T1 == TEXT("можешь") || T1 == TEXT("можете")))
            {
                Frame.bIsAbilityCheck = true;
                Frame.Subject    = T0;
                Frame.Predicate  = T1;
            }
        }
    }

    return Frame;
}
