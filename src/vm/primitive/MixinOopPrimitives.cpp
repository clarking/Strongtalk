//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitive/MixinOopPrimitives.hpp"
#include "vm/platform/platform.hpp"
#include "vm/utility/ObjectIDTable.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/oop/MixinOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/runtime/VMOperation.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/memory/Scavenge.hpp"

TRACE_FUNC(TraceMixinPrims, "mixin")

std::int32_t MixinOopPrimitives::number_of_calls;

PRIM_DECL_1(MixinOopPrimitives::number_of_methods, Oop mixin) {
	PROLOGUE_1("number_of_methods", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return smiOopFromValue(MixinOop(mixin)->number_of_methods());
}

PRIM_DECL_2(MixinOopPrimitives::method_at, Oop mixin, Oop index) {
	PROLOGUE_2("method_at", mixin, index);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	std::int32_t i = SmallIntegerOop(index)->value();
	if (i > 0 and i <= MixinOop(mixin)->number_of_methods())
		return MixinOop(mixin)->method_at(i);
	return markSymbol(vmSymbols::out_of_bounds());
}

PRIM_DECL_2(MixinOopPrimitives::add_method, Oop mixin, Oop method) {
	PROLOGUE_2("add_method", mixin, method);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not method->is_method())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	BlockScavenge bs;
	MixinOop(mixin)->add_method(MethodOop(method));
	return mixin;
}

PRIM_DECL_2(MixinOopPrimitives::remove_method_at, Oop mixin, Oop index) {
	PROLOGUE_2("remove_method_at", mixin, index);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	BlockScavenge bs;
	std::int32_t i = SmallIntegerOop(index)->value();
	if (i > 0 and i <= MixinOop(mixin)->number_of_methods())
		return MixinOop(mixin)->remove_method_at(i);
	return markSymbol(vmSymbols::out_of_bounds());
}

PRIM_DECL_1(MixinOopPrimitives::methods, Oop mixin) {
	PROLOGUE_1("methods", mixin);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	return MixinOop(mixin)->methods();
}

PRIM_DECL_1(MixinOopPrimitives::number_of_instance_variables, Oop mixin) {
	PROLOGUE_1("number_of_instance_variables", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return smiOopFromValue(MixinOop(mixin)->number_of_instVars());
}

PRIM_DECL_2(MixinOopPrimitives::instance_variable_at, Oop mixin, Oop index) {
	PROLOGUE_2("instance_variable_at", mixin, index);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	std::int32_t i = SmallIntegerOop(index)->value();
	if (i > 0 and i <= MixinOop(mixin)->number_of_instVars())
		return MixinOop(mixin)->instVar_at(i);
	return markSymbol(vmSymbols::out_of_bounds());
}

PRIM_DECL_2(MixinOopPrimitives::add_instance_variable, Oop mixin, Oop name) {
	PROLOGUE_2("add_instance_variable", mixin, name);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not name->isSymbol())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	BlockScavenge bs;
	MixinOop(mixin)->add_instVar(SymbolOop(name));
	return mixin;
}

PRIM_DECL_2(MixinOopPrimitives::remove_instance_variable_at, Oop mixin, Oop index) {
	PROLOGUE_2("remove_instance_variable_at", mixin, index);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	BlockScavenge bs;
	std::int32_t i = SmallIntegerOop(index)->value();
	if (i > 0 and i <= MixinOop(mixin)->number_of_instVars())
		return MixinOop(mixin)->remove_instVar_at(i);
	return markSymbol(vmSymbols::out_of_bounds());
}

PRIM_DECL_1(MixinOopPrimitives::instance_variables, Oop mixin) {
	PROLOGUE_1("instance_variables", mixin);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	return MixinOop(mixin)->instVars();
}

PRIM_DECL_1(MixinOopPrimitives::number_of_class_variables, Oop mixin) {
	PROLOGUE_1("number_of_class_variables", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return smiOopFromValue(MixinOop(mixin)->number_of_classVars());
}

PRIM_DECL_2(MixinOopPrimitives::class_variable_at, Oop mixin, Oop index) {
	PROLOGUE_2("class_variable_at", mixin, index);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	std::int32_t i = SmallIntegerOop(index)->value();
	if (i > 0 and i <= MixinOop(mixin)->number_of_classVars())
		return MixinOop(mixin)->classVar_at(i);
	return markSymbol(vmSymbols::out_of_bounds());
}

PRIM_DECL_2(MixinOopPrimitives::add_class_variable, Oop mixin, Oop name) {
	PROLOGUE_2("add_class_variable", mixin, name);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not name->isSymbol())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	BlockScavenge bs;
	MixinOop(mixin)->add_classVar(SymbolOop(name));
	return mixin;
}

PRIM_DECL_2(MixinOopPrimitives::remove_class_variable_at, Oop mixin, Oop index) {
	PROLOGUE_2("remove_class_variable_at", mixin, index);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	BlockScavenge bs;
	std::int32_t i = SmallIntegerOop(index)->value();
	if (i > 0 and i <= MixinOop(mixin)->number_of_classVars())
		return MixinOop(mixin)->remove_classVar_at(i);
	return markSymbol(vmSymbols::out_of_bounds());
}

PRIM_DECL_1(MixinOopPrimitives::class_variables, Oop mixin) {
	PROLOGUE_1("class_variables", mixin);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	return MixinOop(mixin)->classVars();
}

PRIM_DECL_1(MixinOopPrimitives::primary_invocation, Oop mixin) {
	PROLOGUE_1("primary_invocation", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return MixinOop(mixin)->primary_invocation();
}

PRIM_DECL_2(MixinOopPrimitives::set_primary_invocation, Oop mixin, Oop klass) {
	PROLOGUE_2("class_mixin", mixin, klass);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not klass->is_klass())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	MixinOop(mixin)->set_primary_invocation(KlassOop(klass));
	return mixin;
}

PRIM_DECL_1(MixinOopPrimitives::class_mixin, Oop mixin) {
	PROLOGUE_1("class_mixin", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return MixinOop(mixin)->class_mixin();
}

PRIM_DECL_2(MixinOopPrimitives::set_class_mixin, Oop mixin, Oop class_mixin) {
	PROLOGUE_2("class_mixin", mixin, class_mixin);
	// Check parameters
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (not class_mixin->is_mixin())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	// Check the mixin is not installed
	if (MixinOop(mixin)->is_installed())
		return markSymbol(vmSymbols::is_installed());

	MixinOop(mixin)->set_class_mixin(MixinOop(class_mixin));
	return mixin;
}

PRIM_DECL_1(MixinOopPrimitives::is_installed, Oop mixin) {
	PROLOGUE_1("is_installed", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return MixinOop(mixin)->is_installed() ? trueObject : falseObject;
}

PRIM_DECL_1(MixinOopPrimitives::set_installed, Oop mixin) {
	PROLOGUE_1("set_installed", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	MixinOop instance_mixin = MixinOop(mixin);
	MixinOop class_mixin = instance_mixin->class_mixin();

	instance_mixin->set_installed(trueObject);
	if (class_mixin->is_mixin()) {
		class_mixin->set_installed(trueObject);
	}
	return mixin;
}

PRIM_DECL_1(MixinOopPrimitives::set_uninstalled, Oop mixin) {
	PROLOGUE_1("set_uninstalled", mixin);
	// Check parameter
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	MixinOop(mixin)->set_installed(falseObject);
	return mixin;
}
