#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <iostream>
#include <vector>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <thread>
#include <mutex>

const char *vertexSource =
#include "vertex.glsl"

const char *fragmentSource =
#include "fragment.glsl"


/**
 * The OpenGL Application.
 */
class App {
public:
    /// Initialize and run the application
    void run ();

    /// Clean up the application
    ~App ();

private:
    /// Create the window context
    void makeContext ();

    /// Create the shader program
    void makeProgram ();

    /// Compile and link a shader.
    unsigned makeShader (const char** data, unsigned type);

    /// Create the mesh objects.
    void makeVAO ();

    /// Spawn the input-reading thread.
    void makeInputThread ();

    /// Create projection matrix
    void makeProjection ();

    /// Update mesh when notified of new data.
    void updateMesh ();

    /// The window handle
    GLFWwindow *window;

    /// Projection and view matrices for rendering - model matrix is not needed
    /// as the model is generated with in-place coordinates.
    glm::mat4 projection, view;

    /// Vertex-related objects and shader program handle
    unsigned vao, vbo, program;

    /// Number of vertices in the VBO
    int vertCount;

    /// Current list of distances read from the input source.
    std::vector<float> distances;

    /// Set to true when new data is available.
    bool notifyNewData = false;

    /// Locks during changes to distance data.
    std::mutex lockNewData;

    /// Thread used to manage stdin input
    std::thread inputThread;
};

/**
 * Initializes the application and executes its main loop.
 */
void App::run ()
{
    makeContext ();
    makeProgram ();
    makeVAO ();
    makeInputThread ();
    makeProjection ();

    while (!glfwWindowShouldClose (window)) {
        glfwPollEvents ();
        updateMesh ();
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray (vao);
	glUseProgram (program);
        glDrawArrays(GL_TRIANGLES, 0, vertCount);
        glfwSwapBuffers (window);
    }
}

/**
 * Creates a Window and OpenGL Context used to render.
 */
void App::makeContext ()
{
    glfwInit ();
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint (GLFW_RESIZABLE, GLFW_FALSE);
    if (GLEW_KHR_debug) {
	    std::cout << "DBGO" << std::endl;
	    glfwWindowHint (GLFW_OPENGL_DEBUG_CONTEXT, 1);
    }
    window = glfwCreateWindow (512, 512, "Scan", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit ();
    glEnable (GL_DEPTH_TEST);
}

/**
 * Creates a shader program used to shade the generated mesh.
 */
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

/**
 * Compiles a GLSL shader given its source and shader type.
 * Type can be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
 */
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

/**
 * Creates the VAO and VBO objects, as well as attribute pointers, used to
 * render the mesh to the screen.
 */
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

void App::makeProjection ()
{
	projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
	view = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, glm::value_ptr(view));
}


/**
 * Handles cleanup of the application.
 */
App::~App ()
{
    inputThread.detach();

    glDeleteProgram (program);
    glDeleteBuffers (1, &vbo);
    glDeleteVertexArrays (1, &vao);
    glfwDestroyWindow (window);
    glfwTerminate ();
}


/**
 * Spawns a thread which constantly polls stdin for new distances and notifies
 * the main thread when data is acquired.
 */
void App::makeInputThread ()
{
    // Check for new vertices on stdin
    inputThread = std::thread([&]
    {
        float value;

        while (std::cin >> value) {
            lockNewData.lock();
            std::cout << "[New Value] " << value << std::endl;
            distances.push_back(value);
            notifyNewData = true;
            lockNewData.unlock();
        }
    });
}

/**
 * Checks if new data has been made available, and generates a new mesh for
 * on-screen viewing if necessary.
 */
void App::updateMesh ()
{
    if (notifyNewData) {
        lockNewData.lock();
        std::cout << "Regenerating mesh here..." << std::endl;

	std::vector<glm::vec2> points;
	std::vector<float> vertices;

	// Convert distances to points
	for (int i = 0; i < distances.size(); i++) {
		glm::vec2 norm (1.0f, 0.0f);
		float angle = i / distances.size() * (glm::pi<float>() * 2);
		norm = glm::rotate (norm, angle);
		glm::vec2 point = norm * distances[i];
		points.push_back(point);
	}

	const float HEIGHT = 10;

	// Triangulate points
	for (int i = 0; i < points.size(); i++) {
		int otheri = i - 1;
		if (i == 0) otheri = points.size() - 1;

		// Triangle A
		// Bottom right -> bottom left -> top right
		vertices.push_back(points[i].x);
		vertices.push_back(points[i].y);
		vertices.push_back(0);

		// Fake normals for now
		vertices.push_back(0);
		vertices.push_back(0);
		vertices.push_back(0);

		vertices.push_back(points[otheri].x);
		vertices.push_back(points[otheri].y);
		vertices.push_back(0);

		// fame nrm for now
		vertices.push_back(0);
		vertices.push_back(0);
		vertices.push_back(0);

		vertices.push_back(points[i].x);
		vertices.push_back(points[i].y);
		vertices.push_back(HEIGHT);

		// fame nrm for now
		vertices.push_back(0);
		vertices.push_back(0);
		vertices.push_back(0);

		// Triangle B
		// Bottom left -> top left -> top right
		vertices.push_back(points[otheri].x);
		vertices.push_back(points[otheri].y);
		vertices.push_back(0);

		// fame nrm for now
		vertices.push_back(0);
		vertices.push_back(0);
		vertices.push_back(0);

		vertices.push_back(points[otheri].x);
		vertices.push_back(points[otheri].y);
		vertices.push_back(HEIGHT);

		// fame nrm for now
		vertices.push_back(0);
		vertices.push_back(0);
		vertices.push_back(0);

		vertices.push_back(points[i].x);
		vertices.push_back(points[i].y);
		vertices.push_back(HEIGHT);

		// fame nrm for now
		vertices.push_back(0);
		vertices.push_back(0);
		vertices.push_back(0);
	}

	glBindVertexArray(vao);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

	vertCount = vertices.size() / 6;

        std::cout << "Done Regenerating Mesh" << std::endl;

        notifyNewData = false;
        lockNewData.unlock();
    }
}

int
main (int argc, char **argv)
{
    App app;
    app.run ();
}
