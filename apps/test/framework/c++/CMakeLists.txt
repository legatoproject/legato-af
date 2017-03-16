if(NOT USE_CLANG EQUAL 1)
    mkexe(cxxTest
            cppComponent/Hello.cpp
            --cxxflags=-std=c++0x
            --cxxflags=-DMUST_BE_DEFINED)

    add_test(cxxTest ${EXECUTABLE_OUTPUT_PATH}/cxxTest)

    mkapp(cxxHello.adef --cxxflags=-DMUST_BE_DEFINED )

    # This is a C test
    add_dependencies(tests_c cxxTest cxxHello)
endif()
