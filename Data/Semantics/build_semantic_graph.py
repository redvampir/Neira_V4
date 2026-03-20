#!/usr/bin/env python3
# build_semantic_graph.py
# v0.1 — Построение семантического графа для Neira из Wiktionary и RuWordNet.
#
# Использование:
#   python build_semantic_graph.py --output semantics.db [опции]
#
# Опции:
#   --words FILE          Файл со списком лемм (по одной на строку)
#   --wiktionary          Включить обогащение через Wiktionary API
#   --ruwordnet FILE      Путь к XML файлу RuWordNet (offline)
#   --delay FLOAT         Задержка между запросами к API (по умолч. 1.0 сек)
#   --limit INT           Ограничить кол-во слов (для тестов)
#   --output FILE         Путь к SQLite файлу (по умолч. semantics.db)
#
# Типы связей:
#   synonym     — кот → кошка (то же самое)
#   antonym     — горячий → холодный (противоположное)
#   hypernym    — кот → животное (is-a, родовое понятие)
#   hyponym     — животное → кот (has-subtype, видовое понятие)
#   meronym     — кот → лапа (has-part, часть)
#   collocation — часто встречается вместе

import argparse
import json
import os
import re
import sqlite3
import sys
import time
import urllib.request
import urllib.parse
import xml.etree.ElementTree as ET
from collections import defaultdict


# ---------------------------------------------------------------------------
# Схема базы данных
# ---------------------------------------------------------------------------
SCHEMA = """
CREATE TABLE IF NOT EXISTS words (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    lemma       TEXT    NOT NULL UNIQUE,
    pos         TEXT    DEFAULT '',
    created_at  TEXT    DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS relations (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    word_id       INTEGER NOT NULL,
    target_lemma  TEXT    NOT NULL,
    relation_type TEXT    NOT NULL,
    weight        REAL    DEFAULT 1.0,
    source        TEXT    DEFAULT 'wiktionary',
    FOREIGN KEY(word_id) REFERENCES words(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_words_lemma       ON words(lemma);
CREATE INDEX IF NOT EXISTS idx_relations_word    ON relations(word_id);
CREATE INDEX IF NOT EXISTS idx_relations_type    ON relations(relation_type);
"""


# ---------------------------------------------------------------------------
# Встроенные слова-семена из словаря-ядра Neira (если --words не задан)
# ---------------------------------------------------------------------------
SEED_WORDS = [
    # Существительные
    "кот", "кошка", "собака", "животное", "человек", "ребёнок", "мир",
    "вопрос", "ответ", "знание", "анализ", "фраза", "гипотеза", "контекст",
    "задача", "результат", "смысл", "слово", "язык", "текст", "речь",
    "мысль", "идея", "факт", "информация", "данные", "система", "процесс",
    "время", "место", "день", "ночь", "утро", "вечер", "год", "час",
    "дом", "город", "страна", "дорога", "вода", "еда", "работа", "игра",
    # Глаголы (инфинитивы)
    "говорить", "думать", "понять", "делать", "знать", "видеть", "слышать",
    "хотеть", "любить", "идти", "ходить", "бежать", "стоять", "сидеть",
    "дать", "взять", "купить", "получить", "найти", "открыть", "закрыть",
    "начать", "кончить", "помочь", "спросить", "ответить", "объяснить",
    # Прилагательные
    "хороший", "плохой", "большой", "маленький", "новый", "старый",
    "важный", "правильный", "сложный", "простой", "умный", "быстрый",
    "красный", "синий", "белый", "чёрный", "горячий", "холодный",
    # Наречия
    "быстро", "медленно", "хорошо", "плохо", "очень", "немного",
    "сегодня", "вчера", "завтра", "сейчас", "здесь", "там",
]


# ---------------------------------------------------------------------------
# Инициализация базы данных
# ---------------------------------------------------------------------------
def init_db(path: str) -> sqlite3.Connection:
    conn = sqlite3.connect(path)
    conn.executescript(SCHEMA)
    conn.commit()
    return conn


def get_or_create_word(conn: sqlite3.Connection, lemma: str, pos: str = "") -> int:
    cur = conn.execute("SELECT id FROM words WHERE lemma = ?", (lemma,))
    row = cur.fetchone()
    if row:
        return row[0]
    cur = conn.execute("INSERT INTO words (lemma, pos) VALUES (?, ?)", (lemma, pos))
    conn.commit()
    return cur.lastrowid


def add_relation(conn: sqlite3.Connection, word_id: int, target: str,
                 rel_type: str, weight: float = 1.0, source: str = "wiktionary"):
    if not target or not target.strip():
        return
    target = target.strip().lower()
    # Проверить, не дубль ли
    cur = conn.execute(
        "SELECT id FROM relations WHERE word_id=? AND target_lemma=? AND relation_type=?",
        (word_id, target, rel_type)
    )
    if cur.fetchone():
        return
    conn.execute(
        "INSERT INTO relations (word_id, target_lemma, relation_type, weight, source) "
        "VALUES (?, ?, ?, ?, ?)",
        (word_id, target, rel_type, weight, source)
    )


def word_already_processed(conn: sqlite3.Connection, lemma: str) -> bool:
    """Слово считается обработанным, если у него есть хотя бы одна связь."""
    cur = conn.execute(
        "SELECT COUNT(*) FROM relations r JOIN words w ON r.word_id=w.id WHERE w.lemma=?",
        (lemma,)
    )
    return cur.fetchone()[0] > 0


# ---------------------------------------------------------------------------
# Wiktionary API — парсинг русского Wiktionary
# ---------------------------------------------------------------------------
WIKTIONARY_API = "https://ru.wiktionary.org/w/api.php"

SECTION_PATTERNS = {
    "synonym":    re.compile(r"=== ?Синонимы ?===\s*\n(.*?)(?===|\Z)", re.DOTALL),
    "antonym":    re.compile(r"=== ?Антонимы ?===\s*\n(.*?)(?===|\Z)", re.DOTALL),
    "hypernym":   re.compile(r"=== ?Гиперонимы ?===\s*\n(.*?)(?===|\Z)", re.DOTALL),
    "hyponym":    re.compile(r"=== ?Гипонимы ?===\s*\n(.*?)(?===|\Z)", re.DOTALL),
    "meronym":    re.compile(r"=== ?Меронимы ?===\s*\n(.*?)(?===|\Z)", re.DOTALL),
    "collocation":re.compile(r"=== ?Устойчивые сочетания ?===\s*\n(.*?)(?===|\Z)", re.DOTALL),
}

# Только русский раздел
RU_SECTION_RE = re.compile(r"== ?Русский ?==\s*\n(.*?)(?:== ?[А-ЯA-Z]|\Z)", re.DOTALL)

# Извлечение слов из викиразметки: [[кошка]], [[слово|форма]], * кошка, кошка, котик
WORD_LINK_RE = re.compile(r"\[\[([^\]|]+)(?:\|[^\]]+)?\]\]|(?:^|\*\s*)([а-яёА-ЯЁa-z\-]+)", re.MULTILINE)


def fetch_wiktionary_raw(word: str) -> str | None:
    params = urllib.parse.urlencode({
        "action": "query",
        "prop": "revisions",
        "rvprop": "content",
        "rvslots": "main",
        "format": "json",
        "titles": word,
    })
    url = f"{WIKTIONARY_API}?{params}"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "NeiraBot/0.1 (educational NLP)"})
        with urllib.request.urlopen(req, timeout=10) as resp:
            data = json.loads(resp.read())
        pages = data.get("query", {}).get("pages", {})
        for page in pages.values():
            if "revisions" in page:
                slot = page["revisions"][0].get("slots", {}).get("main", {})
                return slot.get("*", "")
    except Exception as e:
        print(f"  [WARN] Wiktionary fetch error for '{word}': {e}")
    return None


def extract_words_from_section(text: str) -> list[str]:
    results = []
    for m in WORD_LINK_RE.finditer(text):
        w = (m.group(1) or m.group(2) or "").strip().lower()
        # Фильтр: только кириллица, длиннее 1 символа, без спецсимволов
        if w and len(w) > 1 and re.match(r"^[а-яёa-z\-]+$", w):
            results.append(w)
    return results


def enrich_from_wiktionary(conn: sqlite3.Connection, lemma: str, delay: float):
    raw = fetch_wiktionary_raw(lemma)
    time.sleep(delay)

    if not raw:
        return

    # Выделить русский раздел
    ru_match = RU_SECTION_RE.search(raw)
    ru_text = ru_match.group(1) if ru_match else raw

    word_id = get_or_create_word(conn, lemma)

    for rel_type, pattern in SECTION_PATTERNS.items():
        m = pattern.search(ru_text)
        if not m:
            continue
        words = extract_words_from_section(m.group(1))
        for w in words:
            if w != lemma:
                add_relation(conn, word_id, w, rel_type, weight=0.9, source="wiktionary")

    conn.commit()


# ---------------------------------------------------------------------------
# RuWordNet — парсинг XML offline
# ---------------------------------------------------------------------------
RUWORDNET_REL_MAP = {
    "hypernym":   "hypernym",
    "hyponym":    "hyponym",
    "domain":     "hypernym",
    "meronym":    "meronym",
    "holonym":    "hypernym",
    "synonym":    "synonym",
    "antonym":    "antonym",
}


def enrich_from_ruwordnet(conn: sqlite3.Connection, xml_path: str):
    print(f"Загружаю RuWordNet из {xml_path}...")
    try:
        tree = ET.parse(xml_path)
        root = tree.getroot()
    except Exception as e:
        print(f"  [ERROR] Не удалось разобрать RuWordNet XML: {e}")
        return

    count = 0
    # Структура RuWordNet: <synsets><synset ...><sense lemma="..."/></synset></synsets>
    # <relations><relation parent_id="..." child_id="..." name="..."/></relations>
    synset_lemmas: dict[str, list[str]] = {}

    for synset in root.findall(".//synset"):
        sid = synset.get("id", "")
        lemmas = []
        for sense in synset.findall("sense"):
            lemma = (sense.get("lemma") or sense.get("name") or "").strip().lower()
            if lemma:
                lemmas.append(lemma)
        if sid and lemmas:
            synset_lemmas[sid] = lemmas
            # Синонимы внутри synset
            for i, l1 in enumerate(lemmas):
                wid = get_or_create_word(conn, l1)
                for j, l2 in enumerate(lemmas):
                    if i != j:
                        add_relation(conn, wid, l2, "synonym", weight=1.0, source="ruwordnet")
                        count += 1

    for rel in root.findall(".//relation"):
        parent_id = rel.get("parent_id") or rel.get("from")
        child_id  = rel.get("child_id")  or rel.get("to")
        rel_name  = (rel.get("name") or rel.get("type") or "").lower()
        mapped    = RUWORDNET_REL_MAP.get(rel_name, "")
        if not mapped or parent_id not in synset_lemmas or child_id not in synset_lemmas:
            continue
        for parent_lemma in synset_lemmas[parent_id]:
            wid = get_or_create_word(conn, parent_lemma)
            for child_lemma in synset_lemmas[child_id]:
                add_relation(conn, wid, child_lemma, mapped, weight=1.0, source="ruwordnet")
                count += 1

    conn.commit()
    print(f"  RuWordNet: добавлено {count} связей.")


# ---------------------------------------------------------------------------
# Статистика
# ---------------------------------------------------------------------------
def print_stats(conn: sqlite3.Connection):
    n_words = conn.execute("SELECT COUNT(*) FROM words").fetchone()[0]
    n_rels  = conn.execute("SELECT COUNT(*) FROM relations").fetchone()[0]
    print(f"\n=== Итого: {n_words} слов, {n_rels} связей ===")

    by_type = conn.execute(
        "SELECT relation_type, COUNT(*) FROM relations GROUP BY relation_type ORDER BY COUNT(*) DESC"
    ).fetchall()
    for rel_type, cnt in by_type:
        print(f"  {rel_type:<15} {cnt}")

    by_source = conn.execute(
        "SELECT source, COUNT(*) FROM relations GROUP BY source"
    ).fetchall()
    print()
    for src, cnt in by_source:
        print(f"  source={src:<15} {cnt} связей")


# ---------------------------------------------------------------------------
# Загрузка списка слов
# ---------------------------------------------------------------------------
def load_word_list(path: str) -> list[str]:
    with open(path, encoding="utf-8") as f:
        return [line.strip().lower() for line in f if line.strip()]


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="Построение семантического графа для Neira"
    )
    parser.add_argument("--output",      default="semantics.db",
                        help="Путь к SQLite файлу (по умолч. semantics.db)")
    parser.add_argument("--words",       default=None,
                        help="Файл со списком лемм (по одной на строку)")
    parser.add_argument("--wiktionary",  action="store_true",
                        help="Обогатить через Wiktionary API")
    parser.add_argument("--ruwordnet",   default=None,
                        help="Путь к XML файлу RuWordNet")
    parser.add_argument("--delay",       type=float, default=1.0,
                        help="Задержка между запросами API (сек)")
    parser.add_argument("--limit",       type=int, default=0,
                        help="Ограничить кол-во слов (0 = без ограничения)")
    args = parser.parse_args()

    # Загрузить список слов
    if args.words:
        words = load_word_list(args.words)
        print(f"Загружено {len(words)} слов из {args.words}")
    else:
        words = SEED_WORDS
        print(f"Используются {len(words)} слов-семян встроенного словаря")

    if args.limit > 0:
        words = words[:args.limit]
        print(f"Ограничение: первые {args.limit} слов")

    # Инициализировать БД
    conn = init_db(args.output)
    print(f"База данных: {args.output}")

    # RuWordNet (offline, сначала — быстрее и не требует сети)
    if args.ruwordnet:
        enrich_from_ruwordnet(conn, args.ruwordnet)

    # Wiktionary API
    if args.wiktionary:
        print(f"\nWiktionary: обрабатываю {len(words)} слов (задержка {args.delay}с)...")
        for i, lemma in enumerate(words, 1):
            if word_already_processed(conn, lemma):
                print(f"  [{i}/{len(words)}] пропуск (уже есть): {lemma}")
                continue
            print(f"  [{i}/{len(words)}] {lemma}...", end=" ", flush=True)
            enrich_from_wiktionary(conn, lemma, args.delay)

            # Прогресс-репорт
            cur = conn.execute(
                "SELECT COUNT(*) FROM relations r JOIN words w ON r.word_id=w.id WHERE w.lemma=?",
                (lemma,)
            )
            n = cur.fetchone()[0]
            print(f"{n} связей")

    if not args.wiktionary and not args.ruwordnet:
        # Без источников — заполнить только words-таблицу из seed
        print("Нет источников обогащения. Заполняю таблицу words из seed-списка.")
        for lemma in words:
            get_or_create_word(conn, lemma)
        conn.commit()

    print_stats(conn)
    conn.close()
    print(f"\nГотово. Файл: {os.path.abspath(args.output)}")
    print("\nПример запроса синонимов:")
    print("  SELECT r.target_lemma FROM relations r")
    print("  JOIN words w ON r.word_id=w.id")
    print("  WHERE w.lemma='кот' AND r.relation_type='synonym';")


if __name__ == "__main__":
    main()
