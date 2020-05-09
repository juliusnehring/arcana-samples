#include <nexus/test.hh>

#include <clean-core/format.hh>

TEST("cc::format")
{
    CHECK("Hello World!" == cc::format("Hello {}!", "World"));
    CHECK("Hello World!" == cc::format("{1} {0}!", "World", "Hello"));
    CHECK("Hello World!" == cc::format("{h} {w}!", cc::arg("w", "World"), cc::arg("h", "Hello")));
    CHECK("Hello World" == cc::format("Hello {}", "World"));
    CHECK("Hello World!" == cc::format("{1} {0}!", "World", "Hello"));
    CHECK("Hello World!" == cc::format("{h} {w}!", cc::arg("w", "World"), cc::arg("h", "Hello")));
    CHECK("The answer is 42." == cc::format("The answer is {}.", 42));
    CHECK("Gravity is approx. 9.8." == cc::format("Gravity is approx. {}.", 9.8));
    CHECK("Gravity is approx. 9.8." == cc::format("Gravity is approx. {}.", 9.8f));
    CHECK("Escape {this}, not this." == cc::format("Escape {{this}}, {}", "not this."));
    CHECK("I did not hit her, it's not true!" == cc::format("I did not hit her, it's not {}!", true));
}
