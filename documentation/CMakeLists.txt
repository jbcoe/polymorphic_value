find_program(CURL curl)
if (CURL)
    add_custom_target(iso_paper
        COMMAND
            pwd
        COMMAND
            ${CURL} -k https://api.csswg.org/bikeshed/ -F file=@${CMAKE_CURRENT_SOURCE_DIR}/p0201.md -F force=1 > p0201.html
        BYPRODUCTS
            draft.html
        COMMENT
            "Generating ISO Paper P1950 revision 2"
    )
endif ()
