//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/exception.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/assembler/Assembler.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/platform/os.hpp"
#include "vm/runtime/ResourceMark.hpp"

void trace(VirtualFrame *from_frame, std::int32_t start_frame, std::int32_t number_of_frames) {
	FlagSetting fs(ActivationShowCode, true);

	SPDLOG_INFO("- Stack trace ({}, {})", start_frame, number_of_frames);
	std::int32_t vframe_no = 1;

	for (VirtualFrame *f = from_frame; f; f = f->sender()) {
		if (vframe_no >= start_frame) {
			if (f->is_delta_frame()) {
				((DeltaVirtualFrame *) f)->print_activation(vframe_no);
			}
			else
				f->print();
			if (vframe_no - start_frame + 1 >= number_of_frames)
				return;
		}
		vframe_no++;
	}
}

void traceCompiledFrame(Frame &f) {
	ResourceMark mark;

	// Find the NativeMethod containing the pc
	CompiledVirtualFrame *vf = (CompiledVirtualFrame *) VirtualFrame::new_vframe(&f);
	st_assert(vf->is_compiled_frame(), "must be compiled frame");
	NativeMethod *nm = vf->code();
	//SPDLOG_INFO( "Found NativeMethod: 0x{0:x}", static_cast<const void *>(nm) );
	nm->print_value_on(_console);

	// SPDLOG_INFO( " @{} called from [0x{0:x}]", vf->scope()->offset(), f.pc() - static_cast<std::int32_t>( Assembler::Constants::sizeOfCall ) );

	trace(vf, 0, 10);
}

void traceInterpretedFrame(Frame &f) {
	ResourceMark mark;
	VirtualFrame *vf = VirtualFrame::new_vframe(&f);

	trace(vf, 0, 10);
}

void traceDeltaFrame(Frame &f) {
	if (f.is_compiled_frame()) {
		traceCompiledFrame(f);
	}
	else if (f.is_interpreted_frame()) {
		traceInterpretedFrame(f);
	}
}

void handle_exception(void *fp, void *sp, void *pc) {

	Frame f((Oop *) sp, (std::int32_t *) fp, (const char *) pc);
	//  SPDLOG_INFO( "ebp: 0x{0:x}, esp: 0x{0:x}, pc: 0x{0:x}", fp, sp, pc );
	if (f.is_delta_frame()) {
		traceDeltaFrame(f);
		return;
	}

	if (DeltaProcess::active() and last_delta_fp) {
		Frame lastf((Oop *) last_delta_sp, (std::int32_t *) last_delta_fp);
		if (lastf.is_delta_frame()) {
			traceDeltaFrame(lastf);
			return;
		}
		Frame next = lastf.sender();
		if (next.is_delta_frame()) {
			traceDeltaFrame(next);
			return;
		}
	}

	if (DeltaProcess::active() and DeltaProcess::active()->last_delta_fp()) {
		Frame activef((Oop *) DeltaProcess::active()->last_delta_sp(), (std::int32_t *) DeltaProcess::active()->last_delta_fp());
		if (activef.is_delta_frame()) {
			traceDeltaFrame(activef);
			return;
		}
	}

	SPDLOG_INFO("Could not trace delta.");
}

//  Set up a new exception handler to catch unhandled exceptions BEFORE the debugger.
//  This is to allow the dumping of ST-level info about the exception location to aid
//  in debugging of the exception.
void except_init() {
	os::add_exception_handler(handle_exception);
}
