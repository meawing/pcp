function(add_catch TARGET)
    string(APPEND TARGET "_task")
    add_executable(${TARGET} ${ARGN})

    target_link_libraries(${TARGET} lines Catch2::Catch2WithMain ${LIBS})
    if(LIBS)
        add_dependencies(${TARGET} ${LIBS})
    endif()
endfunction()

macro(link_test_library)
    list(APPEND LIBS ${ARGV})
endmacro()
