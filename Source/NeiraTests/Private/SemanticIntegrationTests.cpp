// SemanticIntegrationTests.cpp
// Интеграционные тесты семантического графа с FBeliefEngine и FResponseGenerator.
//
// Сценарии:
//   — BeliefEngine находит гипотезу через синоним
//   — BeliefEngine находит гипотезу через гипероним
//   — BeliefEngine без графа: поведение не изменилось
//   — ResponseGenerator: RelatedTerms появляются в ответе
//   — ResponseGenerator: пустой список не меняет ответ

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "FBeliefEngine.h"
#include "FSemanticGraph.h"
#include "FResponseGenerator.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ===========================================================================
// BeliefEngine × SemanticGraph — синонимный поиск
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_BeliefEngine_SynonymExpands_Confirms,
    "Neira.SemanticIntegration.BeliefEngine.SynonymExpansion_Confirms",
    NEIRA_TEST_FLAGS)
bool FSemInt_BeliefEngine_SynonymExpands_Confirms::RunTest(const FString& Parameters)
{
    // Хранилище содержит гипотезу "кот".
    // Граф знает: "кошка" → synonym → "кот".
    // Запрос GetDefinition для "кошка" должен найти "кот" и подтвердить его.

    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("кот");
    const int32 StoredID = Store.Store(H);

    FSemanticGraph Graph;
    Graph.Load(TEXT(":memory:"));
    Graph.AddRelation(TEXT("кошка"), TEXT("кот"), ERelationType::Synonym);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кошка");  // точного совпадения в Store нет
    Intent.Confidence   = 0.9f;

    FBeliefEngine Engine;
    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm, &Graph);

    TestEqual  (TEXT("Action == Confirmed"), D.Action, EBeliefAction::Confirmed);
    TestEqual  (TEXT("HypothesisID == StoredID"), D.HypothesisID, StoredID);
    TestTrue   (TEXT("MatchedVia содержит 'synonym'"), D.MatchedVia.Contains(TEXT("synonym")));
    TestTrue   (TEXT("MatchedVia содержит 'кот'"),     D.MatchedVia.Contains(TEXT("кот")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_BeliefEngine_SynonymExpands_Verifies,
    "Neira.SemanticIntegration.BeliefEngine.SynonymExpansion_Verifies",
    NEIRA_TEST_FLAGS)
bool FSemInt_BeliefEngine_SynonymExpands_Verifies::RunTest(const FString& Parameters)
{
    // Та же логика, но гипотеза уже дважды подтверждена → Verify.
    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("кот");
    int32 ID = Store.Store(H);
    Store.Confirm(ID, TEXT("c1"));
    Store.Confirm(ID, TEXT("c2"));

    FSemanticGraph Graph;
    Graph.Load(TEXT(":memory:"));
    Graph.AddRelation(TEXT("кошка"), TEXT("кот"), ERelationType::Synonym);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кошка");
    Intent.Confidence   = 0.9f;

    FBeliefEngine Engine;
    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm, &Graph);

    TestEqual(TEXT("Action == Verified через синоним"), D.Action, EBeliefAction::Verified);
    TestEqual(TEXT("HypothesisID совпадает"), D.HypothesisID, ID);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_BeliefEngine_HypernymExpands,
    "Neira.SemanticIntegration.BeliefEngine.HypernymExpansion_Confirms",
    NEIRA_TEST_FLAGS)
bool FSemInt_BeliefEngine_HypernymExpands::RunTest(const FString& Parameters)
{
    // Store содержит "животное".
    // Граф: "кот" → hypernym → "животное".
    // GetDefinition "кот" должен найти "животное" как гипероним.

    FHypothesisStore Store;
    FHypothesis H;
    H.Claim = TEXT("животное");
    const int32 StoredID = Store.Store(H);

    FSemanticGraph Graph;
    Graph.Load(TEXT(":memory:"));
    Graph.AddRelation(TEXT("кот"), TEXT("животное"), ERelationType::Hypernym);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кот");
    Intent.Confidence   = 0.85f;

    FBeliefEngine Engine;
    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm, &Graph);

    TestEqual(TEXT("Action == Confirmed через гипероним"), D.Action, EBeliefAction::Confirmed);
    TestEqual(TEXT("HypothesisID == StoredID"), D.HypothesisID, StoredID);
    TestTrue (TEXT("MatchedVia содержит 'hypernym'"), D.MatchedVia.Contains(TEXT("hypernym")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_BeliefEngine_SynonymHasPriorityOverHypernym,
    "Neira.SemanticIntegration.BeliefEngine.SynonymBeforeHypernym",
    NEIRA_TEST_FLAGS)
bool FSemInt_BeliefEngine_SynonymHasPriorityOverHypernym::RunTest(const FString& Parameters)
{
    // Store содержит и "котик" (синоним) и "животное" (гипероним).
    // Должен найтись синоним первым.

    FHypothesisStore Store;
    FHypothesis H1; H1.Claim = TEXT("котик");
    const int32 SynID = Store.Store(H1);
    FHypothesis H2; H2.Claim = TEXT("животное");
    Store.Store(H2);

    FSemanticGraph Graph;
    Graph.Load(TEXT(":memory:"));
    Graph.AddRelation(TEXT("кошка"), TEXT("котик"),   ERelationType::Synonym);
    Graph.AddRelation(TEXT("кошка"), TEXT("животное"), ERelationType::Hypernym);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кошка");
    Intent.Confidence   = 0.9f;

    FBeliefEngine Engine;
    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm, &Graph);

    TestEqual(TEXT("Action == Confirmed"), D.Action, EBeliefAction::Confirmed);
    TestEqual(TEXT("Синоним нашёлся первым"), D.HypothesisID, SynID);
    TestTrue (TEXT("MatchedVia содержит 'synonym'"), D.MatchedVia.Contains(TEXT("synonym")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_BeliefEngine_NoGraphNoChange,
    "Neira.SemanticIntegration.BeliefEngine.NullGraph_BehaviourUnchanged",
    NEIRA_TEST_FLAGS)
bool FSemInt_BeliefEngine_NoGraphNoChange::RunTest(const FString& Parameters)
{
    // Без графа: поведение как раньше — NoMatch если нет точного совпадения.
    FHypothesisStore Store;
    FHypothesis H; H.Claim = TEXT("кот");
    Store.Store(H);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кошка");  // нет в Store
    Intent.Confidence   = 0.9f;

    FBeliefEngine Engine;
    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm, nullptr);

    TestEqual(TEXT("Action == NoMatch без графа"), D.Action, EBeliefAction::NoMatch);
    TestTrue (TEXT("MatchedVia пустой"), D.MatchedVia.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_BeliefEngine_ExactMatchNoMatchedVia,
    "Neira.SemanticIntegration.BeliefEngine.ExactMatch_MatchedViaEmpty",
    NEIRA_TEST_FLAGS)
bool FSemInt_BeliefEngine_ExactMatchNoMatchedVia::RunTest(const FString& Parameters)
{
    // Точное совпадение: MatchedVia должен оставаться пустым.
    FHypothesisStore Store;
    FHypothesis H; H.Claim = TEXT("кот");
    Store.Store(H);

    FSemanticGraph Graph;
    Graph.Load(TEXT(":memory:"));
    Graph.AddRelation(TEXT("кот"), TEXT("кошка"), ERelationType::Synonym);

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кот");  // точное совпадение
    Intent.Confidence   = 0.9f;

    FBeliefEngine Engine;
    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm, &Graph);

    TestEqual(TEXT("Action == Confirmed"), D.Action, EBeliefAction::Confirmed);
    TestTrue (TEXT("MatchedVia пустой при точном совпадении"), D.MatchedVia.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_BeliefEngine_UnloadedGraph_NoExpansion,
    "Neira.SemanticIntegration.BeliefEngine.UnloadedGraph_NoExpansion",
    NEIRA_TEST_FLAGS)
bool FSemInt_BeliefEngine_UnloadedGraph_NoExpansion::RunTest(const FString& Parameters)
{
    // Граф не загружен (нет Load()) — расширение не происходит.
    FHypothesisStore Store;
    FHypothesis H; H.Claim = TEXT("кот");
    Store.Store(H);

    FSemanticGraph Graph;  // нет Load()

    FIntentResult Intent;
    Intent.IntentID     = EIntentID::GetDefinition;
    Intent.EntityTarget = TEXT("кошка");
    Intent.Confidence   = 0.9f;

    FBeliefEngine Engine;
    FBeliefDecision D = Engine.Process(Intent, Store, EHypothesisSource::UserConfirm, &Graph);

    TestEqual(TEXT("Action == NoMatch для незагруженного графа"), D.Action, EBeliefAction::NoMatch);
    return true;
}

// ===========================================================================
// ResponseGenerator × RelatedTerms
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_ResponseGenerator_RelatedTerms_Appended,
    "Neira.SemanticIntegration.ResponseGenerator.RelatedTerms_AppendedToResponse",
    NEIRA_TEST_FLAGS)
bool FSemInt_ResponseGenerator_RelatedTerms_Appended::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;
    const FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);

    FResponseGenerationInput Input;
    Input.ContextKey = TEXT("semantics_test");
    Input.SemanticDecision.IntentID     = EIntentID::GetDefinition;
    Input.SemanticDecision.SemanticCore = TEXT("кот — домашнее животное");
    Input.SemanticDecision.RelatedTerms.Add(TEXT("кошка"));
    Input.SemanticDecision.RelatedTerms.Add(TEXT("котик"));
    Input.SemanticDecision.RelatedTermsLabel = TEXT("Синонимы");

    const FResponseGenerationOutput Out = Generator.Generate(Input, Profile);

    TestTrue(TEXT("Блок RelatedTerms присутствует"),
             Out.ResponseText.Contains(TEXT("Синонимы:")));
    TestTrue(TEXT("кошка в ответе"), Out.ResponseText.Contains(TEXT("кошка")));
    TestTrue(TEXT("котик в ответе"), Out.ResponseText.Contains(TEXT("котик")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_ResponseGenerator_RelatedTerms_DefaultLabel,
    "Neira.SemanticIntegration.ResponseGenerator.RelatedTerms_DefaultLabel",
    NEIRA_TEST_FLAGS)
bool FSemInt_ResponseGenerator_RelatedTerms_DefaultLabel::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;
    const FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);

    FResponseGenerationInput Input;
    Input.ContextKey = TEXT("label_test");
    Input.SemanticDecision.IntentID     = EIntentID::GetWordFact;
    Input.SemanticDecision.SemanticCore = TEXT("синтаксис — раздел лингвистики");
    Input.SemanticDecision.RelatedTerms.Add(TEXT("грамматика"));
    // RelatedTermsLabel пустой → должен быть "Связанные понятия"

    const FResponseGenerationOutput Out = Generator.Generate(Input, Profile);

    TestTrue(TEXT("Метка по умолчанию 'Связанные понятия'"),
             Out.ResponseText.Contains(TEXT("Связанные понятия:")));
    TestTrue(TEXT("грамматика в ответе"), Out.ResponseText.Contains(TEXT("грамматика")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_ResponseGenerator_EmptyRelatedTerms_NoBlock,
    "Neira.SemanticIntegration.ResponseGenerator.EmptyRelatedTerms_NoExtraBlock",
    NEIRA_TEST_FLAGS)
bool FSemInt_ResponseGenerator_EmptyRelatedTerms_NoBlock::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;
    const FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);

    FResponseGenerationInput Input;
    Input.ContextKey = TEXT("session_main");
    Input.SemanticDecision.IntentID     = EIntentID::GetDefinition;
    Input.SemanticDecision.SemanticCore = TEXT("синтаксис — раздел лингвистики о построении предложений");
    // RelatedTerms пустой

    const FResponseGenerationOutput Out = Generator.Generate(Input, Profile);

    TestFalse(TEXT("Нет блока 'Связанные понятия' при пустом списке"),
              Out.ResponseText.Contains(TEXT("Связанные понятия:")));
    TestFalse(TEXT("Нет блока 'Синонимы:' при пустом списке"),
              Out.ResponseText.Contains(TEXT("Синонимы:")));

    // Снапшот-контракт: вывод идентичен оригиналу
    const FString Expected =
        TEXT("[profile=personality_profile_v1; tone=calm; len=short; initiative=low; address=neutral_you; format=v1.intent_1.ctx_session_main.tone_calm.len_short.init_low.addr_neutral_you]\n")
        TEXT("Тон: спокойный.\n")
        TEXT("Обращение: нейтральное, на «вы».\n")
        TEXT("Инициатива: низкая.\n")
        TEXT("Ответ: определение — синтаксис — раздел лингвистики о построении предложений.\n")
        TEXT("Ограничение: факты не выдумываю.");

    TestEqual(TEXT("Снапшот-контракт не сломан"), Out.ResponseText, Expected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSemInt_ResponseGenerator_RelatedTerms_BeforeHallucinationGuard,
    "Neira.SemanticIntegration.ResponseGenerator.RelatedTerms_BeforeHallucinationGuard",
    NEIRA_TEST_FLAGS)
bool FSemInt_ResponseGenerator_RelatedTerms_BeforeHallucinationGuard::RunTest(const FString& Parameters)
{
    FResponseGenerator Generator;
    const FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm, EResponseLength::Short, EResponseInitiative::Low);

    FResponseGenerationInput Input;
    Input.ContextKey = TEXT("order_test");
    Input.SemanticDecision.IntentID     = EIntentID::GetDefinition;
    Input.SemanticDecision.SemanticCore = TEXT("тест");
    Input.SemanticDecision.RelatedTerms.Add(TEXT("проверка"));

    const FResponseGenerationOutput Out = Generator.Generate(Input, Profile);

    // "Синонимы:" должен стоять перед "Ограничение:"
    const int32 RelPos  = Out.ResponseText.Find(TEXT("Связанные понятия:"));
    const int32 LimPos  = Out.ResponseText.Find(TEXT("Ограничение:"));
    TestTrue(TEXT("RelatedTerms перед hallucination guard"), RelPos < LimPos);
    return true;
}
