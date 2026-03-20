# Конвертер словаря OpenCorpora в формат Neira
# Использование: python convert_opencorpora_to_neira.py

import xml.etree.ElementTree as ET
import json
import sys
import os
from collections import defaultdict

# Соответствие тегов OpenCorpora частям речи Neira
POS_MAPPING = {
    'NOUN': 'Noun',
    'VERB': 'Verb',
    'ADJF': 'Adjective',  # полное прилагательное
    'ADJS': 'Adjective',  # краткое прилагательное
    'COMP': 'Adjective',  # компаратив
    'INFN': 'Verb',       # инфинитив
    'PRON': 'Pronoun',
    'PRED': 'Adverb',     # предикатив
    'ADV': 'Adverb',
    'PREP': 'Preposition',
    'CONJ': 'Conjunction',
    'INTJ': 'Particle',
    'PRCL': 'Particle',
    'PART': 'Particle',
    'NUMR': 'Numeral',
}

def parse_opencorpora(xml_path, max_entries=None):
    """Распарсить XML OpenCorpora и вернуть словарь"""
    # Используем UNC путь для Windows с кириллицей
    if os.name == 'nt':
        xml_path = '\\\\?\\' + xml_path
    
    print(f"Parsing {xml_path}...")
    
    dictionary = defaultdict(list)
    total_words = 0
    unique_lemmas = 0
    
    # Итеративный парсинг для большого файла
    context = ET.iterparse(xml_path, events=('end',))
    
    for event, elem in context:
        if elem.tag == 'l':  # lemma
            word_form = elem.get('t', '').lower().strip()
            if not word_form:
                elem.clear()
                continue
            
            # Получить POS
            pos_tag = elem.get('ps', '')
            pos = POS_MAPPING.get(pos_tag, 'Unknown')
            
            # Получить лемму
            lemma_text = elem.get('t', '').strip()
            
            # Добавить в словарь
            dictionary[word_form].append({
                'lemma': lemma_text,
                'pos': pos,
                'source': 'dict'
            })
            
            total_words += 1
            if len(dictionary) % 100000 == 0:
                print(f"  Processed {len(dictionary)} unique words, {total_words} total forms...")
            
            if max_entries and len(dictionary) >= max_entries:
                break
            
            elem.clear()
    
    return dictionary

def save_neira_json(dictionary, output_path):
    """Сохранить словарь в формате Neira JSON"""
    print(f"Saving to {output_path}...")
    
    neira_format = []
    for word, forms in dictionary.items():
        for form in forms:
            neira_format.append({
                'word': word,
                'lemma': form['lemma'],
                'pos': form['pos'],
                'source': 'dict'
            })
    
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump({
            'meta': {
                'name': 'OpenCorpora Dictionary for Neira',
                'version': '0.6',
                'language': 'ru',
                'entries_count': len(neira_format)
            },
            'entries': neira_format
        }, f, ensure_ascii=False, indent=2)
    
    print(f"Saved {len(neira_format)} entries")

def save_neira_cpp_header(dictionary, output_path):
    """Сохранить словарь в формате C++ массива для вставки в код"""
    print(f"Saving C++ header to {output_path}...")
    
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("// OpenCorpora Dictionary for Neira\n")
        f.write("// Auto-generated from annot.opcorpora.xml\n")
        f.write(f"// Total entries: {len(dictionary)}\n\n")
        f.write("namespace {\n")
        f.write("const FDictEntry ExternalDictionary[] = {\n")
        
        count = 0
        for word, forms in dictionary.items():
            # Берём первую форму (наиболее частотную)
            form = forms[0]
            word_escaped = word.replace('\\', '\\\\').replace('"', '\\"')
            lemma_escaped = form['lemma'].replace('\\', '\\\\').replace('"', '\\"')
            
            pos = form['pos']
            f.write(f'    {{ TEXT("{word_escaped}"), TEXT("{lemma_escaped}"), EPosTag::{pos} }},\n')
            count += 1
            
            if count % 10000 == 0:
                print(f"  Written {count} entries...")

        f.write("};\n")
        f.write("} // namespace\n")

    print(f"Saved {count} entries to C++ header")

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    xml_path = os.path.join(script_dir, 'annot.opencorpora.xml')
    json_output = os.path.join(script_dir, 'opencorpora_dict.json')
    cpp_output = os.path.join(script_dir, 'opencorpora_dict.cpp')
    
    # Парсинг
    dictionary = parse_opencorpora(xml_path)
    
    # Сохранение JSON
    save_neira_json(dictionary, json_output)
    
    # Сохранение C++ (опционально, для вставки в код)
    save_neira_cpp_header(dictionary, cpp_output)
    
    # Статистика
    print("\n=== Statistics ===")
    print(f"Unique words: {len(dictionary)}")
    print(f"Total forms: {sum(len(forms) for forms in dictionary.values())}")
    
    # POS distribution
    pos_counts = defaultdict(int)
    for forms in dictionary.values():
        for form in forms:
            pos_counts[form['pos']] += 1
    
    print("\nPOS distribution:")
    for pos, count in sorted(pos_counts.items(), key=lambda x: -x[1]):
        print(f"  {pos}: {count}")

if __name__ == '__main__':
    main()
