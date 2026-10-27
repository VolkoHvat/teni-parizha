# Генератор загрузчика OpenGL 3.3 core для Lutèce из glcorearb.h
import re,sys
src=open(sys.argv[1]).read()
blocks=re.findall(r'#ifndef (GL_VERSION_\d_\d)\n#define \1 1\n(.*?)#endif /\* \1 \*/',src,re.S)
want=[f'GL_VERSION_{a}_{b}' for a,b in [(1,0),(1,1),(1,2),(1,3),(1,4),(1,5),(2,0),(2,1),(3,0),(3,1),(3,2),(3,3)]]
consts=[];funcs=[];types=[]
for name,body in blocks:
    if name not in want: continue
    for l in body.split('\n'):
        m=re.match(r'#define (GL_\w+)\s+(\S+)',l)
        if m and not re.match(r'GL_VERSION_\d_\d$',m.group(1)): consts.append((m.group(1),m.group(2)))
        if l.startswith('typedef') and 'PFNGL' not in l: types.append(l)
    for m in re.finditer(r'GLAPI ([^;]*?)APIENTRY (gl\w+) \(([^;]*?)\);',body,re.S):
        funcs.append((m.group(1).strip(),m.group(2),' '.join(m.group(3).split())))
h=['/* Lutèce Engine — OpenGL 3.3 core: типы, константы, указатели на функции (сгенерировано tools/gen_gl.py) */','#ifndef LU_GL_H','#define LU_GL_H','#include <stddef.h>','#include <stdint.h>',
'#ifdef _WIN32','#define LU_APIENTRY __stdcall','#else','#define LU_APIENTRY','#endif',
'typedef unsigned int GLenum;typedef unsigned char GLboolean;typedef unsigned int GLbitfield;typedef void GLvoid;typedef signed char GLbyte;typedef unsigned char GLubyte;typedef short GLshort;typedef unsigned short GLushort;typedef int GLint;typedef unsigned int GLuint;typedef int GLsizei;typedef float GLfloat;typedef float GLclampf;typedef double GLdouble;typedef double GLclampd;typedef char GLchar;typedef ptrdiff_t GLsizeiptr;typedef ptrdiff_t GLintptr;typedef unsigned short GLhalf;typedef int64_t GLint64;typedef uint64_t GLuint64;typedef struct __GLsync *GLsync;']
seen=set()
for n,v in consts:
    if n in seen: continue
    seen.add(n);h.append(f'#define {n} {v}')
for r,n,a in funcs:
    h.append(f'typedef {r} (LU_APIENTRY *LU_PFN_{n})({a});')
    h.append(f'extern LU_PFN_{n} lu_{n};')
    h.append(f'#define {n} lu_{n}')
h+=['/* загрузка: get(name) возвращает адрес функции; возвращает имя первой отсутствующей или NULL */','const char *lu_gl_load(void *(*get)(const char *));','#endif','']
open(sys.argv[2],'w').write('\n'.join(h))
c=['/* сгенерировано tools/gen_gl.py */','#include "lu_gl.h"']
for r,n,a in funcs: c.append(f'LU_PFN_{n} lu_{n};')
c.append('const char *lu_gl_load(void *(*get)(const char *)) {')
for r,n,a in funcs: c.append(f'    if (!(lu_{n} = (LU_PFN_{n})get("{n}"))) return "{n}";')
c.append('    return NULL;\n}\n')
open(sys.argv[3],'w').write('\n'.join(c))
print(len(consts),'констант,',len(funcs),'функций')
