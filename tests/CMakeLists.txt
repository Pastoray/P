# tests/CMakeLists.txt

find_package(Python3 REQUIRED)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(PARSER_SOURCES
        ${SRC_DIR}/tokenizer.cpp
        ${SRC_DIR}/parser.cpp
)

add_executable(parser_tests
        test_main.cpp
        ${PARSER_SOURCES}
)

target_include_directories(parser_tests PRIVATE ${SRC_DIR})

set_target_properties(parser_tests PROPERTIES OPTIMIZE OFF)

add_test(NAME parser_tests COMMAND parser_tests)


find_program(AS_EXECUTABLE as REQUIRED)
find_program(LD_EXECUTABLE ld REQUIRED)

set(TEST_RUNNER ${CMAKE_CURRENT_SOURCE_DIR}/run_test.py)

function(add_e2e_test name source_file expected_output)
    add_test(NAME e2e_${name} 
              COMMAND ${Python3_EXECUTABLE} ${TEST_RUNNER} 
             --compiler $<TARGET_FILE:P>
             --stdlib $<TARGET_FILE:p>
             --as ${AS_EXECUTABLE}
             --ld ${LD_EXECUTABLE}
             --src ${CMAKE_CURRENT_SOURCE_DIR}/e2e/${source_file}
             --expected "${expected_output}")
endfunction()

add_e2e_test(simple "simple.p" "2")
add_e2e_test(nested "nested.p" "1")
add_e2e_test(call "call.p" "13\n10\n136")
add_e2e_test(shadow "shadow.p" "10\n5")
add_e2e_test(cf "cf.p" "12\n3628800")
add_e2e_test(expr "expr.p" "-289\n17\n3\n5\n4\n5")
add_e2e_test(ptr_swap "ptr_swap.p" "20\n10")
add_e2e_test(arr "arr.p" "16\n2\n3\n4\n5\n6\n7\n1\n67\n0\n1\n2\n1\n2\n3\n2\n3\n4")
add_e2e_test(impcast "impcast.p" "10\n4000\n105")

