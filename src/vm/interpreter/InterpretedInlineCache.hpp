//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/system/asserts.hpp"
#include "vm/lookup/LookupResult.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/primitive/PrimitiveDescriptor.hpp"



// The InterpretedInlineCache describes the contents of an inline cache in the
// byte codes of a method. The inline cache holds a combination of
// selector/method and 0/class oops and is read and modified during
// execution.
//
//
// An InterpretedInlineCache can have four different states:
//
// 1) empty					{selector,         0}
// 2) filled with a call to interpreted code	{method,           klass}
// 3) filled with an interpreter PolymorphicInlineCache		{selector,         objectArrayObject}
// 4) filled with a call to compiled code	{jump table entry, klass}
//
//
// Layout:
//
//    send byte code			<-----	send_code_addr()
//    ...
//    halt byte codes for alignment
//    ...
// 0: selector/method/jump table entry	<-----	first_word_addr()
// 4: 0/klass/objectArray			<-----	second_word_addr()
// 8: ...

class InterpretedInlineCache : ValueObject {

public:
	static constexpr std::int32_t size = 8;    // inline cache size in words
	static constexpr std::int32_t first_word_offset = 0;    // layout info: first word
	static constexpr std::int32_t second_word_offset = 4;    // layout info: second word


public:

	// Conversion (Bytecode* -> InterpretedInlineCache*)
	friend InterpretedInlineCache *as_InterpretedIC(const char *address_of_next_instr);

	// find send bytecode, given address of selector; return nullptr/IllegalByteCodeIndex if not in a send
	static std::uint8_t *findStartOfSend(std::uint8_t *selector_addr);

	static std::int32_t findStartOfSend(MethodOop m, std::int32_t byteCodeIndex);

private:
	// field access
	const char *addr_at(std::int32_t offset) const {
		return (const char *) this + offset;
	}

	Oop *first_word_addr() const {
		return (Oop *) addr_at(first_word_offset);
	}

	Oop *second_word_addr() const {
		return (Oop *) addr_at(second_word_offset);
	}

	void set(ByteCodes::Code send_code, Oop first_word, Oop second_word);

public:
	// Raw inline cache access
	std::uint8_t *send_code_addr() const {
		return findStartOfSend((std::uint8_t *) this);
	}

	ByteCodes::Code send_code() const {
		return ByteCodes::Code(*send_code_addr());
	}

	Oop first_word() const {
		return *first_word_addr();
	}

	Oop second_word() const {
		return *second_word_addr();
	}

	// Returns the POLYMORPHIC inline cache array. Assert fails if no pic is present.
	ObjectArrayOop pic_array();

	// Inline cache information
	bool is_empty() const {
		return second_word() == nullptr;
	}

	SymbolOop selector() const;        // the selector
	JumpTableEntry *jump_table_entry() const;    // only legal to call if compiled send

	std::int32_t nof_arguments() const;        // the number of arguments
	ByteCodes::SendType send_type() const;    // the send type
	ByteCodes::ArgumentSpec argument_spec() const;// the argument spec

	// Manipulation
	void clear();                    // clears the inline cache
	void cleanup();                               // cleanup the inline cache
	void clear_without_deallocation_pic();        // clears the inline cache without deallocating the pic
	void replace(NativeMethod *nm);            // replaces the appropriate target with a nm
	void replace(LookupResult result, KlassOop receiver_klass); // replaces the inline cache with a lookup result

	// Debugging
	void print();

	// Cache miss
	static Oop *inline_cache_miss();        // the inline cache miss handler

private:
	// helpers for inline_cache_miss
	static void update_inline_cache(InterpretedInlineCache *ic, Frame *f, ByteCodes::Code send_code, KlassOop klass, LookupResult result);

	static Oop does_not_understand(Oop receiver, InterpretedInlineCache *ic, Frame *f);

	static void trace_inline_cache_miss(InterpretedInlineCache *ic, KlassOop klass, LookupResult result);
};

InterpretedInlineCache *as_InterpretedIC(const char *address_of_next_instr);


// Interpreter_PICs handles the allocation and deallocation of interpreter PICs.

static constexpr std::int32_t size_of_smallest_interpreterPIC = 2;
static constexpr std::int32_t size_of_largest_interpreterPIC = 5;
static constexpr std::int32_t number_of_interpreterPolymorphicInlineCache_sizes = size_of_largest_interpreterPIC - size_of_smallest_interpreterPIC + 1;


// An InterpretedInlineCacheIterator is used to iterate through the entries of an inline cache in a methodOop.
//
// Whenever possible, one should use this abstraction instead of the (raw) InterpretedInlineCache


class InterpretedInlineCacheIterator : public InlineCacheIterator {

private:
	InterpretedInlineCache *_ic;            // the inline cache
	ObjectArrayOop _pic;            // the PolymorphicInlineCache if there is one

	// state machine
	std::int32_t _number_of_targets;    // the no. of InlineCache entries
	InlineCacheShape _info;                 // send site information
	std::int32_t _index;                // the current entry no.
	KlassOop _klass;                // the current klass
	MethodOop _method;               // the current method
	NativeMethod *_nativeMethod;        // current NativeMethod (nullptr if none)

	void set_method(Oop m);               // set _method and _nativeMethod
	void set_klass(Oop k);                // don't assign to _klass directly

public:
	InterpretedInlineCacheIterator(InterpretedInlineCache *ic);
	InterpretedInlineCacheIterator() = default;
	virtual ~InterpretedInlineCacheIterator() = default;
	InterpretedInlineCacheIterator(const InterpretedInlineCacheIterator &) = default;
	InterpretedInlineCacheIterator &operator=(const InterpretedInlineCacheIterator &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	// InlineCache information
	std::int32_t number_of_targets() const {
		return _number_of_targets;
	}

	InlineCacheShape shape() const {
		return _info;
	}

	SymbolOop selector() const {
		return _ic->selector();
	}

	bool is_interpreted_ic() const {
		return true;
	}

	bool is_super_send() const;

	InterpretedInlineCache *interpreted_ic() const {
		return _ic;
	}

	// Iterating through entries
	void init_iteration();

	void advance();

	bool at_end() const {
		return _index >= number_of_targets();
	}

	// Accessing entries
	KlassOop klass() const {
		return _klass;
	}

	// answer whether current target method is compiled or interpreted
	bool is_interpreted() const {
		return _nativeMethod == nullptr;
	}

	bool is_compiled() const {
		return _nativeMethod not_eq nullptr;
	}

	MethodOop interpreted_method() const;

	NativeMethod *compiled_method() const;

	// Debugging
	void print();
};
