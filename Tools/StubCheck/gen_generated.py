#!/usr/bin/env python3
# Генератор макетных *.generated.h (замена UnrealHeaderTool для проверки синтаксиса).
# Для каждого GENERATED_BODY() создаёт макрос FID_<файл>_<строка>_GENERATED_BODY с typedef Super/ThisClass,
# StaticClass() и конструктором по умолчанию (если класс не объявил свой) — как это делает UHT в UE 4.27.
import os
import re
import sys

DECL_RE = re.compile(r'\b(class|struct)\s+(?:alignas\([^)]*\)\s*)?(?:[A-Z0-9_]+_API\s+)?([A-Za-z_]\w*)\s*(?:final\s*)?(?::\s*(?:public|protected|private)?\s*([\w:]+(?:<[^>{]*>)?)(?:\s*,\s*(?:public|protected|private)?\s*[\w:]+(?:<[^>{]*>)?)*)?\s*\{', re.S)


def strip_comments(text):
    # Убирает комментарии, сохраняя переводы строк (номера строк не сдвигаются).
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append('\n' * text.count('\n', i, j))
            i = j
        elif c == '"' or c == "'":
            q = c
            j = i + 1
            while j < n and text[j] != q:
                if text[j] == '\\':
                    j += 1
                j += 1
            out.append(text[i:j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return ''.join(out)


def file_id(stem):
    return 'FID_' + re.sub(r'\W', '_', stem)


def find_body_end(text, open_pos):
    depth = 0
    for k in range(open_pos, len(text)):
        if text[k] == '{':
            depth += 1
        elif text[k] == '}':
            depth -= 1
            if depth == 0:
                return k
    return len(text)


def analyze(path):
    raw = open(path, encoding='utf-8-sig', errors='replace').read()
    text = strip_comments(raw)
    stem = os.path.splitext(os.path.basename(path))[0]
    result = {'stem': stem, 'bodies': [], 'enums': []}
    # объявления классов/структур с их диапазонами
    decls = []
    for m in DECL_RE.finditer(text):
        open_pos = m.end() - 1
        end = find_body_end(text, open_pos)
        before = text[max(0, m.start() - 400):m.start()]
        kind = None
        mm = list(re.finditer(r'\b(UCLASS|USTRUCT|UINTERFACE)\s*\(', before))
        if mm:
            # макрос должен стоять непосредственно перед объявлением (между ними — только его аргументы)
            tail = before[mm[-1].start():]
            if tail.count(';') == 0 and tail.count('{') == 0:
                kind = mm[-1].group(1)
        decls.append({'key': m.group(1), 'name': m.group(2), 'base': m.group(3), 'start': open_pos, 'end': end, 'kind': kind})
    for m in re.finditer(r'\b(GENERATED_BODY|GENERATED_UCLASS_BODY|GENERATED_USTRUCT_BODY|GENERATED_IINTERFACE_BODY|GENERATED_UINTERFACE_BODY)\s*\(', text):
        line = text.count('\n', 0, m.start()) + 1
        owner = None
        for d in decls:
            if d['start'] < m.start() < d['end'] and (owner is None or d['start'] > owner['start']):
                owner = d
        if owner is None:
            result['bodies'].append({'line': line, 'error': 'GENERATED_BODY вне класса'})
            continue
        body = text[owner['start']:owner['end']]
        has_ctor = re.search(r'(?<![~\w])' + re.escape(owner['name']) + r'\s*\((?!\s*\*)', body) is not None
        legacy = m.group(1) in ('GENERATED_UCLASS_BODY', 'GENERATED_IINTERFACE_BODY', 'GENERATED_UINTERFACE_BODY')
        result['bodies'].append({'line': line, 'name': owner['name'], 'base': owner['base'], 'kind': owner['kind'], 'key': owner['key'], 'has_ctor': has_ctor, 'legacy': legacy})
    for m in re.finditer(r'\bUENUM\s*\([^)]*\)\s*enum\s+class\s+(\w+)\s*(?::\s*(\w+))?', text):
        result['enums'].append((m.group(1), m.group(2)))
    return result


def emit(info):
    stem = info['stem']
    fid = file_id(stem)
    guard = 'TP_GENERATED_' + re.sub(r'\W', '_', stem)
    lines = ['// Автоматически создано Tools/StubCheck/gen_generated.py — макет UHT для UE 4.27', '#ifdef ' + guard,
             '#error "%s.generated.h already included, missing \'#pragma once\' in %s.h"' % (stem, stem), '#endif',
             '#define ' + guard, '#include "TPStub/UObjectCore.h"', '#undef CURRENT_FILE_ID', '#define CURRENT_FILE_ID ' + fid]
    for e, ut in info['enums']:
        if ut:
            lines.append('enum class %s : %s;' % (e, ut))
            lines.append('template<> UEnum* StaticEnum<%s>();' % e)
    for b in info['bodies']:
        macro_name = '%s_%d_GENERATED_BODY' % (fid, b['line'])
        if 'error' in b:
            lines.append('#define %s static_assert(false, "%s");' % (macro_name, b['error']))
            continue
        name, base, kind = b['name'], b['base'], b['kind']
        if kind is None and name.startswith('I') and b['key'] == 'class':
            ucls = 'U' + name[1:]
            body = 'TP_GENERATED_IINTERFACE(%s, %s)' % (name, ucls)
        elif kind in ('UCLASS', 'UINTERFACE'):
            if not base:
                body = 'static_assert(false, "UCLASS %s без базового класса");' % name
            else:
                body = 'TP_GENERATED_UCLASS(%s, %s)' % (name, base)
                if b['legacy']:
                    body += ' public: %s(const FObjectInitializer& ObjectInitializer);' % name
                elif not b['has_ctor']:
                    body += ' TP_GENERATED_UCLASS_DEFCTOR(%s)' % name
                body += ' private:'
        elif kind == 'USTRUCT':
            body = 'TP_GENERATED_USTRUCT_DERIVED(%s, %s)' % (name, base) if base else 'TP_GENERATED_USTRUCT(%s)' % name
        else:
            body = 'static_assert(false, "GENERATED_BODY в %s без UCLASS/USTRUCT");' % name
        lines.append('#define %s %s' % (macro_name, body))
        if b['legacy']:
            lines.append('#define %s_%d_GENERATED_BODY_LEGACY %s public:' % (fid, b['line'], body))
    return '\n'.join(lines) + '\n'


def main():
    if len(sys.argv) < 3:
        print('usage: gen_generated.py OUT_DIR header.h ...')
        return 2
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for h in sys.argv[2:]:
        txt = open(h, encoding='utf-8-sig', errors='replace').read()
        stem = os.path.splitext(os.path.basename(h))[0]
        if '%s.generated.h' % stem not in txt and not re.search(r'\b(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(', txt):
            continue
        info = analyze(h)
        with open(os.path.join(out, stem + '.generated.h'), 'w', encoding='utf-8') as f:
            f.write(emit(info))
    return 0


if __name__ == '__main__':
    sys.exit(main())
