project(injector)

add_executable(${PROJECT_NAME} main.cpp)
target_compile_options(${PROJECT_NAME} PRIVATE /GS-)
target_link_options(${PROJECT_NAME} PRIVATE /NODEFAULTLIB)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/bin)
