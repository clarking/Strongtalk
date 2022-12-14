
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/BlockClosureOopDescriptor.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/oop/SmallIntegerOopDescriptor.hpp"
#include "vm/oop/ContextOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

class ContextKlass : public MemOopKlass {

public:
	// testers
	bool oopIsContext() const {
		return true;
	}

	// allocation properties
	bool can_inline_allocation() const {
		return false;
	}

	// reflective properties
	bool can_have_instance_variables() const {
		return false;
	}

	bool can_be_subclassed() const {
		return false;
	}

	// allocation
	Oop allocateObjectSize(std::int32_t num_of_temps, bool permit_scavenge = true, bool tenured = false);

	static ContextOop allocate_context(std::int32_t num_of_temps);

	// creates invocation
	KlassOop create_subclass(MixinOop mixin, Format format);

	// Format
	Format format() {
		return Format::context_klass;
	}

	// scavenge
	std::int32_t oop_scavenge_contents(Oop obj);

	std::int32_t oop_scavenge_tenured_contents(Oop obj);

	void oop_follow_contents(Oop obj);

	// iterators
	void oop_oop_iterate(Oop obj, OopClosure *blk);

	void oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk);

	// sizing
	std::int32_t oop_header_size() const {
		return ContextOopDescriptor::header_size();
	}

	std::int32_t oop_size(Oop obj) const;

	// printing support
	void oop_print_value_on(Oop obj, ConsoleOutputStream *stream);

	void oop_print_on(Oop obj, ConsoleOutputStream *stream);

	const char *name() const {
		return "context";
	}

	// class creation
	friend void setKlassVirtualTableFromContextKlass(Klass *k);
};
