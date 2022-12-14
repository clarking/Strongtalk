//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/klass/ByteArrayKlass.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/oop/ByteArrayOopDescriptor.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

Oop ByteArrayKlass::allocateObject(bool permit_scavenge, bool tenured) {

	st_unused(permit_scavenge); // unused
	st_unused(tenured); // unused

	st_assert(not can_inline_allocation(), "using nonstandard allocation");

	// This should not be fatal!
	// This could be called erroneously from ST in which case it should result in an ST level error.
	// Instead return a marked symbol to indicate the failure.
	//fatal("should never call allocateObject in byteArrayKlass");
	return markSymbol(vmSymbols::invalid_klass());
}

Oop ByteArrayKlass::allocateObjectSize(std::int32_t size, bool permit_scavenge, bool permit_tenured) {
	KlassOop k = as_klassOop();
	std::int32_t ni_size = non_indexable_size();
	std::int32_t obj_size = ni_size + 1 + roundTo(size, OOP_SIZE) / OOP_SIZE;
	// allocate
	Oop *result = permit_tenured ? Universe::allocate_tenured(obj_size, false) : Universe::allocate(obj_size, (MemOop *) &k, permit_scavenge);

	if (not result)
		return nullptr;

	ByteArrayOop obj = as_byteArrayOop(result);
	// header
	MemOop(obj)->initialize_header(true, k);
	// instance variables
	MemOop(obj)->initialize_body(MemOopDescriptor::header_size(), ni_size);
	// indexables
	Oop *base = (Oop *) obj->addr();
	Oop *end = base + obj_size;
	// %optimized 'obj->set_signed_length(size)'
	base[ni_size] = smiOopFromValue(size);
	// %optimized 'for (std::int32_t index = 1; index <= size; index++)
	//               obj->byte_at_put(index, '\000')'
	base = &base[ni_size + 1];
	while (base < end)
		*base++ = (Oop) 0;
	return obj;
}

KlassOop ByteArrayKlass::create_subclass(MixinOop mixin, Format format) {
	if (format == Format::mem_klass or format == Format::byte_array_klass) {
		return ByteArrayKlass::create_class(as_klassOop(), mixin);
	}
	return nullptr;
}

KlassOop ByteArrayKlass::create_class(KlassOop super_class, MixinOop mixin) {
	ByteArrayKlass o;
	return create_generic_class(super_class, mixin, o.vtbl_value());
}

void ByteArrayKlass::initialize_object(ByteArrayOop obj, const char *value, std::int32_t len) {
	for (std::size_t i = 1; i <= len; i++) {
		obj->byte_at_put(i, value[i - 1]);
	}
}

void setKlassVirtualTableFromByteArrayKlass(Klass *k) {
	ByteArrayKlass o;
	k->set_vtbl_value(o.vtbl_value());
}

bool ByteArrayKlass::oop_verify(Oop obj) {
	st_assert_byteArray(obj, "Argument must be byteArray");
	return ByteArrayOop(obj)->verify();
}

void ByteArrayKlass::oop_print_value_on(Oop obj, ConsoleOutputStream *stream) {
	st_assert_byteArray(obj, "Argument must be byteArray");
	ByteArrayOop array = ByteArrayOop(obj);
	std::int32_t len = array->length();
	std::int32_t n = min(MaxElementPrintSize, len);
	stream->print("'");
	for (std::size_t i = 1; i <= n; i++) {
		char c = array->byte_at(i);
		if (isprint(c))
			stream->print("%c", c);
		else
			stream->print("\\%o", c);
	}
	if (n < len)
		stream->print("...");
	stream->print("'");
}

void ByteArrayKlass::oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk) {
	std::uint8_t *p = ByteArrayOop(obj)->bytes();
	Oop *l = ByteArrayOop(obj)->length_addr();
	std::int32_t len = ByteArrayOop(obj)->length();
	// header + instance variables
	MemOopKlass::oop_layout_iterate(obj, blk);
	// indexables
	blk->begin_indexables();
	blk->do_oop("length", l);
	for (std::size_t i = 1; i <= len; i++) {
		blk->do_indexable_byte(i, p++);
	}
	blk->end_indexables();
}

void ByteArrayKlass::oop_oop_iterate(Oop obj, OopClosure *blk) {
	Oop *l = ByteArrayOop(obj)->length_addr();
	// header + instance variables
	MemOopKlass::oop_oop_iterate(obj, blk);
	blk->do_oop(l);
}

std::int32_t ByteArrayKlass::oop_scavenge_contents(Oop obj) {
	// header + instance variables
	MemOopKlass::oop_scavenge_contents(obj);
	return object_size(ByteArrayOop(obj)->length());
}

std::int32_t ByteArrayKlass::oop_scavenge_tenured_contents(Oop obj) {
	// header + instance variables
	MemOopKlass::oop_scavenge_tenured_contents(obj);
	return object_size(ByteArrayOop(obj)->length());
}
