
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/utility/EventLog.hpp"

#include <cstdarg>
#include <cstdio>

void report_vm_state() {
	static bool recursive_error = false;
	if (not recursive_error) {
		recursive_error = true;
		if (theCompiler) {

			SPDLOG_INFO("-----------------------------------------------------------------------------");
			theCompiler->print_key(_console);
			_console->cr();
			SPDLOG_INFO("-----------------------------------------------------------------------------");
			if (CompilerDebug) {
				SPDLOG_INFO("-----------------------------------------------------------------------------");
				print_cout();
				SPDLOG_INFO("-----------------------------------------------------------------------------");
			}
			else {
				_console->print("(No compiler debug output available -- run with +CompilerDebug to get it)");
			}

		}
		SPDLOG_INFO("Last 10 internal VM events:");
		eventLog->printPartial(10);
		recursive_error = false;
	}
}

void report_error(const char *title, const char *format, ...) {
	//os::fatalExit( EXIT_FAILURE );

	char buffer[2048];
	va_list ap;
	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	SPDLOG_INFO("[A Runtime Error Occurred]");
	_console->print_raw(buffer);

	if (not bootstrappingInProgress)
		report_vm_state();

	if (ShowMessageBoxOnError) {
		strcat(buffer, "\n\nDo you want to debug the problem?");
		if (not os::message_box(title, buffer))
			os::fatalExit(EXIT_FAILURE);
	}
}

void report_assertion_failure(const char *code_str, const char *file_name, std::int32_t line_no, const char *message) {
	report_error("Assertion Failure", "assert(%s, \"%s\")\n%s, %d", code_str, message, file_name, line_no);
}

void report_fatal(const char *file_name, std::int32_t line_no, const char *format, ...) {
	char buffer[2048];
	va_list ap;
	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);
	report_error("Fatal Error", "Fatal: %s\n%s, %d", buffer, file_name, line_no);
}

void report_should_not_call(const char *file_name, std::int32_t line_no) {
	report_error("Should Not Call This Error", "ShouldNotCall()\n%s, %d", file_name, line_no);
}

void report_should_not_reach_here(const char *file_name, std::int32_t line_no) {
	report_error("Should Not Reach Here Error", "ShouldNotReachHere()\n%s, %d", file_name, line_no);
}

void report_subclass_responsibility(const char *file_name, std::int32_t line_no) {
	report_error("Subclass Responsibility Error", "SubclassResponsibility()\n%s, %d", file_name, line_no);
}

void report_unimplemented(const char *file_name, std::int32_t line_no) {
	report_error("Unimplemented Error", "Unimplemented()\n%s, %d", file_name, line_no);
}
