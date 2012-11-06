#ifndef _SRC_HIR_INSTRUCTIONS_H_
#define _SRC_HIR_INSTRUCTIONS_H_

#include "ast.h" // AstNode
#include "scope.h" // ScopeSlot
#include "zone.h" // Zone, ZoneList
#include "utils.h" // PrintBuffer

namespace candor {
namespace internal {

// Forward declarations
class HIRGen;
class HIRBlock;
class HIRInstruction;
class HIRPhi;
class LInstruction;

typedef ZoneList<HIRInstruction*> HIRInstructionList;
typedef ZoneList<HIRPhi*> HIRPhiList;

#define HIR_INSTRUCTION_TYPES(V) \
    V(Nop) \
    V(Nil) \
    V(Entry) \
    V(Return) \
    V(Function) \
    V(LoadArg) \
    V(LoadVarArg) \
    V(StoreArg) \
    V(StoreVarArg) \
    V(AlignStack) \
    V(LoadContext) \
    V(StoreContext) \
    V(LoadProperty) \
    V(StoreProperty) \
    V(DeleteProperty) \
    V(If) \
    V(Literal) \
    V(Goto) \
    V(Not) \
    V(BinOp) \
    V(Typeof) \
    V(Sizeof) \
    V(Keysof) \
    V(Clone) \
    V(Call) \
    V(CollectGarbage) \
    V(GetStackTrace) \
    V(AllocateObject) \
    V(AllocateArray) \
    V(Phi)

#define HIR_INSTRUCTION_ENUM(I) \
    k##I,

class HIRInstruction : public ZoneObject {
 public:
  enum Type {
    HIR_INSTRUCTION_TYPES(HIR_INSTRUCTION_ENUM)
    kNone
  };

  enum Representation {
    kUnknownRepresentation    = 0x00,  // 0000000000
    kNilRepresentation        = 0x01,  // 0000000001
    kNumberRepresentation     = 0x02,  // 0000000010
    kSmiRepresentation        = 0x06,  // 0000000110
    kHeapNumberRepresentation = 0x0A,  // 0000001010
    kStringRepresentation     = 0x10,  // 0000010000
    kBooleanRepresentation    = 0x20,  // 0000100000
    kNumMapRepresentation     = 0x40,  // 0001000000
    kObjectRepresentation     = 0xC0,  // 0011000000
    kArrayRepresentation      = 0x140, // 0101000000
    kFunctionRepresentation   = 0x200, // 1000000000
    kAnyRepresentation        = 0x2FF, // 1111111111

    // no value, not for real use
    kHoleRepresentation       = 0x300 // 10000000000
  };

  HIRInstruction(Type type);
  HIRInstruction(Type type, ScopeSlot* slot);

  virtual void Init(HIRGen* g, HIRBlock* block);

  int id;
  int gcm_visited;

  virtual void ReplaceArg(HIRInstruction* o, HIRInstruction* n);
  bool IsPinned();
  HIRInstruction* Unpin();
  HIRInstruction* Pin();
  virtual void CalculateRepresentation();
  void Remove();
  void RemoveUse(HIRInstruction* i);

  inline HIRInstruction* AddArg(Type type);
  inline HIRInstruction* AddArg(HIRInstruction* instr);
  inline bool Is(Type type);
  inline Type type();
  inline bool IsRemoved();
  virtual void Print(PrintBuffer* p);
  inline const char* TypeToStr(Type type);

  inline Representation representation();
  inline bool IsNumber();
  inline bool IsSmi();
  inline bool IsHeapNumber();
  inline bool IsString();
  inline bool IsBoolean();

  inline HIRBlock* block();
  inline void block(HIRBlock* block);
  inline ScopeSlot* slot();
  inline void slot(ScopeSlot* slot);
  inline AstNode* ast();
  inline void ast(AstNode* ast);
  inline HIRInstructionList* args();
  inline HIRInstructionList* uses();

  inline HIRInstruction* left();
  inline HIRInstruction* right();
  inline HIRInstruction* third();

  inline LInstruction* lir();
  inline void lir(LInstruction* lir);

 protected:
  Type type_;
  ScopeSlot* slot_;
  AstNode* ast_;
  LInstruction* lir_;
  HIRBlock* block_;

  bool removed_;
  bool pinned_;

  // Cached representation
  Representation representation_;

  HIRInstructionList args_;
  HIRInstructionList uses_;
};

#undef HIR_INSTRUCTION_ENUM

#define HIR_DEFAULT_METHODS(V) \
  static inline HIR##V* Cast(HIRInstruction* instr) { \
    assert(instr->type() == k##V); \
    return reinterpret_cast<HIR##V*>(instr); \
  }

class HIRPhi : public HIRInstruction {
 public:
  HIRPhi(ScopeSlot* slot);

  void Init(HIRGen* g, HIRBlock* b);

  void ReplaceArg(HIRInstruction* o, HIRInstruction* n);
  void CalculateRepresentation();

  inline void AddInput(HIRInstruction* instr);
  inline HIRInstruction* InputAt(int i);
  inline void Nilify();

  inline int input_count();
  inline void input_count(int input_count);

  HIR_DEFAULT_METHODS(Phi)

 private:
  int input_count_;
  HIRInstruction* inputs_[2];
};

class HIRLiteral : public HIRInstruction {
 public:
  HIRLiteral(AstNode::Type type, ScopeSlot* slot);

  inline ScopeSlot* root_slot();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(Literal)

 private:
  AstNode::Type type_;
  ScopeSlot* root_slot_;
};

class HIRFunction : public HIRInstruction {
 public:
  HIRFunction(AstNode* ast);

  HIRBlock* body;
  int arg_count;

  void CalculateRepresentation();
  void Print(PrintBuffer* p);

  HIR_DEFAULT_METHODS(Function)
};

class HIRNil : public HIRInstruction {
 public:
  HIRNil();

  HIR_DEFAULT_METHODS(Nil)
};

class HIREntry : public HIRInstruction {
 public:
  HIREntry(int context_slots);

  void Print(PrintBuffer* p);
  inline int context_slots();

  HIR_DEFAULT_METHODS(Entry)

 private:
  int context_slots_;
};

class HIRReturn : public HIRInstruction {
  public:
  HIRReturn();

  HIR_DEFAULT_METHODS(Return)

 private:
};

class HIRIf : public HIRInstruction {
  public:
  HIRIf();

  HIR_DEFAULT_METHODS(If)

 private:
};

class HIRGoto : public HIRInstruction {
  public:
  HIRGoto();

  HIR_DEFAULT_METHODS(Goto)

 private:
};

class HIRCollectGarbage : public HIRInstruction {
  public:
  HIRCollectGarbage();

  HIR_DEFAULT_METHODS(CollectGarbage)

 private:
};

class HIRGetStackTrace : public HIRInstruction {
  public:
  HIRGetStackTrace();

  HIR_DEFAULT_METHODS(GetStackTrace)

 private:
};

class HIRBinOp : public HIRInstruction {
  public:
  HIRBinOp(BinOp::BinOpType type);

  void CalculateRepresentation();
  inline BinOp::BinOpType binop_type();

  HIR_DEFAULT_METHODS(BinOp)

 private:
  BinOp::BinOpType binop_type_;
};

class HIRLoadContext : public HIRInstruction {
 public:
  HIRLoadContext(ScopeSlot* slot);

  inline ScopeSlot* context_slot();

  HIR_DEFAULT_METHODS(LoadContext)

 private:
  ScopeSlot* context_slot_;
};

class HIRStoreContext : public HIRInstruction {
 public:
  HIRStoreContext(ScopeSlot* slot);

  void CalculateRepresentation();
  inline ScopeSlot* context_slot();

  HIR_DEFAULT_METHODS(StoreContext)

 private:
  ScopeSlot* context_slot_;
};

class HIRLoadProperty : public HIRInstruction {
 public:
  HIRLoadProperty();

  HIR_DEFAULT_METHODS(LoadProperty)

 private:
};

class HIRStoreProperty : public HIRInstruction {
 public:
  HIRStoreProperty();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(StoreProperty)

 private:
};

class HIRDeleteProperty : public HIRInstruction {
 public:
  HIRDeleteProperty();

  HIR_DEFAULT_METHODS(DeleteProperty)

 private:
};

class HIRAllocateObject : public HIRInstruction {
 public:
  HIRAllocateObject();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(AllocateObject)

 private:
};

class HIRAllocateArray : public HIRInstruction {
 public:
  HIRAllocateArray();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(AllocateArray)

 private:
};

class HIRLoadArg : public HIRInstruction {
 public:
  HIRLoadArg();

  HIR_DEFAULT_METHODS(LoadArg)

 private:
};

class HIRLoadVarArg : public HIRInstruction {
 public:
  HIRLoadVarArg();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(LoadVarArg)

 private:
};

class HIRStoreArg : public HIRInstruction {
 public:
  HIRStoreArg();

  HIR_DEFAULT_METHODS(StoreArg)

 private:
};

class HIRStoreVarArg : public HIRInstruction {
 public:
  HIRStoreVarArg();

  HIR_DEFAULT_METHODS(StoreVarArg)

 private:
};

class HIRAlignStack : public HIRInstruction {
 public:
  HIRAlignStack();

  HIR_DEFAULT_METHODS(AlignStack)

 private:
};

class HIRCall : public HIRInstruction {
 public:
  HIRCall();

  HIR_DEFAULT_METHODS(Call)

 private:
};

class HIRKeysof : public HIRInstruction {
 public:
  HIRKeysof();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(Keysof)

 private:
};

class HIRSizeof : public HIRInstruction {
 public:
  HIRSizeof();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(Sizeof)

 private:
};

class HIRTypeof : public HIRInstruction {
 public:
  HIRTypeof();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(Typeof)

 private:
};

class HIRClone : public HIRInstruction {
 public:
  HIRClone();

  void CalculateRepresentation();

  HIR_DEFAULT_METHODS(Clone)

 private:
};

#undef HIR_DEFAULT_METHODS

} // namespace internal
} // namespace candor

#endif // _SRC_HIR_INSTRUCTIONS_H_
