# Embed a checked-in binary asset without adding Python to the normal build.
if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
 message(FATAL_ERROR "INPUT and OUTPUT are required")
endif()
file(READ "${INPUT}" bytes HEX)
string(REGEX REPLACE "(..)" "0x\\1," bytes "${bytes}")
string(REPEAT "0x[0-9a-f][0-9a-f]," 16 row)
string(REGEX REPLACE "(${row})" "\\1\n" bytes "${bytes}")
file(WRITE "${OUTPUT}" "/* Generated binary embedding; do not edit. */\nconst unsigned char pui_tabler_png_data[] = {\n${bytes}\n};\n")
