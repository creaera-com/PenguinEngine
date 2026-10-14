# Initialize once. Existing projects are user data and must never be overwritten by a build.
if(NOT EXISTS "${DEST}/project.json")
    file(MAKE_DIRECTORY "${DEST}")
    file(COPY "${SOURCE}/assets" "${SOURCE}/scenes" "${SOURCE}/scripts" DESTINATION "${DEST}")
    # Manifest last: an interrupted copy can be retried on the next build.
    file(COPY "${SOURCE}/project.json" DESTINATION "${DEST}")
endif()

# Add the new script example to older build copies without changing existing scripts.
if(NOT EXISTS "${DEST}/scripts")
    file(COPY "${SOURCE}/scripts" DESTINATION "${DEST}")
endif()
