#ifndef DEBUG_H_
#define DEBUG_H_

#include <cstdlib>
#include <iostream>

namespace debug {

constexpr const char* CSI_expr = "\033[";
constexpr const char* end_expr = "m";

inline namespace {

std::ostream& color_gen(std::ostream& stream, const char* code) {
    stream << CSI_expr << code << end_expr;
    return stream;
}

}  // namespace

std::ostream& reset(std::ostream& stream) { return color_gen(stream, "0"); }

inline namespace color {

std::ostream& black(std::ostream& stream) { return color_gen(stream, "30"); }
std::ostream& red(std::ostream& stream) { return color_gen(stream, "31"); }
std::ostream& green(std::ostream& stream) { return color_gen(stream, "32"); }
std::ostream& yellow(std::ostream& stream) { return color_gen(stream, "33"); }
std::ostream& blue(std::ostream& stream) { return color_gen(stream, "34"); }
std::ostream& magenta(std::ostream& stream) { return color_gen(stream, "35"); }
std::ostream& cyan(std::ostream& stream) { return color_gen(stream, "36"); }
std::ostream& white(std::ostream& stream) { return color_gen(stream, "37"); }

namespace background {

std::ostream& black(std::ostream& stream) { return color_gen(stream, "40"); }
std::ostream& red(std::ostream& stream) { return color_gen(stream, "41"); }
std::ostream& green(std::ostream& stream) { return color_gen(stream, "42"); }
std::ostream& yellow(std::ostream& stream) { return color_gen(stream, "43"); }
std::ostream& blue(std::ostream& stream) { return color_gen(stream, "44"); }
std::ostream& magenta(std::ostream& stream) { return color_gen(stream, "45"); }
std::ostream& cyan(std::ostream& stream) { return color_gen(stream, "46"); }
std::ostream& white(std::ostream& stream) { return color_gen(stream, "47"); }

}  // namespace background
namespace bg = background;

}  // namespace color

}  // namespace debug

#endif
