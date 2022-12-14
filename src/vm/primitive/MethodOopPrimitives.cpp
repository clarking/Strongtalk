
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/klass/MethodKlass.hpp"
#include "vm/klass/ContextKlass.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/primitive/MethodOopPrimitives.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"

TRACE_FUNC(TraceMethodPrims, "method")

std::int32_t MethodOopPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_method(), "receiver must be method")

PRIM_DECL_1(MethodOopPrimitives::selector, Oop receiver) {
	PROLOGUE_1("selector", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->selector();
}

PRIM_DECL_1(MethodOopPrimitives::debug_info, Oop receiver) {
	PROLOGUE_1("debug_info", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->debugInfo();
}

PRIM_DECL_1(MethodOopPrimitives::size_and_flags, Oop receiver) {
	PROLOGUE_1("size_and_flags", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->size_and_flags();
}

PRIM_DECL_1(MethodOopPrimitives::fileout_body, Oop receiver) {
	PROLOGUE_1("fileout_body", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->fileout_body();
}

PRIM_DECL_2(MethodOopPrimitives::setSelector, Oop receiver, Oop name) {
	PROLOGUE_2("setSelector", receiver, name);
	ASSERT_RECEIVER;
	if (not name->isSymbol())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	MethodOop(receiver)->set_selector_or_method(Oop(name));
	return receiver;
}

PRIM_DECL_1(MethodOopPrimitives::numberOfArguments, Oop receiver) {
	PROLOGUE_1("numberOfArguments", receiver);
	ASSERT_RECEIVER;
	return smiOopFromValue(MethodOop(receiver)->number_of_arguments());
}

PRIM_DECL_1(MethodOopPrimitives::outer, Oop receiver) {
	PROLOGUE_1("outer", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->selector_or_method();
}

PRIM_DECL_2(MethodOopPrimitives::setOuter, Oop receiver, Oop method) {
	PROLOGUE_2("setOuter", receiver, method);
	ASSERT_RECEIVER;
	MethodOop(receiver)->set_selector_or_method(Oop(method));
	return receiver;
}

PRIM_DECL_2(MethodOopPrimitives::referenced_instance_variable_names, Oop receiver, Oop mixin) {
	PROLOGUE_2("referenced_instance_variable_names", receiver, mixin);
	ASSERT_RECEIVER;
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return MethodOop(receiver)->referenced_instance_variable_names(MixinOop(mixin));
}

PRIM_DECL_1(MethodOopPrimitives::referenced_class_variable_names, Oop receiver) {
	PROLOGUE_1("referenced_class_variable_names", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->referenced_class_variable_names();
}

PRIM_DECL_1(MethodOopPrimitives::referenced_global_names, Oop receiver) {
	PROLOGUE_1("referenced_global_names", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->referenced_global_names();
}

PRIM_DECL_1(MethodOopPrimitives::senders, Oop receiver) {
	PROLOGUE_1("senders", receiver);
	ASSERT_RECEIVER;
	return MethodOop(receiver)->senders();
}

PRIM_DECL_2(MethodOopPrimitives::prettyPrint, Oop receiver, Oop klass) {
	PROLOGUE_2("prettyPrint", receiver, klass);
	ASSERT_RECEIVER;
	if (klass == nilObject) {
		PrettyPrinter::print(MethodOop(receiver));
	}
	else {
		if (not klass->is_klass())
			return markSymbol(vmSymbols::first_argument_has_wrong_type());
		PrettyPrinter::print(MethodOop(receiver), KlassOop(klass));
	}
	return receiver;
}

PRIM_DECL_2(MethodOopPrimitives::prettyPrintSource, Oop receiver, Oop klass) {
	PROLOGUE_2("prettyPrintSource", receiver, klass);
	ASSERT_RECEIVER;
	ByteArrayOop a;
	if (klass == nilObject) {
		a = PrettyPrinter::print_in_byteArray(MethodOop(receiver));
	}
	else {
		if (not klass->is_klass())
			return markSymbol(vmSymbols::first_argument_has_wrong_type());
		a = PrettyPrinter::print_in_byteArray(MethodOop(receiver), KlassOop(klass));
	}
	return a;
}

PRIM_DECL_1(MethodOopPrimitives::printCodes, Oop receiver) {
	PROLOGUE_1("printCodes", receiver);
	ASSERT_RECEIVER;
	{
		ResourceMark resourceMark;
		MethodOop(receiver)->print_codes();
	}
	return receiver;
}

PRIM_DECL_6(MethodOopPrimitives::constructMethod, Oop selector_or_method, Oop flags, Oop nofArgs, Oop debugInfo, Oop bytes, Oop oops) {
	PROLOGUE_6("constructMethod", selector_or_method, flags, nofArgs, debugInfo, bytes, oops);
	if (not selector_or_method->isSymbol() and (selector_or_method not_eq nilObject))
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not flags->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());
	if (not nofArgs->isSmallIntegerOop())
		return markSymbol(vmSymbols::third_argument_has_wrong_type());
	if (not debugInfo->isObjectArray())
		return markSymbol(vmSymbols::fourth_argument_has_wrong_type());
	if (not bytes->isByteArray())
		return markSymbol(vmSymbols::fifth_argument_has_wrong_type());
	if (not oops->isObjectArray())
		return markSymbol(vmSymbols::sixth_argument_has_wrong_type());

	if (ObjectArrayOop(oops)->length() * OOP_SIZE not_eq ByteArrayOop(bytes)->length()) {
		return markSymbol(vmSymbols::method_construction_failed());
	}

	MethodKlass *k = (MethodKlass *) Universe::methodKlassObject()->klass_part();
	MethodOop result = k->constructMethod(selector_or_method, SmallIntegerOop(flags)->value(), SmallIntegerOop(nofArgs)->value(), ObjectArrayOop(debugInfo), ByteArrayOop(bytes), ObjectArrayOop(oops));
	if (result)
		return result;
	return markSymbol(vmSymbols::method_construction_failed());
}

extern "C" BlockClosureOop allocateBlock(SmallIntegerOop nof);

static Oop allocate_block_for(MethodOop method, Oop self) {
	BlockScavenge bs;

	if (not method->is_customized()) {
		method->customize_for(self->klass(), self->blueprint()->mixin());
	}

	// allocate the context for the block (make room for the self)
	ContextKlass *ok = (ContextKlass *) contextKlassObject->klass_part();
	ContextOop con = ContextOop(ok->allocateObjectSize(1));
	con->kill();
	con->obj_at_put(0, self);

	// allocate the resulting block
	BlockClosureOop blk = allocateBlock(smiOopFromValue(method->number_of_arguments()));
	blk->set_method(method);
	blk->set_lexical_scope(con);

	return blk;
}

PRIM_DECL_1(MethodOopPrimitives::allocate_block, Oop receiver) {
	PROLOGUE_1("allocate_block", receiver);
	ASSERT_RECEIVER;

	if (not MethodOop(receiver)->is_blockMethod())
		return markSymbol(vmSymbols::conversion_failed());

	return allocate_block_for(MethodOop(receiver), nilObject);
}

PRIM_DECL_2(MethodOopPrimitives::allocate_block_self, Oop receiver, Oop self) {
	PROLOGUE_2("allocate_block_self", receiver, self);
	ASSERT_RECEIVER;

	if (not MethodOop(receiver)->is_blockMethod())
		return markSymbol(vmSymbols::conversion_failed());

	return allocate_block_for(MethodOop(receiver), self);
}

static SymbolOop symbol_from_method_inlining_info(MethodOopDescriptor::Method_Inlining_Info info) {
	if (info == MethodOopDescriptor::normal_inline)
		return OopFactory::new_symbol("Normal");
	if (info == MethodOopDescriptor::never_inline)
		return OopFactory::new_symbol("Never");
	if (info == MethodOopDescriptor::always_inline)
		return OopFactory::new_symbol("Always");
	ShouldNotReachHere();
	return nullptr;
}

PRIM_DECL_2(MethodOopPrimitives::set_inlining_info, Oop receiver, Oop info) {
	PROLOGUE_2("set_inlining_info", receiver, info);
	ASSERT_RECEIVER;
	if (not info->isSymbol())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	// Check argument value
	MethodOopDescriptor::Method_Inlining_Info in;
	if (SymbolOop(info)->equals("Never")) {
		in = MethodOopDescriptor::never_inline;
	}
	else if (SymbolOop(info)->equals("Always")) {
		in = MethodOopDescriptor::always_inline;
	}
	else if (SymbolOop(info)->equals("Normal")) {
		in = MethodOopDescriptor::normal_inline;
	}
	else {
		return markSymbol(vmSymbols::argument_is_invalid());
	}
	MethodOopDescriptor::Method_Inlining_Info old = MethodOop(receiver)->method_inlining_info();
	MethodOop(receiver)->set_method_inlining_info(in);
	return symbol_from_method_inlining_info(old);
}

PRIM_DECL_1(MethodOopPrimitives::inlining_info, Oop receiver) {
	PROLOGUE_1("inlining_info", receiver);
	ASSERT_RECEIVER;
	return symbol_from_method_inlining_info(MethodOop(receiver)->method_inlining_info());
}
