//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/runtime/Bootstrap.hpp"

void ProxyOopDescriptor::bootstrap_object(Bootstrap *stream) {
	MemOopDescriptor::bootstrap_header(stream);
	set_pointer((void *) stream->read_uint32_t());
	MemOopDescriptor::bootstrap_body(stream, header_size());
}
