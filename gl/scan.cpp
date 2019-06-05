#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <thread>

const char *vertexSource =
#include "vertex.glsl"

const char *fragmentSource =
#include "fragment.glsl"


class App {
public:
    void run ();
    ~App ();

private:
    void makeContext ();
    void makeProgram ();
    unsigned makeShader (const char** data, unsigned type);
    void makeVAO ();
    void pollNewDepths ();
    GLFWwindow *window;
    glm::mat4 projection;
    glm::mat4 view;
    unsigned vao, vbo, program;
    int vertCount;

    // Current list of read distances
    std::vector<float> distances;

    bool threadShouldDie = false;
    std::thread inputThread;
};

void App::run ()
{
    makeContext ();
    makeProgram ();
    makeVAO ();

    while (!glfwWindowShouldClose (window)) {
        glfwPollEvents ();
        pollNewDepths ();
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, vertCount);
        glfwSwapBuffers (window);
    }
}

void App::makeContext ()
{
    glfwInit ();
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow (512, 512, "Scan", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit ();
    glEnable (GL_DEPTH_TEST);
}

void App::makeProgram ()
{
    program = glCreateProgram ();
    auto vertex = makeShader (&vertexSource, GL_VERTEX_SHADER);
    auto fragment = makeShader (&fragmentSource, GL_FRAGMENT_SHADER);
    glAttachShader (program, vertex);
    glAttachShader (program, fragment);
    glLinkProgram (program);
    glUseProgram (program);
    glDeleteShader (vertex);
    glDeleteShader (fragment);

    int err;
    glGetProgramiv (program, GL_LINK_STATUS, &err);

    if (!err) {
        char log[512];
        glGetProgramInfoLog (program, 512, nullptr, log);
        throw std::runtime_error(log);
    }
}

unsigned App::makeShader (const char **source, unsigned type)
{
    unsigned shader = glCreateShader (type);
    glShaderSource (shader, 1, source, nullptr);
    glCompileShader (shader);

    int err;
    glGetShaderiv (shader, GL_COMPILE_STATUS, &err);

    if (!err) {
        char log[512];
        glGetShaderInfoLog (shader, 512, nullptr, log);
        glDeleteShader (shader);
        throw std::runtime_error(log);
    }

    return shader;
}

void App::makeVAO ()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*) 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    vertCount = 0;
}

App::~App ()
{
    glDeleteProgram (program);
    glDeleteBuffers (1, &vbo);
    glDeleteVertexArrays (1, &vao);
    glfwDestroyWindow (window);
    glfwTerminate ();
}

void App::pollNewDepths ()
{
    // Check for new vertices on stdin
    // // TODO: I have absolutely no damn idea how to make it nonblocking

    float value;
    std::cin >> value;
    std::cout << "[New Value] " << value << std::endl;
    distances.push_back(value);
    // TODO: Rebuild entire 3D model
}

int
main (int argc, char **argv)
{
    App app;
    app.run ();
}
