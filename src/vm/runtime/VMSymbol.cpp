
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/VMSymbol.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/memory/OopFactory.hpp"

std::array<SymbolOop, terminating_enum> vm_symbols;

#define VMSYMBOL_INIT(name, string) \
  vm_symbols[ VMSYMBOL_ENUM_NAME( name ) ] = OopFactory::new_symbol( string );

void vmSymbols::initialize() {
	VMSYMBOLS(VMSYMBOL_INIT)
}

void vmSymbols::switch_pointers(Oop from, Oop to) {
	for (std::size_t i = 0; i < terminating_enum; i++) {
		Oop *p = (Oop *) &vm_symbols[i];
		SWITCH_POINTERS_TEMPLATE(p)
	}
}

void vmSymbols::follow_contents() {
	for (std::size_t i = 0; i < terminating_enum; i++) {
		MarkSweep::follow_root((Oop *) &vm_symbols[i]);
	}
}

void vmSymbols::relocate() {
	for (std::size_t i = 0; i < terminating_enum; i++) {
		Oop *p = (Oop *) &vm_symbols[i];
		RELOCATE_TEMPLATE(p);
	}
}

void vmSymbols::verify() {

}
