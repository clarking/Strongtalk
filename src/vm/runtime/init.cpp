
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/init.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/utility/Console.hpp"

void init_globals() {

	ResourceMark resourceMark;

	// general
	logging_init();
	console_init();
	os_init();
	except_init();
	primitives_init();
	eventlog_init();
	bytecodes_init();
	universe_init();

	//
	generatedPrimitives_init_before_interpreter();
	interpreter_init();
	dispatchTable_init();
	costModel_init();
	sweeper_init();
	fprofiler_init();
	systemAverage_init();
	preemption_init();
	generatedPrimitives_init_after_interpreter();

	// compiler
	compiler_init();
	mapping_init();
	opcode_init();

	if (not UseTimers) {
		SweeperUseTimer = false;
	}
}

void exit_globals() {

	static bool destructorsCalled = false;
	if (destructorsCalled)
		return;

	destructorsCalled = true;

	lprintf_exit();
	os_exit();

}
