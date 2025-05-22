# Kaey-Compiler

Esse projeto tem como objetivo a criação de um compilador de uma nova linguagem de programação, usando o LLVM como back-end.

Exemplo de input de texto ao compilador:

```
//Esta é minha linguagem de programação.
/*
  Este é um comentário
  de multiplas
  linhas.
*/
//Funções importadas da biblioteca padrão C.
//Qualquer função que tiver sido exportada do modo padrão em C ('extern "C"' em C++) pode ser importada dessa maneira.
#[ExternC] [] printf(fmt: char*, ...) : i32;
#[ExternC] [] puts(str: char*) : i32;

[] TestPrint(count: i32 = 16)
{
    for (var i = 0; i < count; ++i)
        printf(+"Hello: %i\n", i + 1);
}

[] Main(argc: i32, argv: char**) : i32
{
    TestPrint();
    val ptr = new (i32, i64, f32);
    for (var i = 0; i < argc; ++i)
        puts(argv[i]);
    delete ptr;
    return 0;
}
```

Árvore gerada pelo parser através do front-end, usando a integração com o Graphviz:

![tree](https://github.com/user-attachments/assets/3a38af9a-926e-4e1f-85c5-d67e64a5801b)

Usando a AST (Abstract Syntax Tree) o compilador traduz e avalia todas as chamadas e referências feitas no código fonte, traduzindo essa árvore para o byte code do LLVM.
A seguir o output em texto do código já optimizado que é gerado pelo LLVM:

```
; ModuleID = 'Source'
source_filename = "Source"
target datalayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"

@0 = internal constant [11 x i8] c"Hello: %i\0A\00"

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly %0, ...) local_unnamed_addr #0

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr nocapture noundef readonly %0) local_unnamed_addr #0

; Function Attrs: nofree nounwind
define void @"TestPrint(i32)"(i32 %"$count") local_unnamed_addr #0 {
Entry:
  %0 = icmp sgt i32 %"$count", 0
  br i1 %0, label %.lr.ph, label %._crit_edge

.lr.ph:                                           ; preds = %Entry, %.lr.ph
  %i.03 = phi i32 [ %1, %.lr.ph ], [ 0, %Entry ]
  %1 = add nuw nsw i32 %i.03, 1
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 %1)
  %3 = icmp slt i32 %1, %"$count"
  br i1 %3, label %.lr.ph, label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph, %Entry
  ret void
}

define noundef i32 @Main(i32 %"$argc", ptr nocapture readonly %"$argv") local_unnamed_addr {
Entry:
  %0 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 1)
  %1 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 2)
  %2 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 3)
  %3 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 4)
  %4 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 5)
  %5 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 6)
  %6 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 7)
  %7 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 8)
  %8 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 9)
  %9 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 10)
  %10 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 11)
  %11 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 12)
  %12 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 13)
  %13 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 14)
  %14 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 15)
  %15 = tail call i32 (ptr, ...) @printf(ptr nonnull dereferenceable(1) @0, i32 16)
  %16 = tail call dereferenceable_or_null(24) ptr @malloc(i64 24)
  tail call void @llvm.memset.p0.i64(ptr noundef nonnull align 4 dereferenceable(16) %16, i8 0, i64 16, i1 false)
  %17 = icmp sgt i32 %"$argc", 0
  br i1 %17, label %.lr.ph, label %._crit_edge

.lr.ph:                                           ; preds = %Entry, %.lr.ph
  %i.05 = phi i32 [ %22, %.lr.ph ], [ 0, %Entry ]
  %18 = zext nneg i32 %i.05 to i64
  %19 = getelementptr ptr, ptr %"$argv", i64 %18
  %20 = load ptr, ptr %19, align 8
  %21 = tail call i32 @puts(ptr nonnull dereferenceable(1) %20)
  %22 = add nuw nsw i32 %i.05, 1
  %23 = icmp slt i32 %22, %"$argc"
  br i1 %23, label %.lr.ph, label %._crit_edge

._crit_edge:                                      ; preds = %.lr.ph, %Entry
  %24 = tail call ptr @free(ptr %16)
  ret i32 0
}
```
Output do compilador executando o código através do JIT (Just-In-Time) fornecido pelo LLVM.
```
Compilation Success!
Executing target main.


Hello: 1
Hello: 2
Hello: 3
Hello: 4
Hello: 5
Hello: 6
Hello: 7
Hello: 8
Hello: 9
Hello: 10
Hello: 11
Hello: 12
Hello: 13
Hello: 14
Hello: 15
Hello: 16
```
