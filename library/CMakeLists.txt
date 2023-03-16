project(library)

add_library(${PROJECT_NAME} SHARED library.cpp)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/bin)
