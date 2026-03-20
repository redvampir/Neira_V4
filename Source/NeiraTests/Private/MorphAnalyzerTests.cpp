// MorphAnalyzerTests.cpp
// Тесты для FMorphAnalyzer (v0.2) и обновлённого FHypothesisStore (v0.2)
//
// Запуск: Unreal Automation Tool → фильтр "Neira.MorphAnalyzer" / "Neira.HypothesisStoreV2"

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FMorphAnalyzer.h"
#include "FHypothesisStore.h"
#include <cstdio>

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

namespace
{
    bool FixtureFileExists(const FString& Path)
    {
        FILE* File = std::fopen(TCHAR_TO_ANSI(*Path), "rb");
        if (!File)
        {
            return false;
        }

        std::fclose(File);
        return true;
    }

    FString ResolveExternalDictionaryFixturePath()
    {
        const TArray<FString> Candidates = {
            TEXT("Data/Dictionaries/opencorpora_dict.json"),
            TEXT("../Data/Dictionaries/opencorpora_dict.json"),
            TEXT("../../Data/Dictionaries/opencorpora_dict.json"),
            TEXT("../../../Data/Dictionaries/opencorpora_dict.json")
        };

        for (const FString& Candidate : Candidates)
        {
            if (FixtureFileExists(Candidate))
            {
                return Candidate;
            }
        }

        return FString();
    }
}

// ===========================================================================
// FMorphAnalyzer — словарный путь
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_DictVerb,
    "Neira.MorphAnalyzer.Dict_KnownVerb_ReturnsVerb",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_DictVerb::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("открой"));
    TestEqual  (TEXT("POS → Verb"),      R.PartOfSpeech, EPosTag::Verb);
    TestEqual  (TEXT("Lemma → открыть"), R.Lemma,        FString(TEXT("открыть")));
    TestEqual  (TEXT("Source → dict"),   R.Source,       FString(TEXT("dict")));
    TestTrue   (TEXT("Confidence ≥ 0.9"),R.Confidence >= 0.9f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_DictNoun,
    "Neira.MorphAnalyzer.Dict_KnownNoun_ReturnsNoun",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_DictNoun::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("кот"));
    TestEqual(TEXT("POS → Noun"),    R.PartOfSpeech, EPosTag::Noun);
    TestEqual(TEXT("Lemma → кот"),   R.Lemma,        FString(TEXT("кот")));
    TestEqual(TEXT("Source → dict"), R.Source,       FString(TEXT("dict")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_DictPronoun,
    "Neira.MorphAnalyzer.Dict_KnownPronoun_ReturnsPronoun",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_DictPronoun::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("ты"));
    TestEqual(TEXT("POS → Pronoun"), R.PartOfSpeech, EPosTag::Pronoun);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_DictPreposition,
    "Neira.MorphAnalyzer.Dict_KnownPreposition_ReturnsPreposition",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_DictPreposition::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("для"));
    TestEqual(TEXT("POS → Preposition"), R.PartOfSpeech, EPosTag::Preposition);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_CaseInsensitive,
    "Neira.MorphAnalyzer.Dict_UpperCase_Normalised",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_CaseInsensitive::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    // «Открой» с заглавной — словарь хранит нижний регистр
    FMorphResult R = A.Analyze(TEXT("Открой"));
    TestEqual(TEXT("POS → Verb (регистронезависимо)"), R.PartOfSpeech, EPosTag::Verb);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_DictInflected,
    "Neira.MorphAnalyzer.Dict_InflectedForm_ReturnsBaseLemma",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_DictInflected::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    // «котом» — форма в словаре, лемма должна быть «кот»
    FMorphResult R = A.Analyze(TEXT("котом"));
    TestEqual(TEXT("Lemma → кот"),  R.Lemma, FString(TEXT("кот")));
    TestEqual(TEXT("POS → Noun"),   R.PartOfSpeech, EPosTag::Noun);
    return true;
}

// ===========================================================================
// FMorphAnalyzer — суффиксный путь
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_SuffixInfinitive,
    "Neira.MorphAnalyzer.Suffix_Infinitive_ать_ReturnsVerb",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_SuffixInfinitive::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    // При наличии внешнего словаря приоритет может перейти с suffix на ext_dict.
    FMorphResult R = A.Analyze(TEXT("программировать"));
    TestEqual  (TEXT("POS → Verb"),       R.PartOfSpeech, EPosTag::Verb);
    TestTrue   (TEXT("Source → suffix | ext_dict"),
        R.Source == TEXT("suffix") || R.Source == TEXT("ext_dict"));
    if (R.Source == TEXT("ext_dict"))
    {
        TestTrue(TEXT("ext_dict confidence >= 0.9"), R.Confidence >= 0.9f);
    }
    else
    {
        TestTrue(TEXT("suffix confidence < 0.9"), R.Confidence < 0.9f);
        TestTrue(TEXT("suffix confidence > 0.5"), R.Confidence > 0.5f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_SuffixNoun_ость,
    "Neira.MorphAnalyzer.Suffix_ость_ReturnsNoun",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_SuffixNoun_ость::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("скорость"));
    TestEqual(TEXT("POS → Noun"),      R.PartOfSpeech, EPosTag::Noun);
    TestTrue(TEXT("Source → suffix | ext_dict"),
        R.Source == TEXT("suffix") || R.Source == TEXT("ext_dict"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_SuffixAdjective,
    "Neira.MorphAnalyzer.Suffix_ский_ReturnsAdjective",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_SuffixAdjective::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("лингвистический"));
    TestEqual(TEXT("POS → Adjective"), R.PartOfSpeech, EPosTag::Adjective);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_SuffixNoun_ение,
    "Neira.MorphAnalyzer.Suffix_ение_ReturnsNoun",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_SuffixNoun_ение::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("объяснение"));
    TestEqual(TEXT("POS → Noun"), R.PartOfSpeech, EPosTag::Noun);
    return true;
}

// ===========================================================================
// FMorphAnalyzer — граничные случаи
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_Unknown,
    "Neira.MorphAnalyzer.UnknownWord_ReturnsUnknown",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_Unknown::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("вжух"));
    TestEqual(TEXT("POS → Unknown"),      R.PartOfSpeech, EPosTag::Unknown);
    TestEqual(TEXT("Source → unknown"),   R.Source,       FString(TEXT("unknown")));
    TestTrue (TEXT("Confidence < 0.5"),   R.Confidence < 0.5f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_PhraseAnalysis,
    "Neira.MorphAnalyzer.Phrase_TokenizedCorrectly",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_PhraseAnalysis::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    TArray<FMorphResult> Results = A.AnalyzePhrase(TEXT("открой окно"));
    TestEqual(TEXT("2 токена"), Results.Num(), 2);
    TestEqual(TEXT("токен 0 → Verb"), Results[0].PartOfSpeech, EPosTag::Verb);
    TestEqual(TEXT("токен 1 → Noun"), Results[1].PartOfSpeech, EPosTag::Noun);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_StripQuestionMark,
    "Neira.MorphAnalyzer.Phrase_QuestionMark_Stripped",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_StripQuestionMark::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    // «кот?» — знак вопроса должен быть удалён перед анализом
    FMorphResult R = A.Analyze(TEXT("кот?"));
    TestEqual(TEXT("POS → Noun (без '?')"), R.PartOfSpeech, EPosTag::Noun);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_EmptyString,
    "Neira.MorphAnalyzer.EmptyString_ReturnsUnknown",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_EmptyString::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT(""));
    TestEqual(TEXT("Пустая строка → POS Unknown"), R.PartOfSpeech, EPosTag::Unknown);
    TestTrue(TEXT("Пустая строка → Confidence < 0.5"), R.Confidence < 0.5f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_LatinWord,
    "Neira.MorphAnalyzer.LatinWord_ReturnsUnknown",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_LatinWord::RunTest(const FString& Parameters)
{
    // Словарь и суффиксы ориентированы на кириллицу
    FMorphAnalyzer A;
    FMorphResult R = A.Analyze(TEXT("hello"));
    TestEqual(TEXT("Латинское слово → POS Unknown"), R.PartOfSpeech, EPosTag::Unknown);
    return true;
}

// ===========================================================================
// Доменные пакеты v0.4: action/text-diagnostics/memory-knowledge
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_DomainPackages_LemmasAndForms,
    "Neira.MorphAnalyzer.DomainPackages.LemmasAndFrequentForms",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_DomainPackages_LemmasAndForms::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;

    // action_commands
    FMorphResult CheckVerb = A.Analyze(TEXT("проверь"));
    TestEqual(TEXT("action_commands: 'проверь' → lemma=проверить"),
        CheckVerb.Lemma, FString(TEXT("проверить")));

    FMorphResult OpenInf = A.Analyze(TEXT("открыть"));
    TestEqual(TEXT("action_commands: 'открыть' → POS Verb"),
        OpenInf.PartOfSpeech, EPosTag::Verb);

    // text_diagnostics
    FMorphResult ExplainVerb = A.Analyze(TEXT("объясни"));
    TestEqual(TEXT("text_diagnostics: 'объясни' → lemma=объяснить"),
        ExplainVerb.Lemma, FString(TEXT("объяснить")));

    FMorphResult TextGenitive = A.Analyze(TEXT("текста"));
    TestEqual(TEXT("text_diagnostics: 'текста' → lemma=текст"),
        TextGenitive.Lemma, FString(TEXT("текст")));

    // memory_knowledge
    FMorphResult MemoryGenitive = A.Analyze(TEXT("памяти"));
    TestEqual(TEXT("memory_knowledge: 'памяти' → lemma=память"),
        MemoryGenitive.Lemma, FString(TEXT("память")));

    FMorphResult TermGenitive = A.Analyze(TEXT("термина"));
    TestEqual(TEXT("memory_knowledge: 'термина' → POS Noun"),
        TermGenitive.PartOfSpeech, EPosTag::Noun);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_DomainPackages_BoundaryTokens,
    "Neira.MorphAnalyzer.DomainPackages.BoundaryTokens",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_DomainPackages_BoundaryTokens::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;

    FMorphResult WhatToken = A.Analyze(TEXT("что"));
    TestEqual(TEXT("boundary: 'что' → POS Conjunction"),
        WhatToken.PartOfSpeech, EPosTag::Conjunction);

    FMorphResult HowToken = A.Analyze(TEXT("как"));
    TestEqual(TEXT("boundary: 'как' → POS Conjunction"),
        HowToken.PartOfSpeech, EPosTag::Conjunction);

    FMorphResult MeaningToken = A.Analyze(TEXT("значение"));
    TestEqual(TEXT("boundary: 'значение' → POS Noun"),
        MeaningToken.PartOfSpeech, EPosTag::Noun);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMorphAnalyzer_ExternalDictionary_AutoLoadAndLookup,
    "Neira.MorphAnalyzer.ExternalDictionary.AutoLoadAndLookup",
    NEIRA_TEST_FLAGS)
bool FMorphAnalyzer_ExternalDictionary_AutoLoadAndLookup::RunTest(const FString& Parameters)
{
    FMorphAnalyzer A;
    const FString FixturePath = ResolveExternalDictionaryFixturePath();

    if (FixturePath.IsEmpty())
    {
        std::printf("[SKIP] Neira.MorphAnalyzer.ExternalDictionary.AutoLoadAndLookup: "
                    "fixture Data/Dictionaries/opencorpora_dict.json не найден; "
                    "тест пропущен по контракту.\n");
        return true;
    }

    TestTrue(TEXT("Fixture должен успешно загрузиться"),
        A.LoadExternalDictionary(FixturePath));

    FMorphResult R = A.Analyze(TEXT("абажур"));
    TestEqual(TEXT("external word: POS → Noun"),
        R.PartOfSpeech, EPosTag::Noun);
    TestEqual(TEXT("external word: lemma → абажур"),
        R.Lemma, FString(TEXT("абажур")));
    TestEqual(TEXT("external word: source → ext_dict"),
        R.Source, FString(TEXT("ext_dict")));
    TestTrue(TEXT("external word: confidence >= 0.9"),
        R.Confidence >= 0.9f);
    TestTrue(TEXT("OpenCorpora должен быть загружен после первого внешнего lookup"),
        A.HasExternalDictionary());
    TestTrue(TEXT("Размер внешнего словаря должен быть > 0"),
        A.GetExternalDictionarySize() > 0);

    return true;
}

// ===========================================================================
// FHypothesisStore v0.2 — правила устойчивого перехода
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStoreV2_SingleConfirm_NotEligible,
    "Neira.HypothesisStoreV2.OneConfirm_NotEligibleForVerify",
    NEIRA_TEST_FLAGS)
bool FHypothesisStoreV2_SingleConfirm_NotEligible::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("кот — животное");
    int32 ID = Store.Store(H);

    Store.Confirm(ID, TEXT("источник 1"));

    TestFalse(TEXT("1 подтверждение → не готова к Verify"),
        Store.IsEligibleForVerification(ID));
    TestFalse(TEXT("Verify с 1 подтверждением → false"),
        Store.Verify(ID, TEXT("валидация")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStoreV2_TwoConfirms_Eligible,
    "Neira.HypothesisStoreV2.TwoConfirms_EligibleForVerify",
    NEIRA_TEST_FLAGS)
bool FHypothesisStoreV2_TwoConfirms_Eligible::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("кот — животное");
    int32 ID = Store.Store(H);

    Store.Confirm(ID, TEXT("источник 1"));
    Store.Confirm(ID, TEXT("источник 2"));

    TestTrue(TEXT("2 подтверждения → IsEligible"),
        Store.IsEligibleForVerification(ID));
    TestTrue(TEXT("Verify с 2 подтверждениями → true"),
        Store.Verify(ID, TEXT("валидирована")));

    const FHypothesis* Found = Store.Find(ID);
    TestNotNull(TEXT("Find() не null"), Found);
    if (Found)
    {
        TestEqual(TEXT("State → VerifiedKnowledge"),
            Found->State, EKnowledgeState::VerifiedKnowledge);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStoreV2_ConfirmCount_Increments,
    "Neira.HypothesisStoreV2.ConfirmCount_Increments",
    NEIRA_TEST_FLAGS)
bool FHypothesisStoreV2_ConfirmCount_Increments::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("тест");
    int32 ID = Store.Store(H);

    const FHypothesis* Found = Store.Find(ID);
    TestEqual(TEXT("ConfirmCount после Store = 0"), Found->ConfirmCount, 0);

    Store.Confirm(ID, TEXT("раз"));
    TestEqual(TEXT("ConfirmCount после 1 Confirm = 1"), Found->ConfirmCount, 1);

    Store.Confirm(ID, TEXT("два"));
    TestEqual(TEXT("ConfirmCount после 2 Confirm = 2"), Found->ConfirmCount, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHypothesisStoreV2_ConflictedBlocks,
    "Neira.HypothesisStoreV2.Conflicted_BlocksConfirmAndVerify",
    NEIRA_TEST_FLAGS)
bool FHypothesisStoreV2_ConflictedBlocks::RunTest(const FString& Parameters)
{
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("тест конфликт");
    int32 ID = Store.Store(H);

    Store.Confirm(ID, TEXT("источник 1"));
    Store.Confirm(ID, TEXT("источник 2"));
    Store.MarkConflicted(ID, TEXT("нашли противоречие"));

    TestFalse(TEXT("Confirm после Conflicted → false"),
        Store.Confirm(ID, TEXT("попытка")));
    TestFalse(TEXT("Verify после Conflicted → false"),
        Store.Verify(ID, TEXT("попытка")));
    return true;
}

#undef NEIRA_TEST_FLAGS
