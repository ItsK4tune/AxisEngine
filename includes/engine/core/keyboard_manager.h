#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <unordered_map>

class KeyboardManager
{
public:
    void Init(GLFWwindow *window);

    bool GetKey(int key) const;
    bool GetKeyUp(int key) const;
    bool IsKeyDown(int key);
    
private:
    GLFWwindow *m_Window = nullptr;
    std::unordered_map<int, bool> m_PreviousState;
};