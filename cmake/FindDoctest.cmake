find_path(DOCTEST_INCLUDE_DIR doctest/doctest.h)
add_library(Doctest::Doctest INTERFACE IMPORTED)
set_target_properties(Doctest::Doctest PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${DOCTEST_INCLUDE_DIR})