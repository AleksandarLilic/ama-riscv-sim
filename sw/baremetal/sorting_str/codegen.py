import os
import sys
import random
import nltk
#import collections
from nltk.corpus import words
from nltk.data import find

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from codegen_common import *

def check_if_words_exist(path="./nltk_data"):
    if path not in nltk.data.path:
        nltk.data.path.append(path)
    try:
        find("corpora/words.zip")
        print(f"Using 'words' dataset from '{path}'")
    except LookupError:
        print("Downloading 'words' dataset...")
        nltk.download('words', download_dir=path)

def generate_words(count, allow_duplicates=True):
    # 'en' is large and unlikely to have duplicates for small counts
    # use 'en-basic' instead
    word_list = words.words('en-basic')
    if allow_duplicates:
        return random.choices(word_list, k=count)
    return random.sample(word_list, count)

LEN_MAP = {"tiny": 10, "small": 30, "medium": 100, "large": 400}
LEN_NAMES = list(LEN_MAP.keys())

if len(sys.argv) != 2:
    print("Usage: python3 codegen.py <" + "|".join(LEN_NAMES) + ">")
    sys.exit(1)

len_name = sys.argv[1]
if len_name not in LEN_NAMES:
    print(f"ARR_LEN {len_name} is not in {LEN_NAMES}")
    sys.exit(1)

arr_len = LEN_MAP[len_name]
OUT = f"test_arrays_{len_name}.h"

code = []
code.append(f"#define ARR_LEN {arr_len}\n")

check_if_words_exist()

random.seed(1)
a = generate_words(arr_len)
ref = sorted(a)
code.append(np2c_1d_arr('a', a, "char*", "", str_type=True))
code.append(np2c_1d_arr('ref', ref, "char*", "", str_type=True))
finish_gen(code, OUT, add_assert=False)

#duplicates = [item for item, count
#              in collections.Counter(a).items()
#              if count > 1]
#print(duplicates)
