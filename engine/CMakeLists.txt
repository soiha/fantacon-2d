cmake_minimum_required(VERSION 3.10)
project(Engine2D VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific dependency discovery
# On Windows with vcpkg, use find_package
# On Mac/Linux, use pkg-config
if(WIN32 AND DEFINED ENV{VCPKG_ROOT})
    message(STATUS "Using vcpkg for dependency management")

    # Find SDL2 via vcpkg
    find_package(SDL2 CONFIG REQUIRED)
    find_package(SDL2_image CONFIG REQUIRED)
    find_package(SDL2_ttf CONFIG REQUIRED)

    # Set variables to match pkg-config naming for compatibility
    set(SDL2_LIBRARIES SDL2::SDL2 SDL2::SDL2main)
    set(SDL2_IMAGE_LIBRARIES SDL2_image::SDL2_image)
    set(SDL2_TTF_LIBRARIES SDL2_ttf::SDL2_ttf)

else()
    message(STATUS "Using pkg-config for dependency management")

    # Use pkg-config to find SDL2 and SDL2_image (Mac/Linux)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
endif()

# Find GLM
find_package(glm REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLEW (for OpenGL extension loading)
find_package(GLEW REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

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
    src/AttributedTextGrid.cpp
    src/Mesh3D.cpp
    src/SDLRenderer.cpp
    src/GLRenderer.cpp
    src/VulkanRenderer.cpp
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
)

# Add SDL2 include directories only for pkg-config (vcpkg handles it automatically)
if(NOT WIN32 OR NOT DEFINED ENV{VCPKG_ROOT})
    target_include_directories(engine PUBLIC
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )
endif()

target_link_libraries(engine PUBLIC
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    glm::glm
    nlohmann_json::nlohmann_json
    OpenGL::GL
    GLEW::GLEW
    Vulkan::Vulkan
)

# Add library directories only for pkg-config (vcpkg handles it automatically)
if(NOT WIN32 OR NOT DEFINED ENV{VCPKG_ROOT})
    target_link_directories(engine PUBLIC
        ${SDL2_LIBRARY_DIRS}
        ${SDL2_IMAGE_LIBRARY_DIRS}
        ${SDL2_TTF_LIBRARY_DIRS}
    )
endif()

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
target_link_libraries(font_converter PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
if(NOT WIN32 OR NOT DEFINED ENV{VCPKG_ROOT})
    target_include_directories(font_converter PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_directories(font_converter PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
endif()

add_executable(font_viewer examples/font_viewer.cpp)
target_link_libraries(font_viewer PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
if(NOT WIN32 OR NOT DEFINED ENV{VCPKG_ROOT})
    target_include_directories(font_viewer PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_directories(font_viewer PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
endif()

add_executable(generate_test_font examples/generate_test_font.cpp)
target_link_libraries(generate_test_font PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
if(NOT WIN32 OR NOT DEFINED ENV{VCPKG_ROOT})
    target_include_directories(generate_test_font PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_directories(generate_test_font PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
endif()

add_executable(font_debug examples/font_debug.cpp)
target_link_libraries(font_debug PRIVATE engine)

add_executable(convert_c64_font examples/convert_c64_font.cpp)
target_link_libraries(convert_c64_font PRIVATE ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})
if(NOT WIN32 OR NOT DEFINED ENV{VCPKG_ROOT})
    target_include_directories(convert_c64_font PRIVATE ${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})
    target_link_directories(convert_c64_font PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS})
endif()

add_executable(mesh_demo examples/mesh_demo.cpp)
target_link_libraries(mesh_demo PRIVATE engine)

add_executable(test_normals examples/test_normals.cpp)
target_link_libraries(test_normals PRIVATE engine)

add_executable(mesh_with_normals_demo examples/mesh_with_normals_demo.cpp)
target_link_libraries(mesh_with_normals_demo PRIVATE engine)

add_executable(text_grid_demo examples/text_grid_demo.cpp)
target_link_libraries(text_grid_demo PRIVATE engine)

add_executable(layer_transparency_demo examples/layer_transparency_demo.cpp)
target_link_libraries(layer_transparency_demo PRIVATE engine)
