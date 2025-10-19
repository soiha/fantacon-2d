cmake_minimum_required(VERSION 3.10)
project(Engine2D VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use pkg-config to find SDL2 and SDL2_image
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Find GLM
find_package(glm REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Engine library source files
set(ENGINE_SOURCES
    src/Texture.cpp
    src/Sprite.cpp
    src/Tilemap.cpp
    src/Layer.cpp
    src/PixelBuffer.cpp
    src/IndexedPixelBuffer.cpp
    src/Palette.cpp
    src/PixelFont.cpp
    src/CharacterLayer.cpp
    src/SDLRenderer.cpp
    src/GLRenderer.cpp
    src/Engine.cpp
    src/FPSCounter.cpp
    src/Font.cpp
    src/Text.cpp
    src/GameObject.cpp
    src/Input.cpp
    src/ResourceManager.cpp
)

# Create engine library
add_library(engine STATIC ${ENGINE_SOURCES})

target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
)

target_link_libraries(engine PUBLIC
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    glm::glm
    nlohmann_json::nlohmann_json
    OpenGL::GL
)

target_link_directories(engine PUBLIC
    ${SDL2_LIBRARY_DIRS}
    ${SDL2_IMAGE_LIBRARY_DIRS}
    ${SDL2_TTF_LIBRARY_DIRS}
)

# Example executables
add_executable(example examples/main.cpp)
target_link_libraries(example PRIVATE engine)

add_executable(gameobject_demo examples/gameobject_demo.cpp)
target_link_libraries(gameobject_demo PRIVATE engine)

add_executable(ldtk_demo examples/ldtk_demo.cpp)
target_link_libraries(ldtk_demo PRIVATE engine)

add_executable(indexed_palette_demo examples/indexed_palette_demo.cpp)
target_link_libraries(indexed_palette_demo PRIVATE engine)

add_executable(fire_demo examples/fire_demo.cpp)
target_link_libraries(fire_demo PRIVATE engine)

add_executable(font_demo examples/font_demo.cpp)
target_link_libraries(font_demo PRIVATE engine)

add_executable(sine_scroller_demo examples/sine_scroller_demo.cpp)
target_link_libraries(sine_scroller_demo PRIVATE engine)

add_executable(color_pulse_scroller_demo examples/color_pulse_scroller_demo.cpp)
target_link_libraries(color_pulse_scroller_demo PRIVATE engine)

add_executable(vector_lines_demo examples/vector_lines_demo.cpp)
target_link_libraries(vector_lines_demo PRIVATE engine)

add_executable(character_layer_demo examples/character_layer_demo.cpp)
target_link_libraries(character_layer_demo PRIVATE engine)

add_executable(font_converter examples/font_converter.cpp)
target_include_directories(font_converter PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_directories(font_converter PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
target_link_libraries(font_converter PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

add_executable(font_viewer examples/font_viewer.cpp)
target_include_directories(font_viewer PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_directories(font_viewer PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
target_link_libraries(font_viewer PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

add_executable(generate_test_font examples/generate_test_font.cpp)
target_include_directories(generate_test_font PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_directories(generate_test_font PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
target_link_libraries(generate_test_font PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

add_executable(font_debug examples/font_debug.cpp)
target_link_libraries(font_debug PRIVATE engine)

add_executable(convert_c64_font examples/convert_c64_font.cpp)
target_include_directories(convert_c64_font PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
target_link_directories(convert_c64_font PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
target_link_libraries(convert_c64_font PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
