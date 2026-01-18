#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    unsigned int ID;

    void setVec4(const std::string& name, const glm::vec4& value);


    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
};
