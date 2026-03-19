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
    // Предлоги-маркеры адресата
    bool IsRecipientPrep(const FMorphResult& Token)
    {
        const FString L = Token.Lemma.ToLower();
        return (L == TEXT("для") || L == TEXT("к") || L == TEXT("ко"))
               && Token.PartOfSpeech == EPosTag::Preposition;
    }

    // Союзы, маркирующие вложенную клаузу
    bool IsNestedMarker(const FMorphResult& Token)
    {
        if (Token.PartOfSpeech != EPosTag::Conjunction)
            return false;
        const FString L = Token.Lemma.ToLower();
        return L == TEXT("что") || L == TEXT("чтобы") || L == TEXT("если");
    }

    // Частицы отрицания
    bool IsNegation(const FMorphResult& Token)
    {
        if (Token.PartOfSpeech != EPosTag::Particle)
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
}

FSemanticFrame FSyntaxParser::Parse(const FString& Phrase, EPhraseType PhraseType) const
{
    FSemanticFrame Frame;

    TArray<FMorphResult> Tokens = MorphAnalyzer.AnalyzePhrase(Phrase);
    if (Tokens.IsEmpty())
        return Frame;

    // Шаг 1: найти индекс первого глагола (Predicate)
    int32 VerbIdx = INDEX_NONE;
    for (int32 i = 0; i < Tokens.Num(); ++i)
    {
        if (Tokens[i].PartOfSpeech == EPosTag::Verb)
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
        if (IsNounOrPronoun(Tokens[i].PartOfSpeech))
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
        if (IsRecipientPrep(Tokens[i]) && IsNounOrPronoun(Tokens[i + 1].PartOfSpeech))
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
        if (IsNounOrPronoun(Tokens[i].PartOfSpeech))
        {
            Frame.Object = Tokens[i].Lemma.IsEmpty()
                           ? Tokens[i].OriginalWord : Tokens[i].Lemma;
            break;
        }
    }

    // Шаг 5: Флаги
    for (const FMorphResult& T : Tokens)
    {
        if (IsNestedMarker(T))
            Frame.bHasNestedClause = true;

        if (IsNegation(T))
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
