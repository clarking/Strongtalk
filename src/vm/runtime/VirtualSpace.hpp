//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/runtime/ReservedSpace.hpp"


// VirtualSpace is a data structure for reserving a contiguous chunk of memory and then committing to using the reserved chunk bit by bit.
// Perfect for implementing growable stack without relocation.

class VirtualSpace : public ValueObject {

private:
	// Reserved area
	const char *_low_boundary;
	const char *_high_boundary;

	// Committed area
	const char *_low;
	const char *_high;

	// Grow direction
	bool _low_to_high;

	VirtualSpace *next;

	friend class VirtualSpaces;

public:
	const char *low() const {
		return _low;
	}

	const char *high() const {
		return _high;
	}

	const char *low_boundary() const {
		return _low_boundary;
	}

	const char *high_boundary() const {
		return _high_boundary;
	}

public:
	VirtualSpace(std::int32_t reserved_size, std::int32_t committed_size, bool low_to_high = true);

	VirtualSpace(ReservedSpace reserved, std::int32_t committed_size, bool low_to_high = true);

	VirtualSpace();
	VirtualSpace(const VirtualSpace &) = default;
	VirtualSpace &operator=(const VirtualSpace &) = default;

	void initialize(ReservedSpace reserved, std::int32_t committed_size, bool low_to_high = true);

	virtual ~VirtualSpace();

	// testers
	std::int32_t committed_size() const;

	std::int32_t reserved_size() const;

	std::int32_t uncommitted_size() const;

	bool contains(void *p) const;

	bool low_to_high() const;

	// operations
	void expand(std::int32_t size);

	void shrink(std::int32_t size);

	void release();

	// debugging
	void print();

	// page faults
	virtual void page_fault() {
	}
};

class VirtualSpaces : AllStatic {
private:
	static VirtualSpace *head;

	static void add(VirtualSpace *sp);

	static void remove(VirtualSpace *sp);

	friend class VirtualSpace;

public:
	static std::int32_t committed_size();

	static std::int32_t reserved_size();

	static std::int32_t uncommitted_size();

	static void print();

	static void test();
};
