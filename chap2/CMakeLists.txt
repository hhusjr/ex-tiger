project(tiger_chap3 C)
cmake_minimum_required(VERSION 3.15)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ./)

find_package(FLEX REQUIRED)

set(CMAKE_C_STANDARD 99)

FLEX_TARGET(scanner tiger.lex ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

add_executable(tiger driver.c errormsg.c util.c ${FLEX_scanner_OUTPUTS})
