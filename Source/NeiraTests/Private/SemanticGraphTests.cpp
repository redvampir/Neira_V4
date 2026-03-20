// SemanticGraphTests.cpp
// Тесты для FSemanticGraph (v0.1)
//
// Все тесты используют in-memory SQLite (":memory:"), не требуют файла на диске.
// Запуск: make run (из Source/Tests/)

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FSemanticGraph.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ===========================================================================
// Загрузка и базовое состояние
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_NotLoadedByDefault,
    "Neira.SemanticGraph.NotLoadedByDefault",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_NotLoadedByDefault::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    TestFalse(TEXT("IsLoaded → false без Load()"), G.IsLoaded());
    TestEqual(TEXT("GetWordCount → 0"), G.GetWordCount(), 0);
    TestEqual(TEXT("GetRelationCount → 0"), G.GetRelationCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_LoadMemory,
    "Neira.SemanticGraph.LoadMemory_Succeeds",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_LoadMemory::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    bool Ok = G.Load(TEXT(":memory:"));
    TestTrue(TEXT("Load(':memory:') → true"), Ok);
    TestTrue(TEXT("IsLoaded() → true"), G.IsLoaded());
    TestEqual(TEXT("WordCount начально 0"), G.GetWordCount(), 0);
    TestEqual(TEXT("RelationCount начально 0"), G.GetRelationCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_LoadTwice,
    "Neira.SemanticGraph.LoadTwice_Idempotent",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_LoadTwice::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    bool SecondLoad = G.Load(TEXT(":memory:"));
    TestTrue(TEXT("Повторный Load() → true (idempotent)"), SecondLoad);
    return true;
}

// ===========================================================================
// AddWord / GetWordCount
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_AddWord,
    "Neira.SemanticGraph.AddWord_IncreasesCount",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_AddWord::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));

    bool Added = G.AddWord(TEXT("кот"), TEXT("Noun"));
    TestTrue(TEXT("AddWord 'кот' → true"), Added);
    TestEqual(TEXT("WordCount → 1"), G.GetWordCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_AddWordDuplicate,
    "Neira.SemanticGraph.AddWord_DuplicateIgnored",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_AddWordDuplicate::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));

    G.AddWord(TEXT("кот"));
    bool Second = G.AddWord(TEXT("кот"));
    TestFalse(TEXT("Повторный AddWord → false"), Second);
    TestEqual(TEXT("WordCount = 1 (дубль не считается)"), G.GetWordCount(), 1);
    return true;
}

// ===========================================================================
// Синонимы
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_Synonyms,
    "Neira.SemanticGraph.GetSynonyms_ReturnsMapped",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_Synonyms::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("кот"), TEXT("кошка"), ERelationType::Synonym);
    G.AddRelation(TEXT("кот"), TEXT("котик"), ERelationType::Synonym);

    TArray<FString> Syns = G.GetSynonyms(TEXT("кот"));
    TestEqual(TEXT("GetSynonyms → 2 результата"), Syns.Num(), 2);
    TestTrue(TEXT("содержит 'кошка'"), Syns.Contains(FString(TEXT("кошка"))));
    TestTrue(TEXT("содержит 'котик'"), Syns.Contains(FString(TEXT("котик"))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_SynonymsEmptyForUnknown,
    "Neira.SemanticGraph.GetSynonyms_EmptyForUnknownWord",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_SynonymsEmptyForUnknown::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));

    TArray<FString> Syns = G.GetSynonyms(TEXT("несуществующее_слово_xyz"));
    TestEqual(TEXT("GetSynonyms неизвестного → 0"), Syns.Num(), 0);
    return true;
}

// ===========================================================================
// Антонимы
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_Antonyms,
    "Neira.SemanticGraph.GetAntonyms_ReturnsMapped",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_Antonyms::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("горячий"), TEXT("холодный"), ERelationType::Antonym, 0.95f);

    TArray<FString> Ants = G.GetAntonyms(TEXT("горячий"));
    TestEqual(TEXT("GetAntonyms → 1"), Ants.Num(), 1);
    TestTrue(TEXT("содержит 'холодный'"), Ants.Contains(FString(TEXT("холодный"))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_SynonymNotAntonym,
    "Neira.SemanticGraph.Synonym_NotReturnedAsAntonym",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_SynonymNotAntonym::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("кот"), TEXT("кошка"),   ERelationType::Synonym);
    G.AddRelation(TEXT("кот"), TEXT("холодный"), ERelationType::Antonym);

    TArray<FString> Ants = G.GetAntonyms(TEXT("кот"));
    TestEqual(TEXT("GetAntonyms → только антонимы"), Ants.Num(), 1);
    TestFalse(TEXT("синоним не в антонимах"), Ants.Contains(FString(TEXT("кошка"))));
    return true;
}

// ===========================================================================
// Гиперонимы / Гипонимы
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_Hypernyms,
    "Neira.SemanticGraph.GetHypernyms_IsA",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_Hypernyms::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("кот"),      TEXT("животное"), ERelationType::Hypernym);
    G.AddRelation(TEXT("животное"), TEXT("кот"),      ERelationType::Hyponym);

    TArray<FString> Hyper = G.GetHypernyms(TEXT("кот"));
    TestEqual(TEXT("GetHypernyms кот → 1"), Hyper.Num(), 1);
    TestTrue(TEXT("содержит 'животное'"), Hyper.Contains(FString(TEXT("животное"))));

    TArray<FString> Hypo = G.GetHyponyms(TEXT("животное"));
    TestEqual(TEXT("GetHyponyms животное → 1"), Hypo.Num(), 1);
    TestTrue(TEXT("содержит 'кот'"), Hypo.Contains(FString(TEXT("кот"))));
    return true;
}

// ===========================================================================
// Меронимы
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_Meronyms,
    "Neira.SemanticGraph.GetMeronyms_HasPart",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_Meronyms::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("кот"), TEXT("лапа"),  ERelationType::Meronym);
    G.AddRelation(TEXT("кот"), TEXT("хвост"), ERelationType::Meronym);

    TArray<FString> Parts = G.GetMeronyms(TEXT("кот"));
    TestEqual(TEXT("GetMeronyms → 2"), Parts.Num(), 2);
    TestTrue(TEXT("содержит 'лапа'"),  Parts.Contains(FString(TEXT("лапа"))));
    TestTrue(TEXT("содержит 'хвост'"), Parts.Contains(FString(TEXT("хвост"))));
    return true;
}

// ===========================================================================
// GetRelations — все связи сразу
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_GetAllRelations,
    "Neira.SemanticGraph.GetRelations_AllTypes",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_GetAllRelations::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("кот"), TEXT("кошка"),   ERelationType::Synonym);
    G.AddRelation(TEXT("кот"), TEXT("животное"), ERelationType::Hypernym);
    G.AddRelation(TEXT("кот"), TEXT("лапа"),     ERelationType::Meronym);

    TArray<FSemanticRelation> Rels = G.GetRelations(TEXT("кот"));
    TestEqual(TEXT("GetRelations → 3"), Rels.Num(), 3);

    bool HasSyn = false, HasHyper = false, HasMero = false;
    for (const auto& R : Rels)
    {
        if (R.Type == ERelationType::Synonym  && R.TargetLemma == TEXT("кошка"))   HasSyn   = true;
        if (R.Type == ERelationType::Hypernym && R.TargetLemma == TEXT("животное")) HasHyper = true;
        if (R.Type == ERelationType::Meronym  && R.TargetLemma == TEXT("лапа"))     HasMero  = true;
    }
    TestTrue(TEXT("содержит Synonym→кошка"),     HasSyn);
    TestTrue(TEXT("содержит Hypernym→животное"),  HasHyper);
    TestTrue(TEXT("содержит Meronym→лапа"),       HasMero);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_GetRelationsEmpty,
    "Neira.SemanticGraph.GetRelations_EmptyForNoData",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_GetRelationsEmpty::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));

    TArray<FSemanticRelation> Rels = G.GetRelations(TEXT("несуществующее"));
    TestEqual(TEXT("GetRelations пустого → 0"), Rels.Num(), 0);
    return true;
}

// ===========================================================================
// AreRelated
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_AreRelated_True,
    "Neira.SemanticGraph.AreRelated_FindsDirectLink",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_AreRelated_True::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("горячий"), TEXT("холодный"), ERelationType::Antonym);

    ERelationType OutType = ERelationType::Unknown;
    bool Found = G.AreRelated(TEXT("горячий"), TEXT("холодный"), OutType);
    TestTrue(TEXT("AreRelated → true"), Found);
    TestEqual(TEXT("OutType → Antonym"),
              OutType, ERelationType::Antonym);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_AreRelated_False,
    "Neira.SemanticGraph.AreRelated_FalseForNoLink",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_AreRelated_False::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("кот"), TEXT("кошка"), ERelationType::Synonym);

    ERelationType OutType = ERelationType::Unknown;
    bool Found = G.AreRelated(TEXT("кот"), TEXT("собака"), OutType);
    TestFalse(TEXT("AreRelated несвязанных → false"), Found);
    TestEqual(TEXT("OutType остаётся Unknown"),
              OutType, ERelationType::Unknown);
    return true;
}

// ===========================================================================
// RelationCount / дубли
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_NoDuplicateRelations,
    "Neira.SemanticGraph.AddRelation_NoDuplicates",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_NoDuplicateRelations::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));

    G.AddRelation(TEXT("кот"), TEXT("кошка"), ERelationType::Synonym);
    bool Dup = G.AddRelation(TEXT("кот"), TEXT("кошка"), ERelationType::Synonym);
    TestFalse(TEXT("Повторная связь → false"), Dup);
    TestEqual(TEXT("RelationCount = 1 (дубль не добавлен)"), G.GetRelationCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_RelationCount,
    "Neira.SemanticGraph.RelationCount_Tracks",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_RelationCount::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    TestEqual(TEXT("начально 0"), G.GetRelationCount(), 0);

    G.AddRelation(TEXT("кот"),     TEXT("кошка"),   ERelationType::Synonym);
    TestEqual(TEXT("после 1"),     G.GetRelationCount(), 1);

    G.AddRelation(TEXT("горячий"), TEXT("холодный"), ERelationType::Antonym);
    TestEqual(TEXT("после 2"),     G.GetRelationCount(), 2);

    G.AddRelation(TEXT("кот"),     TEXT("животное"), ERelationType::Hypernym);
    TestEqual(TEXT("после 3"),     G.GetRelationCount(), 3);
    return true;
}

// ===========================================================================
// Граф без Load() — всё возвращает пустое, не крашится
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_UnloadedSafe,
    "Neira.SemanticGraph.Unloaded_SafeQueries",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_UnloadedSafe::RunTest(const FString& Parameters)
{
    FSemanticGraph G;  // нет Load()

    TestEqual(TEXT("GetSynonyms без Load → 0"),  G.GetSynonyms(TEXT("кот")).Num(),  0);
    TestEqual(TEXT("GetAntonyms без Load → 0"),  G.GetAntonyms(TEXT("кот")).Num(),  0);
    TestEqual(TEXT("GetHypernyms без Load → 0"), G.GetHypernyms(TEXT("кот")).Num(), 0);
    TestEqual(TEXT("GetRelations без Load → 0"), G.GetRelations(TEXT("кот")).Num(), 0);

    ERelationType T = ERelationType::Unknown;
    TestFalse(TEXT("AreRelated без Load → false"), G.AreRelated(TEXT("a"), TEXT("b"), T));
    TestFalse(TEXT("AddRelation без Load → false"),
              G.AddRelation(TEXT("a"), TEXT("b"), ERelationType::Synonym));
    return true;
}

// ===========================================================================
// Веса — сортировка по убыванию
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemanticGraph_WeightOrdering,
    "Neira.SemanticGraph.GetRelations_OrderedByWeightDesc",
    NEIRA_TEST_FLAGS)
bool FSemanticGraph_WeightOrdering::RunTest(const FString& Parameters)
{
    FSemanticGraph G;
    G.Load(TEXT(":memory:"));
    G.AddRelation(TEXT("слово"), TEXT("лексема"),  ERelationType::Synonym, 0.7f);
    G.AddRelation(TEXT("слово"), TEXT("термин"),   ERelationType::Synonym, 1.0f);
    G.AddRelation(TEXT("слово"), TEXT("выражение"), ERelationType::Synonym, 0.5f);

    TArray<FSemanticRelation> Rels = G.GetRelations(TEXT("слово"));
    TestEqual(TEXT("3 связи"), Rels.Num(), 3);
    TestTrue(TEXT("первый — наибольший вес"),  Rels[0].Weight >= Rels[1].Weight);
    TestTrue(TEXT("второй — не меньше третьего"), Rels[1].Weight >= Rels[2].Weight);
    return true;
}
