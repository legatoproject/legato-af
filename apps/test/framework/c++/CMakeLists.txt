if(NOT USE_CLANG EQUAL 1)
    mkexe(cxxTest
            cppComponent/Hello.cpp
            --cxxflags=-std=c++0x
            --cxxflags=-DMUST_BE_DEFINED
          # TODO: Remove the following when the mk tools do dependency checking.
          DEPENDS * cppComponent/* )

    add_test(cxxTest ${EXECUTABLE_OUTPUT_PATH}/cxxTest)

    mkapp(cxxHello.adef --cxxflags=-DMUST_BE_DEFINED )
endif()
