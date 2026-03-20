/**
 * Neira Dialog Test — Автоматический тест диалога
 * Запускает набор фраз и показывает ответы Нейры
 */

#include "../Tests/ue_compat.h"
#include "../NeiraCore/Public/NeiraTypes.h"
#include "../NeiraCore/Public/FPhraseClassifier.h"
#include "../NeiraCore/Public/FIntentExtractor.h"
#include "../NeiraCore/Public/FActionRegistry.h"
#include "../NeiraCore/Public/FResponseGenerator.h"
#include "../NeiraCore/Public/FMorphAnalyzer.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// Тестовые фразы
// ---------------------------------------------------------------------------
const TCHAR* TestPhrases[] = {
    TEXT("что такое нейра?"),
    TEXT("ты можешь нарисовать квадрат?"),
    TEXT("найди значение слова дом"),
    TEXT("кот - это животное"),
    TEXT("расскажи про машинное обучение"),
    TEXT("кто такой пушкин?"),
    TEXT("объясни что такое синтаксис"),
    TEXT("печь жарко топили"),
    TEXT("печь пироги умела"),
    TEXT("что означает слово гипотеза"),
    TEXT("как называется столица россии"),
    TEXT("ты понимаешь русский язык?"),
    TEXT("запомни что москва столица россии"),
    TEXT("что я сказал?"),
};

const int TestPhrasesCount = sizeof(TestPhrases) / sizeof(TestPhrases[0]);

// ---------------------------------------------------------------------------
// Helper: вывод типа фразы
// ---------------------------------------------------------------------------
FString PhraseTypeToString(EPhraseType Type)
{
    switch (Type)
    {
    case EPhraseType::Question:   return TEXT("Вопрос");
    case EPhraseType::Command:    return TEXT("Команда");
    case EPhraseType::Statement:  return TEXT("Утверждение");
    case EPhraseType::Request:    return TEXT("Просьба");
    default:                      return TEXT("Неизвестно");
    }
}

// ---------------------------------------------------------------------------
// Helper: вывод намерения
// ---------------------------------------------------------------------------
FString IntentToString(EIntentID Intent)
{
    switch (Intent)
    {
    case EIntentID::GetDefinition:    return TEXT("GET_DEFINITION");
    case EIntentID::GetWordFact:      return TEXT("GET_WORD_FACT");
    case EIntentID::FindMeaning:      return TEXT("FIND_MEANING");
    case EIntentID::AnswerAbility:    return TEXT("ANSWER_ABILITY");
    case EIntentID::StoreFact:        return TEXT("STORE_FACT");
    default:                          return TEXT("UNKNOWN");
    }
}

// ---------------------------------------------------------------------------
// Helper: вывод POS
// ---------------------------------------------------------------------------
FString POSToString(EPosTag POS)
{
    switch (POS)
    {
    case EPosTag::Noun:         return TEXT("сущ.");
    case EPosTag::Verb:         return TEXT("гл.");
    case EPosTag::Adjective:    return TEXT("прил.");
    case EPosTag::Adverb:       return TEXT("нареч.");
    case EPosTag::Pronoun:      return TEXT("мест.");
    case EPosTag::Preposition:  return TEXT("предл.");
    case EPosTag::Conjunction:  return TEXT("союз");
    case EPosTag::Particle:     return TEXT("частица");
    case EPosTag::Numeral:      return TEXT("числ.");
    case EPosTag::Unknown:
    default:                    return TEXT("???");
    }
}

// ---------------------------------------------------------------------------
// Обработка фразы (полный pipeline)
// ---------------------------------------------------------------------------
void ProcessPhrase(const FString& Input)
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ВХОД: \"%s\"\n", *Input);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    // -------------------------------------------------------------------------
    // Шаг 1: Классификация фразы
    // -------------------------------------------------------------------------
    FPhraseClassifier Classifier;
    EPhraseType PhraseType = Classifier.Classify(Input);
    
    printf("1. КЛАССИФИКАЦИЯ: %s\n\n", *PhraseTypeToString(PhraseType));
    
    // -------------------------------------------------------------------------
    // Шаг 2: Морфологический анализ
    // -------------------------------------------------------------------------
    printf("2. МОРФОЛОГИЯ:\n");
    FMorphAnalyzer Analyzer;
    TArray<FMorphResult> Results = Analyzer.AnalyzePhrase(Input);
    
    for (const auto& Result : Results)
    {
        printf("   '%s' → лемма: '%s', POS: %s (conf: %.2f, source: %s)\n",
               *Result.OriginalWord,
               *Result.Lemma,
               *POSToString(Result.PartOfSpeech),
               Result.Confidence,
               *Result.Source);
    }
    printf("\n");
    
    // -------------------------------------------------------------------------
    // Шаг 3: Извлечение намерения
    // -------------------------------------------------------------------------
    FIntentExtractor Extractor;
    FIntentResult IntentResult = Extractor.Extract(Input, PhraseType);
    
    printf("3. НАМЕРЕНИЕ: %s\n", *IntentToString(IntentResult.IntentID));
    printf("   Объект: %s\n", IntentResult.EntityTarget.IsEmpty() ? TEXT("(нет)") : *IntentResult.EntityTarget);
    printf("   Уверенность: %.2f\n", IntentResult.Confidence);
    printf("   Trace: %s\n\n", *IntentResult.DecisionTrace);
    
    // -------------------------------------------------------------------------
    // Шаг 4: Выполнение действия
    // -------------------------------------------------------------------------
    FActionRegistry Registry;
    
    // Регистрируем обработчики
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("«%s» — это слово требует определения. Словарь OpenCorpora (532 МБ) загружен, ожидает интеграции."), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::FindMeaning, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Значение слова «%s»: требуется интеграция OpenCorpora для полного ответа."), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::GetWordFact, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Факт о «%s»: база знаний будет доступна после подключения внешних словарей."), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::AnswerAbility, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        FString Action = Req.EntityTarget.IsEmpty() ? TEXT("это") : Req.EntityTarget;
        Result.ResultText = FString::Printf(TEXT("Да, я могу %s. Я — языковое ядро Neira v0.5, работаю на правилах, а не на статистике. Встроенный словарь: ~1000 слов."), *Action);
        return Result;
    });
    
    Registry.Register(EActionID::StoreFact, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = TEXT("Запомнила. Факт сохранён в памяти гипотез (FHypothesisStore).");
        return Result;
    });
    
    FActionRequest Request;
    Request.ActionID = static_cast<EActionID>(IntentResult.IntentID);
    Request.EntityTarget = IntentResult.EntityTarget;
    Request.Confidence = IntentResult.Confidence;
    
    FActionResult ActionResult = Registry.Execute(Request);
    
    printf("4. ДЕЙСТВИЕ: %s\n", ActionResult.bSuccess ? TEXT("Успех") : TEXT("Неудача"));
    if (ActionResult.bSuccess)
    {
        printf("   Результат: %s\n\n", *ActionResult.ResultText);
    }
    else
    {
        printf("   FailReason: %s\n\n", *ActionResult.DiagnosticNote);
    }
    
    // -------------------------------------------------------------------------
    // Шаг 5: Генерация ответа
    // -------------------------------------------------------------------------
    FResponseGenerator ResponseGen;
    
    FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm,
        EResponseLength::Short,
        EResponseInitiative::Low,
        EResponseAddressStyle::NeutralYou
    );
    
    FResponseSemanticDecision Decision;
    Decision.IntentID = IntentResult.IntentID;
    Decision.SemanticCore = IntentResult.EntityTarget;
    Decision.bHasUncertainty = (IntentResult.Confidence < 0.7f);
    
    FResponseGenerationInput GenInput;
    GenInput.ContextKey = TEXT("default");
    GenInput.SemanticDecision = Decision;
    
    FResponseGenerationOutput Output = ResponseGen.Generate(GenInput, Profile);
    
    printf("5. ОТВЕТ АГЕНТА:\n");
    printf("   %s\n\n", *Output.ResponseText);
}

// ---------------------------------------------------------------------------
// Главная функция
// ---------------------------------------------------------------------------
int main()
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║      NEIRA DIALOG TEST — Автоматический тест диалога      ║\n");
    printf("║                    версия v0.5                            ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Ядро: C++17, без зависимостей от Unreal Engine\n");
    printf("  Словарь: встроенный (~1000 слов) + суффиксные правила\n");
    printf("  OpenCorpora: загружен (532 МБ), ожидает интеграции\n");
    printf("\n");
    printf("  Запуск %d тестовых фраз...\n\n", TestPhrasesCount);
    
    for (int i = 0; i < TestPhrasesCount; i++)
    {
        ProcessPhrase(TestPhrases[i]);
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ТЕСТ ЗАВЕРШЁН\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    
    return 0;
}
