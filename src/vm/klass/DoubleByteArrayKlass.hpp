//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/DoubleByteArrayOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

class DoubleByteArrayKlass : public MemOopKlass {

public:
	// allocation properties
	bool can_inline_allocation() const {
		return false;
	}

	// Return the Oop size for a doubleByteArrayOop
	std::int32_t object_size(std::int32_t number_of_doubleBytes) const {
		return non_indexable_size() + 1 + roundTo(number_of_doubleBytes * 2, OOP_SIZE) / OOP_SIZE;
	}

	// creation operations
	Oop allocateObject(bool permit_scavenge = true, bool tenured = false);

	Oop allocateObjectSize(std::int32_t bytes, bool permit_scavenge = true, bool tenured = false);

	// creates invocation
	KlassOop create_subclass(MixinOop mixin, Format format);

	static KlassOop create_class(KlassOop super_class, MixinOop mixin);

	// Format
	Format format() {
		return Format::double_byte_array_klass;
	}

	friend void setKlassVirtualTableFromDoubleByteArrayKlass(Klass *k);

	const char *name() const {
		return "doubleByteArray";
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
		return DoubleByteArrayOopDescriptor::header_size();
	}

	std::int32_t oop_size(Oop obj) const {
		return object_size(DoubleByteArrayOop(obj)->length());
	}

	// testers
	bool oopIsDoubleByteArray() const {
		return true;
	}

	bool oopIsIndexable() const {
		return true;
	}
};

void setKlassVirtualTableFromDoubleByteArrayKlass(Klass *k);
