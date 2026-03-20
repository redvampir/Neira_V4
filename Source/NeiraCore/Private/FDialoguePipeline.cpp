// FDialoguePipeline.cpp
// v0.1 — единая точка сборки NLU/knowledge пайплайна с семантическим графом.
//
// Порядок обработки каждого запроса:
//   1. FPhraseClassifier::Classify(Input)           → EPhraseType
//   2. FIntentExtractor::Extract(Input, PhraseType)  → FIntentResult
//   3. FBeliefEngine::Process(Intent, Store, Source, Graph) → FBeliefDecision
//   4. BuildSemanticDecision(Intent, Decision)       → FResponseSemanticDecision
//        └─ EnrichRelatedTerms(EntityTarget)         ← FSemanticGraph
//   5. FResponseGenerator::Generate(Input, Profile)  → FResponseGenerationOutput

#include "FDialoguePipeline.h"
#include <cstdio>

// ---------------------------------------------------------------------------
// Конструктор — загрузка семантического графа
// ---------------------------------------------------------------------------
FDialoguePipeline::FDialoguePipeline(const FDialoguePipelineConfig& Config)
    : Cfg(Config)
{
    if (!Cfg.SemanticGraphPath.IsEmpty())
    {
        const bool Ok = Graph.Load(Cfg.SemanticGraphPath);
        if (!Ok)
        {
            fprintf(stderr,
                "[DialoguePipeline] Предупреждение: не удалось загрузить семантический граф '%s'.\n",
                *Cfg.SemanticGraphPath);
        }
    }
}

// ---------------------------------------------------------------------------
// Основной метод
// ---------------------------------------------------------------------------
FString FDialoguePipeline::ProcessText(const FString& Input)
{
    const FString Trimmed = Input.TrimStartAndEnd();

    // Защита от пустого ввода
    if (Trimmed.IsEmpty())
    {
        FResponseGenerationInput RInput;
        RInput.ContextKey                     = TEXT("empty_input");
        RInput.SemanticDecision.IntentID      = EIntentID::Unknown;
        RInput.SemanticDecision.SemanticCore  = TEXT("пустой запрос");
        RInput.SemanticDecision.bHasUncertainty  = true;
        RInput.SemanticDecision.UncertaintyReason = TEXT("запрос пустой или состоит из пробелов");
        return Generator.Generate(RInput, Cfg.Personality).ResponseText;
    }

    // Шаг 1: классификация фразы
    const EPhraseType PhraseType = Classifier.Classify(Trimmed);

    // Шаг 2: извлечение намерения
    const FIntentResult Intent = Extractor.Extract(Trimmed, PhraseType);

    // Шаг 3: обновление хранилища знаний (с семантическим расширением)
    const FBeliefDecision Decision = Belief.Process(
        Intent,
        Store,
        Cfg.DefaultSource,
        Graph.IsLoaded() ? &Graph : nullptr);

    // Шаг 4: построить семантическое решение (+ связанные понятия из графа)
    FResponseSemanticDecision Semantic = BuildSemanticDecision(Intent, Decision);

    // Шаг 5: генерация ответа
    FResponseGenerationInput RInput;
    RInput.ContextKey       = TEXT("dialogue");
    RInput.SemanticDecision = Semantic;

    return Generator.Generate(RInput, Cfg.Personality).ResponseText;
}

// ---------------------------------------------------------------------------
// BuildSemanticDecision
// ---------------------------------------------------------------------------
FResponseSemanticDecision FDialoguePipeline::BuildSemanticDecision(
    const FIntentResult&   Intent,
    const FBeliefDecision& Decision) const
{
    FResponseSemanticDecision Semantic;
    Semantic.IntentID = Intent.IntentID;

    switch (Decision.Action)
    {
    case EBeliefAction::Created:
        Semantic.SemanticCore = FString::Printf(
            TEXT("факт принят: %s"),
            Intent.EntityTarget.IsEmpty() ? TEXT("(без объекта)") : *Intent.EntityTarget);
        break;

    case EBeliefAction::Confirmed:
    case EBeliefAction::Verified:
    {
        // Получить текст гипотезы из хранилища
        const FHypothesis* H = Store.Find(Decision.HypothesisID);
        const FString Claim = (H && !H->Claim.IsEmpty()) ? H->Claim : Intent.EntityTarget;

        const FString MatchNote = Decision.MatchedVia.IsEmpty()
            ? TEXT("")
            : FString::Printf(TEXT(" (найдено через %s)"), *Decision.MatchedVia);

        Semantic.SemanticCore = Claim + MatchNote;
        break;
    }

    case EBeliefAction::Rejected:
        Semantic.SemanticCore      = FString::Printf(
            TEXT("запрос обработан: %s"),
            Intent.EntityTarget.IsEmpty() ? TEXT("(без объекта)") : *Intent.EntityTarget);
        Semantic.bHasUncertainty   = true;
        Semantic.UncertaintyReason = Decision.Reason;
        break;

    case EBeliefAction::NoMatch:
    default:
        Semantic.SemanticCore = FString::Printf(
            TEXT("не найдено: %s"),
            Intent.EntityTarget.IsEmpty() ? TEXT("(объект не распознан)") : *Intent.EntityTarget);
        Semantic.bHasUncertainty   = true;
        Semantic.UncertaintyReason = Decision.Reason;
        break;
    }

    // Добавить связанные понятия из семантического графа
    if (!Intent.EntityTarget.IsEmpty())
    {
        EnrichRelatedTerms(Semantic, Intent.EntityTarget);
    }

    return Semantic;
}

// ---------------------------------------------------------------------------
// EnrichRelatedTerms
// ---------------------------------------------------------------------------
void FDialoguePipeline::EnrichRelatedTerms(FResponseSemanticDecision& Semantic,
                                            const FString&             EntityTarget) const
{
    if (!Graph.IsLoaded() || Cfg.MaxRelatedTerms <= 0)
        return;

    TArray<FString> Terms;
    bool bHasSynonyms   = false;
    bool bHasHypernyms  = false;

    // Синонимы — приоритет
    for (const FString& Syn : Graph.GetSynonyms(EntityTarget))
    {
        if (Terms.Num() >= Cfg.MaxRelatedTerms)
            break;
        Terms.Add(Syn);
        bHasSynonyms = true;
    }

    // Гиперонимы — если ещё есть место
    if (Terms.Num() < Cfg.MaxRelatedTerms)
    {
        for (const FString& Hyper : Graph.GetHypernyms(EntityTarget))
        {
            if (Terms.Num() >= Cfg.MaxRelatedTerms)
                break;
            Terms.Add(Hyper);
            bHasHypernyms = true;
        }
    }

    if (Terms.IsEmpty())
        return;

    Semantic.RelatedTerms = Terms;

    // Выбрать метку
    if (bHasSynonyms && !bHasHypernyms)
        Semantic.RelatedTermsLabel = TEXT("Синонимы");
    else if (!bHasSynonyms && bHasHypernyms)
        Semantic.RelatedTermsLabel = TEXT("Родовые понятия");
    else
        Semantic.RelatedTermsLabel = TEXT("Связанные понятия");
}

// ---------------------------------------------------------------------------
// MakeHandler
// ---------------------------------------------------------------------------
FTextPipelineHandler FDialoguePipeline::MakeHandler()
{
    return [this](const FString& Input) -> FString
    {
        return ProcessText(Input);
    };
}

// ---------------------------------------------------------------------------
// Диагностика
// ---------------------------------------------------------------------------
bool FDialoguePipeline::IsSemanticGraphLoaded() const
{
    return Graph.IsLoaded();
}

FSemanticGraph& FDialoguePipeline::GetGraph()
{
    return Graph;
}

const FHypothesisStore& FDialoguePipeline::GetStore() const
{
    return Store;
}

FHypothesisStore& FDialoguePipeline::GetStore()
{
    return Store;
}
