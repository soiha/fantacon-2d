#include "engine/ResourceManager.h"
#include "engine/Mesh3D.h"
#include <iostream>

using namespace Engine;

int main() {
    ResourceManager resources("examples/resources/");

    std::cout << "Testing OBJ file loading with normals...\n\n";

    // Test 1: Load cube without normals
    std::cout << "Test 1: Loading cube.obj (no normals)\n";
    auto cube = resources.loadMesh3D("cube", "cube.obj");
    if (cube) {
        std::cout << "  Vertices: " << cube->getVertices().size() << "\n";
        std::cout << "  Normals: " << cube->getNormals().size() << "\n";
        std::cout << "  Polygons: " << cube->getPolygons().size() << "\n";
        std::cout << "  Has normals: " << (cube->hasNormals() ? "yes" : "no") << "\n";
    } else {
        std::cout << "  Failed to load!\n";
    }

    std::cout << "\nTest 2: Loading cube_with_normals.obj\n";
    auto cubeWithNormals = resources.loadMesh3D("cube_normals", "cube_with_normals.obj");
    if (cubeWithNormals) {
        std::cout << "  Vertices: " << cubeWithNormals->getVertices().size() << "\n";
        std::cout << "  Normals: " << cubeWithNormals->getNormals().size() << "\n";
        std::cout << "  Polygons: " << cubeWithNormals->getPolygons().size() << "\n";
        std::cout << "  Has normals: " << (cubeWithNormals->hasNormals() ? "yes" : "no") << "\n";

        // Show normal vectors
        std::cout << "\n  Normal vectors:\n";
        const auto& normals = cubeWithNormals->getNormals();
        for (size_t i = 0; i < normals.size(); ++i) {
            std::cout << "    " << i << ": ("
                      << normals[i].x << ", "
                      << normals[i].y << ", "
                      << normals[i].z << ")\n";
        }

        // Show polygon normal indices
        std::cout << "\n  Polygon normal indices:\n";
        const auto& polygons = cubeWithNormals->getPolygons();
        for (size_t i = 0; i < polygons.size(); ++i) {
            std::cout << "    Polygon " << i << ": [";
            for (size_t j = 0; j < polygons[i].normals.size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << polygons[i].normals[j];
            }
            std::cout << "]\n";
        }
    } else {
        std::cout << "  Failed to load!\n";
    }

    std::cout << "\nNormal support test complete!\n";
    return 0;
}
