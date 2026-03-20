// NeiraConsoleSmokeTest.cpp
// Быстрый тест ядра Neira без интерактивного ввода

#include "../Tests/ue_compat.h"
#include "../NeiraCore/Public/NeiraTypes.h"
#include "../NeiraCore/Public/FPhraseClassifier.h"
#include "../NeiraCore/Public/FIntentExtractor.h"
#include "../NeiraCore/Public/FActionRegistry.h"
#include "../NeiraCore/Public/FResponseGenerator.h"

#include <cstdio>

void TestPhrase(const TCHAR* InputText)
{
    FString Input(InputText);
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  ВХОД: \"%s\"\n", *Input);
    printf("═══════════════════════════════════════════════════════════\n\n");
    
    // 1. Классификация
    FPhraseClassifier Classifier;
    EPhraseType Type = Classifier.Classify(Input);
    
    const TCHAR* TypeStr = 
        Type == EPhraseType::Question  ? TEXT("Вопрос") :
        Type == EPhraseType::Command   ? TEXT("Команда") :
        Type == EPhraseType::Statement ? TEXT("Утверждение") :
        Type == EPhraseType::Request   ? TEXT("Просьба") : TEXT("Неизвестно");
    
    printf("1. КЛАССИФИКАЦИЯ: %s\n\n", TypeStr);
    
    // 2. Извлечение намерения
    FIntentExtractor Extractor;
    FIntentResult Intent = Extractor.Extract(Input, Type);
    
    const TCHAR* IntentStr = 
        Intent.IntentID == EIntentID::GetDefinition  ? TEXT("GET_DEFINITION") :
        Intent.IntentID == EIntentID::FindMeaning    ? TEXT("FIND_MEANING") :
        Intent.IntentID == EIntentID::GetWordFact    ? TEXT("GET_WORD_FACT") :
        Intent.IntentID == EIntentID::AnswerAbility  ? TEXT("ANSWER_ABILITY") :
        Intent.IntentID == EIntentID::StoreFact      ? TEXT("STORE_FACT") : TEXT("UNKNOWN");
    
    printf("2. НАМЕРЕНИЕ: %s\n", IntentStr);
    printf("   Объект: %s\n", Intent.EntityTarget.IsEmpty() ? TEXT("(нет)") : *Intent.EntityTarget);
    printf("   Уверенность: %.2f\n", Intent.Confidence);
    printf("   Trace: %s\n\n", *Intent.DecisionTrace);
    
    // 3. Выполнение действия
    FActionRegistry Registry;
    
    // Регистрируем заглушки
    Registry.Register(EActionID::GetDefinition, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Определение для \"%s\": [словарь не подключен]"), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::FindMeaning, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Значение \"%s\": [словарь не подключен]"), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::AnswerAbility, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Да, я могу %s"), *Req.EntityTarget);
        return Result;
    });
    
    Registry.Register(EActionID::StoreFact, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = TEXT("Факт сохранён в память.");
        return Result;
    });
    
    FActionRequest Request;
    Request.ActionID = static_cast<EActionID>(Intent.IntentID);
    Request.EntityTarget = Intent.EntityTarget;
    Request.Confidence = Intent.Confidence;
    
    FActionResult ActionResult = Registry.Execute(Request);
    
    printf("3. ДЕЙСТВИЕ: %s\n", ActionResult.bSuccess ? TEXT("Успех") : TEXT("Неудача"));
    if (ActionResult.bSuccess)
    {
        printf("   Результат: %s\n\n", *ActionResult.ResultText);
    }
    else
    {
        printf("   Причина: %s\n\n", *ActionResult.DiagnosticNote);
    }
    
    // 4. Генерация ответа
    FResponseGenerator ResponseGen;
    
    FResponsePersonalityProfile Profile = FResponsePersonalityProfile::MakeV1(
        EResponseTone::Calm,
        EResponseLength::Short,
        EResponseInitiative::Low,
        EResponseAddressStyle::NeutralYou
    );
    
    FResponseSemanticDecision Decision;
    Decision.IntentID = Intent.IntentID;
    Decision.SemanticCore = Intent.EntityTarget;
    Decision.bHasUncertainty = (Intent.Confidence < 0.7f);
    
    FResponseGenerationInput GenInput;
    GenInput.ContextKey = TEXT("default");
    GenInput.SemanticDecision = Decision;
    
    FResponseGenerationOutput Output = ResponseGen.Generate(GenInput, Profile);
    
    printf("4. ОТВЕТ АГЕНТА:\n");
    printf("   %s\n\n", *Output.ResponseText);
}

int main()
{
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║        NEIRA CORE — Smoke Test (v0.4)                     ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    TestPhrase(TEXT("что такое нейра?"));
    TestPhrase(TEXT("ты можешь нарисовать квадрат?"));
    TestPhrase(TEXT("найди значение слова дом"));
    TestPhrase(TEXT("кот - это животное"));
    TestPhrase(TEXT("расскажи про машинное обучение"));
    
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Smoke Test завершён\n");
    printf("═══════════════════════════════════════════════════════════\n");
    
    return 0;
}
