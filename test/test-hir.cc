#include "test.h"
#include <parser.h>
#include <ast.h>
#include <hir.h>
#include <hir-inl.h>

TEST_START(hir)
  // Simple assignments
  HIR_TEST("a = 1\nb = 1\nreturn a",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[1]\n"
           "i4 = Literal[1]\n"
           "i6 = Return(i2)\n")
  HIR_TEST("return { a: 1 }",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = AllocateObject\n"
           "i4 = Literal[1]\n"
           "i6 = Literal[a]\n"
           "i8 = StoreProperty(i2, i6, i4)\n"
           "i10 = Return(i2)\n")
  HIR_TEST("return ['a']",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = AllocateArray\n"
           "i4 = Literal\n"
           "i6 = Literal[a]\n"
           "i8 = StoreProperty(i2, i4, i6)\n"
           "i10 = Return(i2)\n")
  HIR_TEST("a = {}\na.b = 1\ndelete a.b\nreturn a.b",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = AllocateObject\n"
           "i4 = Literal[1]\n"
           "i6 = Literal[b]\n"
           "i8 = StoreProperty(i2, i6, i4)\n"
           "i10 = Literal[b]\n"
           "i12 = DeleteProperty(i2, i10)\n"
           "i14 = Literal[b]\n"
           "i16 = LoadProperty(i2, i14)\n"
           "i18 = Return(i16)\n")
  HIR_TEST("a = global\nreturn a:b(1,2,3)",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = LoadContext\n"
           "i4 = Literal[1]\n"
           "i6 = Literal[2]\n"
           "i8 = Literal[3]\n"
           "i10 = Literal[b]\n"
           "i12 = LoadProperty(i2, i10)\n"
           "i14 = Call(i12, i2, i4, i6, i8)\n"
           "i16 = Return(i14)\n")

  // Unary operations
  HIR_TEST("i = 0\nreturn !i",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[0]\n"
           "i4 = Not(i2)\n"
           "i6 = Return(i4)\n")
  HIR_TEST("i = 1\nreturn +i",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[1]\n"
           "i4 = Literal[0]\n"
           "i6 = BinOp[](i4, i2)\n"
           "i8 = Return(i6)\n")
  HIR_TEST("i = 0\nreturn ++i",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[0]\n"
           "i4 = Literal[1]\n"
           "i6 = BinOp[](i4, i2)\n"
           "i8 = Return(i6)\n")
  HIR_TEST("i = 0\nreturn i++",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[0]\n"
           "i4 = Literal[1]\n"
           "i6 = BinOp[](i4, i2)\n"
           "i8 = Return(i2)\n")

  // Logical operations
  HIR_TEST("i = 0\nreturn i && 1",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[0]\n"
           "i4 = Goto\n"
           "# succ: 1\n"
           "--------\n"
           "# Block 1\n"
           "i6 = If(i2)\n"
           "# succ: 2 3\n"
           "--------\n"
           "# Block 2\n"
           "i8 = Literal[1]\n"
           "i10 = Goto\n"
           "# succ: 4\n"
           "--------\n"
           "# Block 3\n"
           "i12 = Goto\n"
           "# succ: 4\n"
           "--------\n"
           "# Block 4\n"
           "i14 = Phi(i8, i2)\n"
           "i16 = Return(i14)\n")
  HIR_TEST("i = 0\nreturn i || 1",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[0]\n"
           "i4 = Goto\n"
           "# succ: 1\n"
           "--------\n"
           "# Block 1\n"
           "i6 = If(i2)\n"
           "# succ: 2 3\n"
           "--------\n"
           "# Block 2\n"
           "i10 = Goto\n"
           "# succ: 4\n"
           "--------\n"
           "# Block 3\n"
           "i8 = Literal[1]\n"
           "i12 = Goto\n"
           "# succ: 4\n"
           "--------\n"
           "# Block 4\n"
           "i14 = Phi(i2, i8)\n"
           "i16 = Return(i14)\n")

  // Multiple blocks and phi
  HIR_TEST("if (a) { a = 2 }\nreturn a",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Nil\n"
           "i4 = If(i2)\n"
           "# succ: 1 2\n"
           "--------\n"
           "# Block 1\n"
           "i6 = Literal[2]\n"
           "i8 = Goto\n"
           "# succ: 3\n"
           "--------\n"
           "# Block 2\n"
           "i10 = Goto\n"
           "# succ: 3\n"
           "--------\n"
           "# Block 3\n"
           "i12 = Phi(i6, i2)\n"
           "i14 = Return(i12)\n")

  HIR_TEST("if (a) { a = 2 } else { a = 3 }\nreturn a",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Nil\n"
           "i4 = If(i2)\n"
           "# succ: 1 2\n"
           "--------\n"
           "# Block 1\n"
           "i6 = Literal[2]\n"
           "i10 = Goto\n"
           "# succ: 3\n"
           "--------\n"
           "# Block 2\n"
           "i8 = Literal[3]\n"
           "i12 = Goto\n"
           "# succ: 3\n"
           "--------\n"
           "# Block 3\n"
           "i14 = Phi(i6, i8)\n"
           "i16 = Return(i14)\n")

  HIR_TEST("a = 1\nif (a) {\n" 
           "  a = 2\n"
           "} else {\n"
           "  if (a) {\n"
           "    if (a) {\n"
           "      a = 3\n"
           "    }\n"
           "  } else {\n"
           "    a = 4\n"
           "  }\n"
           "}\n"
           "return a",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[1]\n"
           "i4 = If(i2)\n"
           "# succ: 1 2\n"
           "--------\n"
           "# Block 1\n"
           "i6 = Literal[2]\n"
           "i32 = Goto\n"
           "# succ: 9\n"
           "--------\n"
           "# Block 2\n"
           "i10 = If(i2)\n"
           "# succ: 3 4\n"
           "--------\n"
           "# Block 3\n"
           "i14 = If(i2)\n"
           "# succ: 5 6\n"
           "--------\n"
           "# Block 4\n"
           "i24 = Literal[4]\n"
           "i28 = Goto\n"
           "# succ: 8\n"
           "--------\n"
           "# Block 5\n"
           "i16 = Literal[3]\n"
           "i18 = Goto\n"
           "# succ: 7\n"
           "--------\n"
           "# Block 6\n"
           "i20 = Goto\n"
           "# succ: 7\n"
           "--------\n"
           "# Block 7\n"
           "i22 = Phi(i16, i2)\n"
           "i26 = Goto\n"
           "# succ: 8\n"
           "--------\n"
           "# Block 8\n"
           "i30 = Phi(i22, i24)\n"
           "i34 = Goto\n"
           "# succ: 9\n"
           "--------\n"
           "# Block 9\n"
           "i36 = Phi(i6, i30)\n"
           "i38 = Return(i36)\n")

  // While loop
  HIR_TEST("a = 0\nwhile (true) { b = a\na = 2 }\nreturn a",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[0]\n"
           "i4 = Goto\n"
           "# succ: 1\n"
           "--------\n"
           "# Block 1 (loop)\n"
           "i6 = Phi(i2, i18)\n"
           "i10 = Goto\n"
           "# succ: 2\n"
           "--------\n"
           "# Block 2\n"
           "i12 = Literal[true]\n"
           "i14 = While(i12)\n"
           "# succ: 3 5\n"
           "--------\n"
           "# Block 3\n"
           "i18 = Literal[2]\n"
           "i20 = Goto\n"
           "# succ: 4\n"
           "--------\n"
           "# Block 4\n"
           "i22 = Goto\n"
           "# succ: 1\n"
           "--------\n"
           "# Block 5\n"
           "i24 = Goto\n"
           "# succ: 6\n"
           "--------\n"
           "# Block 6\n"
           "i28 = Return(i6)\n")

  // Break, continue
  HIR_TEST("a = 1\n"
           "while(nil) {\n"
           "  a = 2\n"
           "  if (true) { continue }\n"
           "  a = 3\n"
           "}\n"
           "return a",
           "# Block 0\n"
           "i0 = Entry\n"
           "i2 = Literal[1]\n"
           "i4 = Goto\n"
           "# succ: 1\n"
           "--------\n"
           "# Block 1 (loop)\n"
           "i6 = Phi(i2, i30)\n"
           "i8 = Goto\n"
           "# succ: 2\n"
           "--------\n"
           "# Block 2\n"
           "i10 = Nil\n"
           "i12 = While(i10)\n"
           "# succ: 3 5\n"
           "--------\n"
           "# Block 3\n"
           "i14 = Literal[2]\n"
           "i16 = Literal[true]\n"
           "i18 = If(i16)\n"
           "# succ: 6 7\n"
           "--------\n"
           "# Block 4\n"
           "i34 = Goto\n"
           "# succ: 1\n"
           "--------\n"
           "# Block 5\n"
           "i36 = Goto\n"
           "# succ: 10\n"
           "--------\n"
           "# Block 6\n"
           "i20 = Goto\n"
           "# succ: 8\n"
           "--------\n"
           "# Block 7\n"
           "i24 = Goto\n"
           "# succ: 9\n"
           "--------\n"
           "# Block 8\n"
           "i30 = Phi(i14, i26)\n"
           "i32 = Goto\n"
           "# succ: 4\n"
           "--------\n"
           "# Block 9\n"
           "i26 = Literal[3]\n"
           "i28 = Goto\n"
           "# succ: 8\n"
           "--------\n"
           "# Block 10\n"
           "i40 = Return(i6)\n")
TEST_END(hir)
