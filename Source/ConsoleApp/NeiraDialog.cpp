/**
 * Neira Dialog Console — консольный диалог с Нейрой.
 *
 * Цель этой версии:
 * - не падать в интерактивном режиме и при EOF;
 * - держать простую память сессии;
 * - честно отвечать на базовые пользовательские сценарии.
 */

#include "../Tests/ue_compat.h"
#include "../NeiraCore/Public/NeiraTypes.h"
#include "../NeiraCore/Public/FPhraseClassifier.h"
#include "../NeiraCore/Public/FIntentExtractor.h"
#include "../NeiraCore/Public/FActionRegistry.h"
#include "../NeiraCore/Public/FResponseGenerator.h"
#include "../NeiraCore/Public/FHypothesisStore.h"
#include "../NeiraCore/Public/FBeliefEngine.h"

#include <iostream>
#include <string>

namespace
{
struct FDialogSession
{
    FPhraseClassifier Classifier;
    FIntentExtractor Extractor;
    FHypothesisStore MemoryStore;
    TArray<FString> UserHistory;
    int32 LastStoredHypothesisID = INDEX_NONE;
};

FString PhraseTypeToString(EPhraseType Type)
{
    switch (Type)
    {
    case EPhraseType::Question:      return TEXT("Вопрос");
    case EPhraseType::Command:       return TEXT("Команда");
    case EPhraseType::Statement:     return TEXT("Утверждение");
    case EPhraseType::Request:       return TEXT("Просьба");
    default:                         return TEXT("Неизвестно");
    }
}

FString IntentToString(EIntentID Intent)
{
    switch (Intent)
    {
    case EIntentID::GetDefinition:   return TEXT("GET_DEFINITION");
    case EIntentID::GetWordFact:     return TEXT("GET_WORD_FACT");
    case EIntentID::FindMeaning:     return TEXT("FIND_MEANING");
    case EIntentID::AnswerAbility:   return TEXT("ANSWER_ABILITY");
    case EIntentID::StoreFact:       return TEXT("STORE_FACT");
    case EIntentID::RetrieveMemory:  return TEXT("RETRIEVE_MEMORY");
    case EIntentID::CheckMemoryLoad: return TEXT("CHECK_MEMORY_LOAD");
    default:                         return TEXT("UNKNOWN");
    }
}

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

FString TrimPunctuation(const FString& Value)
{
    FString Result = Value.TrimStartAndEnd();

    while (!Result.IsEmpty())
    {
        const char Tail = Result.Last();
        if (Tail == '?' || Tail == '!' || Tail == '.' || Tail == ',' || Tail == ':')
        {
            Result = Result.LeftChop(1).TrimEnd();
            continue;
        }
        break;
    }

    while (!Result.IsEmpty())
    {
        const char Head = Result[0];
        if (Head == '"' || Head == '\'' || Head == ' ')
        {
            Result = Result.Mid(1).TrimStart();
            continue;
        }
        break;
    }

    return Result;
}

FString NormalizeLookupKey(const FString& Value)
{
    FString Result = TrimPunctuation(Value).ToLower();
    Result = Result.Replace("?", "");
    Result = Result.Replace("!", "");
    Result = Result.Replace(".", "");
    Result = Result.Replace(",", "");
    Result = Result.Replace(":", "");
    return Result.TrimStartAndEnd();
}

bool IsVisualAction(const FString& ActionLower)
{
    return ActionLower.Contains(TEXT("нарис"))
        || ActionLower.Contains(TEXT("рисова"))
        || ActionLower.Contains(TEXT("картин"))
        || ActionLower.Contains(TEXT("изображ"));
}

const FHypothesis* GetLastStoredHypothesis(const FDialogSession& Session)
{
    if (Session.LastStoredHypothesisID != INDEX_NONE)
    {
        const FHypothesis* Hypothesis = Session.MemoryStore.Find(Session.LastStoredHypothesisID);
        if (Hypothesis != nullptr)
            return Hypothesis;
    }

    for (int32 Index = Session.MemoryStore.Count() - 1; Index >= 0; --Index)
    {
        const FHypothesis* Hypothesis = Session.MemoryStore.Find(Index);
        if (Hypothesis != nullptr)
            return Hypothesis;
    }

    return nullptr;
}

const FHypothesis* FindBestMemoryMatch(const FDialogSession& Session, const FString& RawQuery)
{
    const FString Query = NormalizeLookupKey(RawQuery);
    if (Query.IsEmpty())
        return GetLastStoredHypothesis(Session);

    const FHypothesis* Best = nullptr;
    int32 BestScore = -1;

    TArray<FString> QueryTokens;
    Query.ParseIntoArrayWS(QueryTokens);

    for (int32 Index = Session.MemoryStore.Count() - 1; Index >= 0; --Index)
    {
        const FHypothesis* Hypothesis = Session.MemoryStore.Find(Index);
        if (Hypothesis == nullptr)
            continue;

        const FString Claim = NormalizeLookupKey(Hypothesis->Claim);
        int32 Score = -1;

        if (Claim == Query)
        {
            Score = 1000;
        }
        else if (Claim.Contains(Query))
        {
            Score = 800 + Query.Len();
        }
        else if (Query.Contains(Claim))
        {
            Score = 600 + Claim.Len();
        }
        else
        {
            int32 TokenHits = 0;
            for (const FString& Token : QueryTokens)
            {
                if (Token.Len() < 3)
                    continue;
                if (Claim.Contains(Token))
                    ++TokenHits;
            }

            if (TokenHits > 0)
                Score = TokenHits * 100 + Claim.Len();
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            Best = Hypothesis;
        }
    }

    return Best;
}

void RememberUserTurn(FDialogSession& Session, const FString& Input, EIntentID IntentID)
{
    if (IntentID == EIntentID::RetrieveMemory)
        return;

    Session.UserHistory.Add(Input.TrimStartAndEnd());
}

FString BuildFallbackResponse(const FIntentResult& IntentResult, bool bVerbose)
{
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
        Decision.UncertaintyReason = TEXT("низкая уверенность разбора");

    FResponseGenerationInput GenInput;
    GenInput.ContextKey = TEXT("dialog");
    GenInput.SemanticDecision = Decision;

    const FResponseGenerationOutput ResponseOutput = ResponseGen.Generate(GenInput, Profile);

    if (bVerbose)
        printf("  [4] Ответ агента:\n      %s\n", *ResponseOutput.ResponseText);

    return ResponseOutput.ResponseText;
}
}

FString ProcessPhrase(FDialogSession& Session, const FString& Input, bool bVerbose = false)
{
    const FString TrimmedInput = Input.TrimStartAndEnd();
    if (TrimmedInput.IsEmpty())
        return TEXT("Не услышала ничего. Попробуйте ещё раз.");

    const EPhraseType PhraseType = Session.Classifier.Classify(TrimmedInput);

    if (bVerbose)
        printf("  [1] Классификация: %s\n", *PhraseTypeToString(PhraseType));

    const FIntentResult IntentResult = Session.Extractor.Extract(TrimmedInput, PhraseType);

    if (bVerbose)
    {
        printf("  [2] Намерение: %s (conf: %.2f)\n", *IntentToString(IntentResult.IntentID), IntentResult.Confidence);
        printf("      Trace: %s\n", *IntentResult.DecisionTrace);
        if (!IntentResult.DiagnosticNote.IsEmpty())
            printf("      Note: %s\n", *IntentResult.DiagnosticNote);
    }

    FActionRegistry Registry;

    Registry.Register(EActionID::GetDefinition, [&Session](const FActionRequest& Req) {
        FActionResult Result;
        if (const FHypothesis* Match = FindBestMemoryMatch(Session, Req.EntityTarget))
        {
            Result.bSuccess = true;
            Result.ResultText = FString::Printf(TEXT("Я помню такой факт: %s."), *Match->Claim);
            return Result;
        }

        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("«%s» — это запрос на определение. Морфологический словарь подключен, но полноценная база определений еще не собрана, поэтому пока дам только базовый ответ."), *Req.EntityTarget);
        return Result;
    });

    Registry.Register(EActionID::FindMeaning, [&Session](const FActionRequest& Req) {
        FActionResult Result;
        if (const FHypothesis* Match = FindBestMemoryMatch(Session, Req.EntityTarget))
        {
            Result.bSuccess = true;
            Result.ResultText = FString::Printf(TEXT("По памяти сессии у меня есть такой факт: %s."), *Match->Claim);
            return Result;
        }

        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Значение слова «%s» пока не раскрыто полностью: внешний словарь ещё не интегрирован."), *Req.EntityTarget);
        return Result;
    });

    Registry.Register(EActionID::GetWordFact, [&Session](const FActionRequest& Req) {
        FActionResult Result;
        if (const FHypothesis* Match = FindBestMemoryMatch(Session, Req.EntityTarget))
        {
            Result.bSuccess = true;
            Result.ResultText = FString::Printf(TEXT("Из памяти сессии: %s."), *Match->Claim);
            return Result;
        }

        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Про «%s» могу пока сказать только общее: база знаний ещё не подключена."), *Req.EntityTarget);
        return Result;
    });

    Registry.Register(EActionID::AnswerAbility, [](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;

        const FString Action = TrimPunctuation(Req.EntityTarget);
        const FString ActionLower = Action.ToLower();

        if (Action.IsEmpty())
        {
            Result.ResultText = TEXT("Могу отвечать на простые вопросы, запоминать факты в рамках текущей сессии и объяснять, как я поняла фразу.");
        }
        else if (IsVisualAction(ActionLower))
        {
            Result.ResultText = FString::Printf(TEXT("Графически %s в этой консольной сборке я не умею, но могу описать это текстом."), *Action);
        }
        else
        {
            Result.ResultText = FString::Printf(TEXT("Могу попробовать: %s. В этой сборке я работаю как текстовое языковое ядро."), *Action);
        }

        return Result;
    });

    Registry.Register(EActionID::StoreFact, [&Session, IntentResult](const FActionRequest& Req) {
        FActionResult Result;
        const FString Claim = TrimPunctuation(Req.EntityTarget);

        if (Claim.IsEmpty())
        {
            Result.bSuccess = false;
            Result.FailReason = EActionFailReason::PartialParse;
            Result.DiagnosticNote = TEXT("Не удалось извлечь утверждение для сохранения.");
            return Result;
        }

        int32 HypothesisID = Session.MemoryStore.FindByClaim(Claim);
        if (HypothesisID == INDEX_NONE)
        {
            FHypothesis Hypothesis;
            Hypothesis.Claim = Claim;
            Hypothesis.Confidence = IntentResult.Confidence;
            Hypothesis.Source = TEXT("DialogSession");
            Hypothesis.SourceType = EHypothesisSource::UserConfirm;
            HypothesisID = Session.MemoryStore.Store(Hypothesis);
            Session.LastStoredHypothesisID = HypothesisID;

            Result.bSuccess = true;
            Result.ResultText = FString::Printf(TEXT("Запомнила: %s."), *Claim);
            return Result;
        }

        Session.MemoryStore.Confirm(HypothesisID, TEXT("Повторное подтверждение в диалоге"));
        Session.LastStoredHypothesisID = HypothesisID;

        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Я уже помню это и усилила уверенность: %s."), *Claim);
        return Result;
    });

    Registry.Register(EActionID::RetrieveMemory, [&Session](const FActionRequest& Req) {
        FActionResult Result;
        Result.bSuccess = true;

        if (Req.EntityTarget == TEXT("__last_user_utterance__"))
        {
            if (Session.UserHistory.IsEmpty())
            {
                Result.ResultText = TEXT("Пока не могу ничего вспомнить из предыдущих реплик этого диалога.");
                return Result;
            }

            Result.ResultText = FString::Printf(TEXT("Вы сказали: %s."), *Session.UserHistory.back());
            return Result;
        }

        if (Req.EntityTarget == TEXT("__last_stored_fact__"))
        {
            if (const FHypothesis* Last = GetLastStoredHypothesis(Session))
            {
                Result.ResultText = FString::Printf(TEXT("Я помню такой факт: %s."), *Last->Claim);
                return Result;
            }

            Result.ResultText = TEXT("Пока у меня нет сохранённых фактов в этой сессии.");
            return Result;
        }

        if (const FHypothesis* Match = FindBestMemoryMatch(Session, Req.EntityTarget))
        {
            Result.ResultText = FString::Printf(TEXT("По памяти сессии ближе всего подходит: %s."), *Match->Claim);
            return Result;
        }

        Result.ResultText = TEXT("В памяти этой сессии ничего подходящего не нашлось.");
        return Result;
    });

    Registry.Register(EActionID::CheckMemoryLoad, [&Session](const FActionRequest&) {
        FActionResult Result;
        Result.bSuccess = true;
        Result.ResultText = FString::Printf(TEXT("Память сессии активна. Сейчас сохранено %d гипотез."), Session.MemoryStore.Count());
        return Result;
    });

    FActionRequest Request;
    Request.ActionID = static_cast<EActionID>(IntentResult.IntentID);
    Request.EntityTarget = IntentResult.EntityTarget;
    Request.Confidence = IntentResult.Confidence;

    const FActionResult ActionResult = Registry.Execute(Request);

    if (bVerbose)
    {
        printf("  [3] Действие: %s\n", ActionResult.bSuccess ? TEXT("Успех") : TEXT("Неудача"));
        if (ActionResult.bSuccess)
        {
            printf("      Результат: %s\n", *ActionResult.ResultText);
        }
        else
        {
            printf("      FailReason: %s\n", *FailReasonToString(ActionResult.FailReason));
            if (!ActionResult.DiagnosticNote.IsEmpty())
                printf("      Diagnostic: %s\n", *ActionResult.DiagnosticNote);
        }
    }

    const FString Response = (ActionResult.bSuccess && !ActionResult.ResultText.IsEmpty())
        ? ActionResult.ResultText
        : BuildFallbackResponse(IntentResult, bVerbose);

    RememberUserTurn(Session, TrimmedInput, IntentResult.IntentID);
    return Response;
}

void PrintHelp()
{
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("   Neira Dialog — интерактивный диалог с ИИ-агентом\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("  Введите фразу для общения (на русском):\n");
    printf("\n");
    printf("  Примеры:\n");
    printf("    • что такое нейра?\n");
    printf("    • ты можешь нарисовать квадрат?\n");
    printf("    • найди значение слова дом\n");
    printf("    • кот — это животное\n");
    printf("    • запомни что москва столица россии\n");
    printf("    • что ты запомнила?\n");
    printf("    • что я сказал?\n");
    printf("    • кто такой пушкин?\n");
    printf("\n");
    printf("  Команды:\n");
    printf("    help     — показать эту справку\n");
    printf("    verbose  — включить подробный вывод (pipeline)\n");
    printf("    quiet    — отключить подробный вывод\n");
    printf("    quit     — выход\n");
    printf("\n");
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    bool bVerbose = false;

    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║         NEIRA DIALOG — КОНСОЛЬНЫЙ ДИАЛОГ                  ║\n");
    printf("║                    версия v0.5                            ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Ядро: C++17, без зависимостей от Unreal Engine\n");
    printf("  Словарь: встроенный (~1000 слов) + OpenCorpora (lazy-load) + суффиксные правила\n");
    printf("  Память сессии: включена\n");
    printf("\n");

    PrintHelp();

    FDialogSession Session;

    std::string InputLine;

    while (true)
    {
        printf("\n┌──────────────────────────────────────────────────────────┐\n");
        printf("│ Вы: ");

        if (!std::getline(std::cin, InputLine))
        {
            printf("│ Нейра: Ввод завершён. Завершаю диалог.\n");
            printf("└──────────────────────────────────────────────────────────┘\n");
            break;
        }

        if (InputLine.empty())
        {
            printf("│ (пустой ввод — введите 'help' для справки)\n");
            printf("└──────────────────────────────────────────────────────────┘\n");
            continue;
        }

        if (InputLine == "quit" || InputLine == "exit" || InputLine == "выход")
        {
            printf("│ Нейра: До свидания! Было приятно пообщаться.\n");
            printf("└──────────────────────────────────────────────────────────┘\n");
            break;
        }

        if (InputLine == "help" || InputLine == "h" || InputLine == "?")
        {
            PrintHelp();
            continue;
        }

        if (InputLine == "verbose" || InputLine == "v")
        {
            bVerbose = true;
            printf("│ [Подробный режим включён]\n");
            printf("└──────────────────────────────────────────────────────────┘\n");
            continue;
        }

        if (InputLine == "quiet" || InputLine == "q")
        {
            bVerbose = false;
            printf("│ [Подробный режим выключен]\n");
            printf("└──────────────────────────────────────────────────────────┘\n");
            continue;
        }

        const FString Input(InputLine.c_str());

        if (bVerbose)
        {
            printf("├──────────────────────────────────────────────────────────┤\n");
            printf("│ Обработка:\n");
        }

        const FString Response = ProcessPhrase(Session, Input, bVerbose);

        printf("│\n");
        printf("│ Нейра: %s\n", *Response);
        printf("└──────────────────────────────────────────────────────────┘\n");
    }

    printf("\n");
    printf("  Спасибо за диалог с Нейрой!\n");
    printf("\n");

    return 0;
}
