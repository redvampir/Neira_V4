# Конвертер словаря OpenCorpora в формат Neira
# Использование: python convert_opencorpora_to_neira.py

import xml.etree.ElementTree as ET
import json
import os
from collections import defaultdict

# Соответствие тегов OpenCorpora частям речи Neira
POS_MAPPING = {
    'NOUN': 'Noun',
    'VERB': 'Verb',
    'ADJF': 'Adjective',
    'ADJS': 'Adjective',
    'COMP': 'Adjective',
    'INFN': 'Verb',
    'PRON': 'Pronoun',
    'PRED': 'Adverb',
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
    print(f"Parsing {xml_path}...")
    
    dictionary = defaultdict(list)
    total_words = 0
    
    context = ET.iterparse(xml_path, events=('end',))
    
    for event, elem in context:
        if elem.tag == 'l':
            word_form = elem.get('t', '').lower().strip()
            if not word_form:
                elem.clear()
                continue
            
            pos_tag = elem.get('ps', '')
            pos = POS_MAPPING.get(pos_tag, 'Unknown')
            lemma_text = elem.get('t', '').strip()
            
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

def main():
    # Используем путь относительно скрипта
    script_dir = os.path.dirname(os.path.abspath(__file__))
    xml_path = os.path.join(script_dir, 'annot.opencorpora.xml')
    json_output = os.path.join(script_dir, 'opencorpora_dict.json')
    
    print("=" * 60)
    print("OpenCorpora to Neira Dictionary Converter")
    print("=" * 60)
    
    dictionary = parse_opencorpora(xml_path)
    save_neira_json(dictionary, json_output)
    
    print("\n=== Statistics ===")
    print(f"Unique words: {len(dictionary)}")
    print(f"Total forms: {sum(len(forms) for forms in dictionary.values())}")
    
    pos_counts = defaultdict(int)
    for forms in dictionary.values():
        for form in forms:
            pos_counts[form['pos']] += 1
    
    print("\nPOS distribution:")
    for pos, count in sorted(pos_counts.items(), key=lambda x: -x[1]):
        print(f"  {pos}: {count:,}")
    
    print("\nDone!")

if __name__ == '__main__':
    main()
