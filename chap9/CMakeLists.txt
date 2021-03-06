project(tiger_chap9 C)
cmake_minimum_required(VERSION 3.15)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ./)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

set(CMAKE_C_STANDARD 99)

set(ARCH mips)

FLEX_TARGET(scanner tiger.lex ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)
BISON_TARGET(parser tiger.grm ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c)
ADD_FLEX_BISON_DEPENDENCY(scanner parser)

add_executable(tiger
        main.c
        semant.c
        types.c
        util.c
        errormsg.c
        absyn.c
        prabsyn.c
        symbol.c
        table.c
        env.c
        temp.c
        codegen.c
        escape.c
        translate.c
        tree.c
        printtree.c
        assem.c
        canon.c
        ${BISON_parser_OUTPUTS} ${FLEX_scanner_OUTPUTS}

        arch/${ARCH}/${ARCH}frame.c
        arch/${ARCH}/${ARCH}codegen.c
        )
