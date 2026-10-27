#!/usr/bin/env python3
# AST-проверка через libclang (то, что в UE 4.27 ловят MSVC/UHT/сборщик мусора, а clang на макете — нет):
#  1) параметр или локальная переменная с именем поля класса или его базы (MSVC C4458/C4456 — в UE 4.27 с V2 это ошибка);
#  2) указатель на UObject в поле UCLASS/USTRUCT без UPROPERTY() (сборщик мусора удалит объект — падение);
#  3) AddDynamic/BindDynamic на функцию без UFUNCTION() (привязка молча не сработает);
#  4) тело BlueprintImplementableEvent в .cpp (UHT сам создаёт тело — ошибка компоновки).
# Использование: tp_ast.py --root R --src S -- <флаги clang> -- файл.cpp ...
import argparse
import os
import sys

import clang.cindex as ci

K = ci.CursorKind


def annotations(c):
    return [ch.spelling for ch in c.get_children() if ch.kind == K.ANNOTATE_ATTR]


class Checker:
    def __init__(self, root, src):
        self.root = os.path.realpath(root)
        self.src = os.path.realpath(src)
        self.seen = set()
        self.errors = 0
        self.warnings = 0
        self.uobj_cache = {}
        self.fields_cache = {}

    def in_project(self, c):
        f = c.location.file
        return f is not None and os.path.realpath(f.name).startswith(self.src + os.sep)

    def report(self, c, msg, warn=False):
        loc = c.location
        key = (os.path.realpath(loc.file.name), loc.line, msg)
        if key in self.seen:
            return
        self.seen.add(key)
        if warn:
            self.warnings += 1
        else:
            self.errors += 1
        print('%s:%d: %s: %s' % (os.path.relpath(key[0], self.root), loc.line, 'предупреждение' if warn else 'ОШИБКА', msg))

    # ---------- иерархия ----------
    def bases(self, cls):
        for ch in cls.get_children():
            if ch.kind == K.CXX_BASE_SPECIFIER:
                d = ch.type.get_canonical().get_declaration()
                if d is not None and d.kind != K.NO_DECL_FOUND:
                    yield d, ch.access_specifier

    def is_uobject(self, cls):
        if cls is None:
            return False
        cls = cls.get_definition() or cls
        key = cls.get_usr()
        if key in self.uobj_cache:
            return self.uobj_cache[key]
        self.uobj_cache[key] = False
        r = cls.spelling == 'UObject' or any(self.is_uobject(b) for b, _ in self.bases(cls))
        self.uobj_cache[key] = r
        return r

    def is_reflected_struct(self, cls):
        return any(ch.kind == K.CXX_METHOD and ch.spelling == 'StaticStruct' for ch in cls.get_children())

    def field_names(self, cls, own=True):
        cls = cls.get_definition() or cls
        key = (cls.get_usr(), own)
        if key in self.fields_cache:
            return self.fields_cache[key]
        names = {}
        for ch in cls.get_children():
            if ch.kind in (K.FIELD_DECL, K.VAR_DECL) and ch.spelling:
                if own or ch.access_specifier != ci.AccessSpecifier.PRIVATE:
                    names[ch.spelling] = cls.spelling
        for b, acc in self.bases(cls):
            if acc == ci.AccessSpecifier.PRIVATE:
                continue
            for n, owner in self.field_names(b, own=False).items():
                names.setdefault(n, owner)
        self.fields_cache[key] = names
        return names

    # ---------- типы ----------
    def uobject_ptr_in(self, t, depth=0):
        t = t.get_canonical()
        if t.kind == ci.TypeKind.POINTER:
            d = t.get_pointee().get_canonical().get_declaration()
            return d is not None and d.kind in (K.CLASS_DECL, K.STRUCT_DECL, K.CLASS_TEMPLATE) and self.is_uobject(d)
        if depth < 3:
            decl = t.get_declaration()
            if decl is not None and decl.spelling in ('TArray', 'TSet', 'TMap', 'TMultiMap'):
                n = t.get_num_template_arguments()
                for i in range(max(0, n)):
                    a = t.get_template_argument_type(i)
                    if a.kind != ci.TypeKind.INVALID and self.uobject_ptr_in(a, depth + 1):
                        return True
        return False

    # ---------- обход ----------
    def check_function(self, fn, cls):
        # MSVC выдаёт C4458 только там, где у функции есть тело (в объявлении имена параметров ни на что не влияют)
        if not fn.is_definition():
            return
        fields = self.field_names(cls)
        if not fields:
            return
        def walk(c):
            for ch in c.get_children():
                if ch.kind in (K.PARM_DECL, K.VAR_DECL) and ch.spelling in fields and self.in_project(ch):
                    what = 'параметр' if ch.kind == K.PARM_DECL else 'локальная переменная'
                    self.report(ch, '%s «%s» в %s::%s скрывает поле %s::%s (MSVC C4458 — в UE 4.27 ошибка). Переименуйте, например «In%s»'
                                % (what, ch.spelling, cls.spelling, fn.spelling, fields[ch.spelling], ch.spelling, ch.spelling))
                if ch.kind != K.CLASS_DECL and ch.kind != K.STRUCT_DECL:
                    walk(ch)
        walk(fn)

    def check_record(self, cls):
        if not self.in_project(cls) or not cls.is_definition():
            return
        uobj = self.is_uobject(cls)
        reflected = uobj or self.is_reflected_struct(cls)
        for ch in cls.get_children():
            if ch.kind == K.FIELD_DECL and self.in_project(ch):
                ann = annotations(ch)
                has_prop = any(a.startswith('tp_uproperty') for a in ann)
                if self.uobject_ptr_in(ch.type) and not has_prop:
                    if reflected:
                        self.report(ch, 'поле «%s» (%s) хранит UObject без UPROPERTY() — сборщик мусора удалит объект. Добавьте UPROPERTY() или TWeakObjectPtr'
                                    % (ch.spelling, ch.type.spelling))
                    else:
                        self.report(ch, 'поле «%s» хранит UObject в неотражаемой структуре %s — используйте TWeakObjectPtr или держите ссылку в UPROPERTY владельца'
                                    % (ch.spelling, cls.spelling), warn=True)
            if ch.kind in (K.CXX_METHOD, K.FUNCTION_TEMPLATE):
                ann = annotations(ch)
                spec = ' '.join(a for a in ann if a.startswith('tp_ufunction'))
                if 'BlueprintImplementableEvent' in spec:
                    defn = ch.get_definition()
                    if defn is not None and defn != ch:
                        self.report(defn, 'у BlueprintImplementableEvent «%s» не должно быть тела в C++ (UHT создаёт его сам)' % ch.spelling)

    def check_dynamic_bind(self, call):
        name = call.spelling
        if name not in ('__Internal_AddDynamic', '__Internal_AddUniqueDynamic', '__Internal_RemoveDynamic', '__Internal_BindDynamic', '__Internal_IsAlreadyBound'):
            return
        for d in call.walk_preorder():
            if d.kind == K.DECL_REF_EXPR and d.referenced is not None and d.referenced.kind == K.CXX_METHOD:
                m = d.referenced
                decl = m.canonical
                if not any(a.startswith('tp_ufunction') for a in annotations(decl)) and not any(a.startswith('tp_ufunction') for a in annotations(m)):
                    self.report(call, '%s: функция %s::%s не помечена UFUNCTION() — динамический делегат её не вызовет'
                                % (name.replace('__Internal_', ''), m.semantic_parent.spelling, m.spelling))
                return

    def visit(self, tu):
        for c in tu.cursor.walk_preorder():
            k = c.kind
            if k in (K.CLASS_DECL, K.STRUCT_DECL):
                self.check_record(c)
            elif k in (K.CXX_METHOD, K.CONSTRUCTOR, K.DESTRUCTOR, K.FUNCTION_TEMPLATE) and self.in_project(c):
                cls = c.semantic_parent
                if cls is not None and cls.kind in (K.CLASS_DECL, K.STRUCT_DECL, K.CLASS_TEMPLATE):
                    self.check_function(c, cls)
            elif k == K.CALL_EXPR and self.in_project(c):
                self.check_dynamic_bind(c)


def main():
    argv = sys.argv[1:]
    if '--' not in argv:
        print(__doc__)
        return 2
    i = argv.index('--')
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    ap.add_argument('--src', required=True)
    a = ap.parse_args(argv[:i])
    rest = argv[i + 1:]
    j = rest.index('--')
    flags = [f for f in rest[:j] if f not in ('-fsyntax-only',) and not f.startswith('-Werror') and not f.startswith('-ferror-limit')]
    files = rest[j + 1:]
    try:
        import subprocess
        rd = subprocess.run(['clang++', '-print-resource-dir'], capture_output=True, text=True).stdout.strip()
        if rd:
            flags = ['-resource-dir', rd] + flags
    except OSError:
        pass
    chk = Checker(a.root, a.src)
    idx = ci.Index.create()
    broken = 0
    for f in files:
        tu = idx.parse(f, args=['-x', 'c++'] + flags, options=ci.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES * 0)
        errs = [d for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
        if errs:
            broken += 1
            if os.environ.get('TP_AST_DEBUG'):
                for d in errs[:5]:
                    print('  [libclang] %s' % d)
        chk.visit(tu)
    extra = ' (в %d файлах есть ошибки компиляции — AST неполный)' % broken if broken else ''
    print('== AST-проверка: ошибок %d, предупреждений %d%s' % (chk.errors, chk.warnings, extra))
    return 1 if chk.errors else 0


if __name__ == '__main__':
    sys.exit(main())
