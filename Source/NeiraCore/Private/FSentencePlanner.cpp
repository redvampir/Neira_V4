#include "FSentencePlanner.h"

// ---------------------------------------------------------------------------
// Вспомогательные макросы для читаемого заполнения библиотеки
// ---------------------------------------------------------------------------
#define ADD(id, intent, conf, tone, pattern) \
    Library.Add({ TEXT(id), EIntentID::intent, EConfidenceLevel::conf, EResponseTone::tone, TEXT(pattern) })

// ---------------------------------------------------------------------------
// BuildLibrary — ~55 стратегий, только именительный падеж (Фаза 1)
// ---------------------------------------------------------------------------
void FSentencePlanner::BuildLibrary()
{
    // -----------------------------------------------------------------------
    // GetDefinition + Verified
    // -----------------------------------------------------------------------
    ADD("Def_V_Calm_0",  GetDefinition, Verified, Calm,     "{Subject} — это {Object}.");
    ADD("Def_V_Calm_1",  GetDefinition, Verified, Calm,     "{Subject}: {Object}.");
    ADD("Def_V_Calm_2",  GetDefinition, Verified, Calm,     "Понятие «{Subject}»: {Object}.");
    ADD("Def_V_Calm_3",  GetDefinition, Verified, Calm,     "Если коротко: {Subject} — {Object}.");
    ADD("Def_V_Calm_4",  GetDefinition, Verified, Calm,     "О «{Subject}»: {Object}.");

    ADD("Def_V_Bus_0",   GetDefinition, Verified, Business, "{Subject}: {Object}.");
    ADD("Def_V_Bus_1",   GetDefinition, Verified, Business, "Данные по «{Subject}»: {Object}.");
    ADD("Def_V_Bus_2",   GetDefinition, Verified, Business, "Определение: {Subject} — {Object}.");
    ADD("Def_V_Bus_3",   GetDefinition, Verified, Business, "Справка: {Subject} — {Object}.");

    // -----------------------------------------------------------------------
    // GetDefinition + Inferred
    // -----------------------------------------------------------------------
    ADD("Def_I_Calm_0",  GetDefinition, Inferred, Calm,     "Судя по всему, {Subject} — это {Object}.");
    ADD("Def_I_Calm_1",  GetDefinition, Inferred, Calm,     "Насколько мне известно, {Subject} — {Object}.");
    ADD("Def_I_Calm_2",  GetDefinition, Inferred, Calm,     "Полагаю, что {Subject} — это {Object}.");

    ADD("Def_I_Bus_0",   GetDefinition, Inferred, Business, "По имеющимся данным, {Subject} — {Object}.");
    ADD("Def_I_Bus_1",   GetDefinition, Inferred, Business, "Предположительно: {Subject} — {Object}.");
    ADD("Def_I_Bus_2",   GetDefinition, Inferred, Business, "Вероятно: {Subject} — {Object}.");

    // -----------------------------------------------------------------------
    // GetDefinition + Uncertain
    // -----------------------------------------------------------------------
    ADD("Def_U_Calm_0",  GetDefinition, Uncertain, Calm,    "Точных данных нет, но, возможно, {Subject} — {Object}.");
    ADD("Def_U_Calm_1",  GetDefinition, Uncertain, Calm,    "Не уверена, но {Subject} — вероятно, {Object}.");

    ADD("Def_U_Bus_0",   GetDefinition, Uncertain, Business,"Точность не гарантирована: {Subject} — {Object}.");
    ADD("Def_U_Bus_1",   GetDefinition, Uncertain, Business,"Неполные данные: {Subject} — {Object}.");

    // -----------------------------------------------------------------------
    // GetDefinition + Unknown
    // -----------------------------------------------------------------------
    ADD("Def_N_Calm_0",  GetDefinition, Unknown, Calm,      "О «{Subject}» у меня нет данных.");
    ADD("Def_N_Calm_1",  GetDefinition, Unknown, Calm,      "С «{Subject}» я пока незнакома.");
    ADD("Def_N_Calm_2",  GetDefinition, Unknown, Calm,      "«{Subject}» — вне моих текущих знаний.");
    ADD("Def_N_Calm_3",  GetDefinition, Unknown, Calm,      "Не могу ничего сказать о «{Subject}».");

    ADD("Def_N_Bus_0",   GetDefinition, Unknown, Business,  "Данных по «{Subject}» нет.");
    ADD("Def_N_Bus_1",   GetDefinition, Unknown, Business,  "«{Subject}»: нет информации.");
    ADD("Def_N_Bus_2",   GetDefinition, Unknown, Business,  "Информация о «{Subject}» отсутствует.");

    // -----------------------------------------------------------------------
    // GetWordFact + Verified
    // -----------------------------------------------------------------------
    ADD("Fact_V_Calm_0", GetWordFact,   Verified, Calm,     "«{Subject}» — {Object}.");
    ADD("Fact_V_Calm_1", GetWordFact,   Verified, Calm,     "Факт: {Subject} — {Object}.");
    ADD("Fact_V_Calm_2", GetWordFact,   Verified, Calm,     "Знаю о «{Subject}»: {Object}.");

    ADD("Fact_V_Bus_0",  GetWordFact,   Verified, Business, "Факт: {Subject} — {Object}.");
    ADD("Fact_V_Bus_1",  GetWordFact,   Verified, Business, "{Subject}: {Object}.");

    // -----------------------------------------------------------------------
    // GetWordFact + Unknown
    // -----------------------------------------------------------------------
    ADD("Fact_N_Calm_0", GetWordFact,   Unknown, Calm,      "О «{Subject}» у меня нет точных данных.");
    ADD("Fact_N_Calm_1", GetWordFact,   Unknown, Calm,      "Не знаю фактов о «{Subject}».");

    ADD("Fact_N_Bus_0",  GetWordFact,   Unknown, Business,  "Данных по «{Subject}» нет.");
    ADD("Fact_N_Bus_1",  GetWordFact,   Unknown, Business,  "«{Subject}»: информация отсутствует.");

    // -----------------------------------------------------------------------
    // FindMeaning + Verified
    // -----------------------------------------------------------------------
    ADD("Mng_V_Calm_0",  FindMeaning,  Verified, Calm,      "«{Subject}» означает: {Object}.");
    ADD("Mng_V_Calm_1",  FindMeaning,  Verified, Calm,      "Смысл «{Subject}»: {Object}.");
    ADD("Mng_V_Calm_2",  FindMeaning,  Verified, Calm,      "«{Subject}» — это {Object}.");

    ADD("Mng_V_Bus_0",   FindMeaning,  Verified, Business,  "Смысл «{Subject}»: {Object}.");
    ADD("Mng_V_Bus_1",   FindMeaning,  Verified, Business,  "Значение: {Subject} — {Object}.");

    // -----------------------------------------------------------------------
    // FindMeaning + Unknown
    // -----------------------------------------------------------------------
    ADD("Mng_N_Calm_0",  FindMeaning,  Unknown, Calm,       "Смысл «{Subject}» мне неизвестен.");
    ADD("Mng_N_Calm_1",  FindMeaning,  Unknown, Calm,       "Не знаю, что означает «{Subject}».");

    ADD("Mng_N_Bus_0",   FindMeaning,  Unknown, Business,   "«{Subject}»: значение не найдено.");
    ADD("Mng_N_Bus_1",   FindMeaning,  Unknown, Business,   "Значение «{Subject}» отсутствует в базе.");

    // -----------------------------------------------------------------------
    // AnswerAbility + Verified
    // -----------------------------------------------------------------------
    ADD("Abl_V_Calm_0",  AnswerAbility, Verified, Calm,     "Да, с «{Subject}» я работаю.");
    ADD("Abl_V_Calm_1",  AnswerAbility, Verified, Calm,     "Да, умею работать с «{Subject}».");
    ADD("Abl_V_Calm_2",  AnswerAbility, Verified, Calm,     "Такое в моих силах.");

    ADD("Abl_V_Bus_0",   AnswerAbility, Verified, Business, "«{Subject}»: выполнимо.");
    ADD("Abl_V_Bus_1",   AnswerAbility, Verified, Business, "Запрос «{Subject}» поддерживается.");

    // -----------------------------------------------------------------------
    // AnswerAbility + Unknown
    // -----------------------------------------------------------------------
    ADD("Abl_N_Calm_0",  AnswerAbility, Unknown, Calm,      "Пока не уверена насчёт «{Subject}».");
    ADD("Abl_N_Calm_1",  AnswerAbility, Unknown, Calm,      "Не знаю, смогу ли с «{Subject}».");

    ADD("Abl_N_Bus_0",   AnswerAbility, Unknown, Business,  "«{Subject}»: возможность не определена.");
    ADD("Abl_N_Bus_1",   AnswerAbility, Unknown, Business,  "Статус «{Subject}»: неизвестно.");

    // -----------------------------------------------------------------------
    // StoreFact + Uncertain (Created — первое сохранение)
    // -----------------------------------------------------------------------
    ADD("Store_U_Calm_0",StoreFact,    Uncertain, Calm,     "Принято.");
    ADD("Store_U_Calm_1",StoreFact,    Uncertain, Calm,     "Запомнила.");
    ADD("Store_U_Calm_2",StoreFact,    Uncertain, Calm,     "Хорошо, учту.");
    ADD("Store_U_Calm_3",StoreFact,    Uncertain, Calm,     "Понятно, зафиксировала.");
    ADD("Store_U_Calm_4",StoreFact,    Uncertain, Calm,     "Ладно, это запомнила.");

    ADD("Store_U_Bus_0", StoreFact,    Uncertain, Business, "Принято.");
    ADD("Store_U_Bus_1", StoreFact,    Uncertain, Business, "Зафиксировано.");
    ADD("Store_U_Bus_2", StoreFact,    Uncertain, Business, "Записала.");

    // -----------------------------------------------------------------------
    // Unknown / fallback
    // -----------------------------------------------------------------------
    ADD("Unk_N_Calm_0",  Unknown,      Unknown, Calm,       "Не совсем понимаю запрос.");
    ADD("Unk_N_Calm_1",  Unknown,      Unknown, Calm,       "Уточните, пожалуйста.");
    ADD("Unk_N_Calm_2",  Unknown,      Unknown, Calm,       "Не могу обработать запрос.");

    ADD("Unk_N_Bus_0",   Unknown,      Unknown, Business,   "Запрос не распознан.");
    ADD("Unk_N_Bus_1",   Unknown,      Unknown, Business,   "Уточните запрос.");
}

#undef ADD

// ---------------------------------------------------------------------------
// Конструктор
// ---------------------------------------------------------------------------
FSentencePlanner::FSentencePlanner()
{
    BuildLibrary();
}

// ---------------------------------------------------------------------------
// GetCandidates
// ---------------------------------------------------------------------------
TArray<FSyntacticStrategy> FSentencePlanner::GetCandidates(EIntentID        IntentID,
                                                            EConfidenceLevel Confidence,
                                                            EResponseTone    Tone) const
{
    // Попытка 1: точное совпадение
    TArray<FSyntacticStrategy> Result;
    for (const FSyntacticStrategy& S : Library)
    {
        if (S.IntentID == IntentID && S.Confidence == Confidence && S.Tone == Tone)
            Result.Add(S);
    }
    if (!Result.IsEmpty())
        return Result;

    // Попытка 2: тот же IntentID + Confidence, но Calm-тон как запасной
    if (Tone != EResponseTone::Calm)
    {
        for (const FSyntacticStrategy& S : Library)
        {
            if (S.IntentID == IntentID && S.Confidence == Confidence
                && S.Tone == EResponseTone::Calm)
                Result.Add(S);
        }
        if (!Result.IsEmpty())
            return Result;
    }

    // Попытка 3: Unknown-intent как универсальный fallback
    for (const FSyntacticStrategy& S : Library)
    {
        if (S.IntentID == EIntentID::Unknown && S.Confidence == EConfidenceLevel::Unknown
            && S.Tone == Tone)
            Result.Add(S);
    }
    if (!Result.IsEmpty())
        return Result;

    // Попытка 4: Unknown-intent, Calm-тон
    for (const FSyntacticStrategy& S : Library)
    {
        if (S.IntentID == EIntentID::Unknown && S.Confidence == EConfidenceLevel::Unknown
            && S.Tone == EResponseTone::Calm)
            Result.Add(S);
    }
    return Result;
}

// ---------------------------------------------------------------------------
// Fill — заменить {Subject} и {Object}
// ---------------------------------------------------------------------------
FString FSentencePlanner::Fill(const FString& Pattern,
                                const FString& Subject,
                                const FString& Object)
{
    FString Result = Pattern;
    Result = Result.Replace(TEXT("{Subject}"), *Subject);
    Result = Result.Replace(TEXT("{Object}"),  *Object);
    return Result;
}

// ---------------------------------------------------------------------------
// Plan
// ---------------------------------------------------------------------------
FString FSentencePlanner::Plan(EIntentID        IntentID,
                                EConfidenceLevel Confidence,
                                EResponseTone    Tone,
                                const FString&   Subject,
                                const FString&   Object,
                                int32            RotationHint) const
{
    TArray<FSyntacticStrategy> Candidates = GetCandidates(IntentID, Confidence, Tone);
    if (Candidates.IsEmpty())
    {
        // Абсолютный fallback: вернуть Object как есть
        return Object.IsEmpty() ? TEXT("Обработка запроса.") : Object;
    }

    const int32 Index = (RotationHint < 0 ? 0 : RotationHint) % Candidates.Num();
    return Fill(Candidates[Index].PatternFmt, Subject, Object);
}

// ---------------------------------------------------------------------------
// GetStrategyId
// ---------------------------------------------------------------------------
FString FSentencePlanner::GetStrategyId(EIntentID        IntentID,
                                         EConfidenceLevel Confidence,
                                         EResponseTone    Tone,
                                         int32            RotationHint) const
{
    TArray<FSyntacticStrategy> Candidates = GetCandidates(IntentID, Confidence, Tone);
    if (Candidates.IsEmpty())
        return TEXT("fallback");

    const int32 Index = (RotationHint < 0 ? 0 : RotationHint) % Candidates.Num();
    return Candidates[Index].StrategyId;
}

// ---------------------------------------------------------------------------
// GetStrategyCount
// ---------------------------------------------------------------------------
int32 FSentencePlanner::GetStrategyCount(EIntentID        IntentID,
                                          EConfidenceLevel Confidence,
                                          EResponseTone    Tone) const
{
    return GetCandidates(IntentID, Confidence, Tone).Num();
}
