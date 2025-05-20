# vim: syntax=cmake
INCLUDE_DIRECTORIES(${KMPP_TOP}/include)

SET(STUB_TEST_SRC
    stub_test.c
)

ADD_KBUILD(stub_test SRC ${STUB_TEST_SRC})
