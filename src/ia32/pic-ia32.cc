#include "pic.h"
#include "code-space.h" // CodeSpace
#include "stubs.h" // Stubs
#include "macroassembler.h" // Masm

namespace candor {
namespace internal {

#define __ masm->

void PIC::Generate(Masm* masm) {
  __ push(ebp);
  __ mov(ebp, esp);

  // Allocate spills
  __ AllocateSpills();

  Label miss, end;
  Label cases[kMaxSize];
  Operand edx_op(edx, 0);
  Operand proto_op(eax, HObject::kProtoOffset);
  Masm::Spill eax_s(masm, eax), ebx_s(masm, ebx);

  // Fast-case non-object
  __ IsNil(eax, NULL, &miss);
  __ IsUnboxed(eax, NULL, &miss);
  __ IsHeapObject(Heap::kTagObject, eax, &miss, NULL);

  // Load proto
  __ mov(eax, proto_op);
  __ cmpl(eax, Immediate(Heap::kICDisabledValue));
  __ jmp(kEq, &miss);

  // Jump into correct section
  __ jmp(&miss);
  jmp_ = reinterpret_cast<uint32_t*>(static_cast<intptr_t>(
        masm->offset() - 4));

  for (int i = kMaxSize - 1; i >= 0; i--) {
    Label local_miss;

    section_size_ = masm->offset();
    __ bind(&cases[i]);
    __ mov(ebx, Immediate(0));
    protos_[i] = reinterpret_cast<char**>(static_cast<intptr_t>(
          masm->offset() - 4));
    __ cmpl(eax, ebx);
    __ jmp(kNe, &local_miss);
    __ mov(eax, Immediate(0));
    results_[i] = reinterpret_cast<intptr_t*>(static_cast<intptr_t>(
          masm->offset() - 4));
    __ xorl(ebx, ebx);
    __ mov(esp, ebp);
    __ pop(ebp);
    __ ret(0);
    __ bind(&local_miss);
    section_size_ = masm->offset() - section_size_;
  }

  // Cache failed - call runtime
  __ bind(&miss);

  ebx_s.Unspill();
  ebx_s.SpillReg(eax);
  eax_s.Unspill();
  __ Call(space_->stubs()->GetLookupPropertyStub());

  ebx_s.Unspill(ebx);
  __ cmpl(ebx, Immediate(Heap::kICDisabledValue));
  __ jmp(kEq, &end);

  // Amend PIC
  __ Pushad();

  // Miss(this, object, result, ip)
  __ mov(edi, Immediate(reinterpret_cast<intptr_t>(this)));
  eax_s.Unspill(esi);
  __ mov(edx, eax);

  Operand caller_ip(ebp, 4);
  __ mov(ecx, caller_ip);

  MissCallback miss_cb = &Miss;
  __ push(ecx);
  __ push(edx);
  __ push(esi);
  __ push(edi);
  __ mov(scratch, Immediate(*reinterpret_cast<intptr_t*>(&miss_cb)));
  __ Call(scratch);
  __ addl(esp, Immediate(4 * 4));

  __ Popad(reg_nil);

  // Return value
  __ bind(&end);
  __ FinalizeSpills();
  __ xorl(ebx, ebx);
  __ mov(esp, ebp);
  __ pop(ebp);
  __ ret(0);
}

} // namespace internal
} // namespace candor
