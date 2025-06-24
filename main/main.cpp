#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <numbers>
#include <typeinfo>
#include <stdexcept>
#include <iostream>

#include <cstdio>
#include <cstdlib>

#include "../support/error.hpp"
#include "../support/program.hpp"
#include "../support/checkpoint.hpp"
#include "../support/debug_output.hpp"

#include "../vmlib/vec2.hpp"
#include "../vmlib/vec4.hpp"
#include "../vmlib/vec3.hpp"
#include "../vmlib/mat44.hpp"
#include "../vmlib/mat33.hpp"

#include "defaults.hpp"
#include "rapidobj/rapidobj.hpp"
#include "texture.hpp"
#include "simple_mesh.hpp"
#include "loadobj.hpp"
#include "shapes.hpp"

//#define PREPARE_BENCHMARK // Uncomment this to prepare benchmarking
//#define ENABLE_BENCHMARK_FULL // Uncomment this to benchmark full rendering time
//#define ENABLE_BENCHMARK_12 // Uncomment this to benchmark 1.2 rendering time
//#define ENABLE_BENCHMARK_14 // Uncomment this to benchmark 1.4 rendering time
//#define ENABLE_BENCHMARK_15 // Uncomment this to benchmark 1.5 rendering time
//#define CPU_BENCHMARK // Uncomment this to benchmark CPU time

namespace
{
	constexpr char const* kWindowTitle = "COMP3811 - CW2";

	constexpr float kMovementPerSecond_ = 5.f;
	constexpr float kMouseSensitivity_ = 0.01f;

    // Set up query queues for benchmarking
    bool swapQueue = true;
    GLuint queryQueueA[2], queryQueueB[2];
    int frameCount = 0;

	struct Particle_
	{
		bool  active = false;
		float life = 0.f;
		Vec3f position;
		Vec3f direction;
	};

	Vec3f random_spread()
	{
		return {
			// Positive spread along X
			// Positive spread along Y
			// Positive spread along Z
			float(std::rand()) / RAND_MAX * 0.4f,
			float(std::rand()) / RAND_MAX * 0.4f,
			float(std::rand()) / RAND_MAX * 0.4f
		};
	}
	struct State_
	{
		ShaderProgram* prog = nullptr;
		ShaderProgram* landingpadprog = nullptr;
		ShaderProgram* particleprog = nullptr;

		bool splitScreen = false;

		struct CamCtrl_
		{
			bool cameraActive = false;
			float phi = 0.0f;
			float theta = 0.0f;

			Vec3f position{ 0.0f, 5.0f, 10.0f };
			Vec3f groundFixedPosition{ 6.0f, 0.3f, -1.0f };

			float moveSpeed = 0.1f;
			float lastX = 0.0f, lastY = 0.0f;
			float radius;

			// Movement flags
			bool moveForward = false;
			bool moveBackward = false;
			bool moveLeft = false;
			bool moveRight = false;
			bool moveUp = false;
			bool moveDown = false;
			int changeCamera = 0;
		} camControl;


		CamCtrl_ camControl1;
		CamCtrl_ camControl2;

		struct RockCtrl_
		{
			Vec3f position{ 6.f, 0.f, -6.f };
			Vec3f direction{ 0.f, 0.f, 0.f };

			float velocity = 0.0f;
			float maxVelocity = 1.0f;
			float acceleration = 0.02f;
			float time = 0.016f;
			float rotation = 0.0f;

			bool play = false;
			bool pause = false;
			bool reset = false;

			Vec3f rocketPos[3] = {
				{ 1.5f, 2.0f, 1.5f },
				{ -0.5f, 2.0f, -0.5f },
				{ .0f, -1.0f, .0f }
			};
		} rockControl;

		struct ParticleSys_
		{
			static constexpr int kMaxParticles_ = 200;

			std::array<Particle_, kMaxParticles_> particles_;

			float emitRate = 50.f;
			float deltatimeEmit = 0.f; 
			GLuint texture = 0;         
		} particleSys;

		GLuint particleVao = 0;
		GLuint particleVbo = 0;
	};

	void glfw_callback_error_(int, char const*);

	void glfw_callback_key_(GLFWwindow*, int, int, int, int);

	void glfw_callback_motion_(GLFWwindow*, double, double);

	void glfw_callback_mouse_button_(GLFWwindow*, int, int, int);

	struct GLFWCleanupHelper
	{
		~GLFWCleanupHelper();
	};
	struct GLFWWindowDeleter
	{
		~GLFWWindowDeleter();
		GLFWwindow* window;
	};

	Vec3f transform_position(const Mat44f& model2world, const Vec3f& localPos)
	{
		Vec4f transformed = model2world * Vec4f(localPos.x, localPos.y, localPos.z, 1.0f);
		return Vec3f{ transformed.x, transformed.y, transformed.z };
	}
	void rendervaotext(
		const Mat44f& projCameraWorld,
		const Mat33f& normalMatrix,
		GLuint texture,
		GLuint vao,
		std::size_t vertexCount) {

		glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorld.v);
		glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

    void rendertexture(GLuint texture) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
    }

	void renderlight(
		const State_& state,
		Vec3f* pointLightPos = nullptr,
		Vec3f* pointLightsColor = nullptr) {
		//light
		Vec3f lightDirRocket = normalize(Vec3f{ 0.f, 1.f, -1.f });
		glUniform3fv(2, 1, &lightDirRocket.x);

		glUniform3f(3, 0.9f, 0.9f, 0.6f);
		glUniform3f(4, 0.05f, 0.05f, 0.05f);
		glUniform3fv(5, 3, &pointLightPos[0].x);
		glUniform3fv(8, 3, &pointLightsColor[0].x);
		glUniform3f(11, state.camControl.position.x, state.camControl.position.y, state.camControl.position.z);
		glUniform1f(12, 32.0f);
	}
	void rendervao(
		const Mat44f& projCameraWorld,
		const Mat44f& model2world,
		const Mat33f& normalMatrix,
		GLuint vao,
		std::size_t vertexCount) {
		glUniformMatrix4fv(0, 1, GL_TRUE, projCameraWorld.v);
		glUniformMatrix3fv(1, 1, GL_TRUE, normalMatrix.v);
		glUniformMatrix4fv(13, 1, GL_TRUE, model2world.v);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
		glBindVertexArray(0);
	}



	void update_camera(State_& state, State_::CamCtrl_& camControl, GLFWwindow* window, float deltaTime)
	{
		// Calculate the direction vector (negative z-axis)
		Vec3f direction;
		direction.x = std::sin(camControl.phi) * std::cos(camControl.theta);
		direction.y = -std::sin(camControl.theta);
		direction.z = -std::cos(camControl.phi) * std::cos(camControl.theta);

		// Normalize vector
		direction = normalize(direction);

		// Up vector
		Vec3f up{ 0.0f, 1.0f, 0.0f };

		//positive x - axis vector
		Vec3f cameraRight;
		//cross product
		cameraRight.x = direction.y * up.z - direction.z * up.y;
		cameraRight.y = direction.z * up.x - direction.x * up.z;
		cameraRight.z = direction.x * up.y - direction.y * up.x;
		//normalise vector
		cameraRight = normalize(cameraRight);

		//positive y - axis vector
		Vec3f cameraUp;
		//cross product
		cameraUp.x = cameraRight.y * direction.z - cameraRight.z * direction.y;
		cameraUp.y = cameraRight.z * direction.x - cameraRight.x * direction.z;
		cameraUp.z = cameraRight.x * direction.y - cameraRight.y * direction.x;
		//normalise vector
		cameraUp = normalize(cameraUp);

		// Determine movement speed based on key presses
		float speed = camControl.moveSpeed;
		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			speed = 2.0f;
		else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			speed = 0.5f;

		// Update position based on movement flags and deltaTime
		if (camControl.moveForward)
			camControl.position += direction * speed * deltaTime;
		if (camControl.moveBackward)
			camControl.position -= direction * speed * deltaTime;
		if (camControl.moveLeft)
			camControl.position -= cameraRight * speed * deltaTime;
		if (camControl.moveRight)
			camControl.position += cameraRight * speed * deltaTime;
		if (camControl.moveUp)
			camControl.position += cameraUp * speed * deltaTime;
		if (camControl.moveDown)
			camControl.position -= cameraUp * speed * deltaTime;

		// Handle camera mode changes
		switch (camControl.changeCamera)
		{
			// Follow rocket
		case 1:
			camControl.position = state.rockControl.position + Vec3f{ 5.f, 1.f, 3.f };
			camControl.phi = -1.26f;
			camControl.theta = -0.03f;
			break;

			// Ground fixed camera
		case 2:
			camControl.position = camControl.groundFixedPosition;
			camControl.phi = 0.0f;
			camControl.theta = 0.0f;
			break;

			// Reset to original position
		case 3:
			camControl.position = Vec3f{ 0.0f, 5.0f, 10.0f };
			camControl.phi = 0.0f;
			camControl.theta = 0.0f;
			break;

		default:
			break;
		}
	}
	// UPDATE PARTICLES
	void update_particle_system_(State_& state, float deltaTime)
	{
		// Inspiration from 
		// https://learnopengl.com/In-Practice/2D-Game/Particles
		// https://www.youtube.com/watch?v=6PkjU9LaDTQ
		// https://www.youtube.com/watch?v=JXhOYS8mZzg
		// https://gamedev.stackexchange.com/questions/1679/fastest-way-to-create-a-simple-particle-effect
		// Skip updating if the rocket is paused
		if (state.rockControl.pause) return;

		// Handle particle emission only if the rocket is playing
		if (state.rockControl.play)
		{
			state.particleSys.deltatimeEmit += deltaTime;
			int numParticle = static_cast<int>(state.particleSys.emitRate * state.particleSys.deltatimeEmit);
			state.particleSys.deltatimeEmit -= numParticle / state.particleSys.emitRate;

			//calculate exhaust position
			Vec3f exhaustPos = state.rockControl.position + Vec3f{ 0.f, -1.f, 0.f };

			for (auto& p : state.particleSys.particles_)
			{
				//stop if limit reached
				if (numParticle <= 0) break;
				if (!p.active)
				{
					p.active = true;
					p.life = 0.5f;
					p.position = exhaustPos;

					// Downward velocity spread
					p.direction = Vec3f{ 0.f, -2.f, 0.f } + random_spread();
					numParticle--;
				}
			}
		}

		// Update existing particles
		for (auto& p : state.particleSys.particles_)
		{
			if (!p.active) continue;

			p.life -= deltaTime;
			if (p.life <= 0.f)
			{
				p.active = false;
			}
			else
			{
				p.position += p.direction * deltaTime;
			}
		}
	}

	//render particles
	void render_particle_system_(const State_& state, const Mat44f& projView, const Vec3f& camRight, const Vec3f& camUp)
	{
		// Enable blending for transparency 
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Use the particle shader program
		glUseProgram(state.particleprog->programId());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, state.particleSys.texture);

		glUniformMatrix4fv(0, 1, GL_TRUE, projView.v);

		// Prepare a vertex for all data 
		std::vector<float> vertexData;

		// Loop through each particle
		for (const auto& particle : state.particleSys.particles_)
		{

			// Skip if no active particles
			if (!particle.active)
				continue; 

			// Define the size of each particle quad
			float size = 0.5f;

			// Calculate the four corners of the quad facing the camera
			Vec3f right = camRight * size;
			Vec3f up = camUp * size;
			Vec3f center = particle.position;

			Vec3f topLeft = center - right + up;
			Vec3f bottomLeft = center - right - up;
			Vec3f bottomRight = center + right - up;
			Vec3f topRight = center + right + up;

			// Define texture coordinates for each corner
			float texLeft = 0.0f;
			float texRight = 1.0f;
			float texTop = 1.0f;
			float texBottom = 0.0f;

			// First Triangle
			vertexData.push_back(topLeft.x); vertexData.push_back(topLeft.y); vertexData.push_back(topLeft.z);
			vertexData.push_back(texLeft);   vertexData.push_back(texTop);

			vertexData.push_back(bottomLeft.x); vertexData.push_back(bottomLeft.y); vertexData.push_back(bottomLeft.z);
			vertexData.push_back(texLeft);        vertexData.push_back(texBottom);

			vertexData.push_back(bottomRight.x); vertexData.push_back(bottomRight.y); vertexData.push_back(bottomRight.z);
			vertexData.push_back(texRight);       vertexData.push_back(texBottom);

			// Second Triangl
			vertexData.push_back(topLeft.x); vertexData.push_back(topLeft.y); vertexData.push_back(topLeft.z);
			vertexData.push_back(texLeft);   vertexData.push_back(texTop);

			vertexData.push_back(bottomRight.x); vertexData.push_back(bottomRight.y); vertexData.push_back(bottomRight.z);
			vertexData.push_back(texRight);       vertexData.push_back(texBottom);

			vertexData.push_back(topRight.x); vertexData.push_back(topRight.y); vertexData.push_back(topRight.z);
			vertexData.push_back(texRight);    vertexData.push_back(texTop);
		}

		// Bind the Particle VAO
		// Bind the Particle VBO
		// Vertex data
		// Draw all particles as triangles

		glBindVertexArray(state.particleVao);

		glBindBuffer(GL_ARRAY_BUFFER, state.particleVbo);
		glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STREAM_DRAW);

		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexData.size() / 5));
		

		//Cleaning
		glBindVertexArray(0);
		glUseProgram(0);

		glDisable(GL_BLEND);
	}


    //https://www.lighthouse3d.com/tutorials/opengl-timer-query/
    //https://chatgpt.com for idea generation
    void getTimes(GLuint* queryQueue, const char* benchmark) {
        GLuint64 startTime = 0, endTime = 0;
        glGetQueryObjectui64v(queryQueue[0], GL_QUERY_RESULT, &startTime);
        glGetQueryObjectui64v(queryQueue[1], GL_QUERY_RESULT, &endTime);

        // Calculate elapsed time
        GLuint64 resultTime = endTime - startTime;
        std::cout << benchmark << resultTime / 1e6 << " ms" << std::endl;
    }

    void startTimer(GLuint* queryQueue) {

    	// Start recording results into queue A
    	glQueryCounter(queryQueue[0], GL_TIMESTAMP);
    }

    void endTimer(GLuint* queryQueue) {

		// Start recording results into queue B
        glQueryCounter(queryQueue[1], GL_TIMESTAMP);
    }

}

int main() try
{
	// Initialize GLFW
	if (GLFW_TRUE != glfwInit())
	{
		char const* msg = nullptr;
		int ecode = glfwGetError(&msg);
		throw Error("glfwInit() failed with '%s' (%d)", msg, ecode);
	}

	// Ensure that we call glfwTerminate() at the end of the program.
	GLFWCleanupHelper cleanupHelper;

	// Configure GLFW and create window
	glfwSetErrorCallback(&glfw_callback_error_);

	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

	//glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_DEPTH_BITS, 24);

#	if !defined(NDEBUG)
	// When building in debug mode, request an OpenGL debug context. This
	// enables additional debugging features. However, this can carry extra
	// overheads. We therefore do not do this for release builds.
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#	endif // ~ !NDEBUG

	#ifdef PREPARE_BENCHMARK
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
	#endif

	GLFWwindow* window = glfwCreateWindow(
		1280,
		720,
		kWindowTitle,
		nullptr, nullptr
	);

	if (!window)
	{
		char const* msg = nullptr;
		int ecode = glfwGetError(&msg);
		throw Error("glfwCreateWindow() failed with '%s' (%d)", msg, ecode);
	}

	GLFWWindowDeleter windowDeleter{ window };


	// Set up event handling
	// TODO: Additional event handling setup

	State_ state{};
	glfwSetWindowUserPointer(window, &state);

	glfwSetKeyCallback(window, &glfw_callback_key_);
	glfwSetCursorPosCallback(window, &glfw_callback_motion_);
	glfwSetMouseButtonCallback(window, &glfw_callback_mouse_button_);

	// Set up drawing stuff
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // V-Sync is on.

	// Turns off V-Sync to prevent the GPU from idling
    #ifdef PREPARE_BENCHMARK
    glfwSwapInterval(0);
    #endif

	// Initialize GLAD
	// This will load the OpenGL API. We mustn't make any OpenGL calls before this!
	if (!gladLoadGLLoader((GLADloadproc)&glfwGetProcAddress))
		throw Error("gladLoaDGLLoader() failed - cannot load GL API!");

	std::printf("RENDERER %s\n", glGetString(GL_RENDERER));
	std::printf("VENDOR %s\n", glGetString(GL_VENDOR));
	std::printf("VERSION %s\n", glGetString(GL_VERSION));
	std::printf("SHADING_LANGUAGE_VERSION %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Ddebug output
#	if !defined(NDEBUG)
	setup_gl_debug_output();
	std::cout << "Debug enabled" << std::endl;
#	endif // ~ !NDEBUG

	// Global GL state
	OGL_CHECKPOINT_ALWAYS();

	// TODO: global GL setup goes here
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

    #ifdef PREPARE_BENCHMARK
    glDisable(GL_DEBUG_OUTPUT);
    glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

   	// Set up queries
    glGenQueries(2, queryQueueA);
    glGenQueries(2, queryQueueB);
    #endif

	OGL_CHECKPOINT_ALWAYS();

	// Get actual framebuffer size.
	// This can be different from the window size, as standard window
	// decorations (title bar, borders, ...) may be included in the window size
	// but not be part of the drawable surface area.
	int iwidth, iheight;
	glfwGetFramebufferSize(window, &iwidth, &iheight);

	glViewport(0, 0, iwidth, iheight);

	// Other initialization & loading
	// Load shader program
	ShaderProgram prog({
		{ GL_VERTEX_SHADER, "assets/cw2/default.vert" },
		{ GL_FRAGMENT_SHADER, "assets/cw2/default.frag" }
		});

	ShaderProgram landingpadProg({
		{ GL_VERTEX_SHADER, "assets/cw2/landingpad_shader.vert" },
		{ GL_FRAGMENT_SHADER, "assets/cw2/landingpad_shader.frag" }
		});
	ShaderProgram particleProg({
	{ GL_VERTEX_SHADER,   "assets/cw2/particle.vert" },
	{ GL_FRAGMENT_SHADER, "assets/cw2/particle.frag" } 
		});

	state.prog = &prog;
	state.landingpadprog = &landingpadProg;
	state.particleprog = &particleProg;

	OGL_CHECKPOINT_ALWAYS();
	//For dinamic VAO
	glGenVertexArrays(1, &state.particleVao);
	glBindVertexArray(state.particleVao);

	glGenBuffers(1, &state.particleVbo);
	glBindBuffer(GL_ARRAY_BUFFER, state.particleVbo);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STREAM_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

	glBindVertexArray(0);

	 //VAO
	SimpleMeshData langerso = load_wavefront_obj("assets/cw2/langerso.obj");
	GLuint vaolangerso = create_vao(langerso);
	std::size_t vertexCountlangerso = langerso.positions.size();

	SimpleMeshData landingpad = load_wavefront_obj("assets/cw2/landingpad.obj");
	GLuint vaolandingpad = create_vao(landingpad);
	std::size_t vertexCountlandingpad = landingpad.positions.size();

	GLuint orthophoto = load_texture_2d("assets/cw2/L3211E-4k.jpg");
	state.particleSys.texture = load_texture_2d("assets/cw2/particle.png");

	double lastTime = glfwGetTime();

	// Centre rocket cylinder
	// make_rotation_z -> horizontal or vertical
	// make_translation -> resize cylinder
	auto rocketCylinder = make_cylinder(true, 128, { 0.5f, 0.5f, 0.5f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(1.5f, 0.2f, 0.2f));

	// Top-cone rocket
	auto rocketCone = make_cone(true, 128, { 0.f, 0.f, 1.f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(0.5f, 0.2f, 0.2f) * make_translation(Vec3f{ 3.f, 0.f, 0.f })); // Blue cone, translated left

	// Side-wing
	auto sideWing1 = make_cube(false, 0, { 0.f, 0.f, 1.f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(0.05f, 0.1f, 0.1f) * make_translation(Vec3f{ 18.f, 2.f, 0.f }));

	// Side-wing 2
	auto sideWing2 = make_cube(false, 0, { 0.f, 0.f, 1.f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(0.05f, 0.1f, 0.1f) * make_translation(Vec3f{ 18.f, -2.f, 0.f }));

	// Side-cone 1
	auto sideCone1 = make_cone(true, 128, { 0.f, 0.f, 1.f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(0.6f, 0.17f, 0.17f) * make_translation(Vec3f{ 0.f, 0.7f, 0.7f })); // Blue cone, translated left

	// Side-cone 2
	auto sideCone2 = make_cone(true, 128, { 0.f, 0.f, 1.f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(0.6f, 0.17f, 0.17f) * make_translation(Vec3f{ 0.f, 0.7f, -0.7f })); // Blue cone, translated left

	// Side-cone 3
	auto sideCone3 = make_cone(true, 128, { 0.f, 0.f, 1.f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(0.6f, 0.17f, 0.17f) * make_translation(Vec3f{ 0.f, -0.7f, 0.7f })); // Blue cone, translated left

	// Side-cone 4
	auto sideCone4 = make_cone(true, 128, { 0.f, 0.f, 1.f },
		make_rotation_z(std::numbers::pi_v<float> / 2.f) * make_scaling(0.6f, 0.17f, 0.17f) * make_translation(Vec3f{ 0.f, -0.7f, -0.7f })); // Blue cone, translated left

	// Concatentate object
	auto rocket = concatenate(sideCone4,
		concatenate(sideCone3,
			concatenate(sideCone2,
				concatenate(sideCone1,
					concatenate(sideWing2,
						concatenate(sideWing1,
							concatenate(rocketCylinder, rocketCone)))))));
	GLuint vao_rocket = create_vao(rocket);
	std::size_t vertex_rocket = rocket.positions.size();

    #ifdef CPU_BENCHMARK
	using clock = std::chrono::high_resolution_clock;
    auto frameStart = clock::now();
  	#endif

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		// Let GLFW process events
		glfwPollEvents();

        #ifdef CPU_BENCHMARK
        auto frameEnd = clock::now();
        std::chrono::duration<float, std::milli> elapsedTime = frameEnd - frameStart;
        float timeDifference = elapsedTime.count();
        frameStart = frameEnd;
        std::cout << "Frame-to-frame time: " << timeDifference << " ms" << std::endl;

        auto renderStart = clock::now();
	    #endif

		double currentTime = glfwGetTime();
		float deltaTime = static_cast<float>(currentTime - lastTime);

		// Check if window was resized.
		float fbwidth, fbheight;
		{
			int nwidth, nheight;
			glfwGetFramebufferSize(window, &nwidth, &nheight);

			fbwidth = static_cast<float>(nwidth);
			fbheight = static_cast<float>(nheight);

			if (nwidth == 0 || nheight == 0)
			{
				// Window minimized? Pause until it is unminimized.
				do
				{
					glfwWaitEvents();
					glfwGetFramebufferSize(window, &nwidth, &nheight);
				} while (nwidth == 0 || nheight == 0);
			}

			glViewport(0, 0, nwidth, nheight);
		}

		if (auto* statePtr = static_cast<State_*>(glfwGetWindowUserPointer(window)))
		{
			// Update cameras
			if (statePtr->splitScreen)
			{
				// Update both split-screen cameras
				update_camera(*statePtr, statePtr->camControl1, window, deltaTime);
				update_camera(*statePtr, statePtr->camControl2, window, deltaTime);
			}
			else
			{
				// Update primary camera
				update_camera(*statePtr, statePtr->camControl, window, deltaTime);
			}
			update_particle_system_(*statePtr, deltaTime);

			// Update rock position and other state
			if (statePtr->rockControl.play)
			{
				statePtr->rockControl.velocity += statePtr->rockControl.acceleration * statePtr->rockControl.time;

				if (statePtr->rockControl.velocity >= statePtr->rockControl.maxVelocity)
				{
					// Clamp the velocity
					 statePtr->rockControl.velocity = statePtr->rockControl.maxVelocity;
                }

				statePtr->rockControl.position.y += std::pow(statePtr->rockControl.velocity, 2);
				statePtr->rockControl.position.z += 0.09f * std::pow(statePtr->rockControl.velocity, 2);

				statePtr->rockControl.rotation = 0.6f * std::atan(statePtr->rockControl.velocity);
			}

			// Reset rocket to launchpad
			if (statePtr->rockControl.reset)
			{
				statePtr->rockControl.play = false;
				statePtr->rockControl.reset = false;
				statePtr->rockControl.velocity = 0.0f;
				statePtr->rockControl.position = Vec3f{ 6.f, 0.f, -6.f };
				statePtr->rockControl.rotation = 0.0f;
			}

			// Handle other camera mode changes
			if (statePtr->camControl.changeCamera == 1)
			{
				statePtr->camControl.moveForward = false;
				statePtr->camControl.moveBackward = false;
				statePtr->camControl.moveLeft = false;
				statePtr->camControl.moveRight = false;
				statePtr->camControl.moveUp = false;
				statePtr->camControl.moveDown = false;
				statePtr->camControl.phi = -1.26f;
				statePtr->camControl.theta = -0.03f;
				statePtr->camControl.position = statePtr->rockControl.position + Vec3f{ 5.f, 1.f, 3.f };
				statePtr->camControl.cameraActive = false;
			}
			if (statePtr->camControl.changeCamera == 2)
			{
				statePtr->camControl.moveForward = false;
				statePtr->camControl.moveBackward = false;
				statePtr->camControl.moveLeft = false;
				statePtr->camControl.moveRight = false;
				statePtr->camControl.moveUp = false;
				statePtr->camControl.moveDown = false;
				statePtr->camControl.phi = 0.0f;
				statePtr->camControl.theta = 0.0f;
				statePtr->camControl.position = statePtr->camControl.groundFixedPosition;
				statePtr->camControl.cameraActive = false;
			}

			if (statePtr->camControl.changeCamera == 3)
			{
				statePtr->camControl.position = Vec3f{ 0.0f, 5.0f, 10.0f };
				statePtr->camControl.changeCamera = 0;

				// After resetting to free view, manually update movement flags based on current key states
				statePtr->camControl.moveForward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
				statePtr->camControl.moveBackward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
				statePtr->camControl.moveLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
				statePtr->camControl.moveRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
				statePtr->camControl.moveUp = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
				statePtr->camControl.moveDown = glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;
			}

			float angle = 0.0f;
			Mat44f model2world = make_rotation_y(angle);
			Mat33f normalMatrix = mat44_to_mat33(transpose(invert(model2world)));

			Mat44f Rx = make_rotation_x(state.camControl.theta);
			Mat44f Ry = make_rotation_y(state.camControl.phi);
			Mat44f T = make_translation({ -state.camControl.position.x, -state.camControl.position.y, -state.camControl.position.z });

			// For first person camera
			// First do the rotation over x and y
			// Then apply translation
			Mat44f world2camera = Rx * Ry * T;

			Mat44f projection = make_perspective_projection(
				60.f * std::numbers::pi_v<float> / 180.f,
				fbwidth / fbheight,
				0.1f, 100.0f
			);

			//landing pad 1 location
			Mat44f model2worldpad1 = make_translation({ 6.f, 0.f, -6.f });
			Mat33f model2worldpad1matrix = mat44_to_mat33(transpose(invert(model2worldpad1)));

			//landing pad 2 location
			Mat44f model2worldpad2 = make_translation({ -10.f, 0.f, -3.f });
			Mat33f model2worldpad2matrix = mat44_to_mat33(transpose(invert(model2worldpad2)));

			// new rocket position
			Mat44f model2world_rocket = make_translation(state.rockControl.position) * make_rotation_x(state.rockControl.rotation);
			Mat33f rocketmatrix = mat44_to_mat33(transpose(invert(model2world_rocket)));
			Vec3f rocket1WorldPos = transform_position(model2world_rocket, statePtr->rockControl.rocketPos[0]);
			Vec3f rocket2WorldPos = transform_position(model2world_rocket, statePtr->rockControl.rocketPos[1]);
			Vec3f rocket3WorldPos = transform_position(model2world_rocket, statePtr->rockControl.rocketPos[2]);
			Vec3f pointLightPos[3] = { rocket1WorldPos, rocket2WorldPos, rocket3WorldPos };
			Vec3f pointLightsColor[3] = { {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} };
			Mat44f projView = projection * world2camera;

            // Invert world2camera to get camera2world
            Mat44f camera2world = invert(world2camera);

            // Extract camera right and up vectors from camera2world matrix
            Vec3f camRight{ camera2world(0, 0), camera2world(1, 0), camera2world(2, 0) };
            Vec3f camUp{ camera2world(0, 1), camera2world(1, 1), camera2world(2, 1) };

			#ifdef ENABLE_BENCHMARK_FULL
            float distancex = std::abs(statePtr->camControl.position.x - statePtr->rockControl.position.x);
            float distancey = std::abs(statePtr->camControl.position.y - statePtr->rockControl.position.y);
            float distancez = std::abs(statePtr->camControl.position.z - statePtr->rockControl.position.z);
            float totalDistance = std::sqrt(std::pow(distancex,2) + std::pow(distancey,2) + std::pow(distancez,2));
            std::cout << totalDistance << " " << frameCount++ << std::endl;
            std::cout << statePtr->camControl.position.x << " " << statePtr->camControl.position.y << " " << statePtr->camControl.position.z << std::endl;
			#endif

			// Clear the screen
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			if (statePtr->splitScreen)
			{
				// Define viewports for split-screen (horizontal split: top and bottom)
				int windowWidth, windowHeight;
				glfwGetFramebufferSize(window, &windowWidth, &windowHeight);

				int viewWidth = windowWidth / 2;
				int viewHeight = windowHeight;

				//Left View
				glViewport(0, 0, viewWidth, viewHeight);


				// Compute camera matrices for Left
				Mat44f projection1 = make_perspective_projection(
					60.f * std::numbers::pi_v<float> / 180.f,
					static_cast<float>(viewWidth) / static_cast<float>(viewHeight),
					0.1f, 100.0f
				);

				Mat44f Rx1 = make_rotation_x(statePtr->camControl1.theta);
				Mat44f Ry1 = make_rotation_y(statePtr->camControl1.phi);
				Mat44f T1 = make_translation({ -statePtr->camControl1.position.x, -statePtr->camControl1.position.y, -statePtr->camControl1.position.z });

				Mat44f world2camera1 = Rx1 * Ry1 * T1;
				Mat44f projView1 = projection1 * world2camera1;
				Mat44f camera2world1 = invert(world2camera1);
				Vec3f camRight1{ camera2world1(0, 0), camera2world1(1, 0), camera2world1(2, 0) };
				Vec3f camUp1{ camera2world1(0, 1), camera2world1(1, 1), camera2world1(2, 1) };

				// Left View
				glUseProgram(statePtr->prog->programId());

                rendertexture(orthophoto);
				rendervaotext(projection1 * world2camera1 * model2world, normalMatrix, orthophoto, vaolangerso, vertexCountlangerso);

				renderlight(*statePtr, pointLightPos, pointLightsColor);

				// Landing pads for View 1
				glUseProgram(statePtr->landingpadprog->programId());
				rendervao(projection1 * world2camera1 * model2worldpad1, model2worldpad1, model2worldpad1matrix, vaolandingpad, vertexCountlandingpad);
				rendervao(projection1 * world2camera1 * model2worldpad2, model2worldpad2, model2worldpad2matrix, vaolandingpad, vertexCountlandingpad);

				// Rocket for View 1
				rendervao(projection1 * world2camera1 * model2world_rocket, model2world_rocket, rocketmatrix, vao_rocket, vertex_rocket);
				
				// Lights
				renderlight(*statePtr, pointLightPos, pointLightsColor);

				// Particles
				render_particle_system_(*statePtr, projView1, camRight1, camUp1);

				// Right view
				glViewport(viewWidth, 0, viewWidth, viewHeight);

				// Coamera matrices for Right
				Mat44f projection2 = make_perspective_projection(
					60.f * std::numbers::pi_v<float> / 180.f,
					static_cast<float>(viewWidth) / static_cast<float>(windowHeight),
					0.1f, 100.0f
				);

				Mat44f Rx2 = make_rotation_x(statePtr->camControl2.theta);
				Mat44f Ry2 = make_rotation_y(statePtr->camControl2.phi);
				Mat44f T2 = make_translation({ -statePtr->camControl2.position.x, -statePtr->camControl2.position.y, -statePtr->camControl2.position.z });

				Mat44f world2camera2 = Rx2 * Ry2 * T2;
				Mat44f projView2 = projection2 * world2camera2;
				Mat44f camera2world2 = invert(world2camera2);
				Vec3f camRight2{ camera2world2(0, 0), camera2world2(1, 0), camera2world2(2, 0) };
				Vec3f camUp2{ camera2world2(0, 1), camera2world2(1, 1), camera2world2(2, 1) };

				// Right view
				glUseProgram(statePtr->prog->programId());

                rendertexture(orthophoto);
				rendervaotext(projection2 * world2camera2 * model2world, normalMatrix, orthophoto, vaolangerso, vertexCountlangerso);

				renderlight(*statePtr, pointLightPos, pointLightsColor);

				// Landing pads for View 2
				glUseProgram(statePtr->landingpadprog->programId());
				rendervao(projection2 * world2camera2 * model2worldpad1, model2worldpad1, model2worldpad1matrix, vaolandingpad, vertexCountlandingpad);
				rendervao(projection2 * world2camera2 * model2worldpad2, model2worldpad2, model2worldpad2matrix, vaolandingpad, vertexCountlandingpad);

				// Rocket for View 2
				rendervao(projection2 * world2camera2 * model2world_rocket, model2world_rocket, rocketmatrix, vao_rocket, vertex_rocket);
				
				// Lights
				renderlight(*statePtr, pointLightPos, pointLightsColor);

				// Particles
				render_particle_system_(*statePtr, projView2, camRight2, camUp2);

				glBindVertexArray(0);
				glUseProgram(0);
			}
			else
			{
				// Single view rendering
				glViewport(0, 0, fbwidth, fbheight);

				// Compute camera matrices for the primary view
				Mat44f projection = make_perspective_projection(
					60.f * std::numbers::pi_v<float> / 180.f,
					fbwidth / fbheight,
					0.1f, 100.0f
				);

				Mat44f Rx = make_rotation_x(statePtr->camControl.theta);
				Mat44f Ry = make_rotation_y(statePtr->camControl.phi);
				Mat44f T = make_translation({ -statePtr->camControl.position.x, -statePtr->camControl.position.y, -statePtr->camControl.position.z });

				Mat44f world2camera = Rx * Ry * T;

                // Start benchmarking for full rendering
                #ifdef ENABLE_BENCHMARK_FULL
                if (swapQueue) {

                    getTimes(queryQueueB, "Full: ");
                    startTimer(queryQueueA);
                }

                else {

                    getTimes(queryQueueA, "Full: ");
                    startTimer(queryQueueB);
                }
                #endif

				// Render the primary view
				glUseProgram(statePtr->prog->programId());

                // Render texture
                rendertexture(orthophoto);

				// Start benchmarking for task 1.2
                #ifdef ENABLE_BENCHMARK_12
                if (swapQueue) {

                    getTimes(queryQueueB, "1.2: ");
                    startTimer(queryQueueA);
                }

                else {

                    getTimes(queryQueueA, "1.2: ");
                    startTimer(queryQueueB);
                }
                #endif

                // Render mesh
				rendervaotext(projection * world2camera * model2world, normalMatrix, orthophoto, vaolangerso, vertexCountlangerso);

                // Finish benchmarking for task 1.2
                #ifdef ENABLE_BENCHMARK_12
                if (swapQueue) {
                	endTimer(queryQueueA);
                }
                else {
                	endTimer(queryQueueB);
                }
                swapQueue = !swapQueue;
                #endif

                // Render lights
				renderlight(*statePtr, pointLightPos, pointLightsColor);

				glUseProgram(statePtr->landingpadprog->programId());

                // Start benchmarking for task 1.4
                #ifdef ENABLE_BENCHMARK_14
                if (swapQueue) {
                    getTimes(queryQueueB, "1.4: ");
                    startTimer(queryQueueA);
                }
                else {
                    getTimes(queryQueueA, "1.4: ");
                    startTimer(queryQueueB);
                }
                #endif

				// Render landing pads
				rendervao(projection * world2camera * model2worldpad1, model2worldpad1, model2worldpad1matrix, vaolandingpad, vertexCountlandingpad);
				rendervao(projection * world2camera * model2worldpad2, model2worldpad2, model2worldpad2matrix, vaolandingpad, vertexCountlandingpad);

                // Finish benchmarking for task 1.4
                #ifdef ENABLE_BENCHMARK_14
                if (swapQueue) {
                	endTimer(queryQueueA);
                }
                else {
                	endTimer(queryQueueB);
                }
                swapQueue = !swapQueue;
                #endif

				// Start benchmarking for task 1.5
                #ifdef ENABLE_BENCHMARK_15
                if (swapQueue) {
                    getTimes(queryQueueB, "1.5: ");
                    startTimer(queryQueueA);
                }

                else {
                    getTimes(queryQueueA, "1.5: ");
                    startTimer(queryQueueB);
                }
                #endif

                // Render rocket
				rendervao(projection * world2camera * model2world_rocket, model2world_rocket, rocketmatrix, vao_rocket, vertex_rocket);

                // Finish benchmarking for task 1.5
                #ifdef ENABLE_BENCHMARK_15
                if (swapQueue) {

                	endTimer(queryQueueA);
                }
                else {

                	endTimer(queryQueueB);
                }
                swapQueue = !swapQueue;
                #endif

				renderlight(*statePtr, pointLightPos, pointLightsColor);
				render_particle_system_(*statePtr, projView, camRight, camUp);

				glBindVertexArray(0);
				glUseProgram(0);

                // Finish CPU benchmarking
                #ifdef CPU_BENCHMARK
                auto renderEnd = clock::now();
                std::chrono::duration<float, std::milli> renderTime = renderEnd - renderStart;
                std::cout << "Render Submission Time: " << renderTime.count() << " ms" << std::endl;
                #endif

                // Finish benchmarking for full rendering
                #ifdef ENABLE_BENCHMARK_FULL
                if (swapQueue) {
                    endTimer(queryQueueA);
                }
                else {
                    endTimer(queryQueueB);
                }
                swapQueue = !swapQueue;
                #endif
			}

			// Swap buffers and update time
			glfwSwapBuffers(window);
			lastTime = currentTime;
		}
	}

	// ... [Cleanup code] ...
	if (state.particleVao)
		glDeleteVertexArrays(1, &state.particleVao);
	if (state.particleVbo)
		glDeleteBuffers(1, &state.particleVbo);
	if (state.particleSys.texture)
		glDeleteTextures(1, &state.particleSys.texture);

    glDeleteBuffers(1, &vao_rocket);
    glDeleteBuffers(1, &vaolandingpad);
    glDeleteBuffers(1, &vaolangerso);

    glDeleteVertexArrays(1, &vao_rocket);
    glDeleteVertexArrays(1, &vaolandingpad);
    glDeleteVertexArrays(1, &vaolangerso);

    glDeleteTextures(1, &orthophoto);

	// program has already been deleted
    state.prog = nullptr;
    state.landingpadprog = nullptr;
    state.particleprog = nullptr;

    #ifdef PREPARE_BENCHMARK
    glDeleteQueries(2, queryQueueA);
    glDeleteQueries(2, queryQueueB);
    #endif

	return 0;
}
catch (std::exception const& eErr)
{
	std::fprintf(stderr, "Top-level Exception (%s):\n", typeid(eErr).name());
	std::fprintf(stderr, "%s\n", eErr.what());
	std::fprintf(stderr, "Bye.\n");
	return 1;
}

namespace
{
void glfw_callback_error_(int aErrNum, char const* aErrDesc)
{
	std::fprintf(stderr, "GLFW error: %s (%d)\n", aErrDesc, aErrNum);
}

void glfw_callback_key_(GLFWwindow* aWindow, int aKey, int, int aAction, int aMods)
{
	if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction)
	{
		glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
		return;
	}

	if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
	{
		if (aAction == GLFW_PRESS)
		{
			if (aKey == GLFW_KEY_V)
			{
				// Toggle split-screen mode
				state->splitScreen = !state->splitScreen;
			}
			else if (aKey == GLFW_KEY_C)
			{
				if (aMods & GLFW_MOD_SHIFT)
				{
					// Cycle camera mode for View 2
					state->camControl2.changeCamera = (state->camControl2.changeCamera + 1) % 4;
				}
				else
				{
					if (state->splitScreen)
					{
						// Cycle camera mode for View 1
						state->camControl1.changeCamera = (state->camControl1.changeCamera + 1) % 4;
					}
					else
					{
						// Cycle camera mode for Main Camera
						state->camControl.changeCamera = (state->camControl.changeCamera + 1) % 4;
					}
				}
			}
			else if (aKey == GLFW_KEY_F)
			{
				state->rockControl.play = !state->rockControl.play;
			}
			else if (aKey == GLFW_KEY_R)
			{
				state->rockControl.reset = true;
			}

			// Handle movement flags based on split-screen
			if (state->splitScreen)
			{
				// Update both cameras
				switch (aKey)
				{
				case GLFW_KEY_W:
					state->camControl1.moveForward = true;
					state->camControl2.moveForward = true;
					break;
				case GLFW_KEY_S:
					state->camControl1.moveBackward = true;
					state->camControl2.moveBackward = true;
					break;
				case GLFW_KEY_A:
					state->camControl1.moveLeft = true;
					state->camControl2.moveLeft = true;
					break;
				case GLFW_KEY_D:
					state->camControl1.moveRight = true;
					state->camControl2.moveRight = true;
					break;
				case GLFW_KEY_E:
					state->camControl1.moveUp = true;
					state->camControl2.moveUp = true;
					break;
				case GLFW_KEY_Q:
					state->camControl1.moveDown = true;
					state->camControl2.moveDown = true;
					break;
				default:
					break;
				}
			}
			else
			{
				// Update main camera only
				switch (aKey)
				{
				case GLFW_KEY_W:
					state->camControl.moveForward = true;
					break;
				case GLFW_KEY_S:
					state->camControl.moveBackward = true;
					break;
				case GLFW_KEY_A:
					state->camControl.moveLeft = true;
					break;
				case GLFW_KEY_D:
					state->camControl.moveRight = true;
					break;
				case GLFW_KEY_E:
					state->camControl.moveUp = true;
					break;
				case GLFW_KEY_Q:
					state->camControl.moveDown = true;
					break;
				default:
					break;
				}
			}
		}
		else if (aAction == GLFW_RELEASE)
		{
			// Handle movement flags based on split-screen
			if (state->splitScreen)
			{
				// Update both cameras
				switch (aKey)
				{
				case GLFW_KEY_W:
					state->camControl1.moveForward = false;
					state->camControl2.moveForward = false;
					break;
				case GLFW_KEY_S:
					state->camControl1.moveBackward = false;
					state->camControl2.moveBackward = false;
					break;
				case GLFW_KEY_A:
					state->camControl1.moveLeft = false;
					state->camControl2.moveLeft = false;
					break;
				case GLFW_KEY_D:
					state->camControl1.moveRight = false;
					state->camControl2.moveRight = false;
					break;
				case GLFW_KEY_E:
					state->camControl1.moveUp = false;
					state->camControl2.moveUp = false;
					break;
				case GLFW_KEY_Q:
					state->camControl1.moveDown = false;
					state->camControl2.moveDown = false;
					break;
				default:
					break;
				}
			}
			else
			{
				// Update main camera only
				switch (aKey)
				{
				case GLFW_KEY_W:
					state->camControl.moveForward = false;
					break;
				case GLFW_KEY_S:
					state->camControl.moveBackward = false;
					break;
				case GLFW_KEY_A:
					state->camControl.moveLeft = false;
					break;
				case GLFW_KEY_D:
					state->camControl.moveRight = false;
					break;
				case GLFW_KEY_E:
					state->camControl.moveUp = false;
					break;
				case GLFW_KEY_Q:
					state->camControl.moveDown = false;
					break;
				default:
					break;
				}
			}
		}
	}
}

void glfw_callback_motion_(GLFWwindow* aWindow, double aX, double aY)
{
	if (auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow)))
	{
		// Update camControl if split-screen is off
		if (!state->splitScreen)
		{
			if (state->camControl.cameraActive)
			{
				auto const dx = float(aX - state->camControl.lastX);
				auto const dy = float(aY - state->camControl.lastY);

				state->camControl.phi += dx * kMouseSensitivity_;
				state->camControl.theta += dy * kMouseSensitivity_;

				if (state->camControl.theta > std::numbers::pi_v<float> / 2.f)
					state->camControl.theta = std::numbers::pi_v<float> / 2.f;
				else if (state->camControl.theta < -std::numbers::pi_v<float> / 2.f)
					state->camControl.theta = -std::numbers::pi_v<float> / 2.f;
			}

			state->camControl.lastX = static_cast<float>(aX);
			state->camControl.lastY = static_cast<float>(aY);
		}
		else
		{
			// If split-screen is active, update camControl left
			if (state->camControl1.cameraActive)
			{
				auto const dx = float(aX - state->camControl1.lastX);
				auto const dy = float(aY - state->camControl1.lastY);

				state->camControl1.phi += dx * kMouseSensitivity_;
				state->camControl1.theta += dy * kMouseSensitivity_;
				if (state->camControl.theta > std::numbers::pi_v<float> / 2.f)
					state->camControl.theta = std::numbers::pi_v<float> / 2.f;
				else if (state->camControl.theta < -std::numbers::pi_v<float> / 2.f)
					state->camControl.theta = -std::numbers::pi_v<float> / 2.f;
			}

			state->camControl1.lastX = static_cast<float>(aX);
			state->camControl1.lastY = static_cast<float>(aY);
		}
	}
}
	void glfw_callback_mouse_button_(GLFWwindow* aWindow, int button, int action, int)
	{
		auto* state = static_cast<State_*>(glfwGetWindowUserPointer(aWindow));
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		{
			state->camControl.cameraActive = !state->camControl.cameraActive;

			if (state->camControl.cameraActive)
				glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
			else
				glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

namespace
{
	GLFWCleanupHelper::~GLFWCleanupHelper()
	{
		glfwTerminate();
	}

	GLFWWindowDeleter::~GLFWWindowDeleter()
	{
		if (window)
			glfwDestroyWindow(window);
	}
}