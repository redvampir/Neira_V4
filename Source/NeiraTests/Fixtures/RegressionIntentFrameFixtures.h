#pragma once

#include "CoreMinimal.h"
#include "NeiraTypes.h"

struct FRegressionFixture
{
    FString CaseID;
    FString Category;
    FString Phrase;
    EPhraseType PhraseType = EPhraseType::Unknown;

    EIntentID BaselineIntent = EIntentID::Unknown;
    FString BaselineSubject;
    FString BaselinePredicate;
    FString BaselineObject;
    bool BaselineAbilityCheck = false;
    bool BaselineNestedClause = false;
    bool BaselineNegation = false;
};

inline const TArray<FRegressionFixture>& GetRUENRegressionFixtures()
{
    static const TArray<FRegressionFixture> Fixtures = {
        // ability check
        { TEXT("ru_ability_01"), TEXT("ability_check"), TEXT("ты можешь объяснять слова?"), EPhraseType::Question,
          EIntentID::AnswerAbility, TEXT("ты"), TEXT("мочь"), TEXT("слово"), true, false, false },
        { TEXT("ru_ability_02"), TEXT("ability_check"), TEXT("вы можете рассказать про синтаксис?"), EPhraseType::Question,
          EIntentID::AnswerAbility, TEXT("вы"), TEXT("мочь"), TEXT("синтаксис"), true, false, false },

        // nested clauses
        { TEXT("ru_nested_01"), TEXT("nested_clauses"), TEXT("расскажи что такое морфология"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("морфология"), false, true, false },
        { TEXT("ru_nested_02"), TEXT("nested_clauses"), TEXT("скажи если кот не спит"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("сказать"), TEXT("кот"), false, true, true },

        // negation
        { TEXT("ru_negation_01"), TEXT("negation"), TEXT("не открой окно"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("открыть"), TEXT("окно"), false, false, true },
        { TEXT("ru_negation_02"), TEXT("negation"), TEXT("нельзя найти определение"), EPhraseType::Command,
          EIntentID::Unknown, TEXT(""), TEXT("найти"), TEXT("определение"), false, false, true },

        // ambiguous tokens
        { TEXT("ru_ambiguous_01"), TEXT("ambiguous_tokens"), TEXT("расскажи что такое синтаксис"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("рассказать"), TEXT("синтаксис"), false, true, false },
        { TEXT("ru_ambiguous_02"), TEXT("ambiguous_tokens"), TEXT("объясни как работает память"), EPhraseType::Request,
          EIntentID::GetWordFact, TEXT(""), TEXT("объяснить"), TEXT("память"), false, false, false },

        // fallback
        { TEXT("en_fallback_01"), TEXT("fallback"), TEXT("what is syntax"), EPhraseType::Question,
          EIntentID::Unknown, TEXT(""), TEXT(""), TEXT(""), false, false, false },
        { TEXT("en_fallback_02"), TEXT("fallback"), TEXT("can you explain morphology"), EPhraseType::Question,
          EIntentID::Unknown, TEXT(""), TEXT(""), TEXT(""), false, false, false },
        { TEXT("ru_fallback_01"), TEXT("fallback"), TEXT("бррр вжух"), EPhraseType::Unknown,
          EIntentID::Unknown, TEXT(""), TEXT(""), TEXT(""), false, false, false }
    };
    return Fixtures;
}
