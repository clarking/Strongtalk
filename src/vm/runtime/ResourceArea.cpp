
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/ResourceArea.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/platform/os.hpp"
#include "vm/memory/util.hpp"
#include "vm/memory/Universe.hpp"



//
// The global "operator new" should never be called since it will usually initiate a memory leak.
//
// Use "CHeapAllocatedObject" as the base class of such objects to make it explicit that they're allocated on the C heap.
//

// Commented out to prevent conflict with dynamically loaded routines.
//
//void *operator new( std::size_t size ) {
//    st_fatal( "should not call global (default) operator new" );
//    return (void *) AllocateHeap( size, "global operator new" );
//}


// -----------------------------------------------------------------------------

Resources resources;
ResourceArea resource_area;



// -----------------------------------------------------------------------------

//
// The resource area holds temporary data structures of the VM.
// Things in the resource area can be deallocated very efficiently using ResourceMarks.
// The destructor of a ResourceMark will deallocate everything that was created since the ResourceMark was created
//

constexpr std::int32_t min_resource_free_size = 32 * 1024;
constexpr std::int32_t min_resource_chunk_size = 256 * 1024;

ResourceAreaChunk::ResourceAreaChunk(std::int32_t min_capacity, ResourceAreaChunk *previous) :
		_bottom{nullptr},
		_top{nullptr},
		_firstFree{nullptr},
		_prev{nullptr},
		_allocated{0},
		_previous_used{0} {

	std::int32_t size = max(min_capacity + min_resource_free_size, min_resource_chunk_size);
	_bottom = (char *) AllocateHeap(size, "resourceAreaChunk");
	_top = _bottom + size;

//    SPDLOG_INFO( "%ResourceAreaChunk-allocated [0x{08:x}] ", resources.capacity() );
//    SPDLOG_INFO( "%ResourceAreaChunk-used [0x{08:x}] ", resources.used() );
//    SPDLOG_INFO( "%ResourceAreaChunk-size [0x{08:x}] ", size );

	SPDLOG_INFO("ResourceAreaChunk: create size [{}], [{}] used out of [{}] ", size, resources.used(), resources.capacity());

	initialize(previous);
}

void ResourceAreaChunk::initialize(ResourceAreaChunk *previous) {

	_firstFree = _bottom;
	_prev = previous;

	_allocated = capacity() + (_prev ? _prev->_allocated : 0);
	_previous_used = _prev ? (_prev->_previous_used + used()) : 0;
}

ResourceAreaChunk::~ResourceAreaChunk() {
	FreeHeap(_bottom);
}

void ResourceAreaChunk::print() {
	if (_prev) {
		_prev->print();
	}
	print_short();
	//SPDLOG_INFO( ": _bottom [0x{0:x}], _top [0x{0:x}], _prev [0x{0:x}]", static_cast<void *>( _bottom ), static_cast<void *>( _top  ), static_cast<void *>( _prev ) );
}

void ResourceAreaChunk::print_short() {
	/// SPDLOG_INFO( "ResourceAreaChunk [0x{0:x}]", static_cast<void *>( this ) );
}

void ResourceAreaChunk::print_alloc(const char *addr, std::int32_t size) {
	// SPDLOG_INFO( "allocating %ld bytes at 0x{0:x}", size, addr );
}

ResourceArea::ResourceArea() :
		_resourceAreaChunk{nullptr},
		_nestingLevel{0} {
}

ResourceArea::~ResourceArea() {
	// deallocate all chunks
	ResourceAreaChunk *prevc;

	for (ResourceAreaChunk *c = _resourceAreaChunk; c not_eq nullptr; c = prevc) {
		prevc = c->_prev;
		resources.addToFreeList(c);
	}
}

char *ResourceArea::allocate_more_bytes(std::int32_t size) {
	_resourceAreaChunk = resources.new_chunk(size, _resourceAreaChunk);
	char *p = _resourceAreaChunk->allocate_bytes(size);
	st_assert(p, "Nothing returned");
	return p;
}

std::int32_t ResourceArea::used() {
	if (_resourceAreaChunk == nullptr)
		return 0;
	return _resourceAreaChunk->used() + (_resourceAreaChunk->_prev ? _resourceAreaChunk->_prev->_previous_used : 0);
}

char *ResourceArea::allocate_bytes(std::int32_t size) {

	if (size < 0) {
		st_fatal("negative size in allocate_bytes");
	}
	st_assert(size >= 0, "negative size in allocate_bytes");

	// NB: don't make it a fatal error -- otherwise, if you call certain functions
	// from the debugger, it might report a leak since there might not be a
	// ResourceMark.

	// However, in all other situations, calling allocate_bytes with nesting == 0
	// is a definitive memory leak.  -Urs 10/95

//            static std::int32_t warned = 0;    // to suppress multiple warnings (e.g. when allocating from the debugger)
//            if (nesting < 1 and not warned++) error("memory leak: allocating w/o ResourceMark");

	if (size == 0) {
		// want to return an invalid pointer for a zero-sized allocation,
		// but not nullptr, because routines may want to use nullptr for failure.
		// gjvc: but the above reason doesn't make much sense -- a zero-sized allocation is immediately useless.
		return (char *) 1;
	}

	size = roundTo(size, OOP_SIZE);
	char *p;
	if (_resourceAreaChunk and (p = _resourceAreaChunk->allocate_bytes(size)))
		return p;
	return allocate_more_bytes(size);
}


// -----------------------------------------------------------------------------

std::int32_t Resources::capacity() {
	return _allocated;
}

std::int32_t Resources::used() {
	return resource_area.used();
}

static bool in_rsrc;
static const char *p_rsrc;

bool Resources::contains(const char *p) {
	in_rsrc = false;
	p_rsrc = p;
	// FIX LATER  processes->processesDo(rsrcf2);
	return in_rsrc;
}

void Resources::addToFreeList(ResourceAreaChunk *c) {
	if (ZapResourceArea)
		c->clear();

	c->_prev = freeChunks;
	freeChunks = c;
}

ResourceAreaChunk *Resources::getFromFreeList(std::int32_t min_capacity) {
	if (not freeChunks)
		return nullptr;

	// Handle the first element special
	if (freeChunks->capacity() >= min_capacity) {
		ResourceAreaChunk *res = freeChunks;
		freeChunks = freeChunks->_prev;
		return res;
	}

	ResourceAreaChunk *cursor = freeChunks;
	while (cursor->_prev) {
		if (cursor->_prev->capacity() >= min_capacity) {
			ResourceAreaChunk *res = cursor->_prev;
			cursor->_prev = cursor->_prev->_prev;
			return res;
		}
		cursor = cursor->_prev;
	}

	// No suitable chunk found
	return nullptr;
}

ResourceAreaChunk *Resources::new_chunk(std::int32_t min_capacity, ResourceAreaChunk *previous) {

	_in_consistent_state = false;
	ResourceAreaChunk *res = getFromFreeList(min_capacity);
	if (res) {
		res->initialize(previous);
	}
	else {
		res = new ResourceAreaChunk(min_capacity, previous);
		_allocated += res->capacity();
		if (PrintResourceChunkAllocation) {
			SPDLOG_INFO("*allocating new resource area chunk of >=0x%08x bytes, new total = 0x%08x bytes", min_capacity, _allocated);
		}
	}

	_in_consistent_state = true;

	st_assert(res, "just checking");

	return res;
}


// -----------------------------------------------------------------------------

char *ResourceAreaChunk::allocate_bytes(std::int32_t size) {

	char *p = _firstFree;
	if (_firstFree + size <= _top) {
		if (PrintResourceAllocation) {
			print_alloc(p, size);
		}
		_firstFree += size;
		return p;
	}
	else {
		return nullptr;
	}

}

void ResourceAreaChunk::freeTo(char *new_first_free) {
	st_assert(new_first_free <= _firstFree, "unfreeing in resource area");
	if (ZapResourceArea)
		clear(new_first_free, _firstFree);
	_firstFree = new_first_free;
}

Resources::Resources() :
		freeChunks{nullptr},
		_allocated{0},
		_in_consistent_state{true} {

}


// -----------------------------------------------------------------------------

NoGCVerifier::NoGCVerifier() :
		old_scavenge_count{Universe::scavengeCount} {
}

NoGCVerifier::~NoGCVerifier() {
	if (old_scavenge_count not_eq Universe::scavengeCount) {
		SPDLOG_WARN("scavenge in a NoGCVerifier secured function");
	}
}

char *AllocatePageAligned(std::int32_t size, const char *name) {

	std::int32_t page_size = Universe::page_size();
	char *block = (char *) align(os::malloc(size + page_size), page_size);
	// if ( PrintHeapAllocation )
	//     SPDLOG_INFO( "Malloc (page-aligned) {}: 0x%08x = 0x{0:x}", name, size, block );

	return block;
}

char *AllocateHeap(std::int32_t size, const char *name) {

	char *bytes = (char *) os::malloc(size);
	// if ( PrintHeapAllocation )
	//     SPDLOG_INFO( "Heap 0x{0:x} {}", size, name );

	return bytes;
}

void FreeHeap(void *p) {
	os::free(p);
}

char *allocateResource(std::int32_t size) {
	return resource_area.allocate_bytes(size);
}
