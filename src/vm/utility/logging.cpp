
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/utility/logging.hpp"

#include <chrono>
#include <ctime>
#include <iostream>
#include <experimental/source_location>

std::string format_time_point(std::chrono::system_clock::time_point &point) {

	static_assert(std::chrono::system_clock::time_point::period::den == 1E9);

	std::string result(29, '0');
	char *buf = &result[0];
	std::time_t now_c = std::chrono::system_clock::to_time_t(point);
	std::strftime(buf, 21, "%Y-%m-%dT%H:%M:%S.", std::localtime(&now_c));
	sprintf(buf + 20, "%09Ld", point.time_since_epoch().count() % static_cast<std::int32_t>(1E9));

	return result;
}

void log_line(const std::string &line) {

	std::chrono::time_point now = std::chrono::system_clock::now();

	std::cout << format_time_point(now) << "  " << line << std::endl;

}

inline void log(std::string_view message, const std::experimental::source_location &location = std::experimental::source_location::current()) {
	std::cout << "info:"
	          << location.file_name() << ':'
	          << location.line() << ' '
	          << message << '\n';
}
