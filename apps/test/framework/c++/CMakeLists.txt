if(NOT USE_CLANG EQUAL 1)
    mkexe(cxxTest
            Hello.cpp
            --cxxflags=-std=c++0x
          # TODO: Remove the following when the mk tools do dependency checking.
          DEPENDS Hello.cpp)

    add_test(cxxTest ${EXECUTABLE_OUTPUT_PATH}/cxxTest)

    mkapp(cxxHello.adef --cxxflags=-std=c++0x)
endif()
