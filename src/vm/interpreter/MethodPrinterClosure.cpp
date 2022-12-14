//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/oop/ProcessOopDescriptor.hpp"
#include "vm/oop/AssociationOopDescriptor.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/primitive/Primitives.hpp"
#include "vm/interpreter/MethodPrinterClosure.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"
#include "vm/interpreter/MethodIterator.hpp"

MethodPrinterClosure::MethodPrinterClosure(ConsoleOutputStream *stream) :
		_outputStream{nullptr} {
	_outputStream = stream ? stream : _console;
}

void MethodPrinterClosure::indent() {

	if (WizardMode) {
		_outputStream->indent();
		_outputStream->print("      <");
		GrowableArray<std::int32_t> *map = method()->expression_stack_mapping(byteCodeIndex());

		for (std::size_t i = 0; i < map->length(); i++) {
			_outputStream->print(" 0x%08x", map->at(i));
		}
		_outputStream->print_cr(" >");
	}

	_outputStream->indent();
	_outputStream->print("[%3d] ", byteCodeIndex());
}

void MethodPrinterClosure::show(const char *str) {
	_outputStream->indent();
	_outputStream->print_cr("      %s", str);
}

void MethodPrinterClosure::print_sendtype(ByteCodes::SendType type) {
	_outputStream->print("(");
	switch (type) {
		case ByteCodes::SendType::INTERPRETED_SEND:
			_outputStream->print("interpreted");
			break;
		case ByteCodes::SendType::COMPILED_SEND:
			_outputStream->print("compiled");
			break;
		case ByteCodes::SendType::POLYMORPHIC_SEND:
			_outputStream->print("POLYMORPHIC");
			break;
		case ByteCodes::SendType::MEGAMORPHIC_SEND:
			_outputStream->print("MEGAMORPHIC");
			break;
		case ByteCodes::SendType::PREDICTED_SEND:
			_outputStream->print("predicted");
			break;
		case ByteCodes::SendType::ACCESSOR_SEND:
			_outputStream->print("access");
			break;
		default: ShouldNotReachHere();
			break;
	}
	_outputStream->print(")");
}

void MethodPrinterClosure::if_node(IfNode *node) {
	indent();
	node->selector()->print_symbol_on(_outputStream);
	_outputStream->print_cr(" {0x%08x} [", node->end_byteCodeIndex());
	MethodIterator mi(node->then_code(), this);
	if (node->else_code() and not node->ignore_else_while_printing()) {
		show("]  [");
		MethodIterator mi(node->else_code(), this);
	}
	show("]");
}

void MethodPrinterClosure::cond_node(CondNode *node) {
	indent();
	node->selector()->print_symbol_on(_outputStream);
	_outputStream->cr();
	show("[");
	MethodIterator mi(node->expr_code(), this);
	show("]");
}

void MethodPrinterClosure::while_node(WhileNode *node) {
	indent();
	node->selector()->print_symbol_on(_outputStream);
	_outputStream->print_cr(" {0x%08x} [", node->end_byteCodeIndex());
	if (node->body_code()) {
		MethodIterator mi(node->body_code(), this);
		show("] [");
	}
	MethodIterator mi(node->expr_code(), this);
	show("]");
}

void MethodPrinterClosure::primitive_call_node(PrimitiveCallNode *node) {
	indent();
	_outputStream->print_cr("primitive call '%s'", node->pdesc()->name());
	if (node->failure_code()) {
		MethodIterator mi(node->failure_code(), this);
		show("]");
	}
}

void MethodPrinterClosure::dll_call_node(DLLCallNode *node) {
	indent();
	_outputStream->print("dll call <");
	node->dll_name()->print_symbol_on(_outputStream);
	_outputStream->print(",");
	node->function_name()->print_symbol_on(_outputStream);
	_outputStream->print_cr(",0x%08x>", node->nofArgs());
	if (node->failure_code()) {
		show("[");
		MethodIterator mi(node->failure_code(), this);
		show("]");
	}
}

void MethodPrinterClosure::allocate_temporaries(std::int32_t nofTemps) {
	indent();
	_outputStream->print_cr("allocate 0x%08x temporaries", nofTemps);
}

void MethodPrinterClosure::push_self() {
	indent();
	_outputStream->print_cr("push self");
}

void MethodPrinterClosure::push_tos() {
	indent();
	_outputStream->print_cr("push tos");
}

void MethodPrinterClosure::push_literal(Oop obj) {
	indent();
	_outputStream->print("push literal ");
	obj->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::push_argument(std::int32_t no) {
	indent();
	_outputStream->print_cr("push arg 0x%08x", no);
}

void MethodPrinterClosure::push_temporary(std::int32_t no) {
	indent();
	_outputStream->print_cr("push temp 0x%08x", no);
}

void MethodPrinterClosure::push_temporary(std::int32_t no, std::int32_t context) {
	indent();
	_outputStream->print_cr("push temp 0x%08x [0x{08:x}]", no, context);
}

void MethodPrinterClosure::push_instVar(std::int32_t offset) {
	indent();
	_outputStream->print_cr("push instVar 0x%08x", offset);
}

void MethodPrinterClosure::push_instVar_name(SymbolOop name) {
	indent();
	_outputStream->print("push instVar ");
	name->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::push_classVar(AssociationOop assoc) {
	indent();
	_outputStream->print("store classVar ");
	assoc->key()->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::push_classVar_name(SymbolOop name) {
	indent();
	_outputStream->print("store classVar ");
	name->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::push_global(AssociationOop obj) {
	indent();
	_outputStream->print("push global ");
	obj->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::store_temporary(std::int32_t no) {
	indent();
	_outputStream->print_cr("store temp 0x%08x", no);
}

void MethodPrinterClosure::store_temporary(std::int32_t no, std::int32_t context) {
	indent();
	_outputStream->print_cr("store temp 0x%08x [0x{08:x}]", no, context);
}

void MethodPrinterClosure::store_instVar(std::int32_t offset) {
	indent();
	_outputStream->print_cr("store instVar 0x%08x", offset);
}

void MethodPrinterClosure::store_instVar_name(SymbolOop name) {
	indent();
	_outputStream->print("store instVar ");
	name->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::store_classVar(AssociationOop assoc) {
	indent();
	_outputStream->print("store classVar ");
	assoc->key()->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::store_classVar_name(SymbolOop name) {
	indent();
	_outputStream->print("store classVar ");
	name->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::store_global(AssociationOop obj) {
	indent();
	_outputStream->print("store global ");
	obj->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::pop() {
	indent();
	_outputStream->print_cr("pop");
}

void MethodPrinterClosure::normal_send(InterpretedInlineCache *ic) {
	indent();
	_outputStream->print("normal send ");
	print_sendtype(ic->send_type());
	_outputStream->print(" ");
	Oop s = Oop(ic->selector());
	if (not s->isSmallIntegerOop() and Universe::is_heap((Oop *) s)) {
		st_assert_symbol(ic->selector(), "selector in ic must be a symbol");
		ic->selector()->print_value_on(_outputStream);
	}
	else {
		_outputStream->print("INVALID SELECTOR, 0x%lx", s);
	}
	_outputStream->cr();
}

void MethodPrinterClosure::self_send(InterpretedInlineCache *ic) {
	indent();
	_outputStream->print("self send ");
	print_sendtype(ic->send_type());
	ic->selector()->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::super_send(InterpretedInlineCache *ic) {
	indent();
	_outputStream->print("super send ");
	print_sendtype(ic->send_type());
	ic->selector()->print_value_on(_outputStream);
	_outputStream->cr();
}

void MethodPrinterClosure::double_equal() {
	indent();
	_outputStream->print_cr("hardwired ==");
}

void MethodPrinterClosure::double_not_equal() {
	indent();
	_outputStream->print_cr("hardwired ~~");
}

void MethodPrinterClosure::method_return(std::int32_t nofArgs) {
	indent();
	_outputStream->print_cr("return (pop 0x%08x args)", nofArgs);
}

void MethodPrinterClosure::nonlocal_return(std::int32_t nofArgs) {
	indent();
	_outputStream->print_cr("non-local return (pop 0x%08x args)", nofArgs);
}

void MethodPrinterClosure::allocate_closure(AllocationType type, std::int32_t nofArgs, MethodOop meth) {
	indent();
	_outputStream->print("allocate closure");
	switch (type) {
		case AllocationType::tos_as_scope:
			_outputStream->print("{tos}");
			break;
		case AllocationType::context_as_scope:
			_outputStream->print("{context}");
			break;
	}
	_outputStream->print_cr(" 0x%08x args", nofArgs);

	{
		_outputStream->inc();
		_outputStream->inc();
		auto temp = MethodPrinterClosure(_outputStream);
		MethodIterator it(meth, &temp);
		_outputStream->dec();
		_outputStream->dec();
	}
}

void MethodPrinterClosure::allocate_context(std::int32_t nofTemps, bool forMethod) {
	indent();
	_outputStream->print_cr("allocate %s context with 0x%08x temporaries", forMethod ? "method" : "block", nofTemps);
}

void MethodPrinterClosure::set_self_via_context() {
	indent();
	_outputStream->print_cr("set self via context");
}

void MethodPrinterClosure::copy_self_into_context() {
	indent();
	_outputStream->print_cr("copy self into context");
}

void MethodPrinterClosure::copy_argument_into_context(std::int32_t argNo, std::int32_t no) {
	indent();
	_outputStream->print_cr("copy argument 0x%08x into context at 0x%08x", argNo, no);
}

void MethodPrinterClosure::zap_scope() {
	indent();
	_outputStream->print_cr("zap block");
}

void MethodPrinterClosure::predict_primitive_call(PrimitiveDescriptor *pdesc, std::int32_t failure_start) {
	st_unused(pdesc); // unused
	st_unused(failure_start); // unused

	indent();
	_outputStream->print_cr("predicted prim method");
}

void MethodPrinterClosure::float_allocate(std::int32_t nofFloatTemps, std::int32_t nofFloatExprs) {
	indent();
	_outputStream->print_cr("float allocate temps=0x%08x, expr=0x%08x", nofFloatTemps, nofFloatExprs);
}

void MethodPrinterClosure::float_floatify(Floats::Function f, std::int32_t tof) {
	indent();
	_outputStream->print("float floatify ");
	Floats::selector_for(f)->print_value_on(_outputStream);
	_outputStream->print_cr(" tof=0x%08x", tof);
}

void MethodPrinterClosure::float_move(std::int32_t tof, std::int32_t from) {
	indent();
	_outputStream->print_cr("float move tof=0x%08x, from=0x%08x", tof, from);
}

void MethodPrinterClosure::float_set(std::int32_t tof, DoubleOop value) {
	indent();
	_outputStream->print_cr("float set tof=0x%08x, value=%1.6g", tof, value->value());
}

void MethodPrinterClosure::float_nullary(Floats::Function f, std::int32_t tof) {
	indent();
	_outputStream->print("float nullary ");
	Floats::selector_for(f)->print_value_on(_outputStream);
	_outputStream->print_cr(" tof=0x%08x", tof);
}

void MethodPrinterClosure::float_unary(Floats::Function f, std::int32_t tof) {
	indent();
	_outputStream->print("float unary ");
	Floats::selector_for(f)->print_value_on(_outputStream);
	_outputStream->print_cr(" tof=0x%08x", tof);
}

void MethodPrinterClosure::float_binary(Floats::Function f, std::int32_t tof) {
	indent();
	_outputStream->print("float binary ");
	Floats::selector_for(f)->print_value_on(_outputStream);
	_outputStream->print_cr(" tof=0x%08x", tof);
}

void MethodPrinterClosure::float_unaryToOop(Floats::Function f, std::int32_t tof) {
	indent();
	_outputStream->print("float unaryToOop ");
	Floats::selector_for(f)->print_value_on(_outputStream);
	_outputStream->print_cr(" tof=0x%08x", tof);
}

void MethodPrinterClosure::float_binaryToOop(Floats::Function f, std::int32_t tof) {
	indent();
	_outputStream->print("float binaryToOop ");
	Floats::selector_for(f)->print_value_on(_outputStream);
	_outputStream->print_cr(" tof=0x%08x", tof);
}
