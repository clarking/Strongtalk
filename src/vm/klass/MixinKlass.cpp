//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/klass/MixinKlass.hpp"
#include "vm/klass/Klass.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"

void setKlassVirtualTableFromMixinKlass(Klass *k) {
	MixinKlass o;
	k->set_vtbl_value(o.vtbl_value());
}

Oop MixinKlass::allocateObject(bool permit_scavenge, bool tenured) {
	KlassOop k = as_klassOop();
	std::int32_t size = non_indexable_size();
	// allocate
	Oop *result = basicAllocate(size, &k, permit_scavenge, tenured);
	if (result == nullptr)
		return nullptr;
	MixinOop obj = as_mixinOop(result);
	// header + instance variables
	MemOop(obj)->initialize_header(true, k);
	MemOop(obj)->initialize_body(MemOopDescriptor::header_size(), size);
	ObjectArrayOop filler = OopFactory::new_objectArray(std::int32_t{0});
	obj->set_methods(filler);
	obj->set_instVars(filler);
	obj->set_classVars(filler);
	obj->set_installed(falseObject);
	return obj;
}

KlassOop MixinKlass::create_subclass(MixinOop mixin, Format format) {
	if (format == Format::mem_klass or format == Format::mixin_klass) {
		return MixinKlass::create_class(as_klassOop(), mixin);
	}
	return nullptr;
}

KlassOop MixinKlass::create_class(KlassOop super_class, MixinOop mixin) {
	MixinKlass o;
	return create_generic_class(super_class, mixin, o.vtbl_value());
}

Oop MixinKlass::oop_shallow_copy(Oop obj, bool tenured) {
	MixinOop copy = MixinOop(MemOopKlass::oop_shallow_copy(obj, tenured));
	copy->set_installed(falseObject);
	return copy;
}

std::int32_t MixinKlass::oop_scavenge_contents(Oop obj) {
	std::int32_t size = non_indexable_size();
	// header + instance variables
	MemOop(obj)->scavenge_header();
	MemOop(obj)->scavenge_body(MemOopDescriptor::header_size(), size);
	return size;
}

std::int32_t MixinKlass::oop_scavenge_tenured_contents(Oop obj) {
	std::int32_t size = non_indexable_size();
	// header + instance variables
	MemOop(obj)->scavenge_tenured_header();
	MemOop(obj)->scavenge_tenured_body(MemOopDescriptor::header_size(), size);
	return size;
}

void MixinKlass::oop_follow_contents(Oop obj) {
	// header + instance variables
	MemOop(obj)->follow_header();
	MemOop(obj)->follow_body(MemOopDescriptor::header_size(), non_indexable_size());
}

void MixinKlass::oop_layout_iterate(Oop obj, ObjectLayoutClosure *blk) {
	// header
	MemOop(obj)->layout_iterate_header(blk);
	blk->do_oop("methods", (Oop *) &MixinOop(obj)->addr()->_methods);
	blk->do_oop("instVars", (Oop *) &MixinOop(obj)->addr()->_inst_vars);
	blk->do_oop("classVars", (Oop *) &MixinOop(obj)->addr()->_class_vars);
	blk->do_oop("primary invocation", (Oop *) &MixinOop(obj)->addr()->_primary_invocation);
	blk->do_oop("class mixin", (Oop *) &MixinOop(obj)->addr()->_class_mixin);
	// instance variables
	MemOop(obj)->layout_iterate_body(blk, MixinOopDescriptor::header_size(), non_indexable_size());
}

void MixinKlass::oop_oop_iterate(Oop obj, OopClosure *blk) {
	// header + instance variables
	MemOop(obj)->oop_iterate_header(blk);
	MemOop(obj)->oop_iterate_body(blk, MemOopDescriptor::header_size(), non_indexable_size());
}
