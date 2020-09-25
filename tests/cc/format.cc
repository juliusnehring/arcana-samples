#include <nexus/test.hh>

#include <clean-core/format.hh>

TEST("cc::format")
{
    using namespace cc::format_literals;
    CHECK("Hello World!" == cc::format("Hello World!"));
    CHECK("Hello World!" == cc::format("Hello {}!", "World"));
    CHECK("Hello World!" == cc::format("{1} {0}!", "World", "Hello"));
    CHECK("Hello World!" == cc::format("{h} {w}!", cc::format_arg("w", "World"), cc::format_arg("h", "Hello")));
    CHECK("Hello World!" == cc::format("{h} {w}!", "w"_a = "World", "h"_a = "Hello"));
    CHECK("Hello World" == cc::format("Hello {}", "World"));
    CHECK("Hello World!" == cc::format("{1} {0}!", "World", "Hello"));
    CHECK("Hello World!" == cc::format("{h} {w}!", cc::format_arg("w", "World"), cc::format_arg("h", "Hello")));
    CHECK("The answer is 42." == cc::format("The answer is {}.", 42));
    CHECK("The answer is 42." == cc::format("The answer is {fourty_two}.", "fourty_two"_a = 42));
    CHECK("Gravity is approx. 9.8." == cc::format("Gravity is approx. {}.", 9.8));
    CHECK("Gravity is approx. 9.8." == cc::format("Gravity is approx. {}.", 9.8f));
    CHECK("Escape {this}, not this." == cc::format("Escape {{this}}, {}", "not this."));
    CHECK("I did not hit her, it's not true!" == cc::format("I did not hit her, it's not {}!", true));
    CHECK("42" == cc::format("{}", 42));
    CHECK("42" == cc::format("{:d}", 42));
    CHECK("101010" == cc::format("{:b}", 42));
    CHECK("0b101010" == cc::format("{:#b}", 42));
    CHECK("101010" == cc::format("{:B}", 42));
    CHECK("0B101010" == cc::format("{:#B}", 42));
    CHECK("52" == cc::format("{:o}", 42));
    CHECK("052" == cc::format("{:#o}", 42));
    CHECK("2a" == cc::format("{:x}", 42));
    CHECK("0x2a" == cc::format("{:#x}", 42));
    CHECK("2A" == cc::format("{:X}", 42));
    CHECK("0X2A" == cc::format("{:#X}", 42));
    CHECK("       foo" == cc::format("{:10}", "foo"));
    CHECK("       foo" == cc::format("{:>10}", "foo"));
    CHECK("foo       " == cc::format("{:<10}", "foo"));
    CHECK("    foo   " == cc::format("{:^10}", "foo"));
    CHECK("   fooo   " == cc::format("{:^10}", "fooo"));
    CHECK("3.142" == cc::format("{:2.3f}", 3.14159265359));
    CHECK("3.142e+00" == cc::format("{:.3e}", 3.14159265359));
    CHECK("0x1.922p+1" == cc::format("{:2.3a}", 3.14159265359));
}
