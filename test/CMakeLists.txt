add_subdirectory(source)

if(ENABLE_FUZZ)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_subdirectory(fuzz)
    else()
        message(
            STATUS
            "Fuzz subdirectory not included (requires Clang compiler)"
        )
    endif()
endif()

if(ENABLE_BENCHMARK)
    add_subdirectory(benchmark)
endif()
