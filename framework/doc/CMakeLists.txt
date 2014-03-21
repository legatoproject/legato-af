#*******************************************************************************
# Copyright (C) 2013, Sierra Wireless Inc., all rights reserved.
#
# Contributors:
#     Sierra Wireless - initial API and implementation
#*******************************************************************************

# Generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)
    # End-user framework documentation.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.cfg.in
                   ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
                   @ONLY
    )

    # Internal framework implementation documentation.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.implementation.cfg.in
                   ${CMAKE_CURRENT_BINARY_DIR}/doxygen.implementation.cfg
                   @ONLY
    )

    function(generate_doc DOC_TARGET DOC_TITLE DOXYGEN_OUTPUT DOXYGEN_CFG)

        add_custom_target(${DOC_TARGET}
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/legato.css ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND mkdir -p ${DOXYGEN_OUTPUT} && ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CFG}
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/navtree.css                            ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/tabs.css                               ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/fonts.css                              ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/opensans.ttf                           ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/opensans-light.ttf                     ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/opensans-bold.ttf                      ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/favicon.ico                            ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/swi-ico-medium.png                     ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2mnode.png                          ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2mlastnode.png                      ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2pnode.png                          ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2plastnode.png                      ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2doc.png                            ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/ftv2folderopen.png                     ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/resize.js                              ${DOXYGEN_OUTPUT}/html
            COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/static/search.css                             ${DOXYGEN_OUTPUT}/html/search
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT ${DOC_TITLE} VERBATIM
        )

    endfunction()

    # Doxygen Generation
    generate_doc(implementation_doc
        "Generating framework implementation documentation with Doxygen"
        ${DOCUMENTATION_OUTPUT_PATH}/implementation
        ${CMAKE_CURRENT_BINARY_DIR}/doxygen.implementation.cfg
    )

    generate_doc(doc
        "Generating API documentation with Doxygen"
        ${DOCUMENTATION_OUTPUT_PATH}/user
        ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
    )

    # PDF Generation
    add_custom_target(pdf
        COMMAND make > ${CMAKE_CURRENT_BINARY_DIR}/doxygen.pdf.log && cp refman.pdf ${DOCUMENTATION_OUTPUT_PATH}/user/legato.pdf
        WORKING_DIRECTORY ${DOCUMENTATION_OUTPUT_PATH}/user/latex
        DEPENDS doc
        COMMENT "Generating PDF for API documentation (log @ ${CMAKE_CURRENT_BINARY_DIR}/doxygen.pdf.log)"
    )


else()
    message(WARNING "Documentation: Doxygen not found, no documentation will be produced.")
endif()
