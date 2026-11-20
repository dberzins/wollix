# A consumer of the installed package, as a user's project would be: it finds
# wollix with find_package and links wollix::wollix alone, nothing else. Built
# and run by the cmake_consumer test (run.cmake), never by the main project.
cmake_minimum_required(VERSION 3.16)
project(wollix_consumer LANGUAGES C)

# Only the prefix the test just installed, so a wollix installed elsewhere on
# the machine cannot stand in for it.
find_package(wollix ${WOLLIX_EXPECT_VERSION} CONFIG REQUIRED
             PATHS ${WOLLIX_PREFIX} NO_DEFAULT_PATH)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE wollix::wollix)
# The option macros override designated defaults on purpose; the target's own
# compile options must keep that quiet, so warnings fail this build.
target_compile_options(consumer PRIVATE
  $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra;-Werror>)

enable_testing()
add_test(NAME consumer COMMAND consumer)
set_tests_properties(consumer PROPERTIES
  PASS_REGULAR_EXPRESSION "wollix ${WOLLIX_EXPECT_VERSION} from the installed package")
