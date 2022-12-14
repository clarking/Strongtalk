//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/oop/DoubleValueArrayOopDescriptor.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

class DoubleValueArrayKlass : public MemOopKlass {

public:

	DoubleValueArrayKlass() = default;
	virtual ~DoubleValueArrayKlass() = default;
	DoubleValueArrayKlass(const DoubleValueArrayKlass &) = default;
	DoubleValueArrayKlass &operator=(const DoubleValueArrayKlass &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	// allocation properties
	bool can_inline_allocation() const {
		return false;
	}

	// Return the Oop size for a DoubleValueArrayOop
	std::int32_t object_size(std::int32_t number_of_doubleValues) const {
		return non_indexable_size() + 1 + roundTo(number_of_doubleValues * sizeof(double), OOP_SIZE) / OOP_SIZE;
	}

	// creation operations
	Oop allocateObject(bool permit_scavenge = true, bool tenured = false);

	Oop allocateObjectSize(std::int32_t size, bool permit_scavenge = true, bool tenured = false);

	// creates invocation
	KlassOop create_subclass(MixinOop mixin, Format format);

	static KlassOop create_class(KlassOop super_class, MixinOop mixin);

	// Format
	Format format() {
		return Format::double_value_array_klass;
	}

	friend void setKlassVirtualTableFromDoubleValueArrayKlass(Klass *k);

	const char *name() const {
		return "doubleValueArray";
	}

	// ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP
public:
	// accessors
	std::int32_t oop_scavenge_contents(Oop obj);

	std::int32_t oop_scavenge_tenured_contents(Oop obj);

	bool oop_verify(Oop obj);

	void oop_print_value_on(Oop obj, ConsoleOutputStream *stream);

	// iterators
	void oop_oop_iterate(Oop obj, OopClosure *blk);

	void oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk);

	// Sizing
	std::int32_t oop_header_size() const {
		return DoubleValueArrayOopDescriptor::header_size();
	}

	std::int32_t oop_size(Oop obj) const {
		return object_size(DoubleValueArrayOop(obj)->length());
	}

	// testers
	bool oopIsDoubleValueArray() const {
		return true;
	}

	bool oopIsIndexable() const {
		return true;
	}
};

void setKlassVirtualTableFromDoubleValueArrayKlass(Klass *k);
