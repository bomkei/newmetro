from calendar import c
import os
import glob
from collections import defaultdict
from colors import *

def com_line_count(folders: list, ext: list):
  total = 0

  for folder in folders:
    path = f'./{folder}/'
    files = glob.glob(f'{path}/**/*', recursive=True)
    print(f'\n{folder}:')

    tmp = [ ]

    for file in files:
      if os.path.splitext(file)[1][1:] in ext:
        count = sum([1 for _ in open(file)])
        total += count
        tmp.append([count, f'  {os.path.basename(file)}: {count}'])

    tmp.sort()
    [print(x[1]) for x in tmp[::-1]]

  print(f'\ntotal: {total}')