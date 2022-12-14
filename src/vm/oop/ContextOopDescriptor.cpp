
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/VirtualFrame.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/utility/EventLog.hpp"
#include "vm/klass/BlockClosureKlass.hpp"
#include "vm/primitive/block_primitives.hpp"
#include "vm/code/JumpTable.hpp"
#include "vm/oop/BlockClosureOopDescriptor.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/oop/ContextOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

// Computes the byte offset from the beginning of an Oop
static constexpr std::int32_t byteOffset(std::int32_t offset) {
	st_assert(offset >= 0, "bad offset");
	return offset * sizeof(Oop) - MEMOOP_TAG;
}

bool ContextOopDescriptor::is_dead() const {
	st_assert(not mark()->has_context_forward(), "checking if context is deoptimized");
	return parent() == Oop(smiOop_zero) or parent() == nilObject;
}

bool ContextOopDescriptor::has_parent_fp() const {
	st_assert(not mark()->has_context_forward(), "checking if context is deoptimized");
	return parent()->isSmallIntegerOop() and not is_dead();
}

bool ContextOopDescriptor::has_outer_context() const {
	st_assert(not mark()->has_context_forward(), "checking if context is deoptimized");
	return parent()->is_context();
}

ContextOop ContextOopDescriptor::outer_context() const {
	if (has_outer_context()) {
		ContextOop con = ContextOop(parent());
		st_assert(con->is_context(), "must be context");
		return con;
	}
	return nullptr;
}

void ContextOopDescriptor::set_unoptimized_context(ContextOop con) {
	st_assert(not mark()->has_context_forward(), "checking if context is deoptimized");
	st_assert(this not_eq con, "Checking for forward cycle");
	set_parent(con);
	set_mark(mark()->set_context_forward());
}

ContextOop ContextOopDescriptor::unoptimized_context() {
	if (mark()->has_context_forward()) {
		ContextOop con = ContextOop(parent());
		st_assert(con->is_context(), "must be context");
		return con;
	}
	return nullptr;
}

std::int32_t ContextOopDescriptor::chain_length() const {

	std::int32_t size = 1;
	GrowableArray<ContextOop> *path = new GrowableArray<ContextOop>(10);

	for (ContextOop cc = ContextOop(this); cc->has_outer_context(); cc = cc->outer_context()) {
		st_assert(path->find(cc) < 0, "cycle detected in context chain")
		path->append(cc);
	}

	for (ContextOop con = ContextOop(this); con->has_outer_context(); con = con->outer_context()) {
		size++;
	}

	return size;
}

void ContextOopDescriptor::print_home_on(ConsoleOutputStream *stream) {
	if (mark()->has_context_forward()) {
		stream->print("deoptimized to (");
		unoptimized_context()->print_value();
		stream->print(")");
	}
	else if (has_parent_fp()) {
		stream->print("frame 0x%lx", parent_fp());
	}
	else if (has_outer_context()) {
		stream->print("outer context ");
		outer_context()->print_value_on(stream);
	}
	else {
		st_assert(is_dead(), "context must be dead");
		stream->print("dead");
	}
}

std::int32_t ContextOopDescriptor::parent_word_offset() {
	return 2; // word offset of parent context
}

std::int32_t ContextOopDescriptor::temp0_word_offset() {
	return 3; // word offset of first context temp
}

std::int32_t ContextOopDescriptor::parent_byte_offset() {
	return byteOffset(parent_word_offset());
}

std::int32_t ContextOopDescriptor::temp0_byte_offset() {
	return byteOffset(temp0_word_offset());
}
