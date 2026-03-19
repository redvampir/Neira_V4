// FMorphAnalyzer.cpp
// v0.2 — лёгкая морфология: словарь-ядро + суффиксные правила.
//
// Принцип: сначала словарь (confidence 0.95), потом суффиксы (0.65).
// Словарь содержит ~200 частотных слов-зёрен — достаточно для v0.2.
// В v0.3+ он будет пополняться через отдельный DSL/CSV-файл ресурсов.
//
// Суффиксные правила:
//   Глаголы: -ать/-ять/-ить/-еть/-уть/-ыть (инфинитивы)
//             -ает/-яет/-ит/-ет/-ют/-ают (3-е лицо ед.ч.)
//             -ай/-яй/-и/-ите/-айте/-яйте (повелит.)
//   Существительные: -ость/-ность/-ние/-тие/-ция/-сть/-тель/-ик/-щик
//   Прилагательные: -ный/-ной/-ная/-ное/-ные/-кий/-гий/-жий/-щий/-ской

#include "FMorphAnalyzer.h"

namespace
{
    // -----------------------------------------------------------------------
    // Тип записи в словаре
    // -----------------------------------------------------------------------
    struct FDictEntry
    {
        const TCHAR* Word;   // нижний регистр, нормализованная форма
        const TCHAR* Lemma;
        EPosTag      POS;
    };

    // -----------------------------------------------------------------------
    // Словарь-ядро (~200 слов)
    // Охватывает: служебные слова, местоимения, частотные глаголы и существительные.
    // -----------------------------------------------------------------------
    const FDictEntry CoreDictionary[] = {
        // --- местоимения ---
        { TEXT("я"),        TEXT("я"),        EPosTag::Pronoun     },
        { TEXT("ты"),       TEXT("ты"),       EPosTag::Pronoun     },
        { TEXT("он"),       TEXT("он"),       EPosTag::Pronoun     },
        { TEXT("она"),      TEXT("она"),      EPosTag::Pronoun     },
        { TEXT("оно"),      TEXT("оно"),      EPosTag::Pronoun     },
        { TEXT("мы"),       TEXT("мы"),       EPosTag::Pronoun     },
        { TEXT("вы"),       TEXT("вы"),       EPosTag::Pronoun     },
        { TEXT("они"),      TEXT("они"),      EPosTag::Pronoun     },
        { TEXT("мне"),      TEXT("я"),        EPosTag::Pronoun     },
        { TEXT("тебе"),     TEXT("ты"),       EPosTag::Pronoun     },
        { TEXT("меня"),     TEXT("я"),        EPosTag::Pronoun     },
        { TEXT("его"),      TEXT("он"),       EPosTag::Pronoun     },
        { TEXT("её"),       TEXT("она"),      EPosTag::Pronoun     },
        { TEXT("нас"),      TEXT("мы"),       EPosTag::Pronoun     },
        { TEXT("вас"),      TEXT("вы"),       EPosTag::Pronoun     },
        { TEXT("их"),       TEXT("они"),      EPosTag::Pronoun     },

        // --- служебные: предлоги ---
        { TEXT("в"),        TEXT("в"),        EPosTag::Preposition },
        { TEXT("на"),       TEXT("на"),       EPosTag::Preposition },
        { TEXT("из"),       TEXT("из"),       EPosTag::Preposition },
        { TEXT("к"),        TEXT("к"),        EPosTag::Preposition },
        { TEXT("ко"),       TEXT("к"),        EPosTag::Preposition },
        { TEXT("от"),       TEXT("от"),       EPosTag::Preposition },
        { TEXT("до"),       TEXT("до"),       EPosTag::Preposition },
        { TEXT("для"),      TEXT("для"),      EPosTag::Preposition },
        { TEXT("по"),       TEXT("по"),       EPosTag::Preposition },
        { TEXT("за"),       TEXT("за"),       EPosTag::Preposition },
        { TEXT("под"),      TEXT("под"),      EPosTag::Preposition },
        { TEXT("над"),      TEXT("над"),      EPosTag::Preposition },
        { TEXT("перед"),    TEXT("перед"),    EPosTag::Preposition },
        { TEXT("между"),    TEXT("между"),    EPosTag::Preposition },
        { TEXT("о"),        TEXT("о"),        EPosTag::Preposition },
        { TEXT("об"),       TEXT("о"),        EPosTag::Preposition },
        { TEXT("при"),      TEXT("при"),      EPosTag::Preposition },
        { TEXT("через"),    TEXT("через"),    EPosTag::Preposition },
        { TEXT("без"),      TEXT("без"),      EPosTag::Preposition },
        { TEXT("про"),      TEXT("про"),      EPosTag::Preposition },

        // --- служебные: союзы ---
        { TEXT("и"),        TEXT("и"),        EPosTag::Conjunction },
        { TEXT("или"),      TEXT("или"),      EPosTag::Conjunction },
        { TEXT("но"),       TEXT("но"),       EPosTag::Conjunction },
        { TEXT("что"),      TEXT("что"),      EPosTag::Conjunction },
        { TEXT("чтобы"),    TEXT("чтобы"),    EPosTag::Conjunction },
        { TEXT("если"),     TEXT("если"),     EPosTag::Conjunction },
        { TEXT("как"),      TEXT("как"),      EPosTag::Conjunction },
        { TEXT("когда"),    TEXT("когда"),    EPosTag::Conjunction },
        { TEXT("потому"),   TEXT("потому"),   EPosTag::Conjunction },
        { TEXT("хотя"),     TEXT("хотя"),     EPosTag::Conjunction },

        // --- служебные: частицы ---
        { TEXT("не"),       TEXT("не"),       EPosTag::Particle    },
        { TEXT("ни"),       TEXT("ни"),       EPosTag::Particle    },
        { TEXT("же"),       TEXT("же"),       EPosTag::Particle    },
        { TEXT("ли"),       TEXT("ли"),       EPosTag::Particle    },
        { TEXT("бы"),       TEXT("бы"),       EPosTag::Particle    },
        { TEXT("нельзя"),   TEXT("нельзя"),   EPosTag::Particle    },
        { TEXT("лишь"),     TEXT("лишь"),     EPosTag::Particle    },
        { TEXT("только"),   TEXT("только"),   EPosTag::Particle    },
        { TEXT("даже"),     TEXT("даже"),     EPosTag::Particle    },

        // --- глаголы: быть/связочные ---
        { TEXT("это"),      TEXT("это"),      EPosTag::Particle    },  // частица-связка
        { TEXT("есть"),     TEXT("быть"),     EPosTag::Verb        },
        { TEXT("является"),TEXT("являться"),  EPosTag::Verb        },
        { TEXT("был"),      TEXT("быть"),     EPosTag::Verb        },
        { TEXT("была"),     TEXT("быть"),     EPosTag::Verb        },
        { TEXT("было"),     TEXT("быть"),     EPosTag::Verb        },
        { TEXT("были"),     TEXT("быть"),     EPosTag::Verb        },
        { TEXT("будет"),    TEXT("быть"),     EPosTag::Verb        },
        { TEXT("будут"),    TEXT("быть"),     EPosTag::Verb        },

        // --- глаголы: частотные команды/действия ---
        { TEXT("открой"),   TEXT("открыть"),  EPosTag::Verb        },
        { TEXT("открыть"),  TEXT("открыть"),  EPosTag::Verb        },
        { TEXT("закрой"),   TEXT("закрыть"),  EPosTag::Verb        },
        { TEXT("закрыть"),  TEXT("закрыть"),  EPosTag::Verb        },
        { TEXT("найди"),    TEXT("найти"),    EPosTag::Verb        },
        { TEXT("найти"),    TEXT("найти"),    EPosTag::Verb        },
        { TEXT("запусти"),  TEXT("запустить"),EPosTag::Verb        },
        { TEXT("останови"), TEXT("остановить"),EPosTag::Verb       },
        { TEXT("проверь"),  TEXT("проверить"),EPosTag::Verb        },
        { TEXT("проверить"),TEXT("проверить"),EPosTag::Verb        },
        { TEXT("сохрани"),  TEXT("сохранить"),EPosTag::Verb        },
        { TEXT("удали"),    TEXT("удалить"),  EPosTag::Verb        },
        { TEXT("покажи"),   TEXT("показать"), EPosTag::Verb        },
        { TEXT("показать"), TEXT("показать"), EPosTag::Verb        },
        { TEXT("запомни"),  TEXT("запомнить"),EPosTag::Verb        },
        { TEXT("скажи"),    TEXT("сказать"),  EPosTag::Verb        },
        { TEXT("сказать"),  TEXT("сказать"),  EPosTag::Verb        },
        { TEXT("объясни"),  TEXT("объяснить"),EPosTag::Verb        },
        { TEXT("объяснить"),TEXT("объяснить"),EPosTag::Verb        },
        { TEXT("расскажи"), TEXT("рассказать"),EPosTag::Verb       },
        { TEXT("рассказать"),TEXT("рассказать"),EPosTag::Verb      },
        { TEXT("помоги"),   TEXT("помочь"),   EPosTag::Verb        },
        { TEXT("помочь"),   TEXT("помочь"),   EPosTag::Verb        },
        { TEXT("знаешь"),   TEXT("знать"),    EPosTag::Verb        },
        { TEXT("знать"),    TEXT("знать"),    EPosTag::Verb        },
        { TEXT("можешь"),   TEXT("мочь"),     EPosTag::Verb        },
        { TEXT("можете"),   TEXT("мочь"),     EPosTag::Verb        },
        { TEXT("мочь"),     TEXT("мочь"),     EPosTag::Verb        },
        { TEXT("умеешь"),   TEXT("уметь"),    EPosTag::Verb        },
        { TEXT("уметь"),    TEXT("уметь"),    EPosTag::Verb        },
        { TEXT("понимаешь"),TEXT("понимать"), EPosTag::Verb        },
        { TEXT("понимать"), TEXT("понимать"), EPosTag::Verb        },
        { TEXT("видит"),    TEXT("видеть"),   EPosTag::Verb        },
        { TEXT("видеть"),   TEXT("видеть"),   EPosTag::Verb        },
        { TEXT("представляет"),TEXT("представлять"),EPosTag::Verb  },
        { TEXT("называется"),TEXT("называться"),EPosTag::Verb      },
        { TEXT("означает"), TEXT("означать"), EPosTag::Verb        },
        { TEXT("значит"),   TEXT("значить"),  EPosTag::Verb        },

        // --- существительные: частотные ---
        { TEXT("слово"),    TEXT("слово"),    EPosTag::Noun        },
        { TEXT("слова"),    TEXT("слово"),    EPosTag::Noun        },
        { TEXT("слову"),    TEXT("слово"),    EPosTag::Noun        },
        { TEXT("словом"),   TEXT("слово"),    EPosTag::Noun        },
        { TEXT("слове"),    TEXT("слово"),    EPosTag::Noun        },
        { TEXT("кот"),      TEXT("кот"),      EPosTag::Noun        },
        { TEXT("кота"),     TEXT("кот"),      EPosTag::Noun        },
        { TEXT("коту"),     TEXT("кот"),      EPosTag::Noun        },
        { TEXT("котом"),    TEXT("кот"),      EPosTag::Noun        },
        { TEXT("коте"),     TEXT("кот"),      EPosTag::Noun        },
        { TEXT("кошка"),    TEXT("кошка"),    EPosTag::Noun        },
        { TEXT("животное"), TEXT("животное"), EPosTag::Noun        },
        { TEXT("животного"),TEXT("животное"), EPosTag::Noun        },
        { TEXT("москва"),   TEXT("москва"),   EPosTag::Noun        },
        { TEXT("москвы"),   TEXT("москва"),   EPosTag::Noun        },
        { TEXT("столица"),  TEXT("столица"),  EPosTag::Noun        },
        { TEXT("столицей"), TEXT("столица"),  EPosTag::Noun        },
        { TEXT("столицы"),  TEXT("столица"),  EPosTag::Noun        },
        { TEXT("россия"),   TEXT("россия"),   EPosTag::Noun        },
        { TEXT("россии"),   TEXT("россия"),   EPosTag::Noun        },
        { TEXT("окно"),     TEXT("окно"),     EPosTag::Noun        },
        { TEXT("окна"),     TEXT("окно"),     EPosTag::Noun        },
        { TEXT("определение"),TEXT("определение"),EPosTag::Noun    },
        { TEXT("определения"),TEXT("определение"),EPosTag::Noun    },
        { TEXT("значение"), TEXT("значение"), EPosTag::Noun        },
        { TEXT("значения"), TEXT("значение"), EPosTag::Noun        },
        { TEXT("синтаксис"),TEXT("синтаксис"),EPosTag::Noun        },
        { TEXT("морфология"),TEXT("морфология"),EPosTag::Noun      },
        { TEXT("морфологии"),TEXT("морфология"),EPosTag::Noun      },
        { TEXT("текст"),    TEXT("текст"),    EPosTag::Noun        },
        { TEXT("текста"),   TEXT("текст"),    EPosTag::Noun        },
        { TEXT("память"),   TEXT("память"),   EPosTag::Noun        },
        { TEXT("памяти"),   TEXT("память"),   EPosTag::Noun        },
        { TEXT("агент"),    TEXT("агент"),    EPosTag::Noun        },
        { TEXT("агента"),   TEXT("агент"),    EPosTag::Noun        },
        { TEXT("нейра"),    TEXT("нейра"),    EPosTag::Noun        },
        { TEXT("пушкин"),   TEXT("пушкин"),   EPosTag::Noun        },
        { TEXT("пушкина"),  TEXT("пушкин"),   EPosTag::Noun        },

        // --- прилагательные ---
        { TEXT("синтаксический"),TEXT("синтаксический"),EPosTag::Adjective},
        { TEXT("большой"),  TEXT("большой"),  EPosTag::Adjective   },
        { TEXT("новый"),    TEXT("новый"),    EPosTag::Adjective   },
        { TEXT("новая"),    TEXT("новый"),    EPosTag::Adjective   },
        { TEXT("первый"),   TEXT("первый"),   EPosTag::Adjective   },

        // --- наречия ---
        { TEXT("быстро"),   TEXT("быстро"),   EPosTag::Adverb      },
        { TEXT("очень"),    TEXT("очень"),    EPosTag::Adverb      },
        { TEXT("всегда"),   TEXT("всегда"),   EPosTag::Adverb      },
        { TEXT("уже"),      TEXT("уже"),      EPosTag::Adverb      },
        { TEXT("ещё"),      TEXT("ещё"),      EPosTag::Adverb      },
        { TEXT("там"),      TEXT("там"),      EPosTag::Adverb      },
        { TEXT("здесь"),    TEXT("здесь"),    EPosTag::Adverb      },
        { TEXT("так"),      TEXT("так"),      EPosTag::Adverb      },
        { TEXT("тоже"),     TEXT("тоже"),     EPosTag::Adverb      },
    };

    constexpr int32 DictSize = sizeof(CoreDictionary) / sizeof(CoreDictionary[0]);

    // -----------------------------------------------------------------------
    // Суффиксные правила (порядок важен: более длинные суффиксы — выше)
    // -----------------------------------------------------------------------
    struct FSuffixRule
    {
        const TCHAR* Suffix;
        EPosTag      POS;
    };

    const FSuffixRule SuffixRules[] = {
        // Глаголы — инфинитив
        { TEXT("овать"),  EPosTag::Verb       },
        { TEXT("евать"),  EPosTag::Verb       },
        { TEXT("ировать"),EPosTag::Verb       },
        { TEXT("ать"),    EPosTag::Verb       },
        { TEXT("ять"),    EPosTag::Verb       },
        { TEXT("ить"),    EPosTag::Verb       },
        { TEXT("еть"),    EPosTag::Verb       },
        { TEXT("уть"),    EPosTag::Verb       },
        { TEXT("ыть"),    EPosTag::Verb       },
        // Глаголы — спряжение / повелительное
        { TEXT("ает"),    EPosTag::Verb       },
        { TEXT("яет"),    EPosTag::Verb       },
        { TEXT("ует"),    EPosTag::Verb       },
        { TEXT("ают"),    EPosTag::Verb       },
        { TEXT("яют"),    EPosTag::Verb       },
        { TEXT("ует"),    EPosTag::Verb       },
        { TEXT("ите"),    EPosTag::Verb       },
        { TEXT("айте"),   EPosTag::Verb       },
        { TEXT("яйте"),   EPosTag::Verb       },
        { TEXT("ойте"),   EPosTag::Verb       },
        // Существительные
        { TEXT("ность"),  EPosTag::Noun       },
        { TEXT("ость"),   EPosTag::Noun       },
        { TEXT("ание"),   EPosTag::Noun       },
        { TEXT("ение"),   EPosTag::Noun       },
        { TEXT("тие"),    EPosTag::Noun       },
        { TEXT("ация"),   EPosTag::Noun       },
        { TEXT("сть"),    EPosTag::Noun       },
        { TEXT("тель"),   EPosTag::Noun       },
        { TEXT("щик"),    EPosTag::Noun       },
        { TEXT("ник"),    EPosTag::Noun       },
        // Прилагательные
        { TEXT("ский"),   EPosTag::Adjective  },
        { TEXT("ской"),   EPosTag::Adjective  },
        { TEXT("ная"),    EPosTag::Adjective  },
        { TEXT("ное"),    EPosTag::Adjective  },
        { TEXT("ные"),    EPosTag::Adjective  },
        { TEXT("ный"),    EPosTag::Adjective  },
        { TEXT("ной"),    EPosTag::Adjective  },
        { TEXT("жий"),    EPosTag::Adjective  },
        { TEXT("щий"),    EPosTag::Adjective  },
        { TEXT("чий"),    EPosTag::Adjective  },
    };

    constexpr int32 SuffixCount = sizeof(SuffixRules) / sizeof(SuffixRules[0]);

    // Убрать знаки препинания в конце токена
    FString StripPunct(const FString& Token)
    {
        FString Result = Token;
        while (!Result.IsEmpty())
        {
            TCHAR Last = Result[Result.Len() - 1];
            if (Last == TEXT('?') || Last == TEXT('!') || Last == TEXT('.') ||
                Last == TEXT(',') || Last == TEXT(':') || Last == TEXT(';'))
            {
                Result = Result.LeftChop(1);
            }
            else break;
        }
        return Result;
    }
}

// ---------------------------------------------------------------------------
// Реализация
// ---------------------------------------------------------------------------

FMorphResult FMorphAnalyzer::Analyze(const FString& Word) const
{
    FMorphResult Result;
    Result.OriginalWord = Word;

    const FString Cleaned = StripPunct(Word);
    if (Cleaned.IsEmpty())
    {
        Result.Source = TEXT("unknown");
        Result.Confidence = 0.0f;
        return Result;
    }

    const FString Lower = Cleaned.ToLower();

    // 1. Поиск в словаре
    for (int32 i = 0; i < DictSize; ++i)
    {
        if (Lower == CoreDictionary[i].Word)
        {
            Result.Lemma        = CoreDictionary[i].Lemma;
            Result.PartOfSpeech = CoreDictionary[i].POS;
            Result.Confidence   = 0.95f;
            Result.Source       = TEXT("dict");
            return Result;
        }
    }

    // 2. Суффиксные правила
    for (int32 i = 0; i < SuffixCount; ++i)
    {
        if (Lower.EndsWith(SuffixRules[i].Suffix))
        {
            Result.Lemma        = Lower;   // в v0.2 лемма = словоформа для суффикс-пути
            Result.PartOfSpeech = SuffixRules[i].POS;
            Result.Confidence   = 0.65f;
            Result.Source       = TEXT("suffix");
            return Result;
        }
    }

    // 3. Числа → Numeral
    bool bAllDigits = true;
    for (TCHAR C : Lower)
    {
        if (!FChar::IsDigit(C)) { bAllDigits = false; break; }
    }
    if (bAllDigits && !Lower.IsEmpty())
    {
        Result.Lemma        = Lower;
        Result.PartOfSpeech = EPosTag::Numeral;
        Result.Confidence   = 0.85f;
        Result.Source       = TEXT("digit");
        return Result;
    }

    // 4. Не определено
    Result.Lemma        = Lower;
    Result.PartOfSpeech = EPosTag::Unknown;
    Result.Confidence   = 0.1f;
    Result.Source       = TEXT("unknown");
    return Result;
}

TArray<FMorphResult> FMorphAnalyzer::AnalyzePhrase(const FString& Phrase) const
{
    TArray<FMorphResult> Results;

    TArray<FString> Tokens;
    Phrase.ParseIntoArrayWS(Tokens);   // разбивка по пробельным символам

    for (const FString& Token : Tokens)
    {
        Results.Add(Analyze(Token));
    }

    return Results;
}
