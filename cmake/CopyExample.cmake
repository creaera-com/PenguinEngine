# Initialize once. Existing projects are user data and must never be overwritten by a build.
if(NOT EXISTS "${DEST}/project.json")
    file(MAKE_DIRECTORY "${DEST}")
    file(COPY "${SOURCE}/assets" "${SOURCE}/scenes" DESTINATION "${DEST}")
    # Manifest last: an interrupted copy can be retried on the next build.
    file(COPY "${SOURCE}/project.json" DESTINATION "${DEST}")
endif()
