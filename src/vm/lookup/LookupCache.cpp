
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/lookup/LookupCache.hpp"
#include "vm/lookup/CacheElement.hpp"
#include "vm/runtime/VMOperation.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/Sweeper.hpp"
#include "vm/compiler/RecompilationScope.hpp"

std::int32_t LookupCache::number_of_primary_hits;
std::int32_t LookupCache::number_of_secondary_hits;
std::int32_t LookupCache::number_of_misses;

static std::array<CacheElement, primary_cache_size> primary;
static std::array<CacheElement, secondary_cache_size> secondary;

address_t LookupCache::primary_cache_address() {
	return address_t(&primary[0]);
}

address_t LookupCache::secondary_cache_address() {
	return address_t(&secondary[0]);
}

void LookupCache::flush() {

	// Clear primary cache
	for (std::size_t i = 0; i < primary_cache_size; i++)
		primary[i].clear();

	// Clear secondary cache
	for (std::size_t i = 0; i < secondary_cache_size; i++)
		secondary[i].clear();

	// Clear counters
	number_of_primary_hits = 0;
	number_of_secondary_hits = 0;
	number_of_misses = 0;
}

void LookupCache::flush(LookupKey *key) {
	// Flush the entry associated the the lookup key
	std::int32_t primary_index = hash_value(key) % primary_cache_size;
	std::int32_t secondary_index = primary_index % secondary_cache_size;

	// If we have a hit in the primary...
	if (primary[primary_index]._lookupKey.equal(key)) {
		// promote the value in the secondary to the primary
		primary[primary_index] = secondary[secondary_index];

		// clear the secondary
		secondary[secondary_index].clear();
		return;
	}

	if (secondary[secondary_index]._lookupKey.equal(key)) {
		// If we have a secondary hit clear the entry
		secondary[secondary_index].clear();
	}
}

void LookupCache::verify() {

	for (std::size_t i = 0; i < primary_cache_size; i++)
		primary[i].verify();

	for (std::size_t i = 0; i < secondary_cache_size; i++)
		secondary[i].verify();

}

std::uint32_t LookupCache::hash_value(LookupKey *key) {
	return ((std::uint32_t) key->klass() ^ (std::uint32_t) key->selector_or_method()) / sizeof(CacheElement);
}

LookupResult LookupCache::lookup_probe(LookupKey *key) {
	st_assert(key->verify(), "Lookupkey: verify failed");

	std::int32_t primary_index = hash_value(key) % primary_cache_size;

	if (primary[primary_index]._lookupKey.equal(key)) {
		return primary[primary_index]._lookupResult;
	}

	std::int32_t secondary_index = primary_index % secondary_cache_size;

	if (secondary[secondary_index]._lookupKey.equal(key)) {
		CacheElement tmp;
		// swap primary <-> secondary.
		tmp = primary[primary_index];

		primary[primary_index] = secondary[secondary_index];

		secondary[secondary_index] = tmp;
		return primary[primary_index]._lookupResult;
	}

	//
	LookupResult nothing;
	return nothing;
}

LookupResult LookupCache::lookup(LookupKey *key, bool compile) {

	// The cache is implemented as a 2-way associative cache.
	// Recipe for finding a lookup result.
	//  1. Check the primary cache. If hit return result.
	//  2. Check the secondary cache. If hit swap primary and secondary and return result.
	//  3. Move primary to secondary. Perform a manual lookup, place the result in primary and return the result.

	st_assert(key->verify(), "Lookupkey: verify failed");

	// 1. primary entry
	std::int32_t primary_index = hash_value(key) % primary_cache_size;

	if (primary[primary_index]._lookupKey.equal(key)) {
		// this is a good place to put conditional breakpoints using number_of_primary_hits
		number_of_primary_hits++;
		return primary[primary_index]._lookupResult;
	}

	// 2. secondary entry
	std::int32_t secondary_index = primary_index % secondary_cache_size;
	if (secondary[secondary_index]._lookupKey.equal(key)) {
		number_of_secondary_hits++;
		CacheElement tmp;
		// swap primary <-> secondary.
		tmp = primary[primary_index];
		primary[primary_index] = secondary[secondary_index];
		secondary[secondary_index] = tmp;
		return primary[primary_index]._lookupResult;
	}

	// 3. lookup cache miss
	number_of_misses++;
	LookupResult result = cache_miss_lookup(key, compile);
	if (not result.is_empty()) {
		if (UseInliningDatabaseEagerly and result.is_method() and InliningDatabase::lookup(key)) {
			// don't update the cache during inliningDB compiles if the result is a methodOop
			// contained in the inlining DB -- otherwise method won't be compiled eagerly
			st_assert(theCompiler, "should only happen during compilation");   // otherwise ic lookup is broken
		}
		else {
			secondary[secondary_index] = primary[primary_index];
			primary[primary_index].initialize(key, result);
			primary[primary_index].verify();
		}
	}

	return result;
}

LookupResult LookupCache::cache_miss_lookup(LookupKey *key, bool compile) {

	// Tracing
	if (TraceLookupAtMiss) {
		ResourceMark resourceMark;
		key->print();
	}

	if (compile and Sweeper::is_running()) {
		// Do not compile if this is called from the sweeper.
		// The system will crash and burn since a compile does a process switch
		// Lars Bak, 6-19-96 (I don't like overwriting parameters either!)
		compile = false;
	}

	// Check Inlining database
	if (UseInliningDatabase and UseInliningDatabaseEagerly and compile) {
		ResourceMark rm;
		RecompilationScope *rs = InliningDatabase::lookup_and_remove(key);
		if (rs) {
			if (TraceInliningDatabase) {
				_console->print("ID compile: ");
				key->print();
				_console->cr();
			}

			// Remove old NativeMethod if present
			NativeMethod *old_nm = Universe::code->lookup(rs->key());
			VM_OptimizeRScope op(rs);
			VMProcess::execute(&op);
			if (old_nm)
				old_nm->makeZombie(true);

			if (op.result()) {
				LookupResult result(op.result());
				return result;
			}
		}
	}

	// Check the code table
	const NativeMethod *nm = Universe::code->lookup(key);
	if (nm) {
		LookupResult result(nm); // unnecessary?
		// was: return nm;
		return result;
	}

	// Last resort is searching class for the method
	MethodOop method = key->is_normal_type() ? key->klass()->klass_part()->lookup(key->selector()) : key->method();

	if (not method) {
		LookupResult result;
		return result;
	}

	if (CompiledCodeOnly and compile) {
		nm = compile_method(key, method);
		if (nm) {
			LookupResult result(nm); // unnecessary?
			return nm;
		}
	}

	LookupResult result(method); // unnecessary
	// was: return method;
	return result;
}

MethodOop LookupCache::compile_time_normal_lookup(KlassOop receiver_klass, SymbolOop selector) {
	LookupKey key(receiver_klass, selector);
	LookupResult res = lookup(&key, false);
	return res.method_or_null();
}

MethodOop LookupCache::compile_time_super_lookup(KlassOop receiver_klass, SymbolOop selector) {
	MethodOop method = method_lookup(receiver_klass->klass_part()->superKlass(), selector);
	if (method == nullptr)
		return nullptr;
	LookupKey key(receiver_klass, method);
	LookupResult res = lookup(&key, false);
	return res.method_or_null();
}

MethodOop LookupCache::method_lookup(KlassOop receiver_klass, SymbolOop selector) {
	LookupKey key(receiver_klass, selector);
	MethodOop method = key.klass()->klass_part()->lookup(key.selector());
	return method;
}

NativeMethod *LookupCache::compile_method(LookupKey *key, MethodOop method) {
	if (not DeltaProcess::active()->in_vm_operation()) {
		VM_OptimizeMethod op(key, method);
		VMProcess::execute(&op);
		st_assert(op.result(), "NativeMethod must be there");
		st_assert(Universe::code->lookup(key), "NativeMethod must be there");
		return op.result();
	}
	return nullptr;
}

// Lookup support for inline cache
LookupResult LookupCache::ic_lookup(KlassOop receiver_klass, Oop selector_or_method) {
	LookupKey key(receiver_klass, selector_or_method);
	return lookup(&key, true);
}

LookupResult LookupCache::ic_normal_lookup(KlassOop receiver_klass, SymbolOop selector) {
	return LookupCache::ic_lookup(receiver_klass, selector);
}

LookupResult LookupCache::ic_super_lookup(KlassOop receiver_klass, KlassOop superKlass, SymbolOop selector) {
	MethodOop method = method_lookup(superKlass, selector);
	return ic_lookup(receiver_klass, method);
}

LookupResult interpreter_normal_lookup(KlassOop receiver_klass, SymbolOop selector) {
	LookupKey key(receiver_klass, selector);
	return LookupCache::lookup(&key, true);
}

LookupResult interpreter_super_lookup(SymbolOop selector) {
	// super lookup for interpreter: given the receiver and h-code pointer of the current
	// method (which is performing the super send), find the superclass and then the method
	// Note: the interpreter doesn't store the superclass in the InlineCache in order to
	// unify the format of inline caches for normal and super sends (the differences lead to
	// several bugs)

	// On request from Robert (6/19/95) this routine must return a methodOop since
	// the interpreter's super send cannot cope with compiled code.
	// This is a temporary restriction and should be removed soon (please bug Robert)
	//
	// This should not be a problem anymore (gri 1/24/96), clean up the lookup cache interface.
	ResourceMark resourceMark;
	Frame f = DeltaProcess::active()->last_frame();
	st_assert(f.is_interpreted_frame(), "must be interpreter frame");

	// remember to take the home of the executing method.
	MethodOop sendingMethod = f.method()->home();

	KlassOop sendingMethodHolder = f.receiver()->blueprint()->lookup_method_holder_for(sendingMethod);
	// NB: lookup for current method can fail if the method was deleted while still on the stack

	if (sendingMethodHolder == nullptr) {
		st_fatal("sending method holder not found; this might be caused by a programming change -- fix this");
	}
	KlassOop superKlass = sendingMethodHolder->klass_part()->superKlass();
	MethodOop superMethod = LookupCache::method_lookup(superKlass, selector);

	LookupResult result(superMethod);
	return result;
}

LookupResult LookupCache::lookup(LookupKey *key) {
	return ic_lookup(key->klass(), key->selector_or_method());
}

Oop LookupCache::normal_lookup(KlassOop receiver_klass, SymbolOop selector) {
	VerifyNoScavenge vna;
	LookupKey key(receiver_klass, selector);
	LookupResult result = ic_normal_lookup(receiver_klass, selector);
	st_assert(result.value() not_eq nullptr, "message not understood not implemented yet for compiled code");
	return result.value();
}

static void print_counter(const char *title, std::int32_t counter, std::int32_t total) {
	//SPDLOG_INFO( "%20s: %3.1f%% ({:d})", title, total == 0 ? 0.0 : 100.0 * (double) counter / (double) total, counter );
}

void LookupCache::clear_statistics() {
	number_of_primary_hits = 0;
	number_of_secondary_hits = 0;
	number_of_misses = 0;
}

void LookupCache::print_statistics() {
	std::int32_t total = number_of_primary_hits + number_of_secondary_hits + number_of_misses;
	SPDLOG_INFO("Lookup Cache: size({:d}, {:d})", primary_cache_size, secondary_cache_size);
	print_counter("Primary Hit Ratio", number_of_primary_hits, total);
	print_counter("Secondary Hit Ratio", number_of_secondary_hits, total);
	print_counter("Miss Ratio", number_of_misses, total);
}
