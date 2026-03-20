#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#include "FSentencePlanner.h"

#define NEIRA_TEST_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

// ---------------------------------------------------------------------------
// GetDefinition + Verified — основная стратегия
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_GetDefinition_Verified_Calm_ReturnsSentence,
    "Neira.SentencePlanner.GetDefinition.Verified_Calm_ReturnsSentence",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_GetDefinition_Verified_Calm_ReturnsSentence::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;
    const FString Result = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Calm,
        TEXT("кот"), TEXT("животное"), 0);

    TestFalse(TEXT("Результат не пустой"), Result.IsEmpty());
    // Стратегия 0: "кот — это животное."
    TestTrue(TEXT("Содержит субъект"), Result.Contains(TEXT("кот")));
    TestTrue(TEXT("Содержит объект"), Result.Contains(TEXT("животное")));
    return true;
}

// ---------------------------------------------------------------------------
// GetDefinition + Unknown — фраза неопределённости
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_GetDefinition_Unknown_ReturnsUncertaintyPhrase,
    "Neira.SentencePlanner.GetDefinition.Unknown_ReturnsUncertaintyPhrase",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_GetDefinition_Unknown_ReturnsUncertaintyPhrase::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;
    const FString Result = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Unknown, EResponseTone::Calm,
        TEXT("нейросинтаксема"), TEXT(""), 0);

    TestFalse(TEXT("Результат не пустой"), Result.IsEmpty());
    TestTrue(TEXT("Содержит субъект"), Result.Contains(TEXT("нейросинтаксема")));
    // Не должно быть объекта-пустышки
    TestFalse(TEXT("Нет данных недостаточно"), Result.Contains(TEXT("данных недостаточно")));
    return true;
}

// ---------------------------------------------------------------------------
// StoreFact — подтверждение сохранения
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_StoreFact_ReturnsAcknowledgement,
    "Neira.SentencePlanner.StoreFact.ReturnsAcknowledgement",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_StoreFact_ReturnsAcknowledgement::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;
    const FString Result = Planner.Plan(
        EIntentID::StoreFact, EConfidenceLevel::Uncertain, EResponseTone::Calm,
        TEXT("кот"), TEXT("кот"), 0);

    TestFalse(TEXT("Результат не пустой"), Result.IsEmpty());
    // StoreFact Calm: "Принято." / "Запомнила." и т.п.
    const bool bIsAck = Result.Contains(TEXT("Принято"))
                     || Result.Contains(TEXT("Запомнила"))
                     || Result.Contains(TEXT("Зафиксировала"))
                     || Result.Contains(TEXT("учту"))
                     || Result.Contains(TEXT("запомнила"));
    TestTrue(TEXT("StoreFact даёт подтверждение"), bIsAck);
    return true;
}

// ---------------------------------------------------------------------------
// Ротация: разные hints дают разные варианты
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_Rotation_ChangesPhraseVariant,
    "Neira.SentencePlanner.Rotation.ChangesPhraseVariant",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_Rotation_ChangesPhraseVariant::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;

    const FString R0 = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Calm,
        TEXT("кот"), TEXT("животное"), 0);

    const FString R1 = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Calm,
        TEXT("кот"), TEXT("животное"), 1);

    // GetDefinition+Verified+Calm имеет 5 стратегий — 0 и 1 разные
    TestTrue(TEXT("Разные hints дают разные фразы"), R0 != R1);
    return true;
}

// ---------------------------------------------------------------------------
// Детерминизм: одинаковый hint → одинаковый результат
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_Rotation_IsDeterministic,
    "Neira.SentencePlanner.Rotation.IsDeterministic",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_Rotation_IsDeterministic::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;

    const FString R0a = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Calm,
        TEXT("кот"), TEXT("животное"), 0);

    const FString R0b = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Calm,
        TEXT("кот"), TEXT("животное"), 0);

    TestEqual(TEXT("Одинаковый hint → одинаковый результат"), R0a, R0b);
    return true;
}

// ---------------------------------------------------------------------------
// Business-тон отличается от Calm
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_BusinessTone_DiffersFromCalm,
    "Neira.SentencePlanner.Tone.BusinessDiffersFromCalm",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_BusinessTone_DiffersFromCalm::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;

    const FString Calm = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Calm,
        TEXT("синтаксис"), TEXT("раздел лингвистики"), 0);

    const FString Business = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Business,
        TEXT("синтаксис"), TEXT("раздел лингвистики"), 0);

    TestTrue(TEXT("Business отличается от Calm"), Calm != Business);
    return true;
}

// ---------------------------------------------------------------------------
// Все основные комбинации intent×confidence имеют хотя бы одну стратегию
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_AllMainIntents_HaveAtLeastOneStrategy,
    "Neira.SentencePlanner.Coverage.AllMainIntents_HaveAtLeastOneStrategy",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_AllMainIntents_HaveAtLeastOneStrategy::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;

    struct FCombo { EIntentID Intent; EConfidenceLevel Conf; const TCHAR* Label; };
    const FCombo Combos[] = {
        { EIntentID::GetDefinition, EConfidenceLevel::Verified,  TEXT("Def+Verified")  },
        { EIntentID::GetDefinition, EConfidenceLevel::Inferred,  TEXT("Def+Inferred")  },
        { EIntentID::GetDefinition, EConfidenceLevel::Uncertain, TEXT("Def+Uncertain") },
        { EIntentID::GetDefinition, EConfidenceLevel::Unknown,   TEXT("Def+Unknown")   },
        { EIntentID::GetWordFact,   EConfidenceLevel::Verified,  TEXT("Fact+Verified") },
        { EIntentID::GetWordFact,   EConfidenceLevel::Unknown,   TEXT("Fact+Unknown")  },
        { EIntentID::FindMeaning,   EConfidenceLevel::Verified,  TEXT("Mng+Verified")  },
        { EIntentID::FindMeaning,   EConfidenceLevel::Unknown,   TEXT("Mng+Unknown")   },
        { EIntentID::AnswerAbility, EConfidenceLevel::Verified,  TEXT("Abl+Verified")  },
        { EIntentID::AnswerAbility, EConfidenceLevel::Unknown,   TEXT("Abl+Unknown")   },
        { EIntentID::StoreFact,     EConfidenceLevel::Uncertain, TEXT("Store+Uncertain")},
        { EIntentID::Unknown,       EConfidenceLevel::Unknown,   TEXT("Unk+Unknown")   },
    };

    for (const FCombo& C : Combos)
    {
        const int32 Count = Planner.GetStrategyCount(C.Intent, C.Conf, EResponseTone::Calm);
        const FString Desc = FString::Printf(TEXT("%s — есть хотя бы одна стратегия"), C.Label);
        TestTrue(*Desc, Count > 0);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Fill: плейсхолдеры {Subject} и {Object} заменяются корректно
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSentencePlanner_Fill_ReplacesSubjectAndObject,
    "Neira.SentencePlanner.Fill.ReplacesSubjectAndObject",
    NEIRA_TEST_FLAGS)
bool FSentencePlanner_Fill_ReplacesSubjectAndObject::RunTest(const FString& Parameters)
{
    FSentencePlanner Planner;

    // Стратегия 0 для GetDefinition+Verified+Calm: "{Subject} — это {Object}."
    const FString Result = Planner.Plan(
        EIntentID::GetDefinition, EConfidenceLevel::Verified, EResponseTone::Calm,
        TEXT("берёза"), TEXT("дерево"), 0);

    TestTrue(TEXT("Subject заменён"), Result.Contains(TEXT("берёза")));
    TestTrue(TEXT("Object заменён"),  Result.Contains(TEXT("дерево")));
    TestFalse(TEXT("Плейсхолдер {Subject} удалён"), Result.Contains(TEXT("{Subject}")));
    TestFalse(TEXT("Плейсхолдер {Object} удалён"),  Result.Contains(TEXT("{Object}")));
    return true;
}
