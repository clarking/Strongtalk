//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitive/ObjectArrayPrimitives.hpp"
#include "vm/platform/platform.hpp"
#include "vm/utility/ObjectIDTable.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/oop/MixinOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/runtime/VMOperation.hpp"
#include "vm/code/NativeMethod.hpp"

TRACE_FUNC(TraceObjectArrayPrims, "objectArray")

std::int32_t ObjectArrayPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->isObjectArray(), "receiver must be object array")

PRIM_DECL_3(ObjectArrayPrimitives::allocateSize2, Oop receiver, Oop argument, Oop tenured) {
	PROLOGUE_3("allocateSize2", receiver, argument, tenured);
	//Changed assertion to simple test.
	// st_assert(receiver->is_klass() and klassOop(receiver)->klass_part()->oopIsObjectArray(),
	//       "receiver must object array class");
	if (not receiver->is_klass() or not KlassOop(receiver)->klass_part()->oopIsObjectArray())
		return markSymbol(vmSymbols::invalid_klass());

	if (not argument->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (SmallIntegerOop(argument)->value() < 0)
		return markSymbol(vmSymbols::negative_size());

	if (tenured not_eq Universe::trueObject() and tenured not_eq Universe::falseObject())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	KlassOop k = KlassOop(receiver);
	std::int32_t ni_size = k->klass_part()->non_indexable_size();
	std::int32_t obj_size = ni_size + 1 + SmallIntegerOop(argument)->value();

	// allocate
	Oop *result = (tenured == Universe::trueObject()) ? Universe::allocate_tenured(obj_size, false) : Universe::allocate(obj_size, (MemOop *) &k, false);
	if (result == nullptr)
		return markSymbol(vmSymbols::failed_allocation());

	ObjectArrayOop obj = as_objectArrayOop(result);
	// header
	MemOop(obj)->initialize_header(k->klass_part()->has_untagged_contents(), k);
	// instance variables
	MemOop(obj)->initialize_body(MemOopDescriptor::header_size(), ni_size);
	// %optimized 'obj->set_signed_length(size)'
	Oop *base = reinterpret_cast<Oop *>(  obj->addr());
	base[ni_size] = argument;
	MemOop(obj)->initialize_body(ni_size + 1, obj_size);
	return obj;
}

PRIM_DECL_2(ObjectArrayPrimitives::allocateSize, Oop receiver, Oop argument) {
	PROLOGUE_2("allocateSize", receiver, argument);
	st_assert(receiver->is_klass() and KlassOop(receiver)->klass_part()->oopIsObjectArray(), "receiver must object array class");
	if (not argument->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (SmallIntegerOop(argument)->value() < 0)
		return markSymbol(vmSymbols::negative_size());

	KlassOop k = KlassOop(receiver);
	std::int32_t ni_size = k->klass_part()->non_indexable_size();
	std::int32_t obj_size = ni_size + 1 + SmallIntegerOop(argument)->value();
	// allocate
	ObjectArrayOop obj = as_objectArrayOop(Universe::allocate(obj_size, (MemOop *) &k));
	// header
	MemOop(obj)->initialize_header(k->klass_part()->has_untagged_contents(), k);
	// instance variables
	MemOop(obj)->initialize_body(MemOopDescriptor::header_size(), ni_size);
	// %optimized 'obj->set_signed_length(size)'
	Oop *base = reinterpret_cast<Oop *>(  obj->addr());
	base[ni_size] = argument;
	MemOop(obj)->initialize_body(ni_size + 1, obj_size);
	return obj;
}

PRIM_DECL_1(ObjectArrayPrimitives::size, Oop receiver) {
	PROLOGUE_1("size", receiver);
	ASSERT_RECEIVER;
	// do the operation
	return smiOopFromValue(ObjectArrayOop(receiver)->length());
}

PRIM_DECL_2(ObjectArrayPrimitives::at, Oop receiver, Oop index) {
	PROLOGUE_2("at", receiver, index);
	ASSERT_RECEIVER;

	// check index type
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	// check index value
	if (not ObjectArrayOop(receiver)->is_within_bounds(SmallIntegerOop(index)->value()))
		return markSymbol(vmSymbols::out_of_bounds());

	// do the operation
	return ObjectArrayOop(receiver)->obj_at(SmallIntegerOop(index)->value());
}

PRIM_DECL_3(ObjectArrayPrimitives::atPut, Oop receiver, Oop index, Oop value) {
	PROLOGUE_3("atPut", receiver, index, value);
	ASSERT_RECEIVER;

	// check index type
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	// check index value
	if (not ObjectArrayOop(receiver)->is_within_bounds(SmallIntegerOop(index)->value()))
		return markSymbol(vmSymbols::out_of_bounds());

	// do the operation
	ObjectArrayOop(receiver)->obj_at_put(SmallIntegerOop(index)->value(), value);
	return receiver;
}

PRIM_DECL_2(ObjectArrayPrimitives::at_all_put, Oop receiver, Oop obj) {
	PROLOGUE_2("at_all_put", receiver, obj);
	ASSERT_RECEIVER;

	std::size_t length = ObjectArrayOop(receiver)->length();
	if (obj->is_new() and receiver->is_old()) {
		// Do store checks
		for (std::size_t i = 1; i <= length; i++) {
			ObjectArrayOop(receiver)->obj_at_put(i, obj);
		}
	}
	else {
		// Ignore store check for speed
		set_oops(ObjectArrayOop(receiver)->objs(1), length, obj);
	}
	return receiver;
}

PRIM_DECL_5(ObjectArrayPrimitives::replace_from_to, Oop receiver, Oop from, Oop to, Oop source, Oop start) {
	PROLOGUE_5("replace_from_to", receiver, from, to, source, start);
	ASSERT_RECEIVER;

	// check from type
	if (not from->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	// check to type
	if (not to->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// check source type
	if (not source->isObjectArray())
		return markSymbol(vmSymbols::third_argument_has_wrong_type());

	// check start type
	if (not start->isSmallIntegerOop())
		return markSymbol(vmSymbols::third_argument_has_wrong_type());

	// check from > 0
	if (reinterpret_cast<std::int32_t>(SmallIntegerOop(from)) <= std::int32_t{0})
		return markSymbol(vmSymbols::out_of_bounds());

	// check to < self size
	if (SmallIntegerOop(to)->value() > ObjectArrayOop(receiver)->length())
		return markSymbol(vmSymbols::out_of_bounds());

	// check from <= to
	if (SmallIntegerOop(from)->value() > SmallIntegerOop(to)->value())
		return markSymbol(vmSymbols::out_of_bounds());

	// check if source is big enough
	if (SmallIntegerOop(start)->value() + (SmallIntegerOop(to)->value() - SmallIntegerOop(from)->value() + 1) > ObjectArrayOop(source)->length())
		return markSymbol(vmSymbols::out_of_bounds());

	// Dispatch the operation to the array
	ObjectArrayOop(receiver)->replace_from_to(SmallIntegerOop(from)->value(), SmallIntegerOop(to)->value(), ObjectArrayOop(source), SmallIntegerOop(start)->value());

	return receiver;
}

PRIM_DECL_4(ObjectArrayPrimitives::copy_size, Oop receiver, Oop from, Oop start, Oop size) {
	PROLOGUE_4("copy_size", receiver, from, start, size);
	ASSERT_RECEIVER;

	// check from type
	if (not from->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	// check start type
	if (not start->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// check size type
	if (not size->isSmallIntegerOop())
		return markSymbol(vmSymbols::third_argument_has_wrong_type());

	// check from > 0
	//if (SmallIntegerOop(from) <= 0)
	if (reinterpret_cast<std::int32_t>(SmallIntegerOop(from)) <= std::int32_t{0})
		return markSymbol(vmSymbols::out_of_bounds());

	// check start > 0
	//if (SmallIntegerOop(start) <= 0)
	if (reinterpret_cast<std::int32_t>(SmallIntegerOop(start)) <= std::int32_t{0})
		return markSymbol(vmSymbols::out_of_bounds());

	// Check size is positive
	if (SmallIntegerOop(size)->value() < 0)
		return markSymbol(vmSymbols::negative_size());

	HandleMark hm;
	Handle saved_receiver(receiver);

	// allocation of object array
	KlassOop k = receiver->klass();
	std::int32_t ni_size = k->klass_part()->non_indexable_size();
	std::int32_t obj_size = ni_size + 1 + SmallIntegerOop(size)->value();
	// allocate
	ObjectArrayOop obj = as_objectArrayOop(Universe::allocate(obj_size, (MemOop *) &k));

	ObjectArrayOop src = saved_receiver.as_objectArray();

	// header
	MemOop(obj)->initialize_header(k->klass_part()->has_untagged_contents(), k);
	// copy instance variables
	for (std::size_t i = MemOopDescriptor::header_size(); i < ni_size; i++) {
		obj->raw_at_put(i, src->raw_at(i));
	}
	// length
	obj->set_length(SmallIntegerOop(size)->value());
	// fill the array
	obj->replace_and_fill(SmallIntegerOop(from)->value(), SmallIntegerOop(start)->value(), src);

	return obj;
}
