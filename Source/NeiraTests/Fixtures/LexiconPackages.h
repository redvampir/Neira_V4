#pragma once

#include "CoreMinimal.h"

struct FLexiconScenarioPackage
{
    FString Name;
    TArray<FString> RequiredWords;
};

inline const TArray<FLexiconScenarioPackage>& GetLexiconScenarioPackages()
{
    static const TArray<FLexiconScenarioPackage> Packages = {
        {
            TEXT("commands_actions"),
            {
                TEXT("открой"), TEXT("закрой"), TEXT("проверь"), TEXT("запомни"), TEXT("обнови"),
                TEXT("создай"), TEXT("удали"), TEXT("сохрани"), TEXT("загрузи"), TEXT("скачай"),
                TEXT("переименуй"), TEXT("очисти"), TEXT("скопируй"), TEXT("вставь"), TEXT("выдели"),
                TEXT("отмени"), TEXT("повтори"), TEXT("исправь"), TEXT("подтверди"), TEXT("файл"),
                TEXT("папку"), TEXT("документ"), TEXT("лог"), TEXT("ошибку")
            }
        },
        {
            TEXT("household_dialog"),
            {
                TEXT("что"), TEXT("значит"), TEXT("означает"), TEXT("объясни"), TEXT("расскажи"),
                TEXT("скажи"), TEXT("время"), TEXT("дата"), TEXT("почему"), TEXT("когда"),
                TEXT("сколько"), TEXT("ошибка"), TEXT("отчёт"), TEXT("ты"), TEXT("вы"),
                TEXT("можешь"), TEXT("можете"), TEXT("как"), TEXT("про"), TEXT("о")
            }
        },
        {
            TEXT("asr_artifacts"),
            {
                TEXT("аткрой"), TEXT("акрой"), TEXT("акно"), TEXT("фаил"), TEXT("найтить"),
                TEXT("синтакез"), TEXT("марфология"), TEXT("паметь"), TEXT("чо"), TEXT("че"),
                TEXT("щас"), TEXT("ща"), TEXT("пж"), TEXT("плиз"), TEXT("кароч"),
                TEXT("кароче"), TEXT("проверька"), TEXT("запомена"), TEXT("значение"), TEXT("определение")
            }
        },
        {
            TEXT("conversational_variants"),
            {
                TEXT("пожалуйста"), TEXT("алло"), TEXT("пжлст"), TEXT("эээ")
            }
        }
    };

    return Packages;
}
