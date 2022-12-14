
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/runtime/VirtualSpace.hpp"
#include "vm/runtime/ReservedSpace.hpp"
#include "vm/platform/os.hpp"


// -----------------------------------------------------------------------------

VirtualSpace::VirtualSpace(std::int32_t reserved_size, std::int32_t committed_size, bool low_to_high) : VirtualSpace() {
	ReservedSpace rs(reserved_size);
	initialize(rs, committed_size, low_to_high);
}

VirtualSpace::VirtualSpace(ReservedSpace reserved, std::int32_t committed_size, bool low_to_high) : VirtualSpace() {
	initialize(reserved, committed_size, low_to_high);
}

VirtualSpace::VirtualSpace() :
		_low_boundary{nullptr},
		_high_boundary{nullptr},
		_low{nullptr},
		_high{nullptr},
		_low_to_high{true},
		next{nullptr} {
}


// -----------------------------------------------------------------------------

void VirtualSpace::initialize(ReservedSpace reserved, std::int32_t committed_size, bool low_to_high) {
	_low_boundary = reserved.base();
	_high_boundary = low_boundary() + reserved.size();

	_low_to_high = low_to_high;

	if (low_boundary() == nullptr) st_fatal("os::reserve_memory failed");

	// initialize committed area
	_low = low_to_high ? low_boundary() : high_boundary();
	_high = low();

	// commit to initial size
	expand(committed_size);

	VirtualSpaces::add(this);
}

VirtualSpace::~VirtualSpace() {
	release();
}

void VirtualSpace::release() {
	os::release_memory(low_boundary(), reserved_size());
	_low_boundary = nullptr;
	_high_boundary = nullptr;
	_low = nullptr;
	_high = nullptr;
	VirtualSpaces::remove(this);
}

std::int32_t VirtualSpace::committed_size() const {
	return high() - low();
}

std::int32_t VirtualSpace::reserved_size() const {
	return high_boundary() - low_boundary();
}

std::int32_t VirtualSpace::uncommitted_size() const {
	return reserved_size() - committed_size();
}

bool VirtualSpace::contains(void *p) const {
	return low() <= (const char *) p and (const char *) p < high();
}

bool VirtualSpace::low_to_high() const {
	return _low_to_high;
}

void VirtualSpace::expand(std::int32_t size) {
	print();
	st_assert(uncommitted_size() >= size, "not Space enough");
	st_assert((size % os::vm_page_size()) == 0, "size not page aligned");
	if (low() == low_boundary()) {
		if (not os::commit_memory(high(), size)) st_fatal("os::commit_memory failed");
		_high += size;
	}
	else {
		_low -= size;
		if (not os::commit_memory(low(), size)) st_fatal("os::commit_memory failed");
	}
}

void VirtualSpace::shrink(std::int32_t size) {
	st_assert(committed_size() >= size, "not Space enough");
	st_assert((size % os::vm_page_size()) == 0, "size not page aligned");
	if (low() == low_boundary()) {
		_high -= size;
		if (not os::uncommit_memory(high(), size)) {
			st_fatal("os::uncommit_memory failed");
		}
	}
	else {
		if (not os::uncommit_memory(low(), size)) {
			st_fatal("os::uncommit_memory failed");
		}
		_low += size;
	}
}

void VirtualSpace::print() {
	// SPDLOG_INFO( "Virtual Space [] committed_size [{}], uncommitted_size [{}], reserved_size [{}], low [0x{0:x}], high [0x{0:x}], low_boundary [0x{0:x}] high_boundary [0x{0:x}]",
	//              committed_size(), uncommitted_size(), reserved_size(), low(), high(), low_boundary(), high_boundary() );
}
