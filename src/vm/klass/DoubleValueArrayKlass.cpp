//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/oop/DoubleValueArrayOopDescriptor.hpp"
#include "vm/klass/DoubleValueArrayKlass.hpp"
#include "vm/memory/util.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/klass/Klass.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

Oop DoubleValueArrayKlass::allocateObject(bool permit_scavenge, bool tenured) {
	st_unused(permit_scavenge); // unused
	st_unused(tenured); // unused

	st_fatal("should never call allocateObject in doubleValueArrayKlass");
	return MarkOopDescriptor::bad();
}

Oop DoubleValueArrayKlass::allocateObjectSize(std::int32_t size, bool permit_scavenge, bool permit_tenured) {

	st_unused(permit_scavenge); // unused
	st_unused(permit_tenured); // unused

	//
	KlassOop k = as_klassOop();
	std::int32_t ni_size = non_indexable_size();
	std::int32_t obj_size = ni_size + 1 + roundTo(size * sizeof(double), OOP_SIZE) / OOP_SIZE;

	// allocate
	DoubleValueArrayOop obj = as_doubleValueArrayOop(Universe::allocate(obj_size, (MemOop *) &k));

	// header
	MemOop(obj)->initialize_header(true, k);

	// instance variables
	MemOop(obj)->initialize_body(MemOopDescriptor::header_size(), ni_size);

	// indexables
	obj->set_length(size);

	for (std::size_t i = 1; i <= size; i++) {
		obj->double_at_put(i, 0.0);
	}

	return obj;
}

KlassOop DoubleValueArrayKlass::create_subclass(MixinOop mixin, Format format) {
	if (format == Format::mem_klass or format == Format::double_value_array_klass) {
		return DoubleValueArrayKlass::create_class(as_klassOop(), mixin);
	}
	return nullptr;
}

KlassOop DoubleValueArrayKlass::create_class(KlassOop super_class, MixinOop mixin) {
	DoubleValueArrayKlass o;
	return create_generic_class(super_class, mixin, o.vtbl_value());
}

void setKlassVirtualTableFromDoubleValueArrayKlass(Klass *k) {
	DoubleValueArrayKlass o;
	k->set_vtbl_value(o.vtbl_value());
}

bool DoubleValueArrayKlass::oop_verify(Oop obj) {
	st_assert_doubleValueArray(obj, "Argument must be doubleValueArray");
	return DoubleValueArrayOop(obj)->verify();
}

void DoubleValueArrayKlass::oop_print_value_on(Oop obj, ConsoleOutputStream *stream) {
	st_unused(stream); // unused

	st_assert_doubleValueArray(obj, "Argument must be doubleValueArray");
//    DoubleValueArrayOop array = DoubleValueArrayOop( obj );
//    std::int32_t        len   = array->length();
//    std::int32_t        n     = min( MaxElementPrintSize, len );
	Unimplemented();
}

void DoubleValueArrayKlass::oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk) {
//    double       *p  = DoubleValueArrayOop( obj )->double_start();
	Oop *l = DoubleValueArrayOop(obj)->length_addr();
//    std::int32_t len = DoubleValueArrayOop( obj )->length();
	MemOopKlass::oop_layout_iterate(obj, blk);
	blk->do_oop("length", l);
	blk->begin_indexables();
	Unimplemented();
	blk->end_indexables();
}

void DoubleValueArrayKlass::oop_oop_iterate(Oop obj, OopClosure *blk) {
	Oop *l = DoubleValueArrayOop(obj)->length_addr();
	MemOopKlass::oop_oop_iterate(obj, blk);
	blk->do_oop(l);
}

std::int32_t DoubleValueArrayKlass::oop_scavenge_contents(Oop obj) {
	MemOopKlass::oop_scavenge_contents(obj);
	return object_size(DoubleValueArrayOop(obj)->length());
}

std::int32_t DoubleValueArrayKlass::oop_scavenge_tenured_contents(Oop obj) {
	MemOopKlass::oop_scavenge_tenured_contents(obj);
	return object_size(DoubleValueArrayOop(obj)->length());
}
