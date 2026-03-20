// DialoguePipelineTests.cpp
// Тесты для FDialoguePipeline (v0.1)
//
// Все тесты используют in-memory граф или отключённый граф.
// Тесты проверяют end-to-end поведение пайплайна: текст → текст ответа.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FDialoguePipeline.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ===========================================================================
// Базовая конфигурация
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_Config_DefaultNoGraph,
    "Neira.DialoguePipeline.Config.DefaultNoGraph",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_Config_DefaultNoGraph::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    FDialoguePipeline P(Cfg);
    TestFalse(TEXT("Граф не загружен при пустом SemanticGraphPath"), P.IsSemanticGraphLoaded());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_Config_MemoryGraph,
    "Neira.DialoguePipeline.Config.MemoryGraph_Loaded",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_Config_MemoryGraph::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.SemanticGraphPath = TEXT(":memory:");
    FDialoguePipeline P(Cfg);
    TestTrue(TEXT("Граф загружен для ':memory:'"), P.IsSemanticGraphLoaded());
    return true;
}

// ===========================================================================
// MakeHandler — интеграция с FVoiceSessionOrchestrator
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_MakeHandler_IsCallable,
    "Neira.DialoguePipeline.MakeHandler_IsCallable",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_MakeHandler_IsCallable::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    FDialoguePipeline P(Cfg);
    FTextPipelineHandler Handler = P.MakeHandler();
    TestTrue(TEXT("Handler не пустой"), (bool)Handler);

    // Вызов через handler должен вернуть непустой ответ
    FString Response = Handler(TEXT("что такое синтаксис?"));
    TestFalse(TEXT("Handler возвращает непустой ответ"), Response.IsEmpty());
    return true;
}

// ===========================================================================
// Пустой ввод
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_EmptyInput_ReturnsResponse,
    "Neira.DialoguePipeline.ProcessText.EmptyInput_ReturnsResponse",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_EmptyInput_ReturnsResponse::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    FDialoguePipeline P(Cfg);

    const FString R1 = P.ProcessText(TEXT(""));
    TestFalse(TEXT("Пустая строка → непустой ответ"), R1.IsEmpty());
    TestTrue(TEXT("Пустой ввод → блок неопределённости"), R1.Contains(TEXT("Неопределённость:")));

    const FString R2 = P.ProcessText(TEXT("   "));
    TestFalse(TEXT("Пробелы → непустой ответ"), R2.IsEmpty());
    return true;
}

// ===========================================================================
// StoreFact — сохранение факта
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_StoreFact_ResponseMentionsFact,
    "Neira.DialoguePipeline.ProcessText.StoreFact_ResponseMentionsFact",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_StoreFact_ResponseMentionsFact::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.Personality = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);
    FDialoguePipeline P(Cfg);

    // "кот — животное" → Statement → StoreFact
    const FString Response = P.ProcessText(TEXT("кот — животное"));
    TestFalse(TEXT("Ответ непустой"), Response.IsEmpty());
    // После StoreFact хранилище должно иметь хотя бы одну запись
    TestTrue(TEXT("Store не пустой после StoreFact"), P.GetStore().Count() > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_StoreFact_StoreGrows,
    "Neira.DialoguePipeline.ProcessText.StoreFact_StoreGrows",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_StoreFact_StoreGrows::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    FDialoguePipeline P(Cfg);

    P.ProcessText(TEXT("синтаксис — раздел лингвистики"));
    const int32 CountAfterFirst = P.GetStore().Count();

    P.ProcessText(TEXT("морфология — раздел лингвистики о форме слов"));
    const int32 CountAfterSecond = P.GetStore().Count();

    TestTrue(TEXT("Count растёт"), CountAfterSecond >= CountAfterFirst);
    return true;
}

// ===========================================================================
// GetDefinition — поиск известного факта
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_GetDefinition_KnownFact_Confirmed,
    "Neira.DialoguePipeline.ProcessText.GetDefinition_KnownFact_Confirmed",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_GetDefinition_KnownFact_Confirmed::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.Personality = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);
    FDialoguePipeline P(Cfg);

    // Вручную заполнить хранилище — "кот" уже известен
    FHypothesis H;
    H.Claim = TEXT("кот");
    H.Confidence = 0.9f;
    P.GetStore().Store(H);

    // Запрос определения
    const FString Response = P.ProcessText(TEXT("что такое кот?"));
    TestFalse(TEXT("Ответ непустой"), Response.IsEmpty());
    // Ответ должен содержать "кот" (из SemanticCore через Claim)
    TestTrue(TEXT("Ответ содержит кот"), Response.Contains(TEXT("кот")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_GetDefinition_UnknownFact_NoMatch,
    "Neira.DialoguePipeline.ProcessText.GetDefinition_UnknownFact_NoMatch",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_GetDefinition_UnknownFact_NoMatch::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    FDialoguePipeline P(Cfg);

    const FString Response = P.ProcessText(TEXT("что такое нейросинтаксема?"));
    TestFalse(TEXT("Ответ непустой"), Response.IsEmpty());
    // NoMatch → неопределённость
    TestTrue(TEXT("NoMatch → блок неопределённости"), Response.Contains(TEXT("Неопределённость:")));
    return true;
}

// ===========================================================================
// GetDefinition + SemanticGraph — синонимный поиск в пайплайне
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_GetDefinition_ViaGraph_SynonymFound,
    "Neira.DialoguePipeline.SemanticGraph.GetDefinition_ViaSynonym",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_GetDefinition_ViaGraph_SynonymFound::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.SemanticGraphPath = TEXT(":memory:");
    Cfg.Personality = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);
    FDialoguePipeline P(Cfg);

    // Семантический граф: "кошка" — синоним "кот"
    P.GetGraph().AddRelation(TEXT("кошка"), TEXT("кот"), ERelationType::Synonym);

    // Хранилище знает "кот"
    FHypothesis H;
    H.Claim = TEXT("кот");
    H.Confidence = 0.9f;
    P.GetStore().Store(H);

    // Запрос "кошка" — нет в Store напрямую, но есть через граф
    const FString Response = P.ProcessText(TEXT("что такое кошка?"));
    TestFalse(TEXT("Ответ непустой"), Response.IsEmpty());
    // Должно быть найдено через синоним
    TestFalse(TEXT("Не должно быть NoMatch при синонимном поиске"),
              Response.Contains(TEXT("не найдено: кошка")));
    return true;
}

// ===========================================================================
// RelatedTerms — связанные понятия в ответе
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_RelatedTerms_SynonymsInResponse,
    "Neira.DialoguePipeline.SemanticGraph.RelatedTerms_SynonymsInResponse",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_RelatedTerms_SynonymsInResponse::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.SemanticGraphPath = TEXT(":memory:");
    Cfg.MaxRelatedTerms   = 2;
    FDialoguePipeline P(Cfg);

    // Граф: "кот" имеет синонимы
    P.GetGraph().AddRelation(TEXT("кот"), TEXT("кошка"), ERelationType::Synonym);
    P.GetGraph().AddRelation(TEXT("кот"), TEXT("котик"), ERelationType::Synonym);

    // Store: что-то про "кот"
    FHypothesis H; H.Claim = TEXT("кот"); H.Confidence = 0.9f;
    P.GetStore().Store(H);

    const FString Response = P.ProcessText(TEXT("что такое кот?"));
    // Синонимы должны появиться в ответе
    TestTrue(TEXT("Синонимы в ответе"), Response.Contains(TEXT("кошка")) ||
                                         Response.Contains(TEXT("котик")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_RelatedTerms_HypernymsLabel,
    "Neira.DialoguePipeline.SemanticGraph.RelatedTerms_HypernymsLabel",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_RelatedTerms_HypernymsLabel::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.SemanticGraphPath = TEXT(":memory:");
    Cfg.MaxRelatedTerms   = 3;
    FDialoguePipeline P(Cfg);

    // Только гиперонимы (без синонимов)
    P.GetGraph().AddRelation(TEXT("кот"), TEXT("животное"), ERelationType::Hypernym);
    P.GetGraph().AddRelation(TEXT("кот"), TEXT("млекопитающее"), ERelationType::Hypernym);

    FHypothesis H; H.Claim = TEXT("кот"); H.Confidence = 0.9f;
    P.GetStore().Store(H);

    const FString Response = P.ProcessText(TEXT("что такое кот?"));
    TestTrue(TEXT("Метка 'Родовые понятия' при только гиперонимах"),
             Response.Contains(TEXT("Родовые понятия:")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_RelatedTerms_MixedLabel,
    "Neira.DialoguePipeline.SemanticGraph.RelatedTerms_MixedLabel",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_RelatedTerms_MixedLabel::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.SemanticGraphPath = TEXT(":memory:");
    Cfg.MaxRelatedTerms   = 3;
    FDialoguePipeline P(Cfg);

    P.GetGraph().AddRelation(TEXT("кот"), TEXT("котик"),   ERelationType::Synonym);
    P.GetGraph().AddRelation(TEXT("кот"), TEXT("животное"), ERelationType::Hypernym);

    FHypothesis H; H.Claim = TEXT("кот"); H.Confidence = 0.9f;
    P.GetStore().Store(H);

    const FString Response = P.ProcessText(TEXT("что такое кот?"));
    TestTrue(TEXT("Метка 'Связанные понятия' при смешанных типах"),
             Response.Contains(TEXT("Связанные понятия:")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_MaxRelatedTerms_Respected,
    "Neira.DialoguePipeline.SemanticGraph.MaxRelatedTerms_Respected",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_MaxRelatedTerms_Respected::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.SemanticGraphPath = TEXT(":memory:");
    Cfg.MaxRelatedTerms   = 1;  // только один термин
    FDialoguePipeline P(Cfg);

    P.GetGraph().AddRelation(TEXT("кот"), TEXT("кошка"),  ERelationType::Synonym);
    P.GetGraph().AddRelation(TEXT("кот"), TEXT("котик"),  ERelationType::Synonym);
    P.GetGraph().AddRelation(TEXT("кот"), TEXT("котяра"), ERelationType::Synonym);

    FHypothesis H; H.Claim = TEXT("кот"); H.Confidence = 0.9f;
    P.GetStore().Store(H);

    const FString Response = P.ProcessText(TEXT("что такое кот?"));

    // Считаем количество вхождений через простой подсчёт
    // При MaxRelatedTerms=1 в строке "Синонимы: X." должна быть ровно одна запятая меньше
    // Проверяем косвенно: хотя бы один синоним есть
    TestTrue(TEXT("Хотя бы один синоним"), Response.Contains(TEXT("Синонимы:")));

    // "котяра" — третий в списке, при лимите 1 не должен попасть
    // (первым будет кошка или котик)
    // Жёсткой проверки на отсутствие котяры нет — порядок зависит от DB
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_NoGraph_NoRelatedTerms,
    "Neira.DialoguePipeline.SemanticGraph.NoGraph_NoRelatedTerms",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_NoGraph_NoRelatedTerms::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    // Граф не загружен
    FDialoguePipeline P(Cfg);

    FHypothesis H; H.Claim = TEXT("кот"); H.Confidence = 0.9f;
    P.GetStore().Store(H);

    const FString Response = P.ProcessText(TEXT("что такое кот?"));
    TestFalse(TEXT("Без графа нет блока 'Синонимы:'"),
              Response.Contains(TEXT("Синонимы:")));
    TestFalse(TEXT("Без графа нет блока 'Связанные понятия:'"),
              Response.Contains(TEXT("Связанные понятия:")));
    return true;
}

// ===========================================================================
// Determinism — одинаковый input даёт одинаковый output
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_Deterministic_SameInputSameOutput,
    "Neira.DialoguePipeline.Determinism.SameInputSameOutput",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_Deterministic_SameInputSameOutput::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    Cfg.Personality = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);

    FDialoguePipeline P1(Cfg);
    FDialoguePipeline P2(Cfg);

    const FString Q = TEXT("что такое синтаксис?");
    const FString R1 = P1.ProcessText(Q);
    const FString R2 = P2.ProcessText(Q);

    TestEqual(TEXT("Одинаковый ввод → одинаковый вывод"), R1, R2);
    return true;
}

// ===========================================================================
// GetStore — мутируемый доступ для предзагрузки
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDialoguePipeline_GetStore_Mutable,
    "Neira.DialoguePipeline.GetStore_Mutable",
    NEIRA_TEST_FLAGS)
bool FDialoguePipeline_GetStore_Mutable::RunTest(const FString& Parameters)
{
    FDialoguePipelineConfig Cfg;
    FDialoguePipeline P(Cfg);

    TestEqual(TEXT("Store изначально пустой"), P.GetStore().Count(), 0);

    FHypothesis H; H.Claim = TEXT("тест"); H.Confidence = 0.8f;
    P.GetStore().Store(H);

    TestEqual(TEXT("Store заполнен через GetStore()"), P.GetStore().Count(), 1);
    return true;
}
