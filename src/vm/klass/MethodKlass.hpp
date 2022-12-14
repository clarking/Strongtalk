//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

class MethodKlass : public MemOopKlass {
public:
	bool oopIsMethod() const {
		return true;
	}

	// allocation properties
	bool can_inline_allocation() const {
		return false;
	}

	std::int32_t object_size(std::int32_t size_of_codes) const {
		return non_indexable_size() + size_of_codes + 1;
	}

	std::int32_t oop_size(Oop obj) const {
		return object_size(MethodOop(obj)->size_of_codes());
	}

	// Reflective properties
	bool can_have_instance_variables() const {
		return false;
	}

	bool can_be_subclassed() const {
		return false;
	}

	// creates invocation
	KlassOop create_subclass(MixinOop mixin, Format format);

	// Format
	Format format() {
		return Format::method_klass;
	}

	// memory operations
	std::int32_t oop_scavenge_contents(Oop obj);

	std::int32_t oop_scavenge_tenured_contents(Oop obj);

	void oop_follow_contents(Oop obj);

	friend void setKlassVirtualTableFromMethodKlass(Klass *k);

	// iterators
	void oop_oop_iterate(Oop obj, OopClosure *blk);

	void oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk);

	// Sizing
	std::int32_t oop_header_size() const {
		return MethodOopDescriptor::header_size();
	}

	void oop_print_layout(Oop obj);

	// printing support
	void oop_print_on(Oop obj, ConsoleOutputStream *stream);

	void oop_print_value_on(Oop obj, ConsoleOutputStream *stream);

	const char *name() const {
		return "method";
	}

	// Construction (called from primitive)
	MethodOop constructMethod(Oop name, std::int32_t flags, std::int32_t nofArgs, ObjectArrayOop debugInfo, ByteArrayOop bytes, ObjectArrayOop oops);
};

void setKlassVirtualTableFromMethodKlass(Klass *k);
