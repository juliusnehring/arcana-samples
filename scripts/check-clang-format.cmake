# to be executed in arcana-samples root dir

FILE(GLOB_RECURSE files 
    "tests/*.cc"
    "tests/*.hh"
    "extern/*.cc"
    "extern/*.hh"
)

if (NOT files)
    message(FATAL_ERROR "no files found (wrong working directory? CMAKE_CURRENT_SOURCE_DIR is '${CMAKE_CURRENT_SOURCE_DIR}')")
endif()

set(FORMATTER "clang-format-9" CACHE STRING "binary used to format files")
option(ONLY_CHECK "if true, only checks if anything changed, otherwise executes clang format as well" ON)

execute_process(
    COMMAND ${FORMATTER} --version
    OUTPUT_VARIABLE clang_format_version
    RESULT_VARIABLE status
)

if (NOT status EQUAL 0)
    message(FATAL_ERROR 
        "fatal: formatter binary not found as ${FORMATTER} in PATH\n"
        "specify a different binary using -DFORMATTER=\"<yourname>\""
    )
else()
    message(STATUS "using binary ${FORMATTER}")
    message(STATUS "${clang_format_version}")
endif()

set(FINE TRUE)
set(COUNT 0)

foreach(filename ${files})

    # ignore sdl2
    if (filename MATCHES "extern/sdl2-dev/")
        continue()
    endif()
    
    file(READ ${filename} content)

    execute_process(
        COMMAND ${FORMATTER} -style=file ${filename}
        OUTPUT_VARIABLE formatted
        RESULT_VARIABLE status
    )

    MATH(EXPR COUNT "${COUNT}+1")

    if (status EQUAL 0)
        if (content STREQUAL formatted)
            # fine
        else()
            message("${filename} is not formatted")

            if (ONLY_CHECK)
                # read-only mode, output warning at the end and exit(1)
                set(FINE FALSE)
            else()
                message(" .. formatting ${filename}")

                execute_process(
                    COMMAND ${FORMATTER} -i -style=file ${filename}
                    RESULT_VARIABLE status
                )

                if (status EQUAL 0)
                    # fine
                else()
                    message(FATAL_ERROR "clang format -i error for ${filename}")
                endif()
            endif()
        endif()
    else()
        message(FATAL_ERROR "clang format error for ${filename}")
    endif()
endforeach()

if (FINE)
    message(STATUS "found ${COUNT} files that are formatted properly")
else()
    message(FATAL_ERROR "there are unformatted files (run 'cmake -DONLY_CHECK=OFF -P scripts/check-clang-format.cmake' in arcana-samples root to fix this)")
endif()
