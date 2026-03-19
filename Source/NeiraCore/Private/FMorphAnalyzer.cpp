// FMorphAnalyzer.cpp
// v0.4 — расширенный словарь-ядро (1000+ форм) + суффиксные правила.
//
// Принцип: сначала словарь (confidence 0.95), потом суффиксы (0.65).
// v0.2: словарь ~154 слова.
// v0.3: расширен до ~570 форм тематическими блоками:
//   — глаголы познания и речи (говорить, думать, понять, слышать, делать, ...)
//   — существительные общие (вопрос, ответ, знание, задача, результат, ...)
//   — существительные домена Нейры (анализ, фраза, гипотеза, контекст, ...)
//   — прилагательные (хороший, важный, правильный, сложный, ...)
//   — числительные и кванторы (один, два, много, каждый, все, ...)
// v0.4: расширен до 1000+ форм:
//   — глаголы движения (идти, ходить, ехать, бежать, лететь, стоять, ...)
//   — глаголы действия и передачи (дать, взять, купить, получить, ...)
//   — глаголы желания и восприятия (хотеть, любить, нравиться, видеть, ...)
//   — существительные: животные, природа, тело, быт, время
//   — прилагательные: размер, цвет, состояние (маленький, красный, умный, ...)
//   — числительные 6–10 и порядковые (второй, третий, ...)
//   — наречия времени/места/степени (сегодня, рядом, почти, снова, ...)
// В v0.5 словарь будет перенесён в CSV-ресурс для неограниченного роста.
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
    // Словарь-ядро (~400 слов)
    // Структура: служебные слова → глаголы → существительные → прилагательные
    //            → числительные/кванторы → наречия
    // Для каждого слова в словаре даётся его нормализованная лемма.
    // Суффиксные правила покрывают инфинитивы и продуктивные суффиксы —
    // в словарь попадают формы, которые суффиксы не распознают правильно:
    //   - спряжения (-ит/-ишь/-ешь/-ю/-ют), прошедшее время (-л/-ла/-ли)
    //   - нерегулярные существительные
    //   - все формы прилагательных (суффиксы покрывают только часть)
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

        // --- пакет лексики: команды/действия ---
        { TEXT("создай"),   TEXT("создать"),   EPosTag::Verb       },
        { TEXT("создать"),  TEXT("создать"),   EPosTag::Verb       },
        { TEXT("перезапусти"),TEXT("перезапустить"),EPosTag::Verb  },
        { TEXT("перезапустить"),TEXT("перезапустить"),EPosTag::Verb},
        { TEXT("обнови"),   TEXT("обновить"),  EPosTag::Verb       },
        { TEXT("обновить"), TEXT("обновить"),  EPosTag::Verb       },
        { TEXT("загрузи"),  TEXT("загрузить"), EPosTag::Verb       },
        { TEXT("загрузить"),TEXT("загрузить"), EPosTag::Verb       },
        { TEXT("скачай"),   TEXT("скачать"),   EPosTag::Verb       },
        { TEXT("скачать"),  TEXT("скачать"),   EPosTag::Verb       },
        { TEXT("переименуй"),TEXT("переименовать"),EPosTag::Verb   },
        { TEXT("переименовать"),TEXT("переименовать"),EPosTag::Verb},
        { TEXT("очисти"),   TEXT("очистить"),  EPosTag::Verb       },
        { TEXT("очистить"), TEXT("очистить"),  EPosTag::Verb       },
        { TEXT("скопируй"), TEXT("скопировать"),EPosTag::Verb      },
        { TEXT("скопировать"),TEXT("скопировать"),EPosTag::Verb    },
        { TEXT("вставь"),   TEXT("вставить"),  EPosTag::Verb       },
        { TEXT("вставить"), TEXT("вставить"),  EPosTag::Verb       },
        { TEXT("выдели"),   TEXT("выделить"),  EPosTag::Verb       },
        { TEXT("выделить"), TEXT("выделить"),  EPosTag::Verb       },
        { TEXT("отмени"),   TEXT("отменить"),  EPosTag::Verb       },
        { TEXT("отменить"), TEXT("отменить"),  EPosTag::Verb       },
        { TEXT("повтори"),  TEXT("повторить"), EPosTag::Verb       },
        { TEXT("повторить"),TEXT("повторить"), EPosTag::Verb       },
        { TEXT("исправь"),  TEXT("исправить"), EPosTag::Verb       },
        { TEXT("исправить"),TEXT("исправить"), EPosTag::Verb       },
        { TEXT("подтверди"),TEXT("подтвердить"),EPosTag::Verb      },
        { TEXT("подтвердить"),TEXT("подтвердить"),EPosTag::Verb    },

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
        { TEXT("файл"),     TEXT("файл"),     EPosTag::Noun        },
        { TEXT("файла"),    TEXT("файл"),     EPosTag::Noun        },
        { TEXT("папка"),    TEXT("папка"),    EPosTag::Noun        },
        { TEXT("папку"),    TEXT("папка"),    EPosTag::Noun        },
        { TEXT("документ"), TEXT("документ"), EPosTag::Noun        },
        { TEXT("документа"),TEXT("документ"), EPosTag::Noun        },
        { TEXT("список"),   TEXT("список"),   EPosTag::Noun        },
        { TEXT("списка"),   TEXT("список"),   EPosTag::Noun        },
        { TEXT("задача"),   TEXT("задача"),   EPosTag::Noun        },
        { TEXT("задачу"),   TEXT("задача"),   EPosTag::Noun        },
        { TEXT("ошибка"),   TEXT("ошибка"),   EPosTag::Noun        },
        { TEXT("ошибку"),   TEXT("ошибка"),   EPosTag::Noun        },
        { TEXT("лог"),      TEXT("лог"),      EPosTag::Noun        },
        { TEXT("логи"),     TEXT("лог"),      EPosTag::Noun        },
        { TEXT("отчёт"),    TEXT("отчёт"),    EPosTag::Noun        },
        { TEXT("отчет"),    TEXT("отчёт"),    EPosTag::Noun        },
        { TEXT("отчета"),   TEXT("отчёт"),    EPosTag::Noun        },
        { TEXT("время"),    TEXT("время"),    EPosTag::Noun        },
        { TEXT("дата"),     TEXT("дата"),     EPosTag::Noun        },
        { TEXT("дату"),     TEXT("дата"),     EPosTag::Noun        },
        { TEXT("дате"),     TEXT("дата"),     EPosTag::Noun        },
        { TEXT("сегодня"),  TEXT("сегодня"),  EPosTag::Adverb      },
        { TEXT("завтра"),   TEXT("завтра"),   EPosTag::Adverb      },
        { TEXT("вчера"),    TEXT("вчера"),    EPosTag::Adverb      },
        { TEXT("почему"),   TEXT("почему"),   EPosTag::Adverb      },
        { TEXT("когда"),    TEXT("когда"),    EPosTag::Conjunction },
        { TEXT("сколько"),  TEXT("сколько"),  EPosTag::Numeral     },
        { TEXT("который"),  TEXT("который"),  EPosTag::Pronoun     },

        // --- пакет лексики: ASR-артефакты (разговорные/ошибочные формы) ---
        { TEXT("аткрой"),   TEXT("открыть"),  EPosTag::Verb        },
        { TEXT("акрой"),    TEXT("открыть"),  EPosTag::Verb        },
        { TEXT("закройка"), TEXT("закрыть"),  EPosTag::Verb        },
        { TEXT("найтить"),  TEXT("найти"),    EPosTag::Verb        },
        { TEXT("проверька"),TEXT("проверить"),EPosTag::Verb        },
        { TEXT("запомена"), TEXT("запомнить"),EPosTag::Verb        },
        { TEXT("чо"),       TEXT("что"),      EPosTag::Conjunction },
        { TEXT("че"),       TEXT("что"),      EPosTag::Conjunction },
        { TEXT("щас"),      TEXT("сейчас"),   EPosTag::Adverb      },
        { TEXT("ща"),       TEXT("сейчас"),   EPosTag::Adverb      },
        { TEXT("пж"),       TEXT("пожалуйста"),EPosTag::Particle   },
        { TEXT("плиз"),     TEXT("пожалуйста"),EPosTag::Particle   },
        { TEXT("кароч"),    TEXT("короче"),   EPosTag::Adverb      },
        { TEXT("кароче"),   TEXT("короче"),   EPosTag::Adverb      },
        { TEXT("синтакез"), TEXT("синтаксис"),EPosTag::Noun        },
        { TEXT("марфология"),TEXT("морфология"),EPosTag::Noun      },
        { TEXT("акно"),     TEXT("окно"),     EPosTag::Noun        },
        { TEXT("фаил"),     TEXT("файл"),     EPosTag::Noun        },
        { TEXT("паметь"),   TEXT("память"),   EPosTag::Noun        },

        // ---------------------------------------------------------------
        // Глаголы: познание, речь, действие (v0.3)
        // Добавлены формы, которые суффиксные правила не лемматизируют:
        //   спряжения (-ит/-ишь/-ю/-ют), прошедшее время (-л/-ла/-ли),
        //   повелительное (-и/-й) без стандартных суффиксов.
        // ---------------------------------------------------------------

        // говорить
        { TEXT("говорить"), TEXT("говорить"), EPosTag::Verb        },
        { TEXT("говорит"),  TEXT("говорить"), EPosTag::Verb        },
        { TEXT("говорю"),   TEXT("говорить"), EPosTag::Verb        },
        { TEXT("говоришь"), TEXT("говорить"), EPosTag::Verb        },
        { TEXT("говорят"),  TEXT("говорить"), EPosTag::Verb        },
        { TEXT("говорил"),  TEXT("говорить"), EPosTag::Verb        },
        { TEXT("говорила"), TEXT("говорить"), EPosTag::Verb        },
        { TEXT("скажи"),    TEXT("сказать"),  EPosTag::Verb        },  // уже есть, дубль не страшен
        // думать
        { TEXT("думать"),   TEXT("думать"),   EPosTag::Verb        },
        { TEXT("думает"),   TEXT("думать"),   EPosTag::Verb        },
        { TEXT("думаю"),    TEXT("думать"),   EPosTag::Verb        },
        { TEXT("думаешь"),  TEXT("думать"),   EPosTag::Verb        },
        { TEXT("думал"),    TEXT("думать"),   EPosTag::Verb        },
        { TEXT("думала"),   TEXT("думать"),   EPosTag::Verb        },
        { TEXT("подумай"),  TEXT("подумать"), EPosTag::Verb        },
        // понять / понимать
        { TEXT("понять"),   TEXT("понять"),   EPosTag::Verb        },
        { TEXT("понимать"), TEXT("понимать"), EPosTag::Verb        },
        { TEXT("понимает"), TEXT("понимать"), EPosTag::Verb        },
        { TEXT("понимаю"),  TEXT("понимать"), EPosTag::Verb        },
        { TEXT("понял"),    TEXT("понять"),   EPosTag::Verb        },
        { TEXT("поняла"),   TEXT("понять"),   EPosTag::Verb        },
        { TEXT("поняли"),   TEXT("понять"),   EPosTag::Verb        },
        { TEXT("пойми"),    TEXT("понять"),   EPosTag::Verb        },
        // слышать
        { TEXT("слышать"),  TEXT("слышать"),  EPosTag::Verb        },
        { TEXT("слышит"),   TEXT("слышать"),  EPosTag::Verb        },
        { TEXT("слышишь"),  TEXT("слышать"),  EPosTag::Verb        },
        { TEXT("слышал"),   TEXT("слышать"),  EPosTag::Verb        },
        { TEXT("слышала"),  TEXT("слышать"),  EPosTag::Verb        },
        // читать
        { TEXT("читать"),   TEXT("читать"),   EPosTag::Verb        },
        { TEXT("читает"),   TEXT("читать"),   EPosTag::Verb        },
        { TEXT("читаю"),    TEXT("читать"),   EPosTag::Verb        },
        { TEXT("читай"),    TEXT("читать"),   EPosTag::Verb        },
        { TEXT("читал"),    TEXT("читать"),   EPosTag::Verb        },
        // писать
        { TEXT("писать"),   TEXT("писать"),   EPosTag::Verb        },
        { TEXT("пишет"),    TEXT("писать"),   EPosTag::Verb        },
        { TEXT("пишу"),     TEXT("писать"),   EPosTag::Verb        },
        { TEXT("пиши"),     TEXT("писать"),   EPosTag::Verb        },
        { TEXT("писал"),    TEXT("писать"),   EPosTag::Verb        },
        // делать
        { TEXT("делать"),   TEXT("делать"),   EPosTag::Verb        },
        { TEXT("делает"),   TEXT("делать"),   EPosTag::Verb        },
        { TEXT("делаю"),    TEXT("делать"),   EPosTag::Verb        },
        { TEXT("делай"),    TEXT("делать"),   EPosTag::Verb        },
        { TEXT("делал"),    TEXT("делать"),   EPosTag::Verb        },
        { TEXT("сделай"),   TEXT("сделать"),  EPosTag::Verb        },
        // работать
        { TEXT("работать"), TEXT("работать"), EPosTag::Verb        },
        { TEXT("работает"), TEXT("работать"), EPosTag::Verb        },
        { TEXT("работаю"),  TEXT("работать"), EPosTag::Verb        },
        { TEXT("работай"),  TEXT("работать"), EPosTag::Verb        },
        { TEXT("работал"),  TEXT("работать"), EPosTag::Verb        },
        // изучить / изучать
        { TEXT("изучить"),  TEXT("изучить"),  EPosTag::Verb        },
        { TEXT("изучи"),    TEXT("изучить"),  EPosTag::Verb        },
        { TEXT("изучать"),  TEXT("изучать"),  EPosTag::Verb        },
        { TEXT("изучает"),  TEXT("изучать"),  EPosTag::Verb        },
        // начать / начинать
        { TEXT("начать"),   TEXT("начать"),   EPosTag::Verb        },
        { TEXT("начни"),    TEXT("начать"),   EPosTag::Verb        },
        { TEXT("начинать"), TEXT("начинать"), EPosTag::Verb        },
        { TEXT("начинает"), TEXT("начинать"), EPosTag::Verb        },
        { TEXT("начал"),    TEXT("начать"),   EPosTag::Verb        },
        { TEXT("начала"),   TEXT("начать"),   EPosTag::Verb        },
        // закончить / заканчивать
        { TEXT("закончить"),TEXT("закончить"),EPosTag::Verb        },
        { TEXT("закончи"),  TEXT("закончить"),EPosTag::Verb        },
        { TEXT("заканчивает"),TEXT("заканчивать"),EPosTag::Verb    },
        { TEXT("закончил"), TEXT("закончить"),EPosTag::Verb        },
        // продолжить
        { TEXT("продолжить"),TEXT("продолжить"),EPosTag::Verb      },
        { TEXT("продолжи"), TEXT("продолжить"),EPosTag::Verb       },
        { TEXT("продолжает"),TEXT("продолжать"),EPosTag::Verb      },
        { TEXT("продолжал"),TEXT("продолжать"),EPosTag::Verb       },
        // повторить
        { TEXT("повторить"),TEXT("повторить"),EPosTag::Verb        },
        { TEXT("повтори"),  TEXT("повторить"),EPosTag::Verb        },
        { TEXT("повторяет"),TEXT("повторять"),EPosTag::Verb        },
        // спросить / спрашивать
        { TEXT("спросить"), TEXT("спросить"), EPosTag::Verb        },
        { TEXT("спроси"),   TEXT("спросить"), EPosTag::Verb        },
        { TEXT("спрашивает"),TEXT("спрашивать"),EPosTag::Verb      },
        { TEXT("спросил"),  TEXT("спросить"), EPosTag::Verb        },
        // ответить / отвечать
        { TEXT("ответить"), TEXT("ответить"), EPosTag::Verb        },
        { TEXT("ответь"),   TEXT("ответить"), EPosTag::Verb        },
        { TEXT("отвечает"), TEXT("отвечать"), EPosTag::Verb        },
        { TEXT("ответил"),  TEXT("ответить"), EPosTag::Verb        },
        // помнить / вспомнить
        { TEXT("помнить"),  TEXT("помнить"),  EPosTag::Verb        },
        { TEXT("помнит"),   TEXT("помнить"),  EPosTag::Verb        },
        { TEXT("помни"),    TEXT("помнить"),  EPosTag::Verb        },
        { TEXT("помнил"),   TEXT("помнить"),  EPosTag::Verb        },
        { TEXT("вспомнить"),TEXT("вспомнить"),EPosTag::Verb        },
        { TEXT("вспомни"),  TEXT("вспомнить"),EPosTag::Verb        },
        // забыть
        { TEXT("забыть"),   TEXT("забыть"),   EPosTag::Verb        },
        { TEXT("забудь"),   TEXT("забыть"),   EPosTag::Verb        },
        { TEXT("забыл"),    TEXT("забыть"),   EPosTag::Verb        },
        { TEXT("забыла"),   TEXT("забыть"),   EPosTag::Verb        },
        // добавить
        { TEXT("добавить"), TEXT("добавить"), EPosTag::Verb        },
        { TEXT("добавь"),   TEXT("добавить"), EPosTag::Verb        },
        { TEXT("добавляет"),TEXT("добавлять"),EPosTag::Verb        },
        // изменить
        { TEXT("изменить"), TEXT("изменить"), EPosTag::Verb        },
        { TEXT("измени"),   TEXT("изменить"), EPosTag::Verb        },
        { TEXT("изменяет"), TEXT("изменять"), EPosTag::Verb        },
        // создать
        { TEXT("создать"),  TEXT("создать"),  EPosTag::Verb        },
        { TEXT("создай"),   TEXT("создать"),  EPosTag::Verb        },
        { TEXT("создаёт"),  TEXT("создавать"),EPosTag::Verb        },
        { TEXT("создал"),   TEXT("создать"),  EPosTag::Verb        },
        // использовать
        { TEXT("использовать"),TEXT("использовать"),EPosTag::Verb  },
        { TEXT("используй"),TEXT("использовать"),EPosTag::Verb     },
        { TEXT("использует"),TEXT("использовать"),EPosTag::Verb    },
        // показывать
        { TEXT("показывать"),TEXT("показывать"),EPosTag::Verb      },
        { TEXT("показывает"),TEXT("показывать"),EPosTag::Verb      },
        { TEXT("покажи"),   TEXT("показать"), EPosTag::Verb        },  // уже есть

        // ---------------------------------------------------------------
        // Существительные: общие понятия (v0.3)
        // Для каждого слова: именительный + родительный (мин.);
        // множественное число где необходимо для корректного разбора.
        // ---------------------------------------------------------------

        // человек
        { TEXT("человек"),  TEXT("человек"),  EPosTag::Noun        },
        { TEXT("человека"), TEXT("человек"),  EPosTag::Noun        },
        { TEXT("человеку"), TEXT("человек"),  EPosTag::Noun        },
        { TEXT("люди"),     TEXT("человек"),  EPosTag::Noun        },
        { TEXT("людей"),    TEXT("человек"),  EPosTag::Noun        },
        // мир
        { TEXT("мир"),      TEXT("мир"),      EPosTag::Noun        },
        { TEXT("мира"),     TEXT("мир"),      EPosTag::Noun        },
        // время
        { TEXT("время"),    TEXT("время"),    EPosTag::Noun        },
        { TEXT("времени"),  TEXT("время"),    EPosTag::Noun        },
        { TEXT("временем"), TEXT("время"),    EPosTag::Noun        },
        { TEXT("времена"),  TEXT("время"),    EPosTag::Noun        },
        // место
        { TEXT("место"),    TEXT("место"),    EPosTag::Noun        },
        { TEXT("места"),    TEXT("место"),    EPosTag::Noun        },
        { TEXT("мест"),     TEXT("место"),    EPosTag::Noun        },
        // вопрос
        { TEXT("вопрос"),   TEXT("вопрос"),   EPosTag::Noun        },
        { TEXT("вопроса"),  TEXT("вопрос"),   EPosTag::Noun        },
        { TEXT("вопросы"),  TEXT("вопрос"),   EPosTag::Noun        },
        { TEXT("вопросов"), TEXT("вопрос"),   EPosTag::Noun        },
        // ответ
        { TEXT("ответ"),    TEXT("ответ"),    EPosTag::Noun        },
        { TEXT("ответа"),   TEXT("ответ"),    EPosTag::Noun        },
        { TEXT("ответы"),   TEXT("ответ"),    EPosTag::Noun        },
        { TEXT("ответов"),  TEXT("ответ"),    EPosTag::Noun        },
        // информация
        { TEXT("информация"),TEXT("информация"),EPosTag::Noun      },
        { TEXT("информации"),TEXT("информация"),EPosTag::Noun      },
        // данные
        { TEXT("данные"),   TEXT("данные"),   EPosTag::Noun        },
        { TEXT("данных"),   TEXT("данные"),   EPosTag::Noun        },
        { TEXT("данным"),   TEXT("данные"),   EPosTag::Noun        },
        // система
        { TEXT("система"),  TEXT("система"),  EPosTag::Noun        },
        { TEXT("системы"),  TEXT("система"),  EPosTag::Noun        },
        { TEXT("систем"),   TEXT("система"),  EPosTag::Noun        },
        { TEXT("системе"),  TEXT("система"),  EPosTag::Noun        },
        // программа
        { TEXT("программа"),TEXT("программа"),EPosTag::Noun        },
        { TEXT("программы"),TEXT("программа"),EPosTag::Noun        },
        { TEXT("программ"), TEXT("программа"),EPosTag::Noun        },
        // файл
        { TEXT("файл"),     TEXT("файл"),     EPosTag::Noun        },
        { TEXT("файла"),    TEXT("файл"),     EPosTag::Noun        },
        { TEXT("файлы"),    TEXT("файл"),     EPosTag::Noun        },
        { TEXT("файлов"),   TEXT("файл"),     EPosTag::Noun        },
        // язык
        { TEXT("язык"),     TEXT("язык"),     EPosTag::Noun        },
        { TEXT("языка"),    TEXT("язык"),     EPosTag::Noun        },
        { TEXT("языки"),    TEXT("язык"),     EPosTag::Noun        },
        { TEXT("языков"),   TEXT("язык"),     EPosTag::Noun        },
        // правило
        { TEXT("правило"),  TEXT("правило"),  EPosTag::Noun        },
        { TEXT("правила"),  TEXT("правило"),  EPosTag::Noun        },
        { TEXT("правил"),   TEXT("правило"),  EPosTag::Noun        },
        // пример
        { TEXT("пример"),   TEXT("пример"),   EPosTag::Noun        },
        { TEXT("примера"),  TEXT("пример"),   EPosTag::Noun        },
        { TEXT("примеры"),  TEXT("пример"),   EPosTag::Noun        },
        { TEXT("примеров"), TEXT("пример"),   EPosTag::Noun        },
        // задача
        { TEXT("задача"),   TEXT("задача"),   EPosTag::Noun        },
        { TEXT("задачи"),   TEXT("задача"),   EPosTag::Noun        },
        { TEXT("задач"),    TEXT("задача"),   EPosTag::Noun        },
        // результат
        { TEXT("результат"),TEXT("результат"),EPosTag::Noun        },
        { TEXT("результата"),TEXT("результат"),EPosTag::Noun       },
        { TEXT("результаты"),TEXT("результат"),EPosTag::Noun       },
        // процесс
        { TEXT("процесс"),  TEXT("процесс"),  EPosTag::Noun        },
        { TEXT("процесса"), TEXT("процесс"),  EPosTag::Noun        },
        { TEXT("процессе"), TEXT("процесс"),  EPosTag::Noun        },
        // метод
        { TEXT("метод"),    TEXT("метод"),    EPosTag::Noun        },
        { TEXT("метода"),   TEXT("метод"),    EPosTag::Noun        },
        { TEXT("методы"),   TEXT("метод"),    EPosTag::Noun        },
        { TEXT("методов"),  TEXT("метод"),    EPosTag::Noun        },
        // список
        { TEXT("список"),   TEXT("список"),   EPosTag::Noun        },
        { TEXT("списка"),   TEXT("список"),   EPosTag::Noun        },
        { TEXT("списки"),   TEXT("список"),   EPosTag::Noun        },
        // объект
        { TEXT("объект"),   TEXT("объект"),   EPosTag::Noun        },
        { TEXT("объекта"),  TEXT("объект"),   EPosTag::Noun        },
        { TEXT("объекты"),  TEXT("объект"),   EPosTag::Noun        },
        { TEXT("объектов"), TEXT("объект"),   EPosTag::Noun        },
        // имя
        { TEXT("имя"),      TEXT("имя"),      EPosTag::Noun        },
        { TEXT("имени"),    TEXT("имя"),      EPosTag::Noun        },
        { TEXT("имена"),    TEXT("имя"),      EPosTag::Noun        },
        { TEXT("имён"),     TEXT("имя"),      EPosTag::Noun        },
        // название
        { TEXT("название"), TEXT("название"), EPosTag::Noun        },
        { TEXT("названия"), TEXT("название"), EPosTag::Noun        },
        { TEXT("названий"), TEXT("название"), EPosTag::Noun        },
        // история
        { TEXT("история"),  TEXT("история"),  EPosTag::Noun        },
        { TEXT("истории"),  TEXT("история"),  EPosTag::Noun        },
        { TEXT("историй"),  TEXT("история"),  EPosTag::Noun        },
        // сообщение
        { TEXT("сообщение"),TEXT("сообщение"),EPosTag::Noun        },
        { TEXT("сообщения"),TEXT("сообщение"),EPosTag::Noun        },
        { TEXT("сообщений"),TEXT("сообщение"),EPosTag::Noun        },
        // ошибка
        { TEXT("ошибка"),   TEXT("ошибка"),   EPosTag::Noun        },
        { TEXT("ошибки"),   TEXT("ошибка"),   EPosTag::Noun        },
        { TEXT("ошибок"),   TEXT("ошибка"),   EPosTag::Noun        },
        // проблема
        { TEXT("проблема"), TEXT("проблема"), EPosTag::Noun        },
        { TEXT("проблемы"), TEXT("проблема"), EPosTag::Noun        },
        { TEXT("проблем"),  TEXT("проблема"), EPosTag::Noun        },
        // решение (результат — уже выше; решение задачи)
        { TEXT("решение"),  TEXT("решение"),  EPosTag::Noun        },
        { TEXT("решения"),  TEXT("решение"),  EPosTag::Noun        },
        { TEXT("решений"),  TEXT("решение"),  EPosTag::Noun        },
        // запрос
        { TEXT("запрос"),   TEXT("запрос"),   EPosTag::Noun        },
        { TEXT("запроса"),  TEXT("запрос"),   EPosTag::Noun        },
        { TEXT("запросы"),  TEXT("запрос"),   EPosTag::Noun        },
        // знание
        { TEXT("знание"),   TEXT("знание"),   EPosTag::Noun        },
        { TEXT("знания"),   TEXT("знание"),   EPosTag::Noun        },
        { TEXT("знаний"),   TEXT("знание"),   EPosTag::Noun        },
        // факт
        { TEXT("факт"),     TEXT("факт"),     EPosTag::Noun        },
        { TEXT("факта"),    TEXT("факт"),     EPosTag::Noun        },
        { TEXT("факты"),    TEXT("факт"),     EPosTag::Noun        },
        { TEXT("фактов"),   TEXT("факт"),     EPosTag::Noun        },
        // гипотеза
        { TEXT("гипотеза"), TEXT("гипотеза"), EPosTag::Noun        },
        { TEXT("гипотезы"), TEXT("гипотеза"), EPosTag::Noun        },
        { TEXT("гипотез"),  TEXT("гипотеза"), EPosTag::Noun        },
        // понятие
        { TEXT("понятие"),  TEXT("понятие"),  EPosTag::Noun        },
        { TEXT("понятия"),  TEXT("понятие"),  EPosTag::Noun        },
        { TEXT("понятий"),  TEXT("понятие"),  EPosTag::Noun        },
        // термин
        { TEXT("термин"),   TEXT("термин"),   EPosTag::Noun        },
        { TEXT("термина"),  TEXT("термин"),   EPosTag::Noun        },
        { TEXT("термины"),  TEXT("термин"),   EPosTag::Noun        },
        { TEXT("терминов"), TEXT("термин"),   EPosTag::Noun        },
        // модель
        { TEXT("модель"),   TEXT("модель"),   EPosTag::Noun        },
        { TEXT("модели"),   TEXT("модель"),   EPosTag::Noun        },
        { TEXT("моделей"),  TEXT("модель"),   EPosTag::Noun        },
        // правда / ложь
        { TEXT("правда"),   TEXT("правда"),   EPosTag::Noun        },
        { TEXT("правды"),   TEXT("правда"),   EPosTag::Noun        },
        { TEXT("ложь"),     TEXT("ложь"),     EPosTag::Noun        },
        { TEXT("лжи"),      TEXT("ложь"),     EPosTag::Noun        },
        // цель
        { TEXT("цель"),     TEXT("цель"),     EPosTag::Noun        },
        { TEXT("цели"),     TEXT("цель"),     EPosTag::Noun        },
        { TEXT("целей"),    TEXT("цель"),     EPosTag::Noun        },
        // способ
        { TEXT("способ"),   TEXT("способ"),   EPosTag::Noun        },
        { TEXT("способа"),  TEXT("способ"),   EPosTag::Noun        },
        { TEXT("способы"),  TEXT("способ"),   EPosTag::Noun        },
        // причина
        { TEXT("причина"),  TEXT("причина"),  EPosTag::Noun        },
        { TEXT("причины"),  TEXT("причина"),  EPosTag::Noun        },
        { TEXT("причин"),   TEXT("причина"),  EPosTag::Noun        },
        // часть
        { TEXT("часть"),    TEXT("часть"),    EPosTag::Noun        },
        { TEXT("части"),    TEXT("часть"),    EPosTag::Noun        },
        { TEXT("частей"),   TEXT("часть"),    EPosTag::Noun        },
        // тема
        { TEXT("тема"),     TEXT("тема"),     EPosTag::Noun        },
        { TEXT("темы"),     TEXT("тема"),     EPosTag::Noun        },
        { TEXT("тем"),      TEXT("тема"),     EPosTag::Noun        },
        { TEXT("теме"),     TEXT("тема"),     EPosTag::Noun        },

        // ---------------------------------------------------------------
        // Существительные: домен Нейры — лингвистика и ИИ (v0.3)
        // ---------------------------------------------------------------

        // анализ / разбор
        { TEXT("анализ"),   TEXT("анализ"),   EPosTag::Noun        },
        { TEXT("анализа"),  TEXT("анализ"),   EPosTag::Noun        },
        { TEXT("анализы"),  TEXT("анализ"),   EPosTag::Noun        },
        { TEXT("разбор"),   TEXT("разбор"),   EPosTag::Noun        },
        { TEXT("разбора"),  TEXT("разбор"),   EPosTag::Noun        },
        // фраза / предложение
        { TEXT("фраза"),    TEXT("фраза"),    EPosTag::Noun        },
        { TEXT("фразы"),    TEXT("фраза"),    EPosTag::Noun        },
        { TEXT("фраз"),     TEXT("фраза"),    EPosTag::Noun        },
        { TEXT("фразе"),    TEXT("фраза"),    EPosTag::Noun        },
        { TEXT("предложение"),TEXT("предложение"),EPosTag::Noun    },  // предложение = sentence / offer
        { TEXT("предложения"),TEXT("предложение"),EPosTag::Noun    },
        { TEXT("предложений"),TEXT("предложение"),EPosTag::Noun    },
        // токен / лемма
        { TEXT("токен"),    TEXT("токен"),    EPosTag::Noun        },
        { TEXT("токена"),   TEXT("токен"),    EPosTag::Noun        },
        { TEXT("токены"),   TEXT("токен"),    EPosTag::Noun        },
        { TEXT("лемма"),    TEXT("лемма"),    EPosTag::Noun        },
        { TEXT("леммы"),    TEXT("лемма"),    EPosTag::Noun        },
        { TEXT("лемм"),     TEXT("лемма"),    EPosTag::Noun        },
        // суффикс / корень / форма
        { TEXT("суффикс"),  TEXT("суффикс"),  EPosTag::Noun        },
        { TEXT("суффикса"), TEXT("суффикс"),  EPosTag::Noun        },
        { TEXT("суффиксы"), TEXT("суффикс"),  EPosTag::Noun        },
        { TEXT("корень"),   TEXT("корень"),   EPosTag::Noun        },
        { TEXT("корня"),    TEXT("корень"),   EPosTag::Noun        },
        { TEXT("корни"),    TEXT("корень"),   EPosTag::Noun        },
        { TEXT("форма"),    TEXT("форма"),    EPosTag::Noun        },
        { TEXT("формы"),    TEXT("форма"),    EPosTag::Noun        },
        { TEXT("форм"),     TEXT("форма"),    EPosTag::Noun        },
        // словарь / контекст
        { TEXT("словарь"),  TEXT("словарь"),  EPosTag::Noun        },
        { TEXT("словаря"),  TEXT("словарь"),  EPosTag::Noun        },
        { TEXT("словари"),  TEXT("словарь"),  EPosTag::Noun        },
        { TEXT("контекст"), TEXT("контекст"), EPosTag::Noun        },
        { TEXT("контекста"),TEXT("контекст"), EPosTag::Noun        },
        { TEXT("контексте"),TEXT("контекст"), EPosTag::Noun        },

        // ---------------------------------------------------------------
        // Прилагательные (v0.3)
        // Все четыре формы рода/числа; суффиксные правила покрывают часть
        // форм, но не дают лемму — словарь нужен для корректной леммы.
        // ---------------------------------------------------------------

        // хороший
        { TEXT("хороший"),  TEXT("хороший"),  EPosTag::Adjective   },
        { TEXT("хорошая"),  TEXT("хороший"),  EPosTag::Adjective   },
        { TEXT("хорошее"),  TEXT("хороший"),  EPosTag::Adjective   },
        { TEXT("хорошие"),  TEXT("хороший"),  EPosTag::Adjective   },
        { TEXT("хорошего"), TEXT("хороший"),  EPosTag::Adjective   },
        // плохой
        { TEXT("плохой"),   TEXT("плохой"),   EPosTag::Adjective   },
        { TEXT("плохая"),   TEXT("плохой"),   EPosTag::Adjective   },
        { TEXT("плохое"),   TEXT("плохой"),   EPosTag::Adjective   },
        { TEXT("плохие"),   TEXT("плохой"),   EPosTag::Adjective   },
        // правильный
        { TEXT("правильный"),TEXT("правильный"),EPosTag::Adjective },
        { TEXT("правильная"),TEXT("правильный"),EPosTag::Adjective },
        { TEXT("правильное"),TEXT("правильный"),EPosTag::Adjective },
        { TEXT("правильные"),TEXT("правильный"),EPosTag::Adjective },
        { TEXT("правильного"),TEXT("правильный"),EPosTag::Adjective},
        // неправильный
        { TEXT("неправильный"),TEXT("неправильный"),EPosTag::Adjective},
        { TEXT("неправильная"),TEXT("неправильный"),EPosTag::Adjective},
        { TEXT("неправильное"),TEXT("неправильный"),EPosTag::Adjective},
        { TEXT("неправильные"),TEXT("неправильный"),EPosTag::Adjective},
        // важный
        { TEXT("важный"),   TEXT("важный"),   EPosTag::Adjective   },
        { TEXT("важная"),   TEXT("важный"),   EPosTag::Adjective   },
        { TEXT("важное"),   TEXT("важный"),   EPosTag::Adjective   },
        { TEXT("важные"),   TEXT("важный"),   EPosTag::Adjective   },
        { TEXT("важного"),  TEXT("важный"),   EPosTag::Adjective   },
        // нужный
        { TEXT("нужный"),   TEXT("нужный"),   EPosTag::Adjective   },
        { TEXT("нужная"),   TEXT("нужный"),   EPosTag::Adjective   },
        { TEXT("нужное"),   TEXT("нужный"),   EPosTag::Adjective   },
        { TEXT("нужные"),   TEXT("нужный"),   EPosTag::Adjective   },
        // простой
        { TEXT("простой"),  TEXT("простой"),  EPosTag::Adjective   },
        { TEXT("простая"),  TEXT("простой"),  EPosTag::Adjective   },
        { TEXT("простое"),  TEXT("простой"),  EPosTag::Adjective   },
        { TEXT("простые"),  TEXT("простой"),  EPosTag::Adjective   },
        { TEXT("простого"), TEXT("простой"),  EPosTag::Adjective   },
        // сложный
        { TEXT("сложный"),  TEXT("сложный"),  EPosTag::Adjective   },
        { TEXT("сложная"),  TEXT("сложный"),  EPosTag::Adjective   },
        { TEXT("сложное"),  TEXT("сложный"),  EPosTag::Adjective   },
        { TEXT("сложные"),  TEXT("сложный"),  EPosTag::Adjective   },
        { TEXT("сложного"), TEXT("сложный"),  EPosTag::Adjective   },
        // точный
        { TEXT("точный"),   TEXT("точный"),   EPosTag::Adjective   },
        { TEXT("точная"),   TEXT("точный"),   EPosTag::Adjective   },
        { TEXT("точное"),   TEXT("точный"),   EPosTag::Adjective   },
        { TEXT("точные"),   TEXT("точный"),   EPosTag::Adjective   },
        // другой
        { TEXT("другой"),   TEXT("другой"),   EPosTag::Adjective   },
        { TEXT("другая"),   TEXT("другой"),   EPosTag::Adjective   },
        { TEXT("другое"),   TEXT("другой"),   EPosTag::Adjective   },
        { TEXT("другие"),   TEXT("другой"),   EPosTag::Adjective   },
        { TEXT("другого"),  TEXT("другой"),   EPosTag::Adjective   },
        // похожий
        { TEXT("похожий"),  TEXT("похожий"),  EPosTag::Adjective   },
        { TEXT("похожая"),  TEXT("похожий"),  EPosTag::Adjective   },
        { TEXT("похожее"),  TEXT("похожий"),  EPosTag::Adjective   },
        { TEXT("похожие"),  TEXT("похожий"),  EPosTag::Adjective   },
        // разный
        { TEXT("разный"),   TEXT("разный"),   EPosTag::Adjective   },
        { TEXT("разная"),   TEXT("разный"),   EPosTag::Adjective   },
        { TEXT("разное"),   TEXT("разный"),   EPosTag::Adjective   },
        { TEXT("разные"),   TEXT("разный"),   EPosTag::Adjective   },

        // ---------------------------------------------------------------
        // Прилагательные (v0.3, продолжение)
        // ---------------------------------------------------------------

        // --- прилагательные из v0.2 (расширение) ---
        { TEXT("синтаксический"),TEXT("синтаксический"),EPosTag::Adjective},
        { TEXT("морфологический"),TEXT("морфологический"),EPosTag::Adjective},
        { TEXT("морфологическая"),TEXT("морфологический"),EPosTag::Adjective},
        { TEXT("лингвистический"),TEXT("лингвистический"),EPosTag::Adjective},
        { TEXT("большой"),  TEXT("большой"),  EPosTag::Adjective   },
        { TEXT("большая"),  TEXT("большой"),  EPosTag::Adjective   },
        { TEXT("большое"),  TEXT("большой"),  EPosTag::Adjective   },
        { TEXT("большие"),  TEXT("большой"),  EPosTag::Adjective   },
        { TEXT("новый"),    TEXT("новый"),    EPosTag::Adjective   },
        { TEXT("новая"),    TEXT("новый"),    EPosTag::Adjective   },
        { TEXT("новое"),    TEXT("новый"),    EPosTag::Adjective   },
        { TEXT("новые"),    TEXT("новый"),    EPosTag::Adjective   },
        { TEXT("первый"),   TEXT("первый"),   EPosTag::Adjective   },
        { TEXT("первая"),   TEXT("первый"),   EPosTag::Adjective   },
        { TEXT("первое"),   TEXT("первый"),   EPosTag::Adjective   },
        { TEXT("первые"),   TEXT("первый"),   EPosTag::Adjective   },

        // ---------------------------------------------------------------
        // Числительные и кванторы (v0.3)
        // ---------------------------------------------------------------

        { TEXT("один"),     TEXT("один"),     EPosTag::Numeral     },
        { TEXT("одна"),     TEXT("один"),     EPosTag::Numeral     },
        { TEXT("одно"),     TEXT("один"),     EPosTag::Numeral     },
        { TEXT("одного"),   TEXT("один"),     EPosTag::Numeral     },
        { TEXT("одной"),    TEXT("один"),     EPosTag::Numeral     },
        { TEXT("два"),      TEXT("два"),      EPosTag::Numeral     },
        { TEXT("две"),      TEXT("два"),      EPosTag::Numeral     },
        { TEXT("двух"),     TEXT("два"),      EPosTag::Numeral     },
        { TEXT("три"),      TEXT("три"),      EPosTag::Numeral     },
        { TEXT("трёх"),     TEXT("три"),      EPosTag::Numeral     },
        { TEXT("четыре"),   TEXT("четыре"),   EPosTag::Numeral     },
        { TEXT("пять"),     TEXT("пять"),     EPosTag::Numeral     },
        { TEXT("десять"),   TEXT("десять"),   EPosTag::Numeral     },
        { TEXT("много"),    TEXT("много"),    EPosTag::Numeral     },
        { TEXT("многих"),   TEXT("много"),    EPosTag::Numeral     },
        { TEXT("мало"),     TEXT("мало"),     EPosTag::Numeral     },
        { TEXT("малого"),   TEXT("мало"),     EPosTag::Numeral     },
        { TEXT("несколько"),TEXT("несколько"),EPosTag::Numeral     },
        { TEXT("нескольких"),TEXT("несколько"),EPosTag::Numeral    },
        { TEXT("каждый"),   TEXT("каждый"),   EPosTag::Adjective   },
        { TEXT("каждая"),   TEXT("каждый"),   EPosTag::Adjective   },
        { TEXT("каждое"),   TEXT("каждый"),   EPosTag::Adjective   },
        { TEXT("каждые"),   TEXT("каждый"),   EPosTag::Adjective   },
        { TEXT("любой"),    TEXT("любой"),    EPosTag::Adjective   },
        { TEXT("любая"),    TEXT("любой"),    EPosTag::Adjective   },
        { TEXT("любое"),    TEXT("любой"),    EPosTag::Adjective   },
        { TEXT("любые"),    TEXT("любой"),    EPosTag::Adjective   },
        { TEXT("все"),      TEXT("весь"),     EPosTag::Adjective   },
        { TEXT("всех"),     TEXT("весь"),     EPosTag::Adjective   },
        { TEXT("всё"),      TEXT("весь"),     EPosTag::Adjective   },
        { TEXT("весь"),     TEXT("весь"),     EPosTag::Adjective   },
        { TEXT("вся"),      TEXT("весь"),     EPosTag::Adjective   },

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
        { TEXT("потом"),    TEXT("потом"),    EPosTag::Adverb      },
        { TEXT("сейчас"),   TEXT("сейчас"),   EPosTag::Adverb      },
        { TEXT("сюда"),     TEXT("сюда"),     EPosTag::Adverb      },
        { TEXT("туда"),     TEXT("туда"),     EPosTag::Adverb      },
        { TEXT("наверное"), TEXT("наверное"), EPosTag::Adverb      },
        { TEXT("возможно"), TEXT("возможно"), EPosTag::Adverb      },
        { TEXT("точно"),    TEXT("точно"),    EPosTag::Adverb      },
        { TEXT("верно"),    TEXT("верно"),    EPosTag::Adverb      },
        { TEXT("нет"),      TEXT("нет"),      EPosTag::Particle    },
        { TEXT("да"),       TEXT("да"),       EPosTag::Particle    },

        // ---------------------------------------------------------------
        // v0.4: расширение словаря до 1000+ форм
        // Блоки: движение, действие, природа/тело, быт, прилагательные,
        //        числительные, наречия, дополнительные союзы/частицы
        // ---------------------------------------------------------------

        // --- глаголы: движение ---
        { TEXT("идти"),     TEXT("идти"),     EPosTag::Verb        },
        { TEXT("иду"),      TEXT("идти"),     EPosTag::Verb        },
        { TEXT("идёт"),     TEXT("идти"),     EPosTag::Verb        },
        { TEXT("идёшь"),    TEXT("идти"),     EPosTag::Verb        },
        { TEXT("идут"),     TEXT("идти"),     EPosTag::Verb        },
        { TEXT("шёл"),      TEXT("идти"),     EPosTag::Verb        },
        { TEXT("шла"),      TEXT("идти"),     EPosTag::Verb        },
        { TEXT("шли"),      TEXT("идти"),     EPosTag::Verb        },
        { TEXT("иди"),      TEXT("идти"),     EPosTag::Verb        },
        { TEXT("ходить"),   TEXT("ходить"),   EPosTag::Verb        },
        { TEXT("ходит"),    TEXT("ходить"),   EPosTag::Verb        },
        { TEXT("хожу"),     TEXT("ходить"),   EPosTag::Verb        },
        { TEXT("ходил"),    TEXT("ходить"),   EPosTag::Verb        },
        { TEXT("ходила"),   TEXT("ходить"),   EPosTag::Verb        },
        { TEXT("ходи"),     TEXT("ходить"),   EPosTag::Verb        },
        { TEXT("ехать"),    TEXT("ехать"),    EPosTag::Verb        },
        { TEXT("еду"),      TEXT("ехать"),    EPosTag::Verb        },
        { TEXT("едет"),     TEXT("ехать"),    EPosTag::Verb        },
        { TEXT("едут"),     TEXT("ехать"),    EPosTag::Verb        },
        { TEXT("ехал"),     TEXT("ехать"),    EPosTag::Verb        },
        { TEXT("ехала"),    TEXT("ехать"),    EPosTag::Verb        },
        { TEXT("езди"),     TEXT("ехать"),    EPosTag::Verb        },
        { TEXT("бежать"),   TEXT("бежать"),   EPosTag::Verb        },
        { TEXT("бегу"),     TEXT("бежать"),   EPosTag::Verb        },
        { TEXT("бежит"),    TEXT("бежать"),   EPosTag::Verb        },
        { TEXT("бежал"),    TEXT("бежать"),   EPosTag::Verb        },
        { TEXT("беги"),     TEXT("бежать"),   EPosTag::Verb        },
        { TEXT("лететь"),   TEXT("лететь"),   EPosTag::Verb        },
        { TEXT("лечу"),     TEXT("лететь"),   EPosTag::Verb        },
        { TEXT("летит"),    TEXT("лететь"),   EPosTag::Verb        },
        { TEXT("летел"),    TEXT("лететь"),   EPosTag::Verb        },
        { TEXT("летела"),   TEXT("лететь"),   EPosTag::Verb        },
        { TEXT("стоять"),   TEXT("стоять"),   EPosTag::Verb        },
        { TEXT("стою"),     TEXT("стоять"),   EPosTag::Verb        },
        { TEXT("стоит"),    TEXT("стоять"),   EPosTag::Verb        },
        { TEXT("стоял"),    TEXT("стоять"),   EPosTag::Verb        },
        { TEXT("стояла"),   TEXT("стоять"),   EPosTag::Verb        },
        { TEXT("стой"),     TEXT("стоять"),   EPosTag::Verb        },
        { TEXT("сидеть"),   TEXT("сидеть"),   EPosTag::Verb        },
        { TEXT("сижу"),     TEXT("сидеть"),   EPosTag::Verb        },
        { TEXT("сидит"),    TEXT("сидеть"),   EPosTag::Verb        },
        { TEXT("сидел"),    TEXT("сидеть"),   EPosTag::Verb        },
        { TEXT("сидела"),   TEXT("сидеть"),   EPosTag::Verb        },
        { TEXT("сиди"),     TEXT("сидеть"),   EPosTag::Verb        },
        { TEXT("лежать"),   TEXT("лежать"),   EPosTag::Verb        },
        { TEXT("лежу"),     TEXT("лежать"),   EPosTag::Verb        },
        { TEXT("лежит"),    TEXT("лежать"),   EPosTag::Verb        },
        { TEXT("лежал"),    TEXT("лежать"),   EPosTag::Verb        },
        { TEXT("лежала"),   TEXT("лежать"),   EPosTag::Verb        },
        { TEXT("лежи"),     TEXT("лежать"),   EPosTag::Verb        },
        { TEXT("войти"),    TEXT("войти"),    EPosTag::Verb        },
        { TEXT("войдёт"),   TEXT("войти"),    EPosTag::Verb        },
        { TEXT("вошёл"),    TEXT("войти"),    EPosTag::Verb        },
        { TEXT("вошла"),    TEXT("войти"),    EPosTag::Verb        },
        { TEXT("войди"),    TEXT("войти"),    EPosTag::Verb        },
        { TEXT("выйти"),    TEXT("выйти"),    EPosTag::Verb        },
        { TEXT("выйдет"),   TEXT("выйти"),    EPosTag::Verb        },
        { TEXT("вышел"),    TEXT("выйти"),    EPosTag::Verb        },
        { TEXT("вышла"),    TEXT("выйти"),    EPosTag::Verb        },
        { TEXT("выйди"),    TEXT("выйти"),    EPosTag::Verb        },
        { TEXT("прийти"),   TEXT("прийти"),   EPosTag::Verb        },
        { TEXT("придёт"),   TEXT("прийти"),   EPosTag::Verb        },
        { TEXT("пришёл"),   TEXT("прийти"),   EPosTag::Verb        },
        { TEXT("пришла"),   TEXT("прийти"),   EPosTag::Verb        },
        { TEXT("приди"),    TEXT("прийти"),   EPosTag::Verb        },
        { TEXT("уйти"),     TEXT("уйти"),     EPosTag::Verb        },
        { TEXT("уйдёт"),    TEXT("уйти"),     EPosTag::Verb        },
        { TEXT("ушёл"),     TEXT("уйти"),     EPosTag::Verb        },
        { TEXT("ушла"),     TEXT("уйти"),     EPosTag::Verb        },

        // --- глаголы: действие и передача ---
        { TEXT("дать"),     TEXT("дать"),     EPosTag::Verb        },
        { TEXT("дай"),      TEXT("дать"),     EPosTag::Verb        },
        { TEXT("даёт"),     TEXT("давать"),   EPosTag::Verb        },
        { TEXT("дал"),      TEXT("дать"),     EPosTag::Verb        },
        { TEXT("дала"),     TEXT("дать"),     EPosTag::Verb        },
        { TEXT("давать"),   TEXT("давать"),   EPosTag::Verb        },
        { TEXT("взять"),    TEXT("взять"),    EPosTag::Verb        },
        { TEXT("возьми"),   TEXT("взять"),    EPosTag::Verb        },
        { TEXT("берёт"),    TEXT("брать"),    EPosTag::Verb        },
        { TEXT("взял"),     TEXT("взять"),    EPosTag::Verb        },
        { TEXT("взяла"),    TEXT("взять"),    EPosTag::Verb        },
        { TEXT("брать"),    TEXT("брать"),    EPosTag::Verb        },
        { TEXT("купить"),   TEXT("купить"),   EPosTag::Verb        },
        { TEXT("купи"),     TEXT("купить"),   EPosTag::Verb        },
        { TEXT("купит"),    TEXT("купить"),   EPosTag::Verb        },
        { TEXT("купил"),    TEXT("купить"),   EPosTag::Verb        },
        { TEXT("купила"),   TEXT("купить"),   EPosTag::Verb        },
        { TEXT("получить"), TEXT("получить"), EPosTag::Verb        },
        { TEXT("получи"),   TEXT("получить"), EPosTag::Verb        },
        { TEXT("получит"),  TEXT("получить"), EPosTag::Verb        },
        { TEXT("получил"),  TEXT("получить"), EPosTag::Verb        },
        { TEXT("получила"), TEXT("получить"), EPosTag::Verb        },
        { TEXT("отправить"),TEXT("отправить"),EPosTag::Verb        },
        { TEXT("отправь"),  TEXT("отправить"),EPosTag::Verb        },
        { TEXT("отправил"), TEXT("отправить"),EPosTag::Verb        },
        { TEXT("принести"), TEXT("принести"), EPosTag::Verb        },
        { TEXT("принеси"),  TEXT("принести"), EPosTag::Verb        },
        { TEXT("принёс"),   TEXT("принести"), EPosTag::Verb        },
        { TEXT("принесла"), TEXT("принести"), EPosTag::Verb        },
        { TEXT("убрать"),   TEXT("убрать"),   EPosTag::Verb        },
        { TEXT("убери"),    TEXT("убрать"),   EPosTag::Verb        },
        { TEXT("убрал"),    TEXT("убрать"),   EPosTag::Verb        },
        { TEXT("убирает"),  TEXT("убирать"),  EPosTag::Verb        },
        { TEXT("поставить"),TEXT("поставить"),EPosTag::Verb        },
        { TEXT("поставь"),  TEXT("поставить"),EPosTag::Verb        },
        { TEXT("поставил"), TEXT("поставить"),EPosTag::Verb        },

        // --- глаголы: общение/восприятие (расширение) ---
        { TEXT("слушать"),  TEXT("слушать"),  EPosTag::Verb        },
        { TEXT("слушай"),   TEXT("слушать"),  EPosTag::Verb        },
        { TEXT("слушает"),  TEXT("слушать"),  EPosTag::Verb        },
        { TEXT("слушал"),   TEXT("слушать"),  EPosTag::Verb        },
        { TEXT("слушала"),  TEXT("слушать"),  EPosTag::Verb        },
        { TEXT("смотреть"), TEXT("смотреть"), EPosTag::Verb        },
        { TEXT("смотри"),   TEXT("смотреть"), EPosTag::Verb        },
        { TEXT("смотрит"),  TEXT("смотреть"), EPosTag::Verb        },
        { TEXT("смотрел"),  TEXT("смотреть"), EPosTag::Verb        },
        { TEXT("смотрела"), TEXT("смотреть"), EPosTag::Verb        },
        { TEXT("видишь"),   TEXT("видеть"),   EPosTag::Verb        },
        { TEXT("вижу"),     TEXT("видеть"),   EPosTag::Verb        },
        { TEXT("видел"),    TEXT("видеть"),   EPosTag::Verb        },
        { TEXT("видела"),   TEXT("видеть"),   EPosTag::Verb        },
        { TEXT("узнать"),   TEXT("узнать"),   EPosTag::Verb        },
        { TEXT("узнай"),    TEXT("узнать"),   EPosTag::Verb        },
        { TEXT("узнаёт"),   TEXT("узнавать"), EPosTag::Verb        },
        { TEXT("узнал"),    TEXT("узнать"),   EPosTag::Verb        },
        { TEXT("узнавать"), TEXT("узнавать"), EPosTag::Verb        },
        { TEXT("просить"),  TEXT("просить"),  EPosTag::Verb        },
        { TEXT("прошу"),    TEXT("просить"),  EPosTag::Verb        },
        { TEXT("просит"),   TEXT("просить"),  EPosTag::Verb        },
        { TEXT("попросить"),TEXT("попросить"),EPosTag::Verb        },
        { TEXT("попроси"),  TEXT("попросить"),EPosTag::Verb        },
        { TEXT("попросил"), TEXT("попросить"),EPosTag::Verb        },
        { TEXT("сообщить"), TEXT("сообщить"), EPosTag::Verb        },
        { TEXT("сообщи"),   TEXT("сообщить"), EPosTag::Verb        },
        { TEXT("сообщает"), TEXT("сообщать"), EPosTag::Verb        },
        { TEXT("сообщил"),  TEXT("сообщить"), EPosTag::Verb        },
        { TEXT("хотеть"),   TEXT("хотеть"),   EPosTag::Verb        },
        { TEXT("хочу"),     TEXT("хотеть"),   EPosTag::Verb        },
        { TEXT("хочет"),    TEXT("хотеть"),   EPosTag::Verb        },
        { TEXT("хочешь"),   TEXT("хотеть"),   EPosTag::Verb        },
        { TEXT("хотел"),    TEXT("хотеть"),   EPosTag::Verb        },
        { TEXT("хотела"),   TEXT("хотеть"),   EPosTag::Verb        },
        { TEXT("любить"),   TEXT("любить"),   EPosTag::Verb        },
        { TEXT("люблю"),    TEXT("любить"),   EPosTag::Verb        },
        { TEXT("любит"),    TEXT("любить"),   EPosTag::Verb        },
        { TEXT("любишь"),   TEXT("любить"),   EPosTag::Verb        },
        { TEXT("любил"),    TEXT("любить"),   EPosTag::Verb        },
        { TEXT("нравится"), TEXT("нравиться"),EPosTag::Verb        },
        { TEXT("нравятся"), TEXT("нравиться"),EPosTag::Verb        },
        { TEXT("понравилось"),TEXT("нравиться"),EPosTag::Verb      },
        { TEXT("знает"),    TEXT("знать"),    EPosTag::Verb        },
        { TEXT("знал"),     TEXT("знать"),    EPosTag::Verb        },
        { TEXT("знала"),    TEXT("знать"),    EPosTag::Verb        },
        { TEXT("знаю"),     TEXT("знать"),    EPosTag::Verb        },
        { TEXT("может"),    TEXT("мочь"),     EPosTag::Verb        },
        { TEXT("могу"),     TEXT("мочь"),     EPosTag::Verb        },
        { TEXT("мог"),      TEXT("мочь"),     EPosTag::Verb        },
        { TEXT("могла"),    TEXT("мочь"),     EPosTag::Verb        },

        // --- существительные: животные ---
        { TEXT("собака"),   TEXT("собака"),   EPosTag::Noun        },
        { TEXT("собаки"),   TEXT("собака"),   EPosTag::Noun        },
        { TEXT("собаку"),   TEXT("собака"),   EPosTag::Noun        },
        { TEXT("собак"),    TEXT("собака"),   EPosTag::Noun        },
        { TEXT("собаке"),   TEXT("собака"),   EPosTag::Noun        },
        { TEXT("пёс"),      TEXT("пёс"),      EPosTag::Noun        },
        { TEXT("пса"),      TEXT("пёс"),      EPosTag::Noun        },
        { TEXT("птица"),    TEXT("птица"),    EPosTag::Noun        },
        { TEXT("птицы"),    TEXT("птица"),    EPosTag::Noun        },
        { TEXT("птицу"),    TEXT("птица"),    EPosTag::Noun        },
        { TEXT("птиц"),     TEXT("птица"),    EPosTag::Noun        },
        { TEXT("рыба"),     TEXT("рыба"),     EPosTag::Noun        },
        { TEXT("рыбы"),     TEXT("рыба"),     EPosTag::Noun        },
        { TEXT("рыбу"),     TEXT("рыба"),     EPosTag::Noun        },
        { TEXT("рыб"),      TEXT("рыба"),     EPosTag::Noun        },
        { TEXT("лошадь"),   TEXT("лошадь"),   EPosTag::Noun        },
        { TEXT("лошади"),   TEXT("лошадь"),   EPosTag::Noun        },
        { TEXT("лошадей"),  TEXT("лошадь"),   EPosTag::Noun        },
        { TEXT("медведь"),  TEXT("медведь"),  EPosTag::Noun        },
        { TEXT("медведя"),  TEXT("медведь"),  EPosTag::Noun        },
        { TEXT("медведей"), TEXT("медведь"),  EPosTag::Noun        },
        { TEXT("волк"),     TEXT("волк"),     EPosTag::Noun        },
        { TEXT("волка"),    TEXT("волк"),     EPosTag::Noun        },
        { TEXT("волков"),   TEXT("волк"),     EPosTag::Noun        },
        { TEXT("лиса"),     TEXT("лиса"),     EPosTag::Noun        },
        { TEXT("лисы"),     TEXT("лиса"),     EPosTag::Noun        },
        { TEXT("заяц"),     TEXT("заяц"),     EPosTag::Noun        },
        { TEXT("зайца"),    TEXT("заяц"),     EPosTag::Noun        },

        // --- существительные: природа ---
        { TEXT("вода"),     TEXT("вода"),     EPosTag::Noun        },
        { TEXT("воды"),     TEXT("вода"),     EPosTag::Noun        },
        { TEXT("воду"),     TEXT("вода"),     EPosTag::Noun        },
        { TEXT("водой"),    TEXT("вода"),     EPosTag::Noun        },
        { TEXT("огонь"),    TEXT("огонь"),    EPosTag::Noun        },
        { TEXT("огня"),     TEXT("огонь"),    EPosTag::Noun        },
        { TEXT("земля"),    TEXT("земля"),    EPosTag::Noun        },
        { TEXT("земли"),    TEXT("земля"),    EPosTag::Noun        },
        { TEXT("земле"),    TEXT("земля"),    EPosTag::Noun        },
        { TEXT("лес"),      TEXT("лес"),      EPosTag::Noun        },
        { TEXT("леса"),     TEXT("лес"),      EPosTag::Noun        },
        { TEXT("лесу"),     TEXT("лес"),      EPosTag::Noun        },
        { TEXT("лесе"),     TEXT("лес"),      EPosTag::Noun        },
        { TEXT("море"),     TEXT("море"),     EPosTag::Noun        },
        { TEXT("моря"),     TEXT("море"),     EPosTag::Noun        },
        { TEXT("морей"),    TEXT("море"),     EPosTag::Noun        },
        { TEXT("река"),     TEXT("река"),     EPosTag::Noun        },
        { TEXT("реки"),     TEXT("река"),     EPosTag::Noun        },
        { TEXT("реке"),     TEXT("река"),     EPosTag::Noun        },
        { TEXT("реку"),     TEXT("река"),     EPosTag::Noun        },
        { TEXT("рек"),      TEXT("река"),     EPosTag::Noun        },
        { TEXT("гора"),     TEXT("гора"),     EPosTag::Noun        },
        { TEXT("горы"),     TEXT("гора"),     EPosTag::Noun        },
        { TEXT("горе"),     TEXT("гора"),     EPosTag::Noun        },
        { TEXT("гор"),      TEXT("гора"),     EPosTag::Noun        },
        { TEXT("небо"),     TEXT("небо"),     EPosTag::Noun        },
        { TEXT("неба"),     TEXT("небо"),     EPosTag::Noun        },
        { TEXT("небе"),     TEXT("небо"),     EPosTag::Noun        },
        { TEXT("солнце"),   TEXT("солнце"),   EPosTag::Noun        },
        { TEXT("солнца"),   TEXT("солнце"),   EPosTag::Noun        },
        { TEXT("дождь"),    TEXT("дождь"),    EPosTag::Noun        },
        { TEXT("дождя"),    TEXT("дождь"),    EPosTag::Noun        },
        { TEXT("дожди"),    TEXT("дождь"),    EPosTag::Noun        },
        { TEXT("снег"),     TEXT("снег"),     EPosTag::Noun        },
        { TEXT("снега"),    TEXT("снег"),     EPosTag::Noun        },
        { TEXT("ветер"),    TEXT("ветер"),    EPosTag::Noun        },
        { TEXT("ветра"),    TEXT("ветер"),    EPosTag::Noun        },
        { TEXT("ветром"),   TEXT("ветер"),    EPosTag::Noun        },

        // --- существительные: тело ---
        { TEXT("голова"),   TEXT("голова"),   EPosTag::Noun        },
        { TEXT("головы"),   TEXT("голова"),   EPosTag::Noun        },
        { TEXT("голове"),   TEXT("голова"),   EPosTag::Noun        },
        { TEXT("голову"),   TEXT("голова"),   EPosTag::Noun        },
        { TEXT("рука"),     TEXT("рука"),     EPosTag::Noun        },
        { TEXT("руки"),     TEXT("рука"),     EPosTag::Noun        },
        { TEXT("руке"),     TEXT("рука"),     EPosTag::Noun        },
        { TEXT("руку"),     TEXT("рука"),     EPosTag::Noun        },
        { TEXT("рук"),      TEXT("рука"),     EPosTag::Noun        },
        { TEXT("нога"),     TEXT("нога"),     EPosTag::Noun        },
        { TEXT("ноги"),     TEXT("нога"),     EPosTag::Noun        },
        { TEXT("ноге"),     TEXT("нога"),     EPosTag::Noun        },
        { TEXT("ног"),      TEXT("нога"),     EPosTag::Noun        },
        { TEXT("глаз"),     TEXT("глаз"),     EPosTag::Noun        },
        { TEXT("глаза"),    TEXT("глаз"),     EPosTag::Noun        },
        { TEXT("глазе"),    TEXT("глаз"),     EPosTag::Noun        },
        { TEXT("сердце"),   TEXT("сердце"),   EPosTag::Noun        },
        { TEXT("сердца"),   TEXT("сердце"),   EPosTag::Noun        },

        // --- существительные: быт и окружение ---
        { TEXT("дом"),      TEXT("дом"),      EPosTag::Noun        },
        { TEXT("дома"),     TEXT("дом"),      EPosTag::Noun        },
        { TEXT("доме"),     TEXT("дом"),      EPosTag::Noun        },
        { TEXT("домой"),    TEXT("дом"),      EPosTag::Noun        },
        { TEXT("домов"),    TEXT("дом"),      EPosTag::Noun        },
        { TEXT("квартира"), TEXT("квартира"), EPosTag::Noun        },
        { TEXT("квартиры"), TEXT("квартира"), EPosTag::Noun        },
        { TEXT("квартире"), TEXT("квартира"), EPosTag::Noun        },
        { TEXT("квартиру"), TEXT("квартира"), EPosTag::Noun        },
        { TEXT("комната"),  TEXT("комната"),  EPosTag::Noun        },
        { TEXT("комнаты"),  TEXT("комната"),  EPosTag::Noun        },
        { TEXT("комнате"),  TEXT("комната"),  EPosTag::Noun        },
        { TEXT("улица"),    TEXT("улица"),    EPosTag::Noun        },
        { TEXT("улицы"),    TEXT("улица"),    EPosTag::Noun        },
        { TEXT("улице"),    TEXT("улица"),    EPosTag::Noun        },
        { TEXT("город"),    TEXT("город"),    EPosTag::Noun        },
        { TEXT("города"),   TEXT("город"),    EPosTag::Noun        },
        { TEXT("городе"),   TEXT("город"),    EPosTag::Noun        },
        { TEXT("городов"),  TEXT("город"),    EPosTag::Noun        },
        { TEXT("страна"),   TEXT("страна"),   EPosTag::Noun        },
        { TEXT("страны"),   TEXT("страна"),   EPosTag::Noun        },
        { TEXT("стране"),   TEXT("страна"),   EPosTag::Noun        },
        { TEXT("книга"),    TEXT("книга"),    EPosTag::Noun        },
        { TEXT("книги"),    TEXT("книга"),    EPosTag::Noun        },
        { TEXT("книге"),    TEXT("книга"),    EPosTag::Noun        },
        { TEXT("книг"),     TEXT("книга"),    EPosTag::Noun        },
        { TEXT("стол"),     TEXT("стол"),     EPosTag::Noun        },
        { TEXT("стола"),    TEXT("стол"),     EPosTag::Noun        },
        { TEXT("столе"),    TEXT("стол"),     EPosTag::Noun        },
        { TEXT("столов"),   TEXT("стол"),     EPosTag::Noun        },
        { TEXT("стул"),     TEXT("стул"),     EPosTag::Noun        },
        { TEXT("стула"),    TEXT("стул"),     EPosTag::Noun        },
        { TEXT("стуле"),    TEXT("стул"),     EPosTag::Noun        },
        { TEXT("машина"),   TEXT("машина"),   EPosTag::Noun        },
        { TEXT("машины"),   TEXT("машина"),   EPosTag::Noun        },
        { TEXT("машине"),   TEXT("машина"),   EPosTag::Noun        },
        { TEXT("машин"),    TEXT("машина"),   EPosTag::Noun        },
        { TEXT("телефон"),  TEXT("телефон"),  EPosTag::Noun        },
        { TEXT("телефона"), TEXT("телефон"),  EPosTag::Noun        },
        { TEXT("телефоне"), TEXT("телефон"),  EPosTag::Noun        },
        { TEXT("компьютер"),TEXT("компьютер"),EPosTag::Noun        },
        { TEXT("компьютера"),TEXT("компьютер"),EPosTag::Noun       },
        { TEXT("компьютере"),TEXT("компьютер"),EPosTag::Noun       },
        { TEXT("деньги"),   TEXT("деньги"),   EPosTag::Noun        },
        { TEXT("денег"),    TEXT("деньги"),   EPosTag::Noun        },
        { TEXT("деньгам"),  TEXT("деньги"),   EPosTag::Noun        },
        { TEXT("работа"),   TEXT("работа"),   EPosTag::Noun        },
        { TEXT("работы"),   TEXT("работа"),   EPosTag::Noun        },
        { TEXT("работе"),   TEXT("работа"),   EPosTag::Noun        },
        { TEXT("дело"),     TEXT("дело"),     EPosTag::Noun        },
        { TEXT("дела"),     TEXT("дело"),     EPosTag::Noun        },
        { TEXT("делом"),    TEXT("дело"),     EPosTag::Noun        },
        { TEXT("дел"),      TEXT("дело"),     EPosTag::Noun        },
        { TEXT("день"),     TEXT("день"),     EPosTag::Noun        },
        { TEXT("дня"),      TEXT("день"),     EPosTag::Noun        },
        { TEXT("дней"),     TEXT("день"),     EPosTag::Noun        },
        { TEXT("днём"),     TEXT("день"),     EPosTag::Noun        },
        { TEXT("ночь"),     TEXT("ночь"),     EPosTag::Noun        },
        { TEXT("ночи"),     TEXT("ночь"),     EPosTag::Noun        },
        { TEXT("ночью"),    TEXT("ночь"),     EPosTag::Noun        },
        { TEXT("утро"),     TEXT("утро"),     EPosTag::Noun        },
        { TEXT("утра"),     TEXT("утро"),     EPosTag::Noun        },
        { TEXT("утром"),    TEXT("утро"),     EPosTag::Noun        },
        { TEXT("вечер"),    TEXT("вечер"),    EPosTag::Noun        },
        { TEXT("вечера"),   TEXT("вечер"),    EPosTag::Noun        },
        { TEXT("вечером"),  TEXT("вечер"),    EPosTag::Noun        },
        { TEXT("год"),      TEXT("год"),      EPosTag::Noun        },
        { TEXT("года"),     TEXT("год"),      EPosTag::Noun        },
        { TEXT("годов"),    TEXT("год"),      EPosTag::Noun        },
        { TEXT("лет"),      TEXT("год"),      EPosTag::Noun        },
        { TEXT("неделя"),   TEXT("неделя"),   EPosTag::Noun        },
        { TEXT("недели"),   TEXT("неделя"),   EPosTag::Noun        },
        { TEXT("недель"),   TEXT("неделя"),   EPosTag::Noun        },
        { TEXT("месяц"),    TEXT("месяц"),    EPosTag::Noun        },
        { TEXT("месяца"),   TEXT("месяц"),    EPosTag::Noun        },
        { TEXT("месяцев"),  TEXT("месяц"),    EPosTag::Noun        },
        { TEXT("еда"),      TEXT("еда"),      EPosTag::Noun        },
        { TEXT("еды"),      TEXT("еда"),      EPosTag::Noun        },
        { TEXT("хлеб"),     TEXT("хлеб"),     EPosTag::Noun        },
        { TEXT("хлеба"),    TEXT("хлеб"),     EPosTag::Noun        },
        { TEXT("молоко"),   TEXT("молоко"),   EPosTag::Noun        },
        { TEXT("молока"),   TEXT("молоко"),   EPosTag::Noun        },
        { TEXT("чай"),      TEXT("чай"),      EPosTag::Noun        },
        { TEXT("чая"),      TEXT("чай"),      EPosTag::Noun        },
        { TEXT("кофе"),     TEXT("кофе"),     EPosTag::Noun        },
        { TEXT("цвет"),     TEXT("цвет"),     EPosTag::Noun        },
        { TEXT("цвета"),    TEXT("цвет"),     EPosTag::Noun        },
        { TEXT("цветов"),   TEXT("цвет"),     EPosTag::Noun        },

        // --- прилагательные (v0.4 расширение) ---
        { TEXT("маленький"),TEXT("маленький"),EPosTag::Adjective   },
        { TEXT("маленькая"),TEXT("маленький"),EPosTag::Adjective   },
        { TEXT("маленькое"),TEXT("маленький"),EPosTag::Adjective   },
        { TEXT("маленькие"),TEXT("маленький"),EPosTag::Adjective   },
        { TEXT("старый"),   TEXT("старый"),   EPosTag::Adjective   },
        { TEXT("старая"),   TEXT("старый"),   EPosTag::Adjective   },
        { TEXT("старое"),   TEXT("старый"),   EPosTag::Adjective   },
        { TEXT("старые"),   TEXT("старый"),   EPosTag::Adjective   },
        { TEXT("старого"),  TEXT("старый"),   EPosTag::Adjective   },
        { TEXT("молодой"),  TEXT("молодой"),  EPosTag::Adjective   },
        { TEXT("молодая"),  TEXT("молодой"),  EPosTag::Adjective   },
        { TEXT("молодое"),  TEXT("молодой"),  EPosTag::Adjective   },
        { TEXT("молодые"),  TEXT("молодой"),  EPosTag::Adjective   },
        { TEXT("умный"),    TEXT("умный"),    EPosTag::Adjective   },
        { TEXT("умная"),    TEXT("умный"),    EPosTag::Adjective   },
        { TEXT("умное"),    TEXT("умный"),    EPosTag::Adjective   },
        { TEXT("умные"),    TEXT("умный"),    EPosTag::Adjective   },
        { TEXT("красивый"), TEXT("красивый"), EPosTag::Adjective   },
        { TEXT("красивая"), TEXT("красивый"), EPosTag::Adjective   },
        { TEXT("красивое"), TEXT("красивый"), EPosTag::Adjective   },
        { TEXT("красивые"), TEXT("красивый"), EPosTag::Adjective   },
        { TEXT("быстрый"),  TEXT("быстрый"),  EPosTag::Adjective   },
        { TEXT("быстрая"),  TEXT("быстрый"),  EPosTag::Adjective   },
        { TEXT("быстрое"),  TEXT("быстрый"),  EPosTag::Adjective   },
        { TEXT("быстрые"),  TEXT("быстрый"),  EPosTag::Adjective   },
        { TEXT("медленный"),TEXT("медленный"),EPosTag::Adjective   },
        { TEXT("медленная"),TEXT("медленный"),EPosTag::Adjective   },
        { TEXT("медленное"),TEXT("медленный"),EPosTag::Adjective   },
        { TEXT("длинный"),  TEXT("длинный"),  EPosTag::Adjective   },
        { TEXT("длинная"),  TEXT("длинный"),  EPosTag::Adjective   },
        { TEXT("длинное"),  TEXT("длинный"),  EPosTag::Adjective   },
        { TEXT("длинные"),  TEXT("длинный"),  EPosTag::Adjective   },
        { TEXT("короткий"), TEXT("короткий"), EPosTag::Adjective   },
        { TEXT("короткая"), TEXT("короткий"), EPosTag::Adjective   },
        { TEXT("короткое"), TEXT("короткий"), EPosTag::Adjective   },
        { TEXT("короткие"), TEXT("короткий"), EPosTag::Adjective   },
        { TEXT("лёгкий"),   TEXT("лёгкий"),   EPosTag::Adjective   },
        { TEXT("лёгкая"),   TEXT("лёгкий"),   EPosTag::Adjective   },
        { TEXT("лёгкое"),   TEXT("лёгкий"),   EPosTag::Adjective   },
        { TEXT("лёгкие"),   TEXT("лёгкий"),   EPosTag::Adjective   },
        { TEXT("тяжёлый"),  TEXT("тяжёлый"),  EPosTag::Adjective   },
        { TEXT("тяжёлая"),  TEXT("тяжёлый"),  EPosTag::Adjective   },
        { TEXT("тяжёлое"),  TEXT("тяжёлый"),  EPosTag::Adjective   },
        { TEXT("тяжёлые"),  TEXT("тяжёлый"),  EPosTag::Adjective   },
        { TEXT("интересный"),TEXT("интересный"),EPosTag::Adjective },
        { TEXT("интересная"),TEXT("интересный"),EPosTag::Adjective },
        { TEXT("интересное"),TEXT("интересный"),EPosTag::Adjective },
        { TEXT("интересные"),TEXT("интересный"),EPosTag::Adjective },
        { TEXT("последний"),TEXT("последний"),EPosTag::Adjective   },
        { TEXT("последняя"),TEXT("последний"),EPosTag::Adjective   },
        { TEXT("последнее"),TEXT("последний"),EPosTag::Adjective   },
        { TEXT("последние"),TEXT("последний"),EPosTag::Adjective   },
        { TEXT("следующий"),TEXT("следующий"),EPosTag::Adjective   },
        { TEXT("следующая"),TEXT("следующий"),EPosTag::Adjective   },
        { TEXT("следующее"),TEXT("следующий"),EPosTag::Adjective   },
        { TEXT("следующие"),TEXT("следующий"),EPosTag::Adjective   },
        { TEXT("красный"),  TEXT("красный"),  EPosTag::Adjective   },
        { TEXT("красная"),  TEXT("красный"),  EPosTag::Adjective   },
        { TEXT("красное"),  TEXT("красный"),  EPosTag::Adjective   },
        { TEXT("красные"),  TEXT("красный"),  EPosTag::Adjective   },
        { TEXT("синий"),    TEXT("синий"),    EPosTag::Adjective   },
        { TEXT("синяя"),    TEXT("синий"),    EPosTag::Adjective   },
        { TEXT("синее"),    TEXT("синий"),    EPosTag::Adjective   },
        { TEXT("синие"),    TEXT("синий"),    EPosTag::Adjective   },
        { TEXT("зелёный"),  TEXT("зелёный"),  EPosTag::Adjective   },
        { TEXT("зелёная"),  TEXT("зелёный"),  EPosTag::Adjective   },
        { TEXT("зелёное"),  TEXT("зелёный"),  EPosTag::Adjective   },
        { TEXT("зелёные"),  TEXT("зелёный"),  EPosTag::Adjective   },
        { TEXT("белый"),    TEXT("белый"),    EPosTag::Adjective   },
        { TEXT("белая"),    TEXT("белый"),    EPosTag::Adjective   },
        { TEXT("белое"),    TEXT("белый"),    EPosTag::Adjective   },
        { TEXT("белые"),    TEXT("белый"),    EPosTag::Adjective   },
        { TEXT("чёрный"),   TEXT("чёрный"),   EPosTag::Adjective   },
        { TEXT("чёрная"),   TEXT("чёрный"),   EPosTag::Adjective   },
        { TEXT("чёрное"),   TEXT("чёрный"),   EPosTag::Adjective   },
        { TEXT("чёрные"),   TEXT("чёрный"),   EPosTag::Adjective   },
        { TEXT("жёлтый"),   TEXT("жёлтый"),   EPosTag::Adjective   },
        { TEXT("жёлтая"),   TEXT("жёлтый"),   EPosTag::Adjective   },
        { TEXT("жёлтое"),   TEXT("жёлтый"),   EPosTag::Adjective   },
        { TEXT("жёлтые"),   TEXT("жёлтый"),   EPosTag::Adjective   },
        { TEXT("добрый"),   TEXT("добрый"),   EPosTag::Adjective   },
        { TEXT("добрая"),   TEXT("добрый"),   EPosTag::Adjective   },
        { TEXT("доброе"),   TEXT("добрый"),   EPosTag::Adjective   },
        { TEXT("добрые"),   TEXT("добрый"),   EPosTag::Adjective   },
        { TEXT("злой"),     TEXT("злой"),     EPosTag::Adjective   },
        { TEXT("злая"),     TEXT("злой"),     EPosTag::Adjective   },
        { TEXT("злое"),     TEXT("злой"),     EPosTag::Adjective   },
        { TEXT("злые"),     TEXT("злой"),     EPosTag::Adjective   },
        { TEXT("целый"),    TEXT("целый"),    EPosTag::Adjective   },
        { TEXT("целая"),    TEXT("целый"),    EPosTag::Adjective   },
        { TEXT("целое"),    TEXT("целый"),    EPosTag::Adjective   },
        { TEXT("полный"),   TEXT("полный"),   EPosTag::Adjective   },
        { TEXT("полная"),   TEXT("полный"),   EPosTag::Adjective   },
        { TEXT("полное"),   TEXT("полный"),   EPosTag::Adjective   },
        { TEXT("полные"),   TEXT("полный"),   EPosTag::Adjective   },
        { TEXT("пустой"),   TEXT("пустой"),   EPosTag::Adjective   },
        { TEXT("пустая"),   TEXT("пустой"),   EPosTag::Adjective   },
        { TEXT("пустое"),   TEXT("пустой"),   EPosTag::Adjective   },
        { TEXT("пустые"),   TEXT("пустой"),   EPosTag::Adjective   },
        { TEXT("свободный"),TEXT("свободный"),EPosTag::Adjective   },
        { TEXT("свободная"),TEXT("свободный"),EPosTag::Adjective   },
        { TEXT("свободное"),TEXT("свободный"),EPosTag::Adjective   },
        { TEXT("открытый"), TEXT("открытый"), EPosTag::Adjective   },
        { TEXT("открытая"), TEXT("открытый"), EPosTag::Adjective   },
        { TEXT("открытое"), TEXT("открытый"), EPosTag::Adjective   },
        { TEXT("закрытый"), TEXT("закрытый"), EPosTag::Adjective   },
        { TEXT("закрытая"), TEXT("закрытый"), EPosTag::Adjective   },
        { TEXT("закрытое"), TEXT("закрытый"), EPosTag::Adjective   },

        // --- числительные (v0.4 расширение) ---
        { TEXT("шесть"),    TEXT("шесть"),    EPosTag::Numeral     },
        { TEXT("шести"),    TEXT("шесть"),    EPosTag::Numeral     },
        { TEXT("семь"),     TEXT("семь"),     EPosTag::Numeral     },
        { TEXT("семи"),     TEXT("семь"),     EPosTag::Numeral     },
        { TEXT("восемь"),   TEXT("восемь"),   EPosTag::Numeral     },
        { TEXT("восьми"),   TEXT("восемь"),   EPosTag::Numeral     },
        { TEXT("девять"),   TEXT("девять"),   EPosTag::Numeral     },
        { TEXT("девяти"),   TEXT("девять"),   EPosTag::Numeral     },
        { TEXT("сто"),      TEXT("сто"),      EPosTag::Numeral     },
        { TEXT("ста"),      TEXT("сто"),      EPosTag::Numeral     },
        { TEXT("тысяча"),   TEXT("тысяча"),   EPosTag::Numeral     },
        { TEXT("тысячи"),   TEXT("тысяча"),   EPosTag::Numeral     },
        { TEXT("тысяч"),    TEXT("тысяча"),   EPosTag::Numeral     },
        { TEXT("второй"),   TEXT("второй"),   EPosTag::Numeral     },
        { TEXT("вторая"),   TEXT("второй"),   EPosTag::Numeral     },
        { TEXT("второе"),   TEXT("второй"),   EPosTag::Numeral     },
        { TEXT("третий"),   TEXT("третий"),   EPosTag::Numeral     },
        { TEXT("третья"),   TEXT("третий"),   EPosTag::Numeral     },
        { TEXT("третье"),   TEXT("третий"),   EPosTag::Numeral     },
        { TEXT("четвёртый"),TEXT("четвёртый"),EPosTag::Numeral     },
        { TEXT("пятый"),    TEXT("пятый"),    EPosTag::Numeral     },
        { TEXT("десятый"),  TEXT("десятый"),  EPosTag::Numeral     },

        // --- наречия (v0.4 расширение) ---
        { TEXT("сегодня"),  TEXT("сегодня"),  EPosTag::Adverb      },
        { TEXT("вчера"),    TEXT("вчера"),    EPosTag::Adverb      },
        { TEXT("завтра"),   TEXT("завтра"),   EPosTag::Adverb      },
        { TEXT("утром"),    TEXT("утром"),    EPosTag::Adverb      },
        { TEXT("вечером"),  TEXT("вечером"),  EPosTag::Adverb      },
        { TEXT("ночью"),    TEXT("ночью"),    EPosTag::Adverb      },
        { TEXT("днём"),     TEXT("днём"),     EPosTag::Adverb      },
        { TEXT("часто"),    TEXT("часто"),    EPosTag::Adverb      },
        { TEXT("редко"),    TEXT("редко"),    EPosTag::Adverb      },
        { TEXT("давно"),    TEXT("давно"),    EPosTag::Adverb      },
        { TEXT("недавно"),  TEXT("недавно"),  EPosTag::Adverb      },
        { TEXT("скоро"),    TEXT("скоро"),    EPosTag::Adverb      },
        { TEXT("справа"),   TEXT("справа"),   EPosTag::Adverb      },
        { TEXT("слева"),    TEXT("слева"),    EPosTag::Adverb      },
        { TEXT("рядом"),    TEXT("рядом"),    EPosTag::Adverb      },
        { TEXT("далеко"),   TEXT("далеко"),   EPosTag::Adverb      },
        { TEXT("близко"),   TEXT("близко"),   EPosTag::Adverb      },
        { TEXT("вместе"),   TEXT("вместе"),   EPosTag::Adverb      },
        { TEXT("немного"),  TEXT("немного"),  EPosTag::Adverb      },
        { TEXT("слишком"),  TEXT("слишком"),  EPosTag::Adverb      },
        { TEXT("достаточно"),TEXT("достаточно"),EPosTag::Adverb    },
        { TEXT("почти"),    TEXT("почти"),    EPosTag::Adverb      },
        { TEXT("снова"),    TEXT("снова"),    EPosTag::Adverb      },
        { TEXT("опять"),    TEXT("опять"),    EPosTag::Adverb      },
        { TEXT("наконец"),  TEXT("наконец"),  EPosTag::Adverb      },
        { TEXT("сначала"),  TEXT("сначала"),  EPosTag::Adverb      },
        { TEXT("вообще"),   TEXT("вообще"),   EPosTag::Adverb      },
        { TEXT("совсем"),   TEXT("совсем"),   EPosTag::Adverb      },
        { TEXT("примерно"), TEXT("примерно"), EPosTag::Adverb      },
        { TEXT("именно"),   TEXT("именно"),   EPosTag::Adverb      },
        { TEXT("просто"),   TEXT("просто"),   EPosTag::Adverb      },

        // --- союзы (дополнение) ---
        { TEXT("либо"),     TEXT("либо"),     EPosTag::Conjunction },
        { TEXT("однако"),   TEXT("однако"),   EPosTag::Conjunction },
        { TEXT("зато"),     TEXT("зато"),     EPosTag::Conjunction },
        { TEXT("тогда"),    TEXT("тогда"),    EPosTag::Conjunction },
        { TEXT("поэтому"),  TEXT("поэтому"),  EPosTag::Conjunction },
        { TEXT("ведь"),     TEXT("ведь"),     EPosTag::Particle    },
        { TEXT("именно"),   TEXT("именно"),   EPosTag::Particle    }, // дубль, союз+частица — ок
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
