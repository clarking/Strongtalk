//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "allocation.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/utility/OutputStream.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"

class PrintObjectClosure : public ObjectLayoutClosure {
private:
	MemOop _memOop;

	ConsoleOutputStream *_stream;
public:
	PrintObjectClosure(ConsoleOutputStream *stream = nullptr);
	PrintObjectClosure() = default;
	virtual ~PrintObjectClosure() = default;
	PrintObjectClosure(const PrintObjectClosure &) = default;
	PrintObjectClosure &operator=(const PrintObjectClosure &) = default;

	void operator delete(void *ptr) {
		(void) (ptr);
	}

	void do_object(MemOop obj);

	void do_mark(MarkOop *m);

	void do_oop(const char *title, Oop *o);

	void do_byte(const char *title, std::uint8_t *b);

	void do_long(const char *title, void **p);

	void do_double(const char *title, double *d);

	void begin_indexables();

	void end_indexables();

	void do_indexable_oop(std::int32_t index, Oop *o);

	void do_indexable_byte(std::int32_t index, std::uint8_t *b);

	void do_indexable_doubleByte(std::int32_t index, std::uint16_t *b);

	void do_indexable_long(std::int32_t index, std::int32_t *l);
};
