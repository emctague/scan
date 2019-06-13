#define GLM_ENABLE_EXPERIMENTAL
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

    /// Handle input
    void handleInput ();

    /// The window handle
    GLFWwindow *window;

    /// Projection and view matrices for rendering - model matrix is not needed
    /// as the model is generated with in-place coordinates.
    glm::mat4 projection, view;

    /// Vertex-related objects and shader program handle
    unsigned vao, vbo, program;

    /// Number of vertices in the VBO
    int vertCount;

    /// Position of camera
    glm::vec3 pos;

    /// Camera front
    glm::vec3 front;

    /// Camera up
    glm::vec3 up;

    /// Look dir
    float pitch, yaw;

    /// Last mouse pos
    glm::vec2 lastPos;

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
    pos = glm::vec3(0, 3, 0);
    front = glm::vec3(1, 0, 0);
    up = glm::vec3(0, 1, 0);
    pitch = 0.0f;
    yaw = 0.0f;

    makeContext ();
    makeProgram ();
    makeVAO ();
    makeInputThread ();

    while (!glfwWindowShouldClose (window)) {
        glfwPollEvents ();
        updateMesh ();
        handleInput ();
        makeProjection ();
        glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
    window = glfwCreateWindow (512, 512, "Scan", nullptr, nullptr);
    glfwSwapInterval(1);
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
	view = glm::lookAt(pos, pos + front, up);
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

#ifdef ARTIFICIAL_INPUT
        for (int i = 0; i < 20; i++) {
            value = 1.0f + (i / 10.0f);
            std::cout << "[New Value] " << value << std::endl;
            lockNewData.lock();
            distances.push_back(value);
            notifyNewData = true;
            lockNewData.unlock();
        }
#else
        while (std::cin >> value) {
            lockNewData.lock();
            std::cout << "[New Value] " << value << std::endl;
            distances.push_back(value);
            notifyNewData = true;
            lockNewData.unlock();
        }
#endif
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
		float angle = (float)i / (float)distances.size() * (glm::pi<float>() * 2.0f);
		norm = glm::rotate (norm, angle);
		glm::vec2 point = norm * distances[i];
		points.push_back(point);
	}

	const float HEIGHT = 10;

	// Triangulate points
	for (int i = 0; i < points.size(); i++) {
		int otheri = i - 1;
		if (i == 0) otheri = points.size() - 1;

        // Compute a normal
        glm::vec3 a = glm::vec3(points[i].x, 0, points[i].y);
        glm::vec3 b = glm::vec3(points[otheri].x, 0, points[otheri].y);
        glm::vec3 c = glm::vec3(points[otheri].x, 10, points[otheri].y);
        glm::vec3 normal = glm::normalize(glm::cross(c - a, b - a));

		// Triangle A
		// Bottom right -> bottom left -> top right
		vertices.push_back(points[i].x);
		vertices.push_back(0);
		vertices.push_back(points[i].y);

		vertices.push_back(normal.x);
		vertices.push_back(normal.y);
		vertices.push_back(normal.z);

		vertices.push_back(points[otheri].x);
		vertices.push_back(0);
		vertices.push_back(points[otheri].y);

		vertices.push_back(normal.x);
		vertices.push_back(normal.y);
		vertices.push_back(normal.z);

		vertices.push_back(points[i].x);
		vertices.push_back(HEIGHT);
		vertices.push_back(points[i].y);

		vertices.push_back(normal.x);
		vertices.push_back(normal.y);
		vertices.push_back(normal.z);

		// Triangle B
		// Bottom left -> top left -> top right
		vertices.push_back(points[otheri].x);
		vertices.push_back(0);
		vertices.push_back(points[otheri].y);

		vertices.push_back(normal.x);
		vertices.push_back(normal.y);
		vertices.push_back(normal.z);

		vertices.push_back(points[otheri].x);
		vertices.push_back(HEIGHT);
		vertices.push_back(points[otheri].y);

		vertices.push_back(normal.x);
		vertices.push_back(normal.y);
		vertices.push_back(normal.z);

		vertices.push_back(points[i].x);
		vertices.push_back(HEIGHT);
		vertices.push_back(points[i].y);

		vertices.push_back(normal.x);
		vertices.push_back(normal.y);
		vertices.push_back(normal.z);
	}

	glBindVertexArray(vao);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

	vertCount = vertices.size() / 6;

        std::cout << "Done Regenerating Mesh" << std::endl;

        notifyNewData = false;
        lockNewData.unlock();
    }
}

void App::handleInput ()
{
    static bool alreadyDone;
    static bool captured;
    static bool once;
    static float ptime;

    float curTime = glfwGetTime ();
    float dt = curTime - ptime;

    float speed = 3.0f;
    float sens = 90.0f;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!once) {
            captured = !captured;
            alreadyDone = false;
            dt = 0.0f;
            glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }
        once = true;
    } else once = false;

    if (captured) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            pos += speed * front * dt;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            pos -= speed * front * dt;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            pos -= glm::normalize(glm::cross(front, up)) * speed * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            pos += glm::normalize(glm::cross(front, up)) * speed * dt;

        double xp, yp;
        glfwGetCursorPos(window, &xp, &yp);
        glm::vec2 newPos(xp, yp);
        auto diff = lastPos - newPos;
        lastPos = newPos;

        bool changed = (diff.x != 0.0f && diff.y != 0.0f);

        if (alreadyDone) {
            yaw -= diff.x * sens * dt;
            pitch += diff.y * sens * dt;

            if (pitch > 89.0f) pitch = 89.0f;
            else if (pitch < -89.0f) pitch = -89.0f;

            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            front = glm::normalize(front);

            glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0,1,0), front));
            up = glm::cross(front, right);
        }

        if (changed) alreadyDone = true;

    }

    ptime = curTime;
}

int
main (int argc, char **argv)
{
    App app;
    app.run ();
}
