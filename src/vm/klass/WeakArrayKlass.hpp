//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/klass/ObjectArrayKlass.hpp"
#include "vm/utility/GrowableArray.hpp"

class WeakArrayKlass : public ObjectArrayKlass {
public:
	friend void setKlassVirtualTableFromWeakArrayKlass(Klass *k);

	const char *name() const {
		return "weakArray";
	}

	// creates invocation
	KlassOop create_subclass(MixinOop mixin, Format format);

	static KlassOop create_class(KlassOop super_class, MixinOop mixin);

	// Format
	Format format() {
		return Format::weak_array_klass;
	}

	// ALL FUNCTIONS BELOW THIS POINT ARE DISPATCHED FROM AN OOP

	// memory operations
	std::int32_t oop_scavenge_contents(Oop obj);

	std::int32_t oop_scavenge_tenured_contents(Oop obj);

	void oop_follow_contents(Oop obj);

	// testers
	bool oopIsWeakArray() const {
		return true;
	}

	// iterators
	void oop_oop_iterate(Oop obj, OopClosure *blk);

	void oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk);

	// Sizing
	std::int32_t oop_header_size() const {
		return WeakArrayOopDescriptor::header_size();
	}
};

void setKlassVirtualTableFromWeakArrayKlass(Klass *k);
// The weak array register is used during memory management to split the object scanning into two parts:
//   1. Transitively traverse all object except the indexable part of weakArrays. Then a weakArray is encountered it is registered
//   2. Using the registered weakArrays continue the transitive traverse.
// Inbetween we can easily compute the set of object with a near death experience.
//
// Scavenge and Mark Sweep use to disjunct parts of the interface.

// Implementation note:
//  During phase1 of Mark Sweep pointers are reversed and a objects
//  structure cannot be used (the klass pointer is gone).
//
//  This makes
//  it necessary to register weakArrays along with their non-indexable sizes.
//  'non_indexable_sizes' contains the non indexable sizes.

// Interface for weak array support
class WeakArrayRegister : AllStatic {
public:
	// Scavenge interface
	static void begin_scavenge();

	static bool scavenge_register(WeakArrayOop obj);

	static void check_and_scavenge_contents();

	// Mark sweep interface
	static void begin_mark_sweep();

	static bool mark_sweep_register(WeakArrayOop obj, std::int32_t non_indexable_size);

	static void check_and_follow_contents();

private:
	// Variables
	static bool during_registration;
	static GrowableArray<WeakArrayOop> *weakArrays;
	static GrowableArray<std::int32_t> *non_indexable_sizes;

	// Scavenge operations
	static void scavenge_contents();

	static inline bool scavenge_is_near_death(Oop obj);

	static void scavenge_check_for_dying_objects();

	// Mark sweep operations
	static void follow_contents();

	static inline bool mark_sweep_is_near_death(Oop obj);

	static void mark_sweep_check_for_dying_objects();
};

// The NotificationQueue holds references to weakArrays
// containing object with a near death experience.
class NotificationQueue : AllStatic {
public:
	// Queue operations
	static void mark_elements();   // Marks all elements as queued (by using the sentinel bit)
	static void clear_elements();  // Reset the sentinel bit

	static bool is_empty();

	static Oop get();

	static void put(Oop obj);

	static void put_if_absent(Oop obj);

	// Memory management
	static void oops_do(void f(Oop *));

private:
	static Oop *array;
	static std::int32_t size;
	static std::int32_t first;
	static std::int32_t last;

	static std::int32_t succ(std::int32_t index);
};
