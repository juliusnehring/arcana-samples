cmake_minimum_required(VERSION 3.11)

file(GLOB_RECURSE SOURCES
    "*.cc"
    "*.hh"
)

add_arcana_test(inc-tests "${SOURCES}")

target_link_libraries(inc-tests PUBLIC
    clean-core
    task-dispatcher
    typed-geometry
    phantasm-hardware-interface
    phantasm-renderer
    arcana-incubator
)

set_property(TARGET inc-tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

