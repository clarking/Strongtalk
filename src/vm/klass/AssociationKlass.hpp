//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/ByteArrayOopDescriptor.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"
#include "vm/oop/AssociationOopDescriptor.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

//
void setKlassVirtualTableFromAssociationKlass(Klass *k);

// associations are cons cells used in the Delta system dictionary.
class AssociationKlass : public MemOopKlass {
public:

	AssociationKlass() = default;
	~AssociationKlass() = default;

	friend void setKlassVirtualTableFromAssociationKlass(Klass *k);

	bool oopIsAssociation() const {
		return true;
	}

	// allocation properties
	bool can_inline_allocation() const {
		return false;
	}

	// allocation operations
	Oop allocateObject(bool permit_scavenge = true, bool tenured = false);

	// creates invocation
	KlassOop create_subclass(MixinOop mixin, Format format);

	static KlassOop create_class(KlassOop super_class, MixinOop mixin);

	// Format
	Format format() {
		return Format::association_klass;
	}

	// memory operations
	bool oop_verify(Oop obj);

	const char *name() const {
		return "association";
	}

	// Reflective properties
	bool can_have_instance_variables() const {
		return false;
	}

	bool can_be_subclassed() const {
		return false;
	}

	// printing operations
	void oop_short_print_on(Oop obj, ConsoleOutputStream *stream);

	void oop_print_value_on(Oop obj, ConsoleOutputStream *stream);

	// memory operations
	std::int32_t oop_scavenge_contents(Oop obj);

	std::int32_t oop_scavenge_tenured_contents(Oop obj);

	void oop_follow_contents(Oop obj);

	// iterators
	void oop_oop_iterate(Oop obj, OopClosure *blk);

	void oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk);

	// sizing
	std::int32_t oop_header_size() const {
		return AssociationOopDescriptor::header_size();
	}
};
