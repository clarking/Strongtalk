//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitive/SystemPrimitives.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/memory/Reflection.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/oop/ProcessOopDescriptor.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/platform/os.hpp"
#include "vm/utility/Integer.hpp"
#include "vm/runtime/VMOperation.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/interpreter/DispatchTable.hpp"
#include "vm/memory/SnapshotDescriptor.hpp"
#include "vm/primitive/Primitives.hpp"
#include "vm/system/dll.hpp"
#include "vm/runtime/FlatProfiler.hpp"
#include "vm/klass/WeakArrayKlass.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/SlidingSystemAverage.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/oop/SmallIntegerOopDescriptor.hpp"

TRACE_FUNC(TraceSystemPrims, "system")

std::int32_t SystemPrimitives::number_of_calls;

PRIM_DECL_5(SystemPrimitives::createNamedInvocation, Oop mixin, Oop name, Oop primary, Oop superclass, Oop format) {
	PROLOGUE_5("createNamedInvocation", mixin, primary, name, superclass, format)

	// Check argument types
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not name->isSymbol())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	if (not(primary == trueObject or primary == falseObject))
		return markSymbol(vmSymbols::third_argument_has_wrong_type());

	if (not superclass->is_klass())
		return markSymbol(vmSymbols::fourth_argument_has_wrong_type());

	if (not format->isSymbol())
		return markSymbol(vmSymbols::fifth_argument_has_wrong_type());

	Klass::Format f = Klass::format_from_symbol(SymbolOop(format));
	if (f == Klass::Format::no_klass) {
		return markSymbol(vmSymbols::argument_is_invalid());
	}

	BlockScavenge bs;

	// Create the new klass
	KlassOop new_klass = KlassOop(superclass)->klass_part()->create_subclass(MixinOop(mixin), f);

	if (new_klass == nullptr) {
		// Create more detailed error message
		return markSymbol(vmSymbols::argument_is_invalid());
	}

	// Set the primary invocation if needed.
	if (primary == trueObject) {
		MixinOop(mixin)->set_primary_invocation(new_klass);
		MixinOop(mixin)->class_mixin()->set_primary_invocation(new_klass->klass());
		MixinOop(mixin)->set_installed(trueObject);
		MixinOop(mixin)->class_mixin()->set_installed(trueObject);
	}

	// Make sure mixin->classMixin is present

	// Add the global
	Universe::add_global(OopFactory::new_association(SymbolOop(name), new_klass, true));
	return new_klass;
}

PRIM_DECL_3(SystemPrimitives::createInvocation, Oop mixin, Oop superclass, Oop format) {
	PROLOGUE_3("createInvocation", mixin, superclass, format)

	// Check argument types
	if (not mixin->is_mixin())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not superclass->is_klass())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	if (not format->isSymbol())
		return markSymbol(vmSymbols::third_argument_has_wrong_type());

	BlockScavenge bs;

	Klass::Format f = Klass::format_from_symbol(SymbolOop(format));
	if (f == Klass::Format::no_klass) {
		return markSymbol(vmSymbols::argument_is_invalid());
	}

	KlassOop new_klass = KlassOop(superclass)->klass_part()->create_subclass(MixinOop(mixin), f);

	if (new_klass == nullptr) {
		// Create more detailed error message
		return markSymbol(vmSymbols::argument_is_invalid());
	}

	MixinOop(mixin)->set_installed(trueObject);
	MixinOop(mixin)->class_mixin()->set_installed(trueObject);

	return new_klass;
}

PRIM_DECL_1(SystemPrimitives::applyChange, Oop change) {
	PROLOGUE_1("applyChange", change)

	// Check argument types
	if (not change->isObjectArray())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	BlockScavenge bs;

	return Reflection::apply_change(ObjectArrayOop(change));
}

PRIM_DECL_0(SystemPrimitives::canScavenge) {
	PROLOGUE_0("canScavenge")

	return Universe::can_scavenge() ? trueObject : falseObject;
}

PRIM_DECL_1(SystemPrimitives::scavenge, Oop receiver) {
	PROLOGUE_1("scavenge", receiver)
	Oop rec = receiver;
	VM_Scavenge op(&rec);
	// The operation takes place in the vmProcess
	VMProcess::execute(&op);
	return rec;
}

PRIM_DECL_0(SystemPrimitives::oopSize) {
	PROLOGUE_0("OOP_SIZE")
	return smiOopFromValue(::OOP_SIZE);
}

PRIM_DECL_1(SystemPrimitives::garbageGollect, Oop receiver) {
	PROLOGUE_1("garbageGollect", receiver);
	Oop rec = receiver;
	VM_GarbageCollect op(&rec);
	// The operation takes place in the vmProcess
	VMProcess::execute(&op);
	return rec;
}

PRIM_DECL_1(SystemPrimitives::expandMemory, Oop sizeOop) {
	PROLOGUE_1("expandMemory", sizeOop);
	if (not sizeOop->isSmallIntegerOop())
		return markSymbol(vmSymbols::argument_has_wrong_type());
	std::int32_t size = SmallIntegerOop(sizeOop)->value();
	if (size < 0)
		return markSymbol(vmSymbols::argument_is_invalid());
	Universe::old_gen.expand(size);
	return trueObject;
}

PRIM_DECL_1(SystemPrimitives::shrinkMemory, Oop sizeOop) {
	PROLOGUE_1("shrinkMemory", sizeOop);
	if (not sizeOop->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	if (SmallIntegerOop(sizeOop)->value() < 0 or SmallIntegerOop(sizeOop)->value() > Universe::old_gen.free())
		return markSymbol(vmSymbols::value_out_of_range());
	Universe::old_gen.shrink(SmallIntegerOop(sizeOop)->value());
	return trueObject;
}

extern "C" std::int32_t expansion_count;
extern "C" void single_step_handler();

PRIM_DECL_0(SystemPrimitives::expansions) {
	PROLOGUE_0("expansions")
	return smiOopFromValue(expansion_count);
}

PRIM_DECL_0(SystemPrimitives::breakpoint) {
	PROLOGUE_0("breakpoint")
	{
		ResourceMark resourceMark;
		StubRoutines::setSingleStepHandler(&single_step_handler);
		DispatchTable::intercept_for_step(nullptr);
	}
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::vmbreakpoint) {
	PROLOGUE_0("vmbreakpoint")
	os::breakpoint();
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::getLastError) {
	PROLOGUE_0("getLastError")
	return smiOopFromValue(os::error_code()); //%TODO% fix this to support errors > 30 bits in length
}

PRIM_DECL_0(SystemPrimitives::halt) {
	PROLOGUE_0("halt")

	// I think this is obsolete, hlt is a privileged instruction, and Object>>halt uses
	// Processor stopWithError: ProcessHaltError new.
	// removed because inline assembly isn't portable -Marc 04/07

	Unimplemented();
	return markSymbol(vmSymbols::not_yet_implemented());

	//  __asm hlt
	//  return trueObject;
}

static Oop fake_time() {
	static std::int32_t time = 0;
	return OopFactory::new_double((double) time++);
}

PRIM_DECL_0(SystemPrimitives::userTime) {
	PROLOGUE_0("userTime")
	if (UseTimers) {
		os::updateTimes();
		return OopFactory::new_double(os::userTime());
	}
	else {
		return fake_time();
	}
}

PRIM_DECL_0(SystemPrimitives::systemTime) {
	PROLOGUE_0("systemTime")
	if (UseTimers) {
		os::updateTimes();
		return OopFactory::new_double(os::systemTime());
	}
	else {
		return fake_time();
	}
}

PRIM_DECL_0(SystemPrimitives::elapsedTime) {
	PROLOGUE_0("elapsedTime")
	if (UseTimers) {
		return OopFactory::new_double(os::elapsedTime());
	}
	else {
		return fake_time();
	}
}

PRIM_DECL_1(SystemPrimitives::writeSnapshot, Oop fileName) {
	PROLOGUE_1("writeSnapshot", fileName);
	SnapshotDescriptor sd;
	const char *name = "fisk.snap";
	sd.write_on(name);
	if (sd.has_error())
		return markSymbol(sd.error_symbol());
	return fileName;
}

PRIM_DECL_1(SystemPrimitives::globalAssociationKey, Oop receiver) {
	PROLOGUE_1("globalAssociationKey", receiver);
	st_assert(receiver->is_association(), "receiver must be association");
	return AssociationOop(receiver)->key();
}

PRIM_DECL_2(SystemPrimitives::globalAssociationSetKey, Oop receiver, Oop key) {
	PROLOGUE_2("globalAssociationSetKey", receiver, key);
	st_assert(receiver->is_association(), "receiver must be association");
	if (not key->isSymbol())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	AssociationOop(receiver)->set_key(SymbolOop(key));
	return receiver;
}

PRIM_DECL_1(SystemPrimitives::globalAssociationValue, Oop receiver) {
	PROLOGUE_1("globalAssociationValue", receiver);
	st_assert(receiver->is_association(), "receiver must be association");
	return AssociationOop(receiver)->value();
}

PRIM_DECL_2(SystemPrimitives::globalAssociationSetValue, Oop receiver, Oop value) {
	PROLOGUE_2("globalAssociationSetValue", receiver, value);
	st_assert(receiver->is_association(), "receiver must be association");
	AssociationOop(receiver)->set_value(value);
	return receiver;
}

PRIM_DECL_1(SystemPrimitives::globalAssociationIsConstant, Oop receiver) {
	PROLOGUE_1("globalAssociationIsConstant", receiver);
	st_assert(receiver->is_association(), "receiver must be association");
	return AssociationOop(receiver)->is_constant() ? trueObject : falseObject;
}

PRIM_DECL_2(SystemPrimitives::globalAssociationSetConstant, Oop receiver, Oop value) {
	PROLOGUE_2("globalAssociationSetConstant", receiver, value);
	st_assert(receiver->is_association(), "receiver must be association");
	Oop old_value = AssociationOop(receiver)->is_constant() ? trueObject : falseObject;

	if (value == trueObject)
		AssociationOop(receiver)->set_is_constant(true);
	else if (value == falseObject)
		AssociationOop(receiver)->set_is_constant(false);
	else
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	return old_value;
}

PRIM_DECL_1(SystemPrimitives::smalltalk_at, Oop index) {
	PROLOGUE_1("smalltalk_at", index);
	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not Universe::systemDictionaryObject()->is_within_bounds(SmallIntegerOop(index)->value()))
		return markSymbol(vmSymbols::out_of_bounds());

	return Universe::systemDictionaryObject()->obj_at(SmallIntegerOop(index)->value());
}

PRIM_DECL_2(SystemPrimitives::smalltalk_at_put, Oop key, Oop value) {
	PROLOGUE_2("smalltalk_at_put", key, value);

	BlockScavenge bs;
	AssociationOop assoc = OopFactory::new_association(SymbolOop(key), value, false);
	Universe::add_global(assoc);
	return assoc;
}

PRIM_DECL_1(SystemPrimitives::smalltalk_remove_at, Oop index) {
	PROLOGUE_1("smalltalk_remove_at", index);

	if (not index->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not Universe::systemDictionaryObject()->is_within_bounds(SmallIntegerOop(index)->value()))
		return markSymbol(vmSymbols::out_of_bounds());

	BlockScavenge bs;

	AssociationOop assoc = AssociationOop(Universe::systemDictionaryObject()->obj_at(SmallIntegerOop(index)->value()));
	Universe::remove_global_at(SmallIntegerOop(index)->value());
	return assoc;
}

PRIM_DECL_0(SystemPrimitives::smalltalk_size) {
	PROLOGUE_0("smalltalk_size");
	return smiOopFromValue(Universe::systemDictionaryObject()->length());
}

PRIM_DECL_0(SystemPrimitives::smalltalk_array) {
	PROLOGUE_0("smalltalk_array");
	return Universe::systemDictionaryObject();
}

PRIM_DECL_0(SystemPrimitives::quit) {
	PROLOGUE_0("quit");
	exit(EXIT_SUCCESS);
	return MarkOopDescriptor::bad();
}

PRIM_DECL_0(SystemPrimitives::printPrimitiveTable) {
	PROLOGUE_0("printPrimitiveTable");
	Primitives::print_table();
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::print_memory) {
	PROLOGUE_0("print_memory");
	Universe::print();
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::print_zone) {
	PROLOGUE_0("print_zone");
	Universe::code->print();
	return trueObject;
}

PRIM_DECL_1(SystemPrimitives::defWindowProc, Oop resultProxy) {
	PROLOGUE_1("defWindowProc", resultProxy);
	if (not resultProxy->is_proxy())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	SPDLOG_INFO("Please use the new Platform DLLLookup system to retrieve DefWindowProcA");
	dll_func_ptr_t func = DLLs::lookup(OopFactory::new_symbol("user"), OopFactory::new_symbol("DefWindowProcA"));
	ProxyOop(resultProxy)->set_pointer((void *) func);
	return resultProxy;
}

PRIM_DECL_1(SystemPrimitives::windowsHInstance, Oop resultProxy) {
	PROLOGUE_1("windowsHInstance", resultProxy);
	if (not resultProxy->is_proxy())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	ProxyOop(resultProxy)->set_pointer(os::get_hInstance());
	return resultProxy;
}

PRIM_DECL_1(SystemPrimitives::windowsHPrevInstance, Oop resultProxy) {
	PROLOGUE_1("windowsHPrevInstance", resultProxy);
	if (not resultProxy->is_proxy())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());
	ProxyOop(resultProxy)->set_pointer(os::get_prevInstance());
	return resultProxy;
}

PRIM_DECL_0(SystemPrimitives::windowsNCmdShow) {
	PROLOGUE_0("windowsNCmdShow");
	return smiOopFromValue(os::get_nCmdShow());
}

PRIM_DECL_1(SystemPrimitives::characterFor, Oop value) {
	PROLOGUE_1("characterFor", value);

	// check value type
	if (not value->isSmallIntegerOop())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if ((std::uint32_t) SmallIntegerOop(value)->value() < 256)
		// return the n+1'th element in asciiCharacter
		return Universe::asciiCharacters()->obj_at(SmallIntegerOop(value)->value() + 1);
	else
		return markSymbol(vmSymbols::out_of_bounds());
}

PRIM_DECL_0(SystemPrimitives::traceStack) {
	PROLOGUE_0("traceStack");
	DeltaProcess::active()->trace_stack();
	return trueObject;
}

// Flat Profiler Primitives

PRIM_DECL_0(SystemPrimitives::flat_profiler_reset) {
	PROLOGUE_0("flat_profiler_reset");
	FlatProfiler::reset();
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::flat_profiler_process) {
	PROLOGUE_0("flat_profiler_process");
	DeltaProcess *proc = FlatProfiler::process();
	return proc == nullptr ? nilObject : proc->processObject();
}

PRIM_DECL_1(SystemPrimitives::flat_profiler_engage, Oop process) {
	PROLOGUE_1("flat_profiler_engage", process);

	// check value type
	if (not process->is_process())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	FlatProfiler::engage(ProcessOop(process)->process());
	return process;
}

PRIM_DECL_0(SystemPrimitives::flat_profiler_disengage) {
	PROLOGUE_0("flat_profiler_disengage");
	DeltaProcess *proc = FlatProfiler::disengage();
	return proc == nullptr ? nilObject : proc->processObject();
}

PRIM_DECL_0(SystemPrimitives::flat_profiler_print) {
	PROLOGUE_0("flat_profiler_print");
	FlatProfiler::print(0);
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::notificationQueueGet) {
	PROLOGUE_0("notificationQueueGet");
	if (NotificationQueue::is_empty())
		return markSymbol(vmSymbols::empty_queue());
	return NotificationQueue::get();
}

PRIM_DECL_1(SystemPrimitives::notificationQueuePut, Oop value) {
	PROLOGUE_1("notificationQueuePut", value);
	NotificationQueue::put(value);
	return value;
}

PRIM_DECL_1(SystemPrimitives::hadNearDeathExperience, Oop value) {
	PROLOGUE_1("hadNearDeathExperience", value);
	return (value->isMemOop() and MemOop(value)->mark()->is_near_death()) ? trueObject : falseObject;
}

PRIM_DECL_2(SystemPrimitives::dll_setup, Oop receiver, Oop selector) {
	PROLOGUE_2("dll_setup", receiver, selector);

	if (not selector->isSymbol())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	if (SymbolOop(selector)->number_of_arguments() not_eq 2)
		return markSymbol(vmSymbols::failed());

	Universe::set_dll_lookup(receiver, SymbolOop(selector));
	return receiver;
}

PRIM_DECL_3(SystemPrimitives::dll_lookup, Oop name, Oop library, Oop result) {
	PROLOGUE_3("dll_lookup", name, library, result);

	if (not name->isSymbol())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not library->is_proxy())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	if (not result->is_proxy())
		return markSymbol(vmSymbols::third_argument_has_wrong_type());

	dll_func_ptr_t res = DLLs::lookup(SymbolOop(name), (DLL *) ProxyOop(library)->get_pointer());
	if (res) {
		ProxyOop(result)->set_pointer((void *) res);
		return result;
	}
	else {
		return markSymbol(vmSymbols::not_found());
	}
}

PRIM_DECL_2(SystemPrimitives::dll_load, Oop name, Oop library) {
	PROLOGUE_2("dll_load", name, library);

	if (not name->isSymbol())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not library->is_proxy())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	DLL *res = DLLs::load(SymbolOop(name));
	if (res) {
		ProxyOop(library)->set_pointer(res);
		return library;
	}
	else {
		return markSymbol(vmSymbols::not_found());
	}
}

PRIM_DECL_1(SystemPrimitives::dll_unload, Oop library) {
	PROLOGUE_1("dll_unload", library);

	if (not library->is_proxy())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	return DLLs::unload((DLL *) ProxyOop(library)->get_pointer()) ? library : markSymbol(vmSymbols::failed());
}

// Inlining Database

PRIM_DECL_0(SystemPrimitives::inlining_database_directory) {
	PROLOGUE_0("inlining_database_directory");
	return OopFactory::new_symbol(InliningDatabase::directory());
}

PRIM_DECL_1(SystemPrimitives::inlining_database_set_directory, Oop name) {
	PROLOGUE_1("inlining_database_set_directory", name);

	// Check type on argument
	if (not name->isByteArray() and not name->isDoubleByteArray())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	ResourceMark resourceMark;

	std::int32_t len = name->isByteArray() ? ByteArrayOop(name)->length() : DoubleByteArrayOop(name)->length();
	char *str = new_c_heap_array<char>(len + 1);
	name->isByteArray() ? ByteArrayOop(name)->copy_null_terminated(str, len + 1) : DoubleByteArrayOop(name)->copy_null_terminated(str, len + 1);
	// Potential memory leak, but this is temporary
	InliningDatabase::set_directory(str);
	return trueObject;
}

PRIM_DECL_1(SystemPrimitives::inlining_database_file_out_class, Oop receiver_class) {
	PROLOGUE_1("inlining_database_file_out_class", receiver_class);

	// Check type on argument
	if (not receiver_class->is_klass())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	// File out the class
	return smiOopFromValue(InliningDatabase::file_out(KlassOop(receiver_class)));
}

PRIM_DECL_0(SystemPrimitives::inlining_database_file_out_all) {
	PROLOGUE_0("inlining_database_file_out_all");

	// File out all nativeMethods
	return smiOopFromValue(InliningDatabase::file_out_all());
}

PRIM_DECL_1(SystemPrimitives::inlining_database_compile, Oop file_name) {
	PROLOGUE_1("inlining_database_compile", file_name);

	// Check type on argument
	if (not file_name->isByteArray() and not file_name->isDoubleByteArray())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	ResourceMark resourceMark;

	std::int32_t len = file_name->isByteArray() ? ByteArrayOop(file_name)->length() : DoubleByteArrayOop(file_name)->length();
	char *str = new_resource_array<char>(len + 1);
	file_name->isByteArray() ? ByteArrayOop(file_name)->copy_null_terminated(str, len + 1) : DoubleByteArrayOop(file_name)->copy_null_terminated(str, len + 1);

	RecompilationScope *rs = InliningDatabase::file_in(str);
	if (rs) {
		// Remove old NativeMethod if present
		NativeMethod *old_nm = Universe::code->lookup(rs->key());
		if (old_nm) {
			old_nm->makeZombie(false);
		}

		VM_OptimizeRScope op(rs);
		VMProcess::execute(&op);

		if (TraceInliningDatabase) {
			SPDLOG_INFO("compiling [{}] completed", str);
			SPDLOG_INFO("[Database]");
			rs->print_inlining_database_on(_console, nullptr, -1, 0);
			SPDLOG_INFO("[Compiled method]");
			op.result()->print_inlining_database_on(_console);
		}
	}
	else {
		if (TraceInliningDatabase) {
			SPDLOG_INFO("compiling [{}] failed", str);
		}
	}
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::inlining_database_compile_next) {
	PROLOGUE_0("inlining_database_compile_next");
	if (not UseInliningDatabase) {
		return falseObject;
	}

	bool end_of_table;
	RecompilationScope *rs = InliningDatabase::select_and_remove(&end_of_table);
	if (rs) {
		VM_OptimizeRScope op(rs);
		VMProcess::execute(&op);
		if (TraceInliningDatabase) {
			_console->print("Compiling ");
			op.result()->_lookupKey.print_on(_console);
			//SPDLOG_INFO( " in background = 0x{0:x}", static_cast<const void *>(op.result()) );
		}
	}
	return end_of_table ? falseObject : trueObject;
}

PRIM_DECL_1(SystemPrimitives::inlining_database_mangle, Oop name) {
	PROLOGUE_1("inlining_database_mangle", name);

	// Check type on argument
	if (not name->isByteArray() and not name->isDoubleByteArray())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	ResourceMark resourceMark;

	std::int32_t len = name->isByteArray() ? ByteArrayOop(name)->length() : DoubleByteArrayOop(name)->length();
	char *str = new_resource_array<char>(len + 1);
	name->isByteArray() ? ByteArrayOop(name)->copy_null_terminated(str, len + 1) : DoubleByteArrayOop(name)->copy_null_terminated(str, len + 1);
	return OopFactory::new_byteArray(InliningDatabase::mangle_name(str));
}

PRIM_DECL_1(SystemPrimitives::inlining_database_demangle, Oop name) {
	PROLOGUE_1("inlining_database_demangle", name);
	// Check type on argument
	if (not name->isByteArray() and not name->isDoubleByteArray())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	ResourceMark resourceMark;

	std::int32_t len = name->isByteArray() ? ByteArrayOop(name)->length() : DoubleByteArrayOop(name)->length();
	char *str = new_resource_array<char>(len + 1);
	name->isByteArray() ? ByteArrayOop(name)->copy_null_terminated(str, len + 1) : DoubleByteArrayOop(name)->copy_null_terminated(str, len + 1);
	return OopFactory::new_byteArray(InliningDatabase::unmangle_name(str));
}

PRIM_DECL_2(SystemPrimitives::inlining_database_add_entry, Oop receiver_class, Oop method_selector) {
	PROLOGUE_2("inlining_database_add_entry", receiver_class, method_selector);

	// Check type of argument
	if (not receiver_class->is_klass())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	// Check type of argument
	if (not method_selector->isSymbol())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	LookupKey key(KlassOop(receiver_class), method_selector);
	InliningDatabase::add_lookup_entry(&key);
	return receiver_class;
}

PRIM_DECL_0(SystemPrimitives::sliding_system_average) {
	PROLOGUE_0("system_sliding_average");

	if (not UseSlidingSystemAverage)
		return markSymbol(vmSymbols::not_active());

//    std::uint32_t * array = SlidingSystemAverage::update();
	std::array<std::uint32_t, SlidingSystemAverage::number_of_cases> _array = SlidingSystemAverage::update();

	ObjectArrayOop result = OopFactory::new_objectArray(SlidingSystemAverage::number_of_cases - 1);

	for (std::size_t i = 1; i < SlidingSystemAverage::number_of_cases; i++) {
		result->obj_at_put(i, smiOopFromValue(_array[i]));
	}

	return result;
}


// Enumeration primitives
// - it is important to exclude contextOops since they should be invisible to the Smalltalk level.

class InstancesOfClosure : public ObjectClosure {

public:
	InstancesOfClosure(KlassOop target, std::int32_t limit) :
			_target{target},
			_limit{limit},
			_result{nullptr} {
		_result = new GrowableArray<Oop>(100);
	}

	InstancesOfClosure() = default;
	virtual ~InstancesOfClosure() = default;
	InstancesOfClosure(const InstancesOfClosure &) = default;
	InstancesOfClosure &operator=(const InstancesOfClosure &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	KlassOop _target;
	std::int32_t _limit;
	GrowableArray<Oop> *_result;

	void do_object(MemOop obj) {
		if (obj->klass() == _target) {
			if (_result->length() < _limit and not obj->is_context()) {
				_result->append(obj);
			}
		}
	}

};

PRIM_DECL_2(SystemPrimitives::instances_of, Oop klass, Oop limit) {
	PROLOGUE_2("instances_of", klass, limit);

	// Check type of argument
	if (not klass->is_klass())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not limit->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	BlockScavenge bs;
	ResourceMark rm;

	InstancesOfClosure blk(KlassOop(klass), SmallIntegerOop(limit)->value());
	Universe::object_iterate(&blk);

	std::int32_t length = blk._result->length();
	ObjectArrayOop result = OopFactory::new_objectArray(length);

	for (std::size_t i = 1; i <= length; i++) {
		result->obj_at_put(i, blk._result->at(i - 1));
	}

	return result;
}

class ConvertClosure : public OopClosure {

private:
	void do_oop(Oop *o) {
		Reflection::convert(o);
	}
};

class HasReferenceClosure : public OopClosure {

private:
	Oop _target;

public:
	HasReferenceClosure(Oop target) :
			_target{target},
			_result{false} {
	}

	HasReferenceClosure() = default;
	virtual ~HasReferenceClosure() = default;
	HasReferenceClosure(const HasReferenceClosure &) = default;
	HasReferenceClosure &operator=(const HasReferenceClosure &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	void do_oop(Oop *o) {
		if (*o == _target)
			_result = true;
	}

	bool _result;
};

class ReferencesToClosure : public ObjectClosure {

public:
	ReferencesToClosure(Oop target, std::int32_t limit) :
			_limit{limit},
			_target{target},
			_result{nullptr} {
		_result = new GrowableArray<Oop>(100);
	}

	ReferencesToClosure() = default;
	virtual ~ReferencesToClosure() = default;
	ReferencesToClosure(const ReferencesToClosure &) = default;
	ReferencesToClosure &operator=(const ReferencesToClosure &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	std::int32_t _limit;
	Oop _target;
	GrowableArray<Oop> *_result;

	bool has_reference(MemOop obj) {
		HasReferenceClosure blk(_target);
		obj->oop_iterate(&blk);
		return blk._result;
	}

	void do_object(MemOop obj) {
		if (has_reference(obj)) {
			if (_result->length() < _limit and not obj->is_context()) {
				_result->append(obj);
			}
		}
	}
};

PRIM_DECL_2(SystemPrimitives::references_to, Oop obj, Oop limit) {
	PROLOGUE_2("references_to", obj, limit);

	// Check type of argument
	if (not limit->isSmallIntegerOop()) {
		return markSymbol(vmSymbols::second_argument_has_wrong_type());
	}

	BlockScavenge bs;
	ResourceMark rm;

	ReferencesToClosure blk(obj, SmallIntegerOop(limit)->value());
	Universe::object_iterate(&blk);

	std::int32_t length = blk._result->length();
	ObjectArrayOop result = OopFactory::new_objectArray(length);
	for (std::int32_t index = 1; index <= length; index++) {
		result->obj_at_put(index, blk._result->at(index - 1));
	}
	return result;
}

class HasInstanceReferenceClosure : public OopClosure {

private:
	KlassOop _target;

public:
	HasInstanceReferenceClosure(KlassOop target) :
			_target{target},
			_result{false} {
	}

	HasInstanceReferenceClosure() = default;
	virtual ~HasInstanceReferenceClosure() = default;
	HasInstanceReferenceClosure(const HasInstanceReferenceClosure &) = default;
	HasInstanceReferenceClosure &operator=(const HasInstanceReferenceClosure &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	void do_oop(Oop *o) {
		if ((*o)->klass() == _target) {
			_result = true;
		}
	}

	bool _result;
};

class ReferencesToInstancesOfClosure : public ObjectClosure {

public:
	ReferencesToInstancesOfClosure(KlassOop target, std::int32_t limit) :
			_result{new GrowableArray<Oop>(100)},
			_target{target},
			_limit{limit} {
	};

	ReferencesToInstancesOfClosure() = default;
	virtual ~ReferencesToInstancesOfClosure() = default;
	ReferencesToInstancesOfClosure(const ReferencesToInstancesOfClosure &) = default;
	ReferencesToInstancesOfClosure &operator=(const ReferencesToInstancesOfClosure &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	GrowableArray<Oop> *_result;
	KlassOop _target;
	std::int32_t _limit;

	bool has_reference(MemOop obj) {
		HasInstanceReferenceClosure blk(_target);
		obj->oop_iterate(&blk);
		return blk._result;
	}

	void do_object(MemOop obj) {
		if (has_reference(obj)) {
			if (_result->length() < _limit and not obj->is_context()) {
				_result->append(obj);
			}
		}
	}

};

PRIM_DECL_2(SystemPrimitives::references_to_instances_of, Oop klass, Oop limit) {
	PROLOGUE_2("references_to_instances_of", klass, limit);

	// Check type of argument
	if (not klass->is_klass())
		return markSymbol(vmSymbols::first_argument_has_wrong_type());

	if (not limit->isSmallIntegerOop())
		return markSymbol(vmSymbols::second_argument_has_wrong_type());

	BlockScavenge bs;
	ResourceMark rm;

	ReferencesToInstancesOfClosure blk(KlassOop(klass), SmallIntegerOop(limit)->value());
	Universe::object_iterate(&blk);

	std::int32_t length = blk._result->length();
	ObjectArrayOop result = OopFactory::new_objectArray(length);

	for (std::int32_t index = 1; index <= length; index++) {
		result->obj_at_put(index, blk._result->at(index - 1));
	}

	return result;
}

class AllObjectsClosure : public ObjectClosure {
public:
	AllObjectsClosure(std::int32_t limit) :
			_limit{limit},
			_result{nullptr} {

		_result = new GrowableArray<Oop>(20000);
	}

	AllObjectsClosure() = default;
	virtual ~AllObjectsClosure() = default;
	AllObjectsClosure(const AllObjectsClosure &) = default;
	AllObjectsClosure &operator=(const AllObjectsClosure &) = default;

	void operator delete(void *ptr) {
		(void) ptr;
	}

	std::int32_t _limit;
	GrowableArray<Oop> *_result;

	void do_object(MemOop obj) {
		if (_result->length() < _limit and not obj->is_context()) {
			_result->append(obj);
		}
	}

};

PRIM_DECL_1(SystemPrimitives::all_objects, Oop limit) {
	PROLOGUE_1("all_objects", limit);

	// Check type of argument
	if (not limit->isSmallIntegerOop()) {
		return markSymbol(vmSymbols::second_argument_has_wrong_type());
	}

	BlockScavenge bs;
	ResourceMark rm;

	AllObjectsClosure blk(SmallIntegerOop(limit)->value());
	Universe::object_iterate(&blk);

	std::int32_t length = blk._result->length();
	ObjectArrayOop result = OopFactory::new_objectArray(length);

	for (std::int32_t index = 1; index <= length; index++) {
		result->obj_at_put(index, blk._result->at(index - 1));
	}

	return result;
}

PRIM_DECL_0(SystemPrimitives::flush_code_cache) {
	PROLOGUE_0("flush_code_cache");
	Universe::code->flush();
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::flush_dead_code) {
	PROLOGUE_0("flush_dead_code");
	Universe::code->flushZombies();
	return trueObject;
}

PRIM_DECL_0(SystemPrimitives::command_line_args) {
	PROLOGUE_0("command_line_args");

	std::int32_t argc = os::argc();
	char **argv = os::argv();

	ObjectArrayOop result = OopFactory::new_objectArray(argc);
	result->set_length(argc);

	for (std::size_t i = 0; i < argc; i++) {
		ByteArrayOop arg = OopFactory::new_byteArray(argv[i]);
		result->obj_at_put(i + 1, arg);
	}

	return result;
}

PRIM_DECL_0(SystemPrimitives::current_thread_id) {
	PROLOGUE_0("current_thread_id");
	return smiOopFromValue(os::current_thread_id());
}

PRIM_DECL_0(SystemPrimitives::object_memory_size) {
	PROLOGUE_0("object_memory_size");
	return OopFactory::new_double(double(Universe::old_gen.used()) / Universe::old_gen.capacity());
}

PRIM_DECL_0(SystemPrimitives::freeSpace) {
	PROLOGUE_0("freeSpace");
	return smiOopFromValue(Universe::old_gen.free());
}

PRIM_DECL_0(SystemPrimitives::nurseryFreeSpace) {
	PROLOGUE_0("nurseryFreeSpace");
	return smiOopFromValue(Universe::new_gen.eden()->free());
}

PRIM_DECL_1(SystemPrimitives::alienMalloc, Oop size) {
	PROLOGUE_0("alienMalloc");

	if (not size->isSmallIntegerOop())
		return markSymbol(vmSymbols::argument_has_wrong_type());

	std::int32_t theSize = SmallIntegerOop(size)->value();
	if (theSize <= 0)
		return markSymbol(vmSymbols::argument_is_invalid());

	return smiOopFromValue((std::int32_t) malloc(theSize));
}

PRIM_DECL_1(SystemPrimitives::alienCalloc, Oop size) {
	PROLOGUE_0("alienCalloc");
	if (not size->isSmallIntegerOop())
		return markSymbol(vmSymbols::argument_has_wrong_type());

	std::int32_t theSize = SmallIntegerOop(size)->value();
	if (theSize <= 0)
		return markSymbol(vmSymbols::argument_is_invalid());

	return smiOopFromValue((std::int32_t) calloc(SmallIntegerOop(size)->value(), 1));
}

PRIM_DECL_1(SystemPrimitives::alienFree, Oop address) {
	PROLOGUE_0("alienFree");

	if (not address->isSmallIntegerOop() and not(address->isByteArray() and address->klass() == Universe::find_global("LargeInteger")))
		return markSymbol(vmSymbols::argument_has_wrong_type());

	if (address->isSmallIntegerOop()) {
		if (SmallIntegerOop(address)->value() == 0)
			return markSymbol(vmSymbols::argument_is_invalid());

		free((void *) SmallIntegerOop(address)->value());

	}
	else { // LargeInteger
		BlockScavenge bs;
		Integer *largeAddress = &ByteArrayOop(address)->number();
		bool ok;

		std::int32_t intAddress = largeAddress->as_int32_t(ok);
		if (intAddress == 0 or not ok)
			return markSymbol(vmSymbols::argument_is_invalid());

		free((void *) intAddress);
	}

	return trueObject;
}
