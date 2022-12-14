//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/primitive/primitive_declarations.hpp"
#include "vm/primitive/primitive_tracing.hpp"


// Primitives for mixins

class MixinOopPrimitives : AllStatic {
private:
	static void inc_calls() {
		number_of_calls++;
	}

public:
	static std::int32_t number_of_calls;

	// METHODS

	//%prim
	// <NoReceiver> primitiveMixinNumberOfMethodsOf: mixin <Mixin>
	//                                       ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
	//   Internal { name  = 'MixinOopPrimitives::number_of_methods' }
	//%
	static PRIM_DECL_1(number_of_methods, Oop mixin);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//                    methodAt: index     <SmallInteger>
	//                      ifFail: failBlock <PrimFailBlock> ^<Method> =
	//   Internal { error = #(OutOfBounds)
	//              name  = 'MixinOopPrimitives::method_at' }
	//%
	static PRIM_DECL_2(method_at, Oop mixin, Oop index);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//                   addMethod: method    <Method>
	//                      ifFail: failBlock <PrimFailBlock> ^<Method> =
	//   Internal { error = #(IsInstalled)
	//              name  = 'MixinOopPrimitives::add_method' }
	//%
	static PRIM_DECL_2(add_method, Oop mixin, Oop method);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//              removeMethodAt: index     <SmallInteger>
	//                      ifFail: failBlock <PrimFailBlock> ^<Method> =
	//   Internal {  error = #(IsInstalled OutOfBounds)
	//               name  = 'MixinOopPrimitives::remove_method_at' }
	//%
	static PRIM_DECL_2(remove_method_at, Oop mixin, Oop index);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//               methodsIfFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { name  = 'MixinOopPrimitives::methods' }
	//%
	static PRIM_DECL_1(methods, Oop mixin);


	// INSTANCE VARIABLES

	//%prim
	// <NoReceiver> primitiveMixinNumberOfInstanceVariablesOf: mixin <Mixin>
	//                                                 ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
	//   Internal { name = 'MixinOopPrimitives::number_of_instance_variables' }
	//%
	static PRIM_DECL_1(number_of_instance_variables, Oop mixin);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//          instanceVariableAt: index     <SmallInteger>
	//                      ifFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { error = #(OutOfBounds)
	//              name  = 'MixinOopPrimitives::instance_variable_at' }
	//%
	static PRIM_DECL_2(instance_variable_at, Oop mixin, Oop index);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//         addInstanceVariable: name      <Symbol>
	//                      ifFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { error = #(IsInstalled OutOfBounds)
	//              name = 'MixinOopPrimitives::add_instance_variable' }
	//%
	static PRIM_DECL_2(add_instance_variable, Oop mixin, Oop name);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//    removeInstanceVariableAt: index     <SmallInteger>
	//                      ifFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { error = #(IsInstalled OutOfBounds)
	//              name  = 'MixinOopPrimitives::remove_instance_variable_at' }
	//%
	static PRIM_DECL_2(remove_instance_variable_at, Oop mixin, Oop index);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//     instanceVariablesIfFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { name  = 'MixinOopPrimitives::instance_variables' }
	//%
	static PRIM_DECL_1(instance_variables, Oop mixin);

	// CLASS VARIABLES

	//%prim
	// <NoReceiver> primitiveMixinNumberOfClassVariablesOf: mixin <Mixin>
	//                                              ifFail: failBlock <PrimFailBlock> ^<SmallInteger> =
	//   Internal { name  = 'MixinOopPrimitives::number_of_class_variables' }
	//%
	static PRIM_DECL_1(number_of_class_variables, Oop mixin);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//             classVariableAt: index     <SmallInteger>
	//                      ifFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { error = #(OutOfBounds)
	//              name  = 'MixinOopPrimitives::class_variable_at' }
	//%
	static PRIM_DECL_2(class_variable_at, Oop mixin, Oop index);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//            addClassVariable: name      <Symbol>
	//                      ifFail: failBlock <PrimFailBlock> ^<Mixin> =
	//   Internal {  error = #(IsInstalled OutOfBounds)
	//               name = 'MixinOopPrimitives::add_class_variable' }
	//%
	static PRIM_DECL_2(add_class_variable, Oop mixin, Oop name);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//       removeClassVariableAt: index     <SmallInteger>
	//                      ifFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { error = #(IsInstalled OutOfBounds)
	//              name  = 'MixinOopPrimitives::remove_class_variable_at' }
	//%
	static PRIM_DECL_2(remove_class_variable_at, Oop mixin, Oop index);

	//%prim
	// <NoReceiver> primitiveMixin: mixin     <Mixin>
	//        classVariablesIfFail: failBlock <PrimFailBlock> ^<Symbol> =
	//   Internal { name  = 'MixinOopPrimitives::class_variables' }
	//%
	static PRIM_DECL_1(class_variables, Oop mixin);


	// PRIMARY INVOCATION

	//%prim
	// <NoReceiver> primitiveMixinPrimaryInvocationOf: mixin <Mixin>
	//                                         ifFail: failBlock <PrimFailBlock> ^<Class> =
	//   Internal { name = 'MixinOopPrimitives::primary_invocation' }
	//%
	static PRIM_DECL_1(primary_invocation, Oop mixin);

	//%prim
	// <NoReceiver> primitiveMixinSetPrimaryInvocationOf: mixin <Mixin>
	//                                                to: class <Class>
	//                                            ifFail: failBlock <PrimFailBlock> ^<Class> =
	//   Internal { error = #(IsInstalled)
	//              name  = 'MixinOopPrimitives::set_primary_invocation' }
	//%
	static PRIM_DECL_2(set_primary_invocation, Oop mixin, Oop klass);

	// CLASS MIXIN

	//%prim
	// <NoReceiver> primitiveMixinClassMixinOf: mixin <Mixin>
	//                                  ifFail: failBlock <PrimFailBlock> ^<Mixin> =
	//   Internal { name = 'MixinOopPrimitives::class_mixin' }
	//%
	static PRIM_DECL_1(class_mixin, Oop mixin);

	//%prim
	// <NoReceiver> primitiveMixinSetClassMixinOf: mixin      <Mixin>
	//                                         to: classMixin <Mixin>
	//                                     ifFail: failBlock <PrimFailBlock> ^<Mixin> =
	//   Internal { error = #(IsInstalled)
	//              name  = 'MixinOopPrimitives::set_class_mixin' }
	//%
	static PRIM_DECL_2(set_class_mixin, Oop mixin, Oop class_mixin);

	//%prim
	// <NoReceiver> primitiveMixinIsInstalled: mixin <Mixin>
	//                                 ifFail: failBlock <PrimFailBlock> ^<Boolean> =
	//   Internal { name = 'MixinOopPrimitives::is_installed' }
	//%
	static PRIM_DECL_1(is_installed, Oop mixin);

	//%prim
	// <NoReceiver> primitiveMixinSetInstalled: mixin <Mixin>
	//                                  ifFail: failBlock <PrimFailBlock> ^<Boolean> =
	//   Internal { name = 'MixinOopPrimitives::set_installed' }
	//%
	static PRIM_DECL_1(set_installed, Oop mixin);

	//%prim
	// <NoReceiver> primitiveMixinSetUnInstalled: mixin <Mixin>
	//                                    ifFail: failBlock <PrimFailBlock> ^<Boolean> =
	//   Internal { name = 'MixinOopPrimitives::set_uninstalled' }
	//%
	static PRIM_DECL_1(set_uninstalled, Oop mixin);

};
