//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/HeapCodeBuffer.hpp"
#include "vm/oop/SmallIntegerOopDescriptor.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/oop/ByteArrayOopDescriptor.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"

void HeapCodeBuffer::align() {
	while (not isAligned()) {
		_bytes->append(0xFF);
	}
}

bool HeapCodeBuffer::isAligned() {
	return (_bytes->length() % OOP_SIZE) == 0;
}

void HeapCodeBuffer::pushByte(std::uint8_t op) {

	if (isAligned()) {
		_oops->append(smiOopFromValue(0));
	}
	_bytes->append(op);
}

void HeapCodeBuffer::pushOop(Oop arg) {
	align();
	_bytes->append(0);
	_bytes->append(0);
	_bytes->append(0);
	_bytes->append(0);

	_oops->append(arg);
}

ByteArrayOop HeapCodeBuffer::bytes() {
	BlockScavenge bs;
	align();
	Klass *klass = Universe::byteArrayKlassObject()->klass_part();
	ByteArrayOop result = ByteArrayOop(klass->allocateObjectSize(byteLength()));

	for (std::size_t i = 0; i < byteLength(); i++) {
		result->byte_at_put(i + 1, (std::uint8_t) _bytes->at(i));
	}

	return result;
}

ObjectArrayOop HeapCodeBuffer::oops() {
	BlockScavenge bs;
	Klass *klass = Universe::objectArrayKlassObject()->klass_part();
	ObjectArrayOop result = ObjectArrayOop(klass->allocateObjectSize(oopLength()));

	for (std::size_t i = 0; i < oopLength(); i++) {
		result->obj_at_put(i + 1, _oops->at(i));
	}

	return result;
}
