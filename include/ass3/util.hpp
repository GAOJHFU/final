#pragma once
//#define TINYOBJLOADER_IMPLEMENTATION
#include <iostream>
#include <chicken3421/chicken3421.hpp>

class Utils {
public:
    static unsigned int LoadTexture(char const* path, bool gammaCorrection = true);
    static unsigned int LoadCubemap(std::vector<std::string> faces);
public:
    Utils() {};
    ~Utils() {};
};