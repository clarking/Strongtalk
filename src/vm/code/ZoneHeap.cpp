
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/platform/os.hpp"
#include "vm/system/asserts.hpp"
#include "vm/code/ZoneHeap.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utility/EventLog.hpp"
#include "vm/oop/Oop.hpp"
#include "vm/memory/util.hpp"


// format of chunks in free map: first & last byte hold chunk size
//
// unused..unusedOvfl-1:unused, length n+1
// used..usedOvfl-1	used, length n - used + 1
// chunkState:: unusedOvfl:          unused, encoded length in next (prev) 3 bytes
// usedOvfl:		used, encoded length in next (prev) 3 bytes
//


extern bool bootstrappingInProgress;


ChunkKlass *asChunkKlass( std::uint8_t *c ) {
    return (ChunkKlass *) c;
}


void ChunkKlass::markSize( std::int32_t nChunks, chunkState s ) {


    // write header
    std::uint8_t *p = asByte();
    std::uint8_t *e = p + nChunks - 1;
    if ( nChunks < maxOneByteLen ) {
        p[ 0 ] = e[ 0 ] = static_cast<std::int32_t>(s) + nChunks - 1;
    } else {
        st_assert( nChunks < ( 1 << ( 3 * MaxDistLog ) ), "chunk too large" );
        unsigned mask = nthMask( MaxDistLog );
        p[ 0 ] = e[ 0 ]  = static_cast<std::uint8_t>( ( s == chunkState::used ) ? chunkState::usedOvfl : chunkState::unusedOvfl );
        p[ 1 ] = e[ -3 ] = static_cast<std::uint8_t>( chunkState::unused ) + ( nChunks >> ( 2 * MaxDistLog ) );
        p[ 2 ] = e[ -2 ] = static_cast<std::uint8_t>( chunkState::unused ) + ( ( nChunks >> MaxDistLog ) & mask );
        p[ 3 ] = e[ -1 ] = static_cast<std::uint8_t>( chunkState::unused ) + ( nChunks & mask );
    }
    st_assert( size() == nChunks, "incorrect size encoding" );
    // mark distance for used blocks
    if ( s == chunkState::unused ) {
        // don't mark unused blocks - not necessary, and would be a performance
        // bug (start: *huge* free block, shrinks with every alloc -> quadratic)
        // however, the first distance byte must be correct (for findStart)
        if ( nChunks > 2 * minHeaderSize )
            p[ headerSize() ] = headerSize();
    } else {
        if ( nChunks < maxOneByteLen ) {
            st_assert( maxOneByteLen <= static_cast<std::int32_t>(chunkState::MaxDistance), "oops" );
            for ( std::size_t i = minHeaderSize; i < nChunks - minHeaderSize; i++ )
                p[ i ] = i;
        } else {
            std::int32_t max = min( static_cast<std::int32_t>(nChunks - 4), static_cast<std::int32_t>( chunkState::MaxDistance ) );
            std::int32_t i   = maxHeaderSize;
            for ( ; i < max; i++ )
                p[ i ] = i;
            // fill rest with large distance values (don't use chunkState::MaxDistance - 1 because
            // the elems chunkState::MaxDistance..MaxDistance+maxHeaderSize-1 would point *into*
            // the header)
            for ( ; i < nChunks - maxHeaderSize; i++ )
                p[ i ] = static_cast<std::int32_t>( chunkState::MaxDistance ) - maxHeaderSize;
        }
    }
}


ChunkKlass *ChunkKlass::findStart( ChunkKlass *mapStart, ChunkKlass *mapEnd ) {
    // this points into the middle of a chunk; return start of chunk
    std::uint8_t *p     = asByte();
    std::uint8_t *start = mapStart->asByte();
    std::uint8_t *end   = mapEnd->asByte();
    ChunkKlass   *m;
    if ( *p < static_cast<std::uint8_t>( chunkState::MaxDistance ) ) {
        // we're outside the header, so just walk down the trail
        while ( *p < static_cast<std::uint8_t>( chunkState::MaxDistance ) )
            p -= *p;
        st_assert( p >= start, "not found" );
        m = asChunkKlass( p );
    } else {
        // pointing to a header, but we don't know whether std::int32_t/std::int16_t etc
        // first walk up to first non-header byte
        // (note that first distance byte of unused blocks is correct, but
        // the others aren't)
        while ( *p >= static_cast<std::uint8_t>( chunkState::MaxDistance ) and p < end )
            p++;
        if ( p < end ) {
            // find start of this block
            while ( *p < static_cast<std::uint8_t>( chunkState::MaxDistance ) )
                p -= *p;
            st_assert( p >= start, "not found" );
        }
        m     = asChunkKlass( p );
        while ( not m->contains( this->asByte() ) )
            m = m->prev();
    }
    st_assert( m->verify(), " chunkState::invalid chunk map" );
    st_assert( m->contains( this->asByte() ), "wrong chunk" );
    return m;
}


bool ChunkKlass::isValid() {
    std::uint8_t *p = asByte();
    bool         ok;
    if ( p[ 0 ] == static_cast<std::uint8_t>( chunkState::invalid ) or p[ 0 ] < static_cast<std::uint8_t>( chunkState::MaxDistance ) ) {
        ok = false;
    } else {
        std::uint8_t *e   = next()->asByte() - 1;
        std::int32_t ovfl = isUsed() ? static_cast<std::int32_t>( chunkState::usedOvfl ) : static_cast<std::int32_t>( chunkState::unusedOvfl );
        ok = p[ 0 ] == e[ 0 ] and (
            ( p[ 0 ] not_eq ovfl ) or
            (
                ( p[ 1 ] == e[ -3 ] ) and
                ( p[ 2 ] == e[ -2 ] ) and
                ( p[ 3 ] == e[ -1 ] )
            )
        );
    }
    return ok;
}


void ChunkKlass::print() {
    //SPDLOG_INFO( "chunk [0x{0:x}..0x{0:x}[", static_cast<const void *>(this), static_cast<const void *>(next()) );
}


bool ChunkKlass::verify() {
    if ( not isValid() ) {
        error( "inconsistent chunk map 0x{0:x}", this );
        return false;
    }
    return true;
}


void FreeList::append( HeapChunk *h ) {
    insert( h );
}


void FreeList::remove( HeapChunk *h ) {
    h->remove();
}


HeapChunk *FreeList::get() {
    if ( isEmpty() ) {
        return nullptr;
    } else {
        HeapChunk *res = anchor()->next();
        remove( res );
        return res;
    }
}


std::size_t FreeList::length() const {
    std::int32_t    i  = 0;
    HeapChunk       *f = anchor();
    for ( HeapChunk *p = f->next(); p not_eq f; p = p->next() )
        i++;
    return i;
}


ZoneHeap::ZoneHeap( std::int32_t s, std::int32_t bs ) :
    size{},
    blockSize{},
    nfree{ 30 },
    log2BS{},
    _bytesUsed{},
    _total{},
    _ifrag{},
    _base{},
    base{},
    _heapKlass{},
    _freeList{},
    _bigList{},
    _lastCombine{},
    _newHeap{},
    _combineOnDeallocation{} {

    st_assert( s % bs == 0, "size not a multiple of blockSize" );
    size = s;
    if ( bs & ( bs - 1 ) ) {
        st_fatal1( "heap block size (%ld) is not a power of 2", bs );
    }
    blockSize = bs;
    log2BS    = 0;
    while ( bs > 1 ) {
        bs >>= 1;
        log2BS++;
    }

    nfree = 30;
//    _base = AllocateHeap( size + blockSize, "zone" );
    _base = os::exec_memory( size + blockSize ); //, "zone");
    base  = (const char *) ( ( std::int32_t( _base ) + blockSize - 1 ) / blockSize * blockSize );
    st_assert( std::int32_t( base ) % blockSize == 0, "base not aligned to blockSize" );
    _heapKlass = (ChunkKlass *) ( AllocateHeap( mapSize() + 2, "zone free map" ) + 1 );
    // + 2 for sentinels
    _freeList  = new_c_heap_array<FreeList>( nfree );
    _bigList   = new_c_heap_array<FreeList>( 1 );
    _newHeap   = nullptr;
    clear();
}


void ZoneHeap::clear() {
    // initialize the statistics
    _bytesUsed = 0;
    _total     = 0;
    _ifrag     = 0;

    // initialize the free lists
    for ( std::size_t i = 0; i < nfree; i++ ) {
        _freeList[ i ].clear();
    }
    _bigList->clear();

    // initialize the map
    _heapKlass->markUnused( mapSize() );              // mark everything as unused
    heapEnd()->markUsed( 1 );                  // start sentinel
    asChunkKlass( _heapKlass->asByte() - 1 )->markUsed( 1 ); // stop sentinel

    // add whole chunk to free list
    addToFreeList( _heapKlass );

    // set the combine variables
    _combineOnDeallocation = false;
    _lastCombine           = _heapKlass;
}


#define between( p, low, high ) ((void *)(p) >= (void *)(low) and (void *)(p) < (void *)(high))


bool ZoneHeap::contains( const void *p ) const {
    return between( p, base, base + capacity() ) or between( p, _freeList, _freeList + nfree );
}


ZoneHeap::~ZoneHeap() {
    set_oops( (Oop *) base, capacity() / OOP_SIZE, nullptr );
    free( const_cast<char *>(_base) );
    free( _heapKlass - 1 );        // -1 to get rid of sentinel
    free( _freeList );
    free( _bigList );
}


void ZoneHeap::removeFromFreeList( ChunkKlass *m ) {
    m->verify();
    HeapChunk *p = (HeapChunk *) blockAddr( m );
    p->remove();
}


bool ZoneHeap::addToFreeList( ChunkKlass *m ) {
    m->verify();
    HeapChunk    *p = (HeapChunk *) blockAddr( m );
    std::int32_t sz = m->size();
    if ( sz <= nfree ) {
        _freeList[ sz - 1 ].append( p );
        return false;
    } else {
        _bigList->append( p );
        p->size = sz;
        return true; // "big"
    }
}


void *ZoneHeap::allocFromLists( std::int32_t wantedBytes ) {
    st_assert( wantedBytes % blockSize == 0, "not a multiple of blockSize" );
    std::int32_t wantedBlocks = wantedBytes >> log2BS;
    st_assert( wantedBlocks > 0, "negative alloc size" );
    std::int32_t blocks = wantedBlocks - 1;
    void         *p     = nullptr;
    while ( not p and ++blocks <= nfree ) {
        p = _freeList[ blocks - 1 ].get();
    }
    if ( not p ) {
        HeapChunk *f = _bigList->anchor();
        HeapChunk *c = f->next();
        for ( ; c not_eq f and c->size < wantedBlocks; c = c->next() );
        if ( c == f ) {
            if ( not _combineOnDeallocation and combineAll() >= wantedBlocks )
                return allocFromLists( wantedBytes );
        } else {
            p      = (void *) c;
            blocks = c->size;
            _bigList->remove( c );
        }
    }
    if ( p ) {
        ChunkKlass *m = mapAddr( p );
        st_assert( m->size() == blocks, "inconsistent sizes" );
        m->markUsed( wantedBlocks );
        if ( blocks > wantedBlocks ) {

#ifdef LOG_LOTSA_STUFF
            if ( not bootstrappingInProgress ) {
                SPDLOG_INFO( "zoneHeap: splitting allocated block" );
            }
#endif
            std::int32_t freeChunkSize = blocks - wantedBlocks;
            ChunkKlass   *freeChunk    = m->next();
            freeChunk->markUnused( freeChunkSize );
            addToFreeList( freeChunk );
        }
    }
    return p;
}


void *ZoneHeap::allocate( std::int32_t wantedBytes ) {
    st_assert( wantedBytes > 0, "Heap::allocate: size <= 0" );
    std::int32_t rounded = ( ( wantedBytes + blockSize - 1 ) >> log2BS ) << log2BS;

    void *p = allocFromLists( rounded );
    if ( p ) {
        _bytesUsed += rounded;
        _total += rounded;
        _ifrag += rounded - wantedBytes;
    }
    if ( VerifyZoneOften ) {
        verify();
    }
    return p;
}


void ZoneHeap::deallocate( void *p, std::int32_t bytes ) {
    ChunkKlass   *m           = mapAddr( p );
    std::int32_t myChunkSize  = m->size();
    std::int32_t blockedBytes = myChunkSize << log2BS;
    _bytesUsed -= blockedBytes;
    _ifrag -= blockedBytes - bytes;
    m->markUnused( myChunkSize );
    bool      big = addToFreeList( m );
    HeapChunk *c  = (HeapChunk *) p;
    if ( _combineOnDeallocation or big )
        combine( c );    // always keep bigList combined

    if ( VerifyZoneOften ) {
        verify();
    }
}


#define INC( p, n )   p = asChunkKlass(p->asByte() + n)


const char *ZoneHeap::compact( void move( const char *from, char *to, std::int32_t nbytes ) ) {

    if ( usedBytes() == capacity() )
        return nullptr;

    ChunkKlass *m   = _heapKlass;
    ChunkKlass *end = heapEnd();

    ChunkKlass *freeChunk = m;
    while ( freeChunk->isUsed() ) {                // find 1st unused blk
        freeChunk = freeChunk->next();
    }
    ChunkKlass *usedChunk = freeChunk;

    while ( true ) {
        while ( usedChunk->isUnused() )
            usedChunk = usedChunk->next();
        if ( usedChunk == end )
            break;
        std::int32_t uSize = usedChunk->size();
        st_assert( freeChunk < usedChunk, "compaction bug" );
        move( blockAddr( usedChunk ), const_cast<char *>( blockAddr( freeChunk ) ), uSize << log2BS );
        freeChunk->markUsed( uSize );      // must come *after* move
        INC( freeChunk, uSize );
        INC( usedChunk, uSize );
    }

    for ( std::size_t i = 0; i < nfree; i++ )
        _freeList[ i ].clear();

    _bigList->clear();
    std::int32_t freeBlocks = end->asByte() - freeChunk->asByte();
    freeChunk->markUnused( freeBlocks );
    addToFreeList( freeChunk );
    st_assert( freeBlocks * blockSize == capacity() - usedBytes(), "usage info inconsistent" );
    _lastCombine = _heapKlass;

    return blockAddr( freeChunk );
}


std::int32_t ZoneHeap::combine( HeapChunk *&c ) {
    // Try to combine c with its neighbors; on return, c will point to
    // the next element in the freeList, and the return value will indicate
    // the size of the combined block.
    ChunkKlass *cm = mapAddr( (const char *) c );
    st_assert( cm < heapEnd(), "beyond heap" );
    ChunkKlass *cmnext = cm->next();
    ChunkKlass *cm1;
    if ( cm == _heapKlass ) {
        cm1 = cm;
    } else {
        cm1 = cm->prev();            // try to combine with prev
        while ( cm1->isUnused() ) {        // will terminate because of sentinel
            ChunkKlass *free = cm1;
            cm1              = free->prev();
            removeFromFreeList( free );
            free->invalidate();        // make sure it doesn't look valid
        }
        cm1 = cm1->next();
    }
    ChunkKlass *cm2    = cmnext;        // try to combine with next
    while ( cm2->isUnused() ) {        // will terminate because of sentinel
        ChunkKlass *free = cm2;
        cm2 = cm2->next();
        removeFromFreeList( free );
        free->invalidate();            // make sure it doesn't look valid
    }

    // The combined block will move to a new free list; make sure that c
    // returns an element in the current list so that iterators work.
    c = c->next();

    if ( cm1 not_eq cm or cm2 not_eq cmnext ) {
        removeFromFreeList( cm );
        cm->invalidate();
        cm1->markUnused( cm2->asByte() - cm1->asByte() );
        addToFreeList( cm1 );
        _lastCombine = cm1;
    }
    st_assert( cm1 >= _heapKlass and cm2 <= heapEnd() and cm1 < cm2, "just checkin'" );
    return cm1->size();
}


// Try to combine adjacent free chunks; return size of biggest chunk (in blks).
std::int32_t ZoneHeap::combineAll() {
    std::int32_t       biggest = 0;
    for ( std::int32_t i       = 0; i < nfree; i++ ) {
        HeapChunk       *f = _freeList[ i ].anchor();
        for ( HeapChunk *c = f->next(); c not_eq f; ) {
            HeapChunk    *c1 = c;
            std::int32_t sz  = combine( c );
            if ( c1 == c ) st_fatal( "infinite loop detected while combining blocks" );
            if ( sz > biggest )
                biggest = sz;
        }
    }
    _combineOnDeallocation     = true;
    if ( VerifyZoneOften ) {
        verify();
    }
    return biggest;
}


const void *ZoneHeap::firstUsed() const {
    if ( usedBytes() == 0 )
        return nullptr;

    if ( _heapKlass->isUsed() ) {
        return base;
    } else {
        return nextUsed( base );
    }
}


// return next used chunk with address > p
const void *ZoneHeap::nextUsed( const void *p ) const {
    ChunkKlass *m = mapAddr( p );
    if ( m->isValid() and not _lastCombine->contains( m->asByte() ) ) {
        if ( VerifyZoneOften ) {
            ChunkKlass *m1 = _heapKlass;
            for ( ; m1 < m; m1 = m1->next() );
            st_assert( m1 == m, "m isn't a valid chunk" );
        }
        st_assert( m->verify(), "valid chunk doesn't verify" );
        m = m->next();
    } else {
        // m is pointing into the middle of a block (because of block combination)

        ChunkKlass *m1 = _heapKlass;
        for ( ; m1 < _lastCombine; m1 = m1->next() );
        st_assert( m1 == _lastCombine, "lastCombine not found" );
        st_assert( _lastCombine->verify(), " chunkState::invalid lastCombine" );

        ChunkKlass *n = _lastCombine;
        for ( ; n <= m; n = n->next() );
        m = n;
        st_assert( m->isValid(), "something's wrong" );
    }

    while ( m->isUnused() ) {
        m = m->next();
    }

    st_assert( m->isValid(), "something's wrong" );
    st_assert( m <= heapEnd(), "past end of heap" );
    if ( m == heapEnd() ) {
        return nullptr;                    // was last one
    } else {
        const void *next = blockAddr( m );
        st_assert( next > p, "must be monotonic" );
        st_assert( not( std::int32_t( next ) & 1 ), "must be even" );
        return next;
    }
}


const void *ZoneHeap::findStartOfBlock( const void *start ) const {
    // used a lot -- help the optimizer a bit
    if ( _newHeap and _newHeap->contains( start ) )
        return _newHeap->findStartOfBlock( start );

    const std::int32_t blockSz = blockSize;
    start = (void *) ( std::int32_t( start ) & ~( blockSz - 1 ) );
    st_assert( contains( start ), "start not in this zone" );
    ChunkKlass *m = mapAddr( start )->findStart( _heapKlass, heapEnd() );
    return blockAddr( m );
}


std::int32_t ZoneHeap::sizeOfBlock( void *p ) const {
    return mapAddr( p )->size() << log2BS;
}


void ZoneHeap::verify() const {
    ChunkKlass *m   = _heapKlass;
    ChunkKlass *end = heapEnd();
    if ( end->isUnused() or end->size() not_eq 1 )
        error( "wrong end-sentinel %d in heap 0x{0:x}", *(std::uint8_t *) end, this );
    ChunkKlass *begin = asChunkKlass( _heapKlass->asByte() - 1 );
    if ( begin->isUnused() or begin->size() not_eq 1 )
        error( "wrong begin-sentinel %d in heap 0x{0:x}", *(std::uint8_t *) begin, this );

    // verify map structure
    while ( m < end ) {
        if ( not m->verify() )
            //SPDLOG_INFO( " in heap {0:x}", static_cast<const void *>(this) );
        m = m->next();
    }

    // verify free lists
    std::int32_t    i  = 0;
    for ( ; i < nfree; i++ ) {
        std::int32_t    j        = 0;
        std::int32_t    lastSize = 0;
        HeapChunk       *f       = _freeList[ i ].anchor();
        for ( HeapChunk *h       = f->next(); h not_eq f; h = h->next(), j++ ) {
            ChunkKlass *p = mapAddr( h );
            if ( not p->verify() )
                SPDLOG_INFO( " in free list{0:d} (elem{0:d}) of heap 0x{0:x}", i, j, static_cast<const void *>(this) );
            if ( p->isUsed() ) {
                error( "inconsistent freeList %ld elem 0x{0:x} in heap 0x{0:x} (map 0x{0:x})", i, h, this, p );
            }
            if ( p->size() not_eq lastSize and j not_eq 0 ) {
                error( "freeList %ld elem 0x{0:x} in heap 0x{0:x} (map 0x{0:x}) has wrong size", i, h, this, p );
            }
            lastSize = p->size();
            if ( h->next() == h ) {
                error( "loop in freeList %ld elem 0x{0:x} in heap 0x{0:x}", i, h, this );
                break;
            }
        }
    }
    std::int32_t    j  = 0;
    HeapChunk       *f = _bigList->anchor();
    for ( HeapChunk *h = f->next(); h not_eq f; h = h->next(), j++ ) {
        ChunkKlass *p = mapAddr( h );
        if ( not p->verify() )
            SPDLOG_INFO( " in bigList (elem %ld) of heap 0x{0:x}", j, static_cast<const void *>(this) );
        if ( p->isUsed() ) {
            error( "inconsistent freeList %ld elem 0x{0:x} in heap 0x{0:x} (map 0xlx)", i, h, this, p );
        }
    }
    if ( not _lastCombine->verify() )
        error( " chunkState::invalid lastCombine in heap 0x{0:x}", this );

}


void ZoneHeap::print() const {
    //SPDLOG_INFO( "0x{0:x}: [0x{0:x}..0x{0:x})", static_cast<const void *>(this), base, base + capacity() );
    printIndent();
    SPDLOG_INFO( "  size %ld (blk %ld), used %ld (%1.1f%%), ifrag %1.1f%%;", capacity(), blockSize, usedBytes(), 100.0 * usedBytes() / capacity(), 100.0 * intFrag() );
    printIndent();
    SPDLOG_INFO( "  grand total allocs = %ld bytes", _total );
    printIndent();
    SPDLOG_INFO( "  free lists: " );
    for ( std::size_t i = 0; i < nfree; i++ )
        SPDLOG_INFO( "%ld ", _freeList[ i ].length() );
    SPDLOG_INFO( "; %ld", _bigList->length() );
}
