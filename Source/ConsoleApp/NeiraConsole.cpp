/**
 * NeiraConsole — консольный интерфейс для тестирования ядра Neira
 * 
 * Запуск:
 *   make all
 *   NeiraConsole.exe
 * 
 * Ввод фраз для анализа, 'quit' для выхода, 'help' для справки.
 */

#include "../Tests/ue_compat.h"
#include "../NeiraCore/Public/NeiraTypes.h"
#include "../NeiraCore/Public/FPhraseClassifier.h"
#include "../NeiraCore/Public/FIntentExtractor.h"
#include "../NeiraCore/Public/FActionRegistry.h"
#include "../NeiraCore/Public/FResponseGenerator.h"
#include "../NeiraCore/Public/FMorphAnalyzer.h"
#include "../NeiraCore/Public/FSyntaxParser.h"

#include <iostream>
#include <string>
#include <sstream>

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
    case EIntentID::CheckMemoryLoad:  return TEXT("CHECK_MEMORY_LOAD");
    default:                          return TEXT("UNKNOWN");
    }
}

// ---------------------------------------------------------------------------
// Helper: вывод части речи
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
// Helper: вывод FailReason
// ---------------------------------------------------------------------------
FString FailReasonToString(EActionFailReason Reason)
{
    switch (Reason)
    {
    case EActionFailReason::None:          return TEXT("OK");
    case EActionFailReason::NotFound:      return TEXT("Не найдено");
    case EActionFailReason::NotSupported:  return TEXT("Не поддерживается");
    case EActionFailReason::LowConfidence: return TEXT("Низкая уверенность");
    case EActionFailReason::EmptyInput:    return TEXT("Пустой ввод");
    case EActionFailReason::UnknownIntent: return TEXT("Неизвестное намерение");
    case EActionFailReason::PartialParse:  return TEXT("Частичный разбор");
    case EActionFailReason::InternalError: return TEXT("Внутренняя ошибка");
    default:                               return TEXT("???");
    }
}

// ---------------------------------------------------------------------------
// Анализ морфологии
// ---------------------------------------------------------------------------
void PrintMorphAnalysis(const FString& Phrase)
{
    printf("\n=== Морфологический анализ ===\n");
    
    FMorphAnalyzer Analyzer;
    TArray<FMorphResult> Results = Analyzer.AnalyzePhrase(Phrase);
    
    if (Results.Num() == 0)
    {
        printf("  Нет результатов\n");
        return;
    }
    
    for (const auto& Result : Results)
    {
        printf("  '%s' → лемма: '%s', POS: %s (conf: %.2f, source: %s)\n",
               *Result.OriginalWord,
               *Result.Lemma,
               *POSToString(Result.PartOfSpeech),
               Result.Confidence,
               *Result.Source);
    }
}

// ---------------------------------------------------------------------------
// Синтаксический анализ
// ---------------------------------------------------------------------------
void PrintSyntaxAnalysis(const FString& Phrase, EPhraseType PhraseType)
{
    printf("\n=== Синтаксический анализ ===\n");
    
    FSyntaxParser Parser;
    FSemanticFrame Frame = Parser.Parse(Phrase, PhraseType);
    
    if (Frame.IsEmpty())
    {
        printf("  Пустой фрейм (не удалось извлечь структуру)\n");
        return;
    }
    
    if (!Frame.Subject.IsEmpty())
        printf("  Субъект: %s\n", *Frame.Subject);
    else
        printf("  Субъект: (нет)\n");
    
    if (!Frame.Predicate.IsEmpty())
        printf("  Предикат: %s\n", *Frame.Predicate);
    else
        printf("  Предикат: (нет)\n");
    
    if (!Frame.Object.IsEmpty())
        printf("  Объект: %s\n", *Frame.Object);
    else
        printf("  Объект: (нет)\n");
    
    if (!Frame.Recipient.IsEmpty())
        printf("  Получатель: %s\n", *Frame.Recipient);
    
    if (Frame.bIsAbilityCheck)
        printf("  [Проверка возможности]\n");
    if (Frame.bHasNestedClause)
        printf("  [Есть придаточное предложение]\n");
    if (Frame.bIsNegated)
        printf("  [Отрицание]\n");
}

// ---------------------------------------------------------------------------
// Полный pipeline
// ---------------------------------------------------------------------------
void ProcessPhrase(const FString& Input)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  ВХОД: \"%s\"\n", *Input);
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    // -------------------------------------------------------------------------
    // Шаг 1: Классификация фразы
    // -------------------------------------------------------------------------
    FPhraseClassifier Classifier;
    EPhraseType PhraseType = Classifier.Classify(Input);
    
    printf("\n┌──────────────────────────────────────────────────────────┐\n");
    printf("│  1. КЛАССИФИКАЦИЯ: %s\n", *PhraseTypeToString(PhraseType));
    printf("└──────────────────────────────────────────────────────────┘\n");
    
    // -------------------------------------------------------------------------
    // Шаг 2: Морфологический анализ
    // -------------------------------------------------------------------------
    PrintMorphAnalysis(Input);
    
    // -------------------------------------------------------------------------
    // Шаг 3: Синтаксический анализ
    // -------------------------------------------------------------------------
    PrintSyntaxAnalysis(Input, PhraseType);
    
    // -------------------------------------------------------------------------
    // Шаг 4: Извлечение намерения
    // -------------------------------------------------------------------------
    FIntentExtractor Extractor;
    FIntentResult IntentResult = Extractor.Extract(Input, PhraseType);
    
    printf("\n┌──────────────────────────────────────────────────────────┐\n");
    printf("│  4. НАМЕРЕНИЕ\n");
    printf("│     Intent: %s\n", *IntentToString(IntentResult.IntentID));
    printf("│     Объект: %s\n", IntentResult.EntityTarget.IsEmpty() ? TEXT("(нет)") : *IntentResult.EntityTarget);
    printf("│     Уверенность: %.2f\n", IntentResult.Confidence);
    printf("│     Trace: %s\n", *IntentResult.DecisionTrace);
    if (IntentResult.FailReason != EActionFailReason::None)
    {
        printf("│     FailReason: %s\n", *FailReasonToString(IntentResult.FailReason));
    }
    if (!IntentResult.DiagnosticNote.IsEmpty())
    {
        printf("│     Diagnostic: %s\n", *IntentResult.DiagnosticNote);
    }
    printf("└──────────────────────────────────────────────────────────┘\n");
    
    // -------------------------------------------------------------------------
    // Шаг 5: Выполнение действия
    // -------------------------------------------------------------------------
    printf("\n┌──────────────────────────────────────────────────────────┐\n");
    printf("│  5. ВЫПОЛНЕНИЕ ДЕЙСТВИЯ\n");
    
    FActionRegistry Registry;
    
    // Регистрируем базовые обработчики (заглушки)
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Определение для \"%s\": [морфологический словарь подключен, но база определений еще не собрана]"), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::FindMeaning, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Значение \"%s\": [морфологический словарь подключен, но база значений еще не собрана]"), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::GetWordFact, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Факт о \"%s\": [база знаний пуста]"), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::AnswerAbility, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Да, я могу %s (в рамках v0.4)"), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::StoreFact, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = TEXT("Факт сохранён в память гипотез.");
        return Result;
    });
    
    // Создаём запрос
    FActionRequest Request;
    Request.ActionID = static_cast<EActionID>(IntentResult.IntentID);
    Request.EntityTarget = IntentResult.EntityTarget;
    Request.Confidence = IntentResult.Confidence;
    
    // Выполняем
    FActionResult ActionResult = Registry.Execute(Request);
    
    printf("│     Успех: %s\n", ActionResult.bSuccess ? TEXT("Да") : TEXT("Нет"));
    if (ActionResult.bSuccess)
    {
        printf("│     Результат: %s\n", *ActionResult.ResultText);
    }
    else
    {
        printf("│     FailReason: %s\n", *FailReasonToString(ActionResult.FailReason));
        if (!ActionResult.DiagnosticNote.IsEmpty())
        {
            printf("│     Diagnostic: %s\n", *ActionResult.DiagnosticNote);
        }
    }
    printf("└──────────────────────────────────────────────────────────┘\n");
    
    // -------------------------------------------------------------------------
    // Шаг 6: Генерация ответа
    // -------------------------------------------------------------------------
    printf("\n┌──────────────────────────────────────────────────────────┐\n");
    printf("│  6. ОТВЕТ АГЕНТА\n");
    
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
    if (Decision.bHasUncertainty)
    {
        Decision.UncertaintyReason = TEXT("низкая уверенность разбора");
    }
    
    FResponseGenerationInput GenInput;
    GenInput.ContextKey = TEXT("default");
    GenInput.SemanticDecision = Decision;
    
    FResponseGenerationOutput ResponseOutput = ResponseGen.Generate(GenInput, Profile);
    
    printf("│     %s\n", *ResponseOutput.ResponseText);
    printf("└──────────────────────────────────────────────────────────┘\n");
    
    printf("\n");
}

// ---------------------------------------------------------------------------
// Вывод справки
// ---------------------------------------------------------------------------
void PrintHelp()
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("   Neira Console — тестирование ядра ИИ-агента\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("  Введите фразу для анализа (на русском или английском):\n");
    printf("\n");
    printf("  Примеры:\n");
    printf("    • что такое нейра?\n");
    printf("    • ты можешь нарисовать квадрат?\n");
    printf("    • найди значение слова дом\n");
    printf("    • кот — это животное\n");
    printf("    • расскажи про машинное обучение\n");
    printf("    • печь жарко топили\n");
    printf("    • печь пироги умела\n");
    printf("\n");
    printf("  Команды:\n");
    printf("    help  — показать эту справку\n");
    printf("    quit  — выход\n");
    printf("\n");
}

// ---------------------------------------------------------------------------
// Главная функция
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║           NEIRA CORE — КОНСОЛЬНЫЙ ИНТЕРФЕЙС              ║\n");
    printf("║                    версия v0.4                           ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Ядро: C++17, без зависимостей от Unreal Engine\n");
    printf("  Словарь: встроенный (~1000 слов) + OpenCorpora (lazy-load) + суффиксные правила\n");
    printf("  Тесты: make run\n");
    printf("\n");
    
    PrintHelp();
    
    std::string InputLine;
    
    while (true)
    {
        printf("┌──────────────────────────────────────────────────────────┐\n");
        printf("│ Ввод: ");
        
        std::getline(std::cin, InputLine);
        
        if (InputLine.empty())
        {
            printf("│ (пустой ввод — введите 'help' для справки)\n");
            printf("└──────────────────────────────────────────────────────────┘\n");
            continue;
        }
        
        // Проверка команд
        if (InputLine == "quit" || InputLine == "exit" || InputLine == "выход")
        {
            printf("│ Выход из программы...\n");
            printf("└──────────────────────────────────────────────────────────┘\n");
            break;
        }
        
        if (InputLine == "help" || InputLine == "h" || InputLine == "?")
        {
            PrintHelp();
            continue;
        }
        
        // Обработка фразы
        FString Input(InputLine.c_str());
        ProcessPhrase(Input);
    }
    
    printf("\n");
    printf("  Спасибо за использование Neira Console!\n");
    printf("\n");
    
    return 0;
}
