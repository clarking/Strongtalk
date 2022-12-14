
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/utility/EventLog.hpp"
#include "vm/runtime/flags.hpp"

EventLog *eventLog;

void eventlog_init() {
	SPDLOG_INFO("system-init:  eventlog_init");
	eventLog = new EventLog;
}

static const char *noEvent = "no event";

void EventLog::init() {
	_eventBuffer = _next = new_c_heap_array<EventLogEvent>(EventLogLength);
	_end = _eventBuffer + EventLogLength;

	for (EventLogEvent *e = _eventBuffer; e < _end; e++) {
		e->_name = noEvent;
	}

}

EventLog::EventLog() :
		_eventBuffer{nullptr},
		_end{nullptr},
		_next{nullptr},
		_nestingDepth{0} {

	init();
}

void EventLog::resize() {
	EventLogEvent *oldBuf = _eventBuffer;
	EventLogEvent *oldEnd = _end;
	EventLogEvent *oldNext = _next;
	init();

	// copy events
	for (EventLogEvent *e = nextEvent(oldNext, oldBuf, oldEnd); e not_eq oldNext; e = nextEvent(e, oldBuf, oldEnd), _next = nextEvent(_next, _eventBuffer, _end)) {
		*_next = *e;
	}

	FreeHeap(oldBuf);
}

void EventLog::printPartial(std::int32_t n) {
	EventLogEvent *e = _next;

	// find starting point
	if (n >= EventLogLength) {
		n = EventLogLength - 1;
	}

	//
	std::int32_t i = 0;

	//
	for (i = 0; i < n; i++, e = prevEvent(e, _eventBuffer, _end)) {
		st_unused(nullptr);
	}

	// skip empty entries
	for (i = 0; e not_eq _next and e->_name == noEvent; i++, e = nextEvent(e, _eventBuffer, _end)) {
		st_unused(nullptr);
	}

	std::int32_t indent = 0;
	for (; i < n and e not_eq _next; i++, e = nextEvent(e, _eventBuffer, _end)) {
		const char *s;
		switch (e->_status) {
			case EventLogEventStatus::starting:
				s = "[ ";
				break;
			case EventLogEventStatus::ending:
				s = "] ";
				indent--;
				break;
			case EventLogEventStatus::atomic:
				s = "- ";
				break;
		}
//        SPDLOG_INFO( "%*.s{}", 2 * indent, " ", s );
		SPDLOG_INFO("{}  {}  {}  {}", e->_name, e->args[0], e->args[1], e->args[2]);
		SPDLOG_INFO("");
		if (e->_status == EventLogEventStatus::starting) {
			indent++;
		}
	}

	if (indent not_eq _nestingDepth) {
		SPDLOG_INFO("Actual event nesting is {} greater than shown.", _nestingDepth - indent);
	}
}

EventLogEvent *EventLog::nextEvent(EventLogEvent *e, EventLogEvent *start, EventLogEvent *end) {
	return e + 1 == end ? start : e + 1;
}

EventLogEvent *EventLog::prevEvent(EventLogEvent *e, EventLogEvent *start, EventLogEvent *end) {
	return (e == start ? end : e) - 1;
}
