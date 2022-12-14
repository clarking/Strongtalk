//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/platform/platform.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include <array>



// Collects statistical information on the interpreter.

class InterpreterStatistics : AllStatic {

private:

	static bool _is_initialized;            // true if InterpreterStatistics has been initialized
	static std::array<std::uint32_t, static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)> _bytecode_counters;
	static std::array<std::int32_t, static_cast<std::int32_t>(ByteCodes::Code::NUMBER_OF_CODES)> _bytecode_generation_order;

	static void reset_bytecode_counters();

	static void reset_bytecode_generation_order();

public:

	static bool is_initialized();

	static ByteCodes::Code ith_bytecode_to_generate(std::int32_t i);

	static void initialize();
};
