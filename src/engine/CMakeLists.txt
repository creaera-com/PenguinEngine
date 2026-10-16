add_library(penguin STATIC
 scene/scene.c
 ecs/entities.c
 ecs/names.c
 ecs/components.c
 ecs/hierarchy.c
 tilemap/tilemap.c
 history/history.c
 camera/camera.c
 collision/collision.c
 runtime/update.c
 scene/serialization.c
 project/project.c
 project/manifest.c
)
target_include_directories(penguin PUBLIC include PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/third_party/cjson)
if(NOT MSVC)
 target_compile_options(penguin PRIVATE -Wall -Wextra -Wpedantic)
 target_link_libraries(penguin PUBLIC m)
endif()
target_link_libraries(penguin PRIVATE penguin_json)
add_library(penguin_script STATIC scripting/text.c scripting/javascript.c)
target_include_directories(penguin_script PUBLIC include)
target_link_libraries(penguin_script PRIVATE penguin_js_parser)
if(NOT MSVC)
 target_compile_options(penguin_script PRIVATE -Wall -Wextra -Wpedantic)
endif()
