# Скрипт для загрузки и конвертации словаря OpenCorpora для Neira
# 
# Использование:
#   python download_opencorpora_dict.py
#
# Требования:
#   pip install requests
#
# Результат:
#   Data/Dictionaries/opencorpora_dict.json - конвертированный словарь для Neira

import requests
import xml.etree.ElementTree as ET
import json
import zipfile
import os
from io import BytesIO

# URL для скачивания словаря OpenCorpora
DICT_URLS = [
    'https://opencorpora.org/files/export/dict/dict.opencorpora.xml.zip',
    'http://opencorpora.org/files/export/dict/dict.opencorpora.xml.zip',
]

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

def download_dict():
    """Скачать словарь OpenCorpora"""
    for url in DICT_URLS:
        try:
            print(f"Trying: {url}")
            response = requests.get(url, timeout=60)
            if response.status_code == 200:
                print(f"Downloaded from: {url}")
                return response.content
        except Exception as e:
            print(f"Failed: {e}")
    return None

def extract_xml(zip_content):
    """Извлечь XML из ZIP архива"""
    with zipfile.ZipFile(BytesIO(zip_content)) as z:
        # Найти XML файл в архиве
        xml_name = [f for f in z.namelist() if f.endswith('.xml')][0]
        return z.read(xml_name)

def parse_opencorpora(xml_content):
    """Распарсить XML OpenCorpora и вернуть словарь"""
    root = ET.fromstring(xml_content)
    
    dictionary = {}
    
    # Парсинг словаря
    for lemma in root.findall('.//l'):
        word_form = lemma.get('t', '').lower()
        if not word_form:
            continue
            
        # Получить POS
        pos_tag = lemma.get('ps', '')
        pos = POS_MAPPING.get(pos_tag, 'Unknown')
        
        # Получить лемму
        lemma_text = lemma.get('t', '')
        
        if word_form not in dictionary:
            dictionary[word_form] = []
        
        dictionary[word_form].append({
            'lemma': lemma_text,
            'pos': pos,
            'source': 'dict'
        })
    
    return dictionary

def save_neira_dict(dictionary, output_path):
    """Сохранить словарь в формате Neira"""
    # Конвертировать в формат Neira
    neira_format = []
    for word, forms in dictionary.items():
        for form in forms:
            neira_format.append({
                'word': word,
                'lemma': form['lemma'],
                'pos': form['pos'],
                'source': 'dict'
            })
    
    # Сохранить JSON
    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(neira_format, f, ensure_ascii=False, indent=2)
    
    print(f"Saved {len(neira_format)} entries to {output_path}")

def main():
    output_dir = 'Data/Dictionaries'
    output_file = os.path.join(output_dir, 'opencorpora_dict.json')
    
    # Создать директорию
    os.makedirs(output_dir, exist_ok=True)
    
    # Скачать словарь
    print("Downloading OpenCorpora dictionary...")
    zip_content = download_dict()
    
    if not zip_content:
        print("Failed to download dictionary")
        return
    
    # Извлечь XML
    print("Extracting XML...")
    xml_content = extract_xml(zip_content)
    
    # Распарсить
    print("Parsing dictionary...")
    dictionary = parse_opencorpora(xml_content)
    
    # Сохранить
    print("Saving dictionary...")
    save_neira_dict(dictionary, output_file)
    
    print("Done!")

if __name__ == '__main__':
    main()
