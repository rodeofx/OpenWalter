file(GLOB SRC "*.cpp" "*h")

add_library(walterAbcExtras STATIC ${SRC})

target_include_directories(walterAbcExtras PUBLIC .)

target_include_directories(walterAbcExtras
    PRIVATE
    ${ALEMBIC_INCLUDE_PATH}
    ${Boost_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR})
