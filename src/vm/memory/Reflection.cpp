
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/memory/Reflection.hpp"
#include "vm/memory/Converter.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/code/Zone.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/runtime/Delta.hpp"
#include "vm/runtime/Timer.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/WaterMark.hpp"
#include "vm/runtime/Processes.hpp"

GrowableArray<ClassChange *> *Reflection::_classChanges = nullptr;
GrowableArray<MemOop> *Reflection::_converted = nullptr;

bool Reflection::needs_schema_change() {
	bool result = false;

	for (std::size_t i = 0; i < _classChanges->length(); i++) {
		bool sub_result = _classChanges->at(i)->needs_schema_change();
		if (TraceApplyChange and sub_result) {
			_classChanges->at(i)->old_klass()->print_value();
			_console->cr();
			SPDLOG_INFO("  needs schema change because: {}.", _classChanges->at(i)->reason_for_schema_change());
		}
		result = result or sub_result;
	}
	return result;
}

void Reflection::forward(MemOop old_obj, MemOop new_obj) {
	if (old_obj == new_obj) {
		return;
	}

	if (old_obj->is_forwarded()) {
		if (old_obj->forwardee() not_eq new_obj) {
			st_fatal("inconsistent forwarding");
		}
		return;
	}

	old_obj->forward_to(new_obj);
	_converted->append(old_obj);
}

bool Reflection::has_methods_changed(MixinOop new_mixin, MixinOop old_mixin) {

	if (new_mixin->number_of_methods() not_eq old_mixin->number_of_methods()) {
		return true;
	}

	for (std::size_t i = 1; i <= new_mixin->number_of_methods(); i++) {
		if (not old_mixin->includes_method(new_mixin->method_at(i)))
			return true;
	}

	return false;
}

bool Reflection::has_class_vars_changed(MixinOop new_mixin, MixinOop old_mixin) {

	if (new_mixin->number_of_classVars() not_eq old_mixin->number_of_classVars())
		return true;

	for (std::size_t i = 1; i <= new_mixin->number_of_classVars(); i++) {
		if (not old_mixin->includes_classVar(new_mixin->classVar_at(i))) {
			return true;
		}
	}

	return false;
}

ClassChange *Reflection::find_change_for(KlassOop klass) {

	for (std::size_t i = 0; i < _classChanges->length(); i++) {
		ClassChange *e = _classChanges->at(i);
		if (e->old_klass() == klass) {
			return e;
		}
	}

	return nullptr;
}

void Reflection::register_class_changes(MixinOop new_mixin, ObjectArrayOop invocations) {

	_classChanges = new GrowableArray<ClassChange *>(100);
	std::size_t length = invocations->length();

	for (std::size_t i = invocations_offset(); i <= length; i++) {
		ObjectArrayOop invocation = ObjectArrayOop(invocations->obj_at(i));
		st_assert(invocation->isObjectArray(), "type check");

		ClassChange *change = new ClassChange(KlassOop(invocation->obj_at(1)), new_mixin, Klass::format_from_symbol(SymbolOop(invocation->obj_at(2))), KlassOop(invocation->obj_at(3)));
		change->set_super_change(find_change_for(change->old_klass()->klass_part()->superKlass()));
		_classChanges->append(change);

		for (std::int32_t j = 4; j <= invocation->length() - 1; j += 2) {
			KlassOop old_klass = KlassOop(invocation->obj_at(j));
			change = new ClassChange(old_klass, Klass::format_from_symbol(SymbolOop(invocation->obj_at(j + 1))));
			change->set_super_change(find_change_for(old_klass->klass_part()->superKlass()));
			_classChanges->append(change);
		}
	}
}

void Reflection::invalidate_classes(bool value) {
	for (std::size_t i = 0; i < _classChanges->length(); i++) {
		KlassOop old_klass = _classChanges->at(i)->old_klass();
		old_klass->set_invalid(value);
		old_klass->klass()->set_invalid(value);
	}
}

void Reflection::update_classes(bool class_vars_changed, bool instance_methods_changed, bool class_methods_changed) {
	for (std::size_t i = 0; i < _classChanges->length(); i++) {
		_classChanges->at(i)->update_class(class_vars_changed, instance_methods_changed, class_methods_changed);
	}
}

void Reflection::setup_schema_change() {

	//
	for (std::size_t i = 0; i < _classChanges->length(); i++) {
		_classChanges->at(i)->setup_schema_change();
	}

	//
	for (std::size_t i = 0; i < _classChanges->length(); i++) {
		// Mark old class for schema change
		_classChanges->at(i)->old_klass()->klass_part()->mark_for_schema_change();
		// Mark old metaclass for schema change
		_classChanges->at(i)->old_klass()->klass()->klass_part()->mark_for_schema_change();
	}

}

void Reflection::apply_change(MixinOop new_mixin, MixinOop old_mixin, ObjectArrayOop invocations) {

	//
	ResourceMark resourceMark;
	if (TraceApplyChange) {
		SPDLOG_INFO("Reflective change  [new] {}  [old] {}", new_mixin->print_value_string(), old_mixin->print_value_string(), invocations->print_value_string());
		Universe::verify();
	}

	register_class_changes(new_mixin, invocations);

	invalidate_classes(true);

	// Invalidate compiled code
	Universe::code->mark_dependents_for_deoptimization();
	Processes::deoptimized_wrt_marked_nativeMethods();
	Universe::code->make_marked_nativeMethods_zombies();

	// check for change mixin format too
	bool format_changed = needs_schema_change();

	bool class_vars_changed = has_class_vars_changed(new_mixin, old_mixin);
	bool instance_methods_changed = has_methods_changed(new_mixin, old_mixin);
	bool class_methods_changed = has_methods_changed(new_mixin->class_mixin(), old_mixin->class_mixin());

	if (format_changed) {

		if (TraceApplyChange) {
			SPDLOG_INFO(" - schema change is needed");
		}

		_converted = new GrowableArray<MemOop>(100);

		setup_schema_change();

		// Do the transformation
		ConvertOopClosure blk;
		ConvertClosure bl;
		Universe::roots_do(&convert);
		Processes::oop_iterate(&bl);

		// Save top of to_space and old_gen
		NewWaterMark eden_mark = Universe::new_gen.eden()->top_mark();
		OldWaterMark old_mark = Universe::old_gen.top_mark();

		Universe::new_gen.object_iterate(&blk);
		Universe::old_gen.object_iterate(&blk);

		while (eden_mark not_eq Universe::new_gen.eden()->top_mark() or old_mark not_eq Universe::old_gen.top_mark()) {
			Universe::new_gen.eden()->object_iterate_from(&eden_mark, &blk);
			Universe::old_gen.object_iterate_from(&old_mark, &blk);
		}

		// NotificationQueue::oops_do(&follow_root);

		// Reset the marks for the converted objects
		for (std::size_t j = 0; j < _converted->length(); j++) {
			MemOop obj = _converted->at(j);
			if (TraceApplyChange) {
				//SPDLOG_INFO( "Old: 0x{0:x}, 0x{0:x}", static_cast<const void *>(obj), static_cast<const void *>(obj->mark()) );
				//SPDLOG_INFO( "New: 0x{0:x}, 0x{0:x}", static_cast<const void *>(obj->forwardee()), static_cast<const void *>(obj->forwardee()->mark()) );
			}
			obj->set_mark(obj->forwardee()->mark());
		}

		// Clear the static variables
		_converted = nullptr;

	}
	else {
		if (TraceApplyChange) {
			//SPDLOG_INFO( " - no schema change ({}{}{})", class_vars_changed ? "class variables " : "", instance_methods_changed ? "instance methods " : "", class_methods_changed ? "class methods " : "" );
		}
		update_classes(class_vars_changed, instance_methods_changed, class_methods_changed);
	}

	invalidate_classes(false);
	_classChanges = nullptr;

	// Clear inline caches
	Universe::flush_inline_caches_in_methods();
	Universe::code->clear_inline_caches();

	LookupCache::flush();
	DeltaCallCache::clearAll();

	if (TraceApplyChange)
		Universe::verify();
}

Oop Reflection::apply_change(ObjectArrayOop change) {
	TraceTime t("ApplyChange", TraceApplyChange);

	// [1]     = new-mixin   <Mixin>
	// [2]     = old-mixin   <Mixin>
	// [3 - n] = invocations <Array>

	// Check array format
	std::size_t length = change->length();

	if (length < 3)
		return markSymbol(vmSymbols::argument_is_invalid());

	MixinOop new_mixin = MixinOop(change->obj_at(1));
	if (not new_mixin->is_mixin())
		return markSymbol(vmSymbols::argument_is_invalid());

	MixinOop old_mixin = MixinOop(change->obj_at(2));
	if (not old_mixin->is_mixin())
		return markSymbol(vmSymbols::argument_is_invalid());

	for (std::size_t i = 3; i <= length; i++) {
		ObjectArrayOop array = ObjectArrayOop(change->obj_at(i));

		if (not array->isObjectArray())
			return markSymbol(vmSymbols::argument_is_invalid());

		if (array->length() < 3)
			return markSymbol(vmSymbols::argument_is_invalid());

		if (not array->obj_at(1)->is_klass())
			return markSymbol(vmSymbols::argument_is_invalid());

		if (not array->obj_at(2)->isSymbol())
			return markSymbol(vmSymbols::argument_is_invalid());

		if (not array->obj_at(3)->is_klass() and array->obj_at(3) not_eq nilObject)
			return markSymbol(vmSymbols::argument_is_invalid());

		for (std::int32_t j = 4; j <= array->length() - 1; j += 2) {
			if (not array->obj_at(j)->is_klass())
				return markSymbol(vmSymbols::argument_is_invalid());
			if (not array->obj_at(j + 1)->isSymbol())
				return markSymbol(vmSymbols::argument_is_invalid());
		}
	}

	apply_change(new_mixin, old_mixin, change);
	return trueObject;
}

void Reflection::convert(Oop *p) {
	Oop obj = *p;

	if (not obj->isMemOop())
		return;

	if (MemOop(obj)->is_forwarded()) {
		// slr mod: only update the memory card if reference is in object memory
		Universe::store(p, MemOop(obj)->forwardee(), Universe::is_heap(p));
		return;
	}

	if (MemOop(obj)->klass()->klass_part()->is_marked_for_schema_change()) {
		// slr mod: only update the memory card if reference is in object memory
		Universe::store(p, convert_object(MemOop(obj)), Universe::is_heap(p));
	}
}

MemOop Reflection::convert_object(MemOop obj) {
	st_assert(obj->klass()->klass_part()->is_marked_for_schema_change(), "just checking");
	ClassChange *change = find_change_for(obj->klass());
	st_assert(change, "change must be present");
	return change->converter()->convert(obj);
}
