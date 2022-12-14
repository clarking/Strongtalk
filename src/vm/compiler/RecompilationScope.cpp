
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/compiler/InliningPolicy.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


RecompilationScope::RecompilationScope( NonDummyRecompilationScope *s, std::int32_t byteCodeIndex ) :
    _sender{ s },
    _senderByteCodeIndex( byteCodeIndex ),
    _invocationCount{ 0 } {

    if ( s ) {
        s->addScope( byteCodeIndex, this );
        _invocationCount = s->_invocationCount;
    } else {
        _invocationCount = -1;
    }

}


GrowableArray<RecompilationScope *> *NullRecompilationScope::subScopes( std::int32_t byteCodeIndex ) const {
    st_unused( byteCodeIndex ); // unused
    return new GrowableArray<RecompilationScope *>( 1 );
}


static std::int32_t compare_pcDescs( ProgramCounterDescriptor **a, ProgramCounterDescriptor **b ) {
    // to sort by descending scope and ascending byteCodeIndex
    std::int32_t diff = ( *b )->_scope - ( *a )->_scope;
    return diff ? diff : ( *a )->_byteCodeIndex - ( *b )->_byteCodeIndex;
}


std::int32_t NonDummyRecompilationScope::compare( NonDummyRecompilationScope **a, NonDummyRecompilationScope **b ) {
    return ( *b )->scopeID() - ( *a )->scopeID();
}


NonDummyRecompilationScope::NonDummyRecompilationScope( NonDummyRecompilationScope *s, std::int32_t byteCodeIndex, MethodOop m, std::int32_t level ) :
    RecompilationScope( s, byteCodeIndex ),
    _level( level ),
    _ncodes( m == nullptr ? 1 : m->size_of_codes() * OOP_SIZE ),
    _subScopes( new_resource_array<GrowableArray<RecompilationScope *> *>( _ncodes + 1 ) ),
    uncommon( 1 ) {
    for ( std::size_t i = 0; i <= _ncodes; i++ )
        _subScopes[ i ] = nullptr;
}


InlinedRecompilationScope::InlinedRecompilationScope( NonDummyRecompilationScope *s, std::int32_t byteCodeIndex, const NativeMethod *n, ScopeDescriptor *d, std::int32_t level ) :
    NonDummyRecompilationScope( s, byteCodeIndex, d->method(), level ), nm( n ), desc( d ) {
}


PICRecompilationScope::PICRecompilationScope( const NativeMethod *c, ProgramCounterDescriptor *pc, CompiledInlineCache *s, KlassOop k, ScopeDescriptor *dsc, NativeMethod *n, MethodOop m, std::int32_t ns, std::int32_t lev, bool tr ) :
    NonDummyRecompilationScope( nullptr, pc->_byteCodeIndex, m, lev ),
    caller{ c },
    _sd{ s },
    programCounterDescriptor{ pc },
    klass{ k },
    nm{ n },
    _method{ m },
    trusted{ tr },
    _extended{ false },
    _desc{ dsc } {

    //
    _invocationCount = ns;
}


InliningDatabaseRecompilationScope::InliningDatabaseRecompilationScope( NonDummyRecompilationScope *sender, std::int32_t byteCodeIndex, KlassOop receiver_klass, MethodOop method, std::int32_t level ) :
    NonDummyRecompilationScope( sender, byteCodeIndex, method, level ),
    _receiver_klass{ receiver_klass },
    _method{ method },
    _key{ nullptr },
    _uncommon{ nullptr } {

    //
    _receiver_klass = receiver_klass;
    _method         = method;
    _key            = LookupKey::allocate( receiver_klass, method->is_blockMethod() ? Oop( method ) : Oop( _method->selector() ) );
    _uncommon       = new GrowableArray<bool>( _ncodes );

    //
    for ( std::size_t i = 0; i <= _ncodes; i++ ) {
        _uncommon->append( false );
    }

}


UntakenRecompilationScope::UntakenRecompilationScope( NonDummyRecompilationScope *sender, ProgramCounterDescriptor *p, bool u ) :
    NonDummyRecompilationScope( sender, p->_byteCodeIndex, nullptr, 0 ), isUncommon( u ), pc( p ) {
//    std::int32_t i = 0;    // to allow setting breakpoints
}


UninlinableRecompilationScope::UninlinableRecompilationScope( NonDummyRecompilationScope *sender, std::int32_t byteCodeIndex ) :
    NullRecompilationScope( sender, byteCodeIndex ) {
//    std::int32_t i = 0;    // to allow setting breakpoints
}


InterpretedRecompilationScope::InterpretedRecompilationScope( NonDummyRecompilationScope *sender, std::int32_t byteCodeIndex, LookupKey *key, MethodOop m, std::int32_t level, bool trusted ) :
    NonDummyRecompilationScope( sender, byteCodeIndex, m, level ),
    _key{ key },
    _method{ m },
    _is_trusted{ trusted },
    extended{ false } {
    _invocationCount = m->invocation_count();
}


LookupKey *InlinedRecompilationScope::key() const {
    return desc->key();
}


LookupKey *PICRecompilationScope::key() const {
    // If we have a NativeMethod, return the key of the NativeMethod
    if ( nm ) {
        return (LookupKey *) &nm->_lookupKey;
    }

    // If we have a scope desc, return the key of the scope desc
    if ( _desc ) {
        return _desc->key();
    }

    // If we have a send desc, return an allocated lookup key
    if ( sd() ) {
        return sd()->isSuperSend() ? LookupKey::allocate( klass, _method ) : LookupKey::allocate( klass, _method->selector() );
    }

    ShouldNotReachHere();
    // return nm ? (LookupKey*)&nm->key : LookupKey::allocate(klass, _method->selector());
    //			// potential bug -- is key correct?  (super sends) -- fix this
    return nullptr;
}


MethodOop InlinedRecompilationScope::method() const {
    return desc->method();
}


bool RecompilationScope::wasNeverExecuted() const {
    MethodOop m   = method();
    SymbolOop sel = m->selector();
    if ( InliningPolicy::isInterpreterPredictedSmiSelector( sel ) or InliningPolicy::isInterpreterPredictedBoolSelector( sel ) ) {
        // predicted methods aren't called by interpreter if prediction succeeds
        return false;
    } else {
        return m->was_never_executed();
    }
}


// equivalent: test whether receiver scope and argument (a InlinedScope or a LookupKey) denote the same source-level scope

bool InterpretedRecompilationScope::equivalent( LookupKey *l ) const {
    return _key->equal( l );
}


bool InterpretedRecompilationScope::equivalent( InlinedScope *s ) const {
    return _key->equal( s->key() );
}


bool InlinedRecompilationScope::equivalent( LookupKey *l ) const {
    return desc->l_equivalent( l );
}


bool InlinedRecompilationScope::equivalent( InlinedScope *s ) const {
    if ( not s->isInlinedScope() )
        return false;
    InlinedScope *ss = (InlinedScope *) s;
    // don't use ss->rscope because it may not be set yet; but ss's sender
    // must have an rscope if ss is equivalent to this.
    return ss->senderByteCodeIndex() == desc->senderByteCodeIndex() and ss->sender()->rscope == sender();
}


bool PICRecompilationScope::equivalent( InlinedScope *s ) const {
    st_unused( s ); // unused
    // an PICRecompilationScope represents a non-inlined scope, so it can't be equivalent to any InlinedScope
    return false;
}


bool PICRecompilationScope::equivalent( LookupKey *l ) const {
    if ( _desc not_eq nullptr )
        return _desc->l_equivalent( l );   // compiled case

    st_assert( not _sd->isSuperSend(), "this code probably doesn't work for super sends" );
    return klass == l->klass() and _method->selector() == l->selector();
}


RecompilationScope *NonDummyRecompilationScope::subScope( std::int32_t byteCodeIndex, LookupKey *k ) const {
    // return the subscope matching the lookup
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < _ncodes, "byteCodeIndex out of range" );
    GrowableArray<RecompilationScope *> *list = _subScopes[ byteCodeIndex ];
    if ( list == nullptr ) {
        return new NullRecompilationScope;
    }

    for ( std::size_t i = 0; i < list->length(); i++ ) {
        RecompilationScope *rs = list->at( i );
        if ( rs->equivalent( k ) )
            return rs;
    }
    return new NullRecompilationScope;
}


GrowableArray<RecompilationScope *> *NonDummyRecompilationScope::subScopes( std::int32_t byteCodeIndex ) const {
    // return all subscopes at byteCodeIndex
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < _ncodes, "byteCodeIndex out of range" );
    GrowableArray<RecompilationScope *> *list = _subScopes[ byteCodeIndex ];
    if ( list == nullptr )
        return new GrowableArray<RecompilationScope *>( 1 );
    return list;
}


bool NonDummyRecompilationScope::hasSubScopes( std::int32_t byteCodeIndex ) const {
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < _ncodes, "byteCodeIndex out of range" );
    return _subScopes[ byteCodeIndex ] not_eq nullptr;
}


void NonDummyRecompilationScope::addScope( std::int32_t byteCodeIndex, RecompilationScope *s ) {
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < _ncodes, "byteCodeIndex out of range" );
    if ( _subScopes[ byteCodeIndex ] == nullptr )
        _subScopes[ byteCodeIndex ] = new GrowableArray<RecompilationScope *>( 5 );

    st_assert( not _subScopes[ byteCodeIndex ]->contains( s ), "already there" );
    // remove uninlineble markers if real scopes are added
    if ( _subScopes[ byteCodeIndex ]->length() == 1 and _subScopes[ byteCodeIndex ]->first()->isUninlinableScope() ) {
        _subScopes[ byteCodeIndex ]->pop();
    }
    _subScopes[ byteCodeIndex ]->append( s );
}


bool InterpretedRecompilationScope::isUncommonAt( std::int32_t byteCodeIndex ) const {
    st_unused( byteCodeIndex ); // unused
    return DeferUncommonBranches;
}


bool NonDummyRecompilationScope::isUncommonAt( std::int32_t byteCodeIndex ) const {
    if ( _subScopes[ byteCodeIndex ] ) {
        RecompilationScope *s = _subScopes[ byteCodeIndex ]->first();
        if ( s and s->isUntakenScope() ) {
            // send was never executed - make it uncommon
            return true;
        }
    }
    return false;
}


bool NonDummyRecompilationScope::isNotUncommonAt( std::int32_t byteCodeIndex ) const {
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < _ncodes, "byteCodeIndex out of range" );

    // check if program got uncommon trap in the past
    for ( std::size_t i = 0; i < uncommon.length(); i++ ) {
        if ( uncommon.at( i )->byteCodeIndex() == byteCodeIndex )
            return true;
    }

    if ( _subScopes[ byteCodeIndex ] ) {
        RecompilationScope *s = _subScopes[ byteCodeIndex ]->first();
        if ( s and not s->isUntakenScope() ) {
            // send was executed at least once - don't make it uncommon
            return true;
        }
    }
    return false;
}


Expression *RecompilationScope::receiverExpression( PseudoRegister *p ) const {
    // guess that true/false map really means true/false object
    // (gives more efficient testing code)
    KlassOop k = receiverKlass();
    if ( k == trueObject->klass() ) {
        return new ConstantExpression( trueObject, p, nullptr );

    } else if ( k == falseObject->klass() ) {
        return new ConstantExpression( falseObject, p, nullptr );

    } else {
        return new KlassExpression( k, p, nullptr );
    }
}


Expression *UntakenRecompilationScope::receiverExpression( PseudoRegister *p ) const {
    return new UnknownExpression( p, nullptr, true );
}


bool InlinedRecompilationScope::isLite() const {
    return desc->is_lite();
}


bool PICRecompilationScope::isLite() const {
    return _desc and _desc->is_lite();
}


KlassOop InterpretedRecompilationScope::receiverKlass() const {
    return _key->klass();
}


KlassOop InlinedRecompilationScope::receiverKlass() const {
    return desc->selfKlass();
}


void NonDummyRecompilationScope::unify( NonDummyRecompilationScope *s ) {
    st_assert( _ncodes == s->_ncodes, "should be the same" );
    for ( std::size_t i = 0; i < _ncodes; i++ ) {
        _subScopes[ i ] = s->_subScopes[ i ];
        if ( _subScopes[ i ] ) {
            for ( std::int32_t j = _subScopes[ i ]->length() - 1; j >= 0; j-- ) {
                _subScopes[ i ]->at( j )->_sender = this;
            }
        }
    }
}


void PICRecompilationScope::unify( NonDummyRecompilationScope *s ) {
    NonDummyRecompilationScope::unify( s );
    if ( s->isPICScope() ) {
        uncommon.appendAll( &( (PICRecompilationScope *) s )->uncommon );
    }

}


constexpr std::int32_t UntrustedPICLimit = 2;
constexpr std::int32_t PICTrustLimit     = 2;


static void getCallees( const NativeMethod *nm, GrowableArray<ProgramCounterDescriptor *> *&taken_uncommon, GrowableArray<ProgramCounterDescriptor *> *&untaken_uncommon, GrowableArray<ProgramCounterDescriptor *> *&uninlinable, GrowableArray<NonDummyRecompilationScope *> *&sends, bool trusted, std::int32_t level ) {
    // return a list of all uncommon branches of nm, plus a list
    // of all nativeMethods called by nm (in the form of PICScopes)
    // all lists are sorted by scope (biggest offset first)
    if ( theCompiler and CompilerDebug ) {
        cout( PrintRScopes )->print( "%*s*searching nm 0x{0:x} \"%s\" (%strusted; %ld callers)\n", 2 * level, "", nm, nm->_lookupKey.selector()->as_string(), trusted ? "" : "not ", nm->ncallers() );
    }
    taken_uncommon   = new GrowableArray<ProgramCounterDescriptor *>( 1 );
    untaken_uncommon = new GrowableArray<ProgramCounterDescriptor *>( 16 );
    uninlinable      = new GrowableArray<ProgramCounterDescriptor *>( 16 );
    sends            = new GrowableArray<NonDummyRecompilationScope *>( 10 );
    RelocationInformationIterator iter( nm );
    while ( iter.next() ) {
        if ( iter.type() == RelocationInformation::RelocationType::uncommon_type ) {
            GrowableArray<ProgramCounterDescriptor *> *l = iter.wasUncommonTrapExecuted() ? taken_uncommon : untaken_uncommon;
            l->append( nm->containingProgramCounterDescriptor( (const char *) iter.word_addr() ) );
        }
    }

    taken_uncommon->sort( &compare_pcDescs );
    untaken_uncommon->sort( &compare_pcDescs );

    if ( TypeFeedback ) {
        RelocationInformationIterator iter( nm );
        while ( iter.next() ) {
            if ( iter.type() not_eq RelocationInformation::RelocationType::ic_type )
                continue;
            CompiledInlineCache      *sd = iter.ic();
            ProgramCounterDescriptor *p  = nm->containingProgramCounterDescriptor( (const char *) sd );
            if ( sd->wasNeverExecuted() ) {
                // this send was never taken
                sends->append( new UntakenRecompilationScope( nullptr, p, false ) );
            } else if ( sd->isUninlinable() or sd->isMegamorphic() ) {
                // don't inline this send
                uninlinable->append( p );
            } else {
                bool useInfo = trusted or sd->ntargets() <= UntrustedPICLimit;
                if ( useInfo ) {
                    CompiledInlineCacheIterator it( sd );
                    while ( not it.at_end() ) {
                        NativeMethod    *callee = it.compiled_method();
                        MethodOop       m       = it.interpreted_method();
                        ScopeDescriptor *desc;
                        std::int32_t    count;
                        if ( callee not_eq nullptr ) {
                            // compiled target
                            desc  = callee->scopes()->root();
                            count = callee->invocation_count() / max( 1, callee->ncallers() );
                        } else {
                            // interpreted target
                            desc  = nullptr;
                            count = m->invocation_count();
                        }
                        sends->append( new PICRecompilationScope( nm, p, sd, it.klass(), desc, callee, m, count, level, trusted ) );
                        it.advance();
                    }
                } else if ( theCompiler and CompilerDebug ) {
                    cout( PrintRScopes )->print( "%*s*not trusting PICs in sd 0x{0:x} \"%s\" (%ld cases)\n", 2 * level, "", sd, sd->selector()->as_string(), sd->ntargets() );
                }
            }
        }
        sends->sort( &PICRecompilationScope::compare );
        uninlinable->sort( &compare_pcDescs );
    }
}


NonDummyRecompilationScope *NonDummyRecompilationScope::constructRScopes( const NativeMethod *nm, bool trusted, std::int32_t level ) {
    // construct nm's RecompilationScope tree and return the root
    // level > 0 means recursive invocation through a PICRecompilationScope (level
    // is the recursion depth); trusted means PICs info is considered accurate
    NonDummyRecompilationScope                  *current = nullptr;
    NonDummyRecompilationScope                  *root    = nullptr;
    GrowableArray<ProgramCounterDescriptor *>   *taken_uncommon;
    GrowableArray<ProgramCounterDescriptor *>   *untaken_uncommon;
    GrowableArray<ProgramCounterDescriptor *>   *uninlinable;
    GrowableArray<NonDummyRecompilationScope *> *sends;
    getCallees( nm, taken_uncommon, untaken_uncommon, uninlinable, sends, trusted, level );

    // visit each scope in the debug info and enter it into the tree
    FOR_EACH_SCOPE( nm->scopes(), s ) {
        // search s' sender RecompilationScope
        ScopeDescriptor            *sender  = s->sender();
        NonDummyRecompilationScope *rsender = current;
        for ( ; rsender; rsender = rsender->sender() ) {
            if ( rsender->isInlinedScope() and ( (InlinedRecompilationScope *) rsender )->desc->is_equal( sender ) )
                break;
        }
        std::int32_t byteCodeIndex = sender ? s->senderByteCodeIndex() : IllegalByteCodeIndex;
        current = new InlinedRecompilationScope( (InlinedRecompilationScope *) rsender, byteCodeIndex, nm, s, level );
        if ( not root ) {
            root = current;
            root->_invocationCount = nm->invocation_count();
        }

        // enter byteCodeIndexs with taken uncommon branches
        while ( taken_uncommon->nonEmpty() and taken_uncommon->top()->_scope == s->offset() ) {
            current->uncommon.push( new RUncommonBranch( current, taken_uncommon->pop() ) );
        }

        // enter info from PICs
        while ( sends->nonEmpty() and sends->top()->scopeID() == s->offset() ) {
            NonDummyRecompilationScope *s = sends->pop();
            s->_sender = current;
            current->addScope( s->senderByteCodeIndex(), s );
        }

        // enter untaken uncommon branches
        ProgramCounterDescriptor *u;
        while ( untaken_uncommon->nonEmpty() and ( u = untaken_uncommon->top() )->_scope == s->offset() ) {
            new UntakenRecompilationScope( current, u, true );    // will add it as subscope of current
            untaken_uncommon->pop();
        }

        // enter uninlinable sends
        while ( uninlinable->nonEmpty() and ( u = uninlinable->top() )->_scope == s->offset() ) {
            // only add uninlinable markers for sends that have no inlined cases
            std::int32_t byteCodeIndex = u->_byteCodeIndex;
            if ( not current->hasSubScopes( byteCodeIndex ) ) {
                new UninlinableRecompilationScope( current, byteCodeIndex );    // will add it as subscope of current
            }
            uninlinable->pop();
        }
    }

    st_assert( sends->isEmpty(), "sends should have been connected to rscopes" );
    st_assert( taken_uncommon->isEmpty(), "taken uncommon branches should have been connected to rscopes" );
    st_assert( untaken_uncommon->isEmpty(), "untaken uncommon branches should have been connected to rscopes" );
    st_assert( uninlinable->isEmpty(), "uninlinable sends should have been connected to rscopes" );
    return root;
}


void NonDummyRecompilationScope::constructSubScopes( bool trusted ) {
    // construct all our (immediate) subscopes
    MethodOop m = method();
    if ( m->is_accessMethod() )
        return;
    CodeIterator iter( m );
    do {
        switch ( iter.send() ) {
            case ByteCodes::SendType::INTERPRETED_SEND:
            case ByteCodes::SendType::COMPILED_SEND:
            case ByteCodes::SendType::PREDICTED_SEND:
            case ByteCodes::SendType::ACCESSOR_SEND:
            case ByteCodes::SendType::POLYMORPHIC_SEND:
            case ByteCodes::SendType::PRIMITIVE_SEND  : {
//                NonDummyRecompilationScope           *s  = nullptr;
                InterpretedInlineCache               *ic = iter.ic();
                for ( InterpretedInlineCacheIterator it( ic ); not it.at_end(); it.advance() ) {
                    if ( it.is_compiled() ) {
                        NativeMethod               *nm = it.compiled_method();
                        NonDummyRecompilationScope *s  = constructRScopes( nm, trusted and trustPICs( m ), _level + 1 );
                        addScope( iter.byteCodeIndex(), s );
                    } else {
                        MethodOop m  = it.interpreted_method();
                        LookupKey *k = LookupKey::allocate( it.klass(), it.selector() );
                        new InterpretedRecompilationScope( this, iter.byteCodeIndex(), k, m, _level + 1, trusted and trustPICs( m ) );
                        // NB: constructor adds callee to our subScope list
                    }
                }
            }
                break;
            case ByteCodes::SendType::MEGAMORPHIC_SEND:
                new UninlinableRecompilationScope( this, iter.byteCodeIndex() );  // constructor adds callee to our subScope list
                break;
            case ByteCodes::SendType::NO_SEND:
                break;
            default: st_fatal1( "unexpected send type %d", iter.send() );
        }
    } while ( iter.advance() );
}


bool NonDummyRecompilationScope::trustPICs( MethodOop m ) {
    // should the PICs in m be trusted?
    SymbolOop sel = m->selector();
    if ( sel == vmSymbols::plus() or sel == vmSymbols::minus() or sel == vmSymbols::multiply() or sel == vmSymbols::divide() ) {
        // code Space optimization: try to avoid unnecessary mixed-type arithmetic
        return false;
    } else {
        return true;    // can't easily determine number of callers
    }
}


bool PICRecompilationScope::trustPICs( const NativeMethod *nm ) {
    // should the PICs in nm be trusted?
    std::int32_t invoc = nm->invocation_count();
    if ( invoc < MinInvocationsBeforeTrust )
        return false;

    std::int32_t ncallers = nm->ncallers();
    SymbolOop    sel      = nm->_lookupKey.selector();

    if ( sel == vmSymbols::plus() or sel == vmSymbols::minus() or sel == vmSymbols::multiply() or sel == vmSymbols::divide() ) {
        // code Space optimization: try to avoid unnecessary mixed-type arithmetic
        return ncallers <= 1;
    } else {
        return ncallers <= PICTrustLimit;
    }
}


void PICRecompilationScope::extend() {
    // try to follow PolymorphicInlineCache info one level deeper (i.e. extend rscope tree)
    if ( _extended )
        return;
    if ( nm and not nm->isZombie() ) {
        // search the callee for type info
        NonDummyRecompilationScope *s = constructRScopes( nm, trusted and trustPICs( nm ), _level + 1 );
        // s and receiver represent the same scope - unify them
        unify( s );
    } else {
        constructSubScopes( false );    // search interpreted inline caches but don't trust their info
    }
    _extended = true;
}


void InterpretedRecompilationScope::extend() {
    // try to follow PolymorphicInlineCache info one level deeper (i.e. extend rscope tree)
    if ( not extended ) {
        // search the inline caches for type info
        constructSubScopes( _is_trusted );
        if ( PrintRScopes )
            printTree( _senderByteCodeIndex, _level );
    }
    extended = true;
}


void RecompilationScope::print() {
  //  SPDLOG_INFO( "; sender: 0x{0:x}@%ld; count %ld", static_cast<const void *>( PrintHexAddresses ? _sender : 0 ), _senderByteCodeIndex, _invocationCount );
}


void NonDummyRecompilationScope::printSubScopes() const {
    std::int32_t i = 0;
    for ( ; i < _ncodes and _subScopes[ i ] == nullptr; i++ );
    if ( i < _ncodes ) {
        _console->print( "{ " );
        for ( std::size_t i = 0; i < _ncodes; i++ ) {
            _console->print( "0x{0:x} ", PrintHexAddresses ? _subScopes[ i ] : 0 );
        }
        _console->print( "}" );
    } else {
        SPDLOG_INFO( "none" );
    }
}


void InterpretedRecompilationScope::print_short() {
   // SPDLOG_INFO( "((InterpretedRecompilationScope*)0x{0:x}) [{}] 0x{0:x}", static_cast<void *>( PrintHexAddresses ? this : 0 ), _key->toString(), _invocationCount );
}


void InlinedRecompilationScope::print_short() {
   // SPDLOG_INFO( "((InlinedRecompilationScope*)0x{0:x}) [{}] 0x{0:x}", static_cast<void *>( PrintHexAddresses ? this : 0 ), desc->selector()->as_string(), _invocationCount );
}


void InlinedRecompilationScope::print() {
    print_short();
    _console->print( ": scope 0x{0:x}; subScopes: ", PrintHexAddresses ? desc : 0 );
    printSubScopes();
    if ( uncommon.nonEmpty() ) {
        _console->print( "; uncommon " );
        uncommon.print();
    }
    RecompilationScope::print();
}


void PICRecompilationScope::print_short() {
   // SPDLOG_INFO( "((PICRecompilationScope*) 0x{0:x}) [{}] 0x{0:x}", static_cast<void *>( PrintHexAddresses ? this : 0), method()->selector()->as_string(), _invocationCount );
}


void PICRecompilationScope::print() {
    print_short();
   // SPDLOG_INFO( ": InlineCache 0x{0:x}; subScopes: ", static_cast<const void *>( PrintHexAddresses ? _sd : 0 ) );
    printSubScopes();
    if ( uncommon.nonEmpty() ) {
        _console->print( "; uncommon " );
        uncommon.print();
    }
}


void UntakenRecompilationScope::print_short() {
    _console->print( "((UntakenRecompilationScope*)0x{0:x}) [{}]", static_cast<const void *>( PrintHexAddresses ? this : 0 ) );
}


void UntakenRecompilationScope::print() {
    print_short();
    st_assert( !*_subScopes, "should have no subscopes" );
    st_assert( uncommon.isEmpty(), "should have no uncommon branches" );
}


void RUncommonBranch::print() {
   // SPDLOG_INFO( "((RUncommonScope*)0x{0:x}) : 0x{0:x}@%ld", static_cast<const void *>(PrintHexAddresses ? this : 0), static_cast<const void *>(PrintHexAddresses ? scope : 0), byteCodeIndex() );
}


void UninlinableRecompilationScope::print_short() {
   // SPDLOG_INFO( "((UninlinableRecompilationScope*)0x{0:x})", static_cast<const void *>(PrintHexAddresses ? this : 0 ) );
}


void NullRecompilationScope::print_short() {
   // SPDLOG_INFO( "((NullRecompilationScope*)0x{0:x})", static_cast<const void *>( PrintHexAddresses ? this : 0 ) );
}


void NullRecompilationScope::printTree( std::int32_t byteCodeIndex, std::int32_t level ) const {
    st_unused( byteCodeIndex ); // unused
    st_unused( level ); // unused
}


void RecompilationScope::printTree( std::int32_t byteCodeIndex, std::int32_t level ) const {
   // SPDLOG_INFO( "{:s} {:3d} ", level * 2, "", byteCodeIndex );
    ( (RecompilationScope *) this )->print_short();
    SPDLOG_INFO( "" );
}


void NonDummyRecompilationScope::printTree( std::int32_t senderByteCodeIndex, std::int32_t level ) const {
    RecompilationScope::printTree( senderByteCodeIndex, level );

    std::int32_t u = 0;          // current position in uncommon

    for ( std::int32_t byteCodeIndex = 0; byteCodeIndex < _ncodes; byteCodeIndex++ ) {
        if ( _subScopes[ byteCodeIndex ] ) {
            for ( std::size_t j = 0; j < _subScopes[ byteCodeIndex ]->length(); j++ ) {
                _subScopes[ byteCodeIndex ]->at( j )->printTree( byteCodeIndex, level + 1 );
            }
        }
        std::int32_t j = u;
        for ( ; j < uncommon.length() and uncommon.at( j )->byteCodeIndex() < byteCodeIndex; u++, j++ );
        if ( j < uncommon.length() and uncommon.at( j )->byteCodeIndex() == byteCodeIndex ) {
            SPDLOG_INFO( "  %*s%3ld: uncommson", level * 2, "", byteCodeIndex );
        }
    }
}


void InliningDatabaseRecompilationScope::print() {
    print_short();
    printSubScopes();
}


void InliningDatabaseRecompilationScope::print_short() {
   // SPDLOG_INFO( "((InliningDatabaseRecompilationScope*)0x{0:x}) [{}]", static_cast<const void *>(PrintHexAddresses ? this : 0), _key->toString() );
}


bool InliningDatabaseRecompilationScope::equivalent( InlinedScope *s ) const {
    st_unused( s ); // unused
    Unimplemented();
    return false;
}


bool InliningDatabaseRecompilationScope::equivalent( LookupKey *l ) const {
    return _key->equal( l );
}


bool InliningDatabaseRecompilationScope::isUncommonAt( std::int32_t byteCodeIndex ) const {
    // if the DB has an uncommon branch at byteCodeIndex, treat it as untaken
    return _uncommon->at( byteCodeIndex );
}


bool InliningDatabaseRecompilationScope::isNotUncommonAt( std::int32_t byteCodeIndex ) const {
    // if there's no uncommon branch in the DB, don't use it here either
    return not _uncommon->at( byteCodeIndex );
}


std::int32_t InlinedRecompilationScope::inlining_database_size() {
    std::int32_t result = 1; // Count this node

    for ( std::size_t i = 0; i < _ncodes; i++ ) {
        if ( _subScopes[ i ] ) {
            for ( std::size_t j = 0; j < _subScopes[ i ]->length(); j++ ) {
                result += _subScopes[ i ]->at( j )->inlining_database_size();
            }
        }
    }
    return result;
}



// don't file out PolymorphicInlineCache scopes in the output since they're not inlined into the current NativeMethod;
// same for interpreted scopes

ProgramCounterDescriptor *next_uncommon( std::int32_t scope, std::int32_t u, GrowableArray<ProgramCounterDescriptor *> *uncommon ) {
    if ( uncommon == nullptr or u >= uncommon->length() )
        return nullptr;   // none left
    ProgramCounterDescriptor *pc = uncommon->at( u );
    return ( pc->_scope == scope ) ? pc : nullptr;
}


void UninlinableRecompilationScope::print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, std::int32_t byteCodeIndex, std::int32_t level ) {
    st_unused( stream ); // unused
    st_unused( uncommon ); // unused
    st_unused( byteCodeIndex ); // unused
    st_unused( level ); // unused

    // not necessary to actually write out this info since DB-driven compilation won't inline anything not inlined in DB
    // stream->print_cr("%*s%d uninlinable", level * 2, "", byteCodeIndex);
}


void InlinedRecompilationScope::print_inlining_database_on( ConsoleOutputStream *stream, GrowableArray<ProgramCounterDescriptor *> *uncommon, std::int32_t byteCodeIndex, std::int32_t level ) {
    // File out level and byteCodeIndex
    if ( byteCodeIndex not_eq -1 ) {
        stream->print( "%*s%d ", level * 2, "", byteCodeIndex );
    }

    LookupKey *k = key();
    k->print_inlining_database_on( stream );
    stream->cr();

    // find scope in uncommon list
    std::int32_t scope = desc->offset();
    std::int32_t u     = 0;
    for ( ; uncommon and u < uncommon->length() - 1 and uncommon->at( u )->_scope < scope; u++ );
    ProgramCounterDescriptor *current_uncommon = next_uncommon( scope, u, uncommon );

    // File out subscopes
    for ( std::size_t i = 0; i < _ncodes; i++ ) {
        if ( _subScopes[ i ] ) {
            for ( std::size_t j = 0; j < _subScopes[ i ]->length(); j++ ) {
                _subScopes[ i ]->at( j )->print_inlining_database_on( stream, uncommon, i, level + 1 );
            }
        }
        if ( current_uncommon and current_uncommon->_byteCodeIndex == i ) {
            // NativeMethod has an uncommon branch at this byteCodeIndex
            stream->print_cr( "%*s%d uncommon", ( level + 1 ) * 2, "", i );
            // advance to next uncommon branch
            u++;
            current_uncommon = next_uncommon( scope, u, uncommon );
        }
    }
}
