#define GLEW_STATIC
#include<stdio.h>
#include<stdlib.h>
#include<math.h> 
#include<GL/glew.h>
#include<GLFW/glfw3.h>
#include<iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include "GLSLShader.h"
#include "Grid.h"
#include"imgui/imgui.h"
#include"imgui/imgui_impl_glfw.h"
#include"imgui/imgui_impl_opengl3.h"
#define GL_CHECK_ERRORS assert(glGetError()== GL_NO_ERROR);
using namespace std;

#pragma region 产生体数据
int Dim[3] = { 200,200,200 };//体数据大小
int* Data = (int*)malloc(sizeof(int) * Dim[0] * Dim[1] * Dim[2]);
GLubyte CData[200][200][200][4];
glm::vec4 smallCubeC = glm::vec4(1.0, 1.0, 0.0, 1.0);
glm::vec4 middleSphereC = glm::vec4(1.0, 0.0, 0.0, 1.0);
glm::vec4 largeCubeC = glm::vec4(1.0, 1.0, 1.0, 1.0);
float smallCubeD = 0.05;
float middleSphereD = 0.015;
float largeCubeD = 0.018;
void GenCube(int x, int y, int z, int side, int density, int* Data, int* Dim)
{
	int max_x = x + side, max_y = y + side, max_z = z + side;
	int Dimxy = Dim[0] * Dim[1];
	for (int k = z; k < max_z; k++)
	{
		for (int j = y; j < max_y; j++)
		{
			for (int i = x; i < max_x; i++)
			{
				Data[k * Dimxy + j * Dim[0] + i] = density;
			}
		}
	}
}


void GenSphere(int x, int y, int z, int radius, int density, int* Data, int* Dim)
{
	int radius2 = radius * radius;
	int Dimxy = Dim[0] * Dim[1];
	for (int k = 0; k < Dim[2]; k++)
	{
		for (int j = 0; j < Dim[1]; j++)
		{
			for (int i = 0; i < Dim[0]; i++)
			{
				if ((i - x) * (i - x) + (j - y) * (j - y) + (k - z) * (k - z) <= radius2)
				{
					Data[k * Dimxy + j * Dim[0] + i] = density;
				}
			}
		}
	}
}

void GenerateVolume(int* Data, int* Dim)
{
	GenCube(0, 0, 0, 200, 100, Data, Dim);//大正方体
	GenSphere(100, 100, 100, 80, 200, Data, Dim);//球体
	GenCube(70, 70, 70, 60, 300, Data, Dim);//小正方体
}

void Classify(GLubyte CData[200][200][200][4], int* Data, int* Dim)
{
	int* LinePS = Data;
	for (int k = 0; k < Dim[2]; k++)
	{
		for (int j = 0; j < Dim[1]; j++)
		{
			for (int i = 0; i < Dim[0]; i++)
			{
				if (LinePS[0] <= 100)
				{
					//白色
					CData[i][j][k][0] = 255.0 * largeCubeC[0];
					CData[i][j][k][1] = 255.0 * largeCubeC[1];
					CData[i][j][k][2] = 255.0 * largeCubeC[2];
					CData[i][j][k][3] = largeCubeD*255.0;
				}
				else if (LinePS[0] <= 200)
				{
					//红色
					CData[i][j][k][0] = 255.0 * middleSphereC[0];
					CData[i][j][k][1] = 255.0 * middleSphereC[1];
					CData[i][j][k][2] = 255.0 * middleSphereC[2];
					CData[i][j][k][3] = middleSphereD*255.0;
				}
				else
				{
					//黄色
					CData[i][j][k][0] = 255.0 * smallCubeC[0];
					CData[i][j][k][1] = 255.0 * smallCubeC[1];
					CData[i][j][k][2] = 255.0 * smallCubeC[2];
					CData[i][j][k][3] = smallCubeD*255.0;
				}
				LinePS++;
			}
		}
	}
	//return CDdata[200][200][200][4];
}
#pragma endregion

#pragma region 初始化全局变量
//screen resolution
const int WIDTH = 1600;
const int HEIGHT = 1200;

//camera transform variables
int state = 0, oldX = 0, oldY = 0;
float rX = 20.0, rY = 45.0, dist = -2.8;
CGrid* grid;

//modelview projection matrices
glm::mat4 P=glm::perspective(45.0f, (float)WIDTH / HEIGHT, 0.001f, 1000.0f);
//cube vertex array and vertex buffer object IDs
GLuint cubeVBOID;
GLuint cubeVAOID;
GLuint cubeIndicesID;

//ray casting shader
GLSLShader shader;

//background colour
glm::vec4 bg = glm::vec4((float)175/255, float(198)/255, float(246) / 255, 1);


//volume dataset filename  
const std::string volume_file = "Engine256.raw";

//volume dimensions
const int XDIM = 256;
const int YDIM = 256;
const int ZDIM = 256;

//volume texture ID
GLuint textureID;
#pragma endregion

#pragma region 产生3D纹理
bool LoadVolume() {
	GenerateVolume(Data, Dim);//生成原始体数据
	Classify(CData, Data, Dim);//对体数据分类
	std::ifstream infile(volume_file.c_str(), std::ios_base::binary);

	if (infile.good()) {
	//	//read the volume data file
		GLubyte* pData = new GLubyte[XDIM * YDIM * ZDIM];
		infile.read(reinterpret_cast<char*>(pData), XDIM * YDIM * ZDIM * sizeof(GLubyte));
		infile.close();

		//generate OpenGL texture
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_3D, textureID);
		// set the texture parameters
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		//set the mipmap levels (base and max)
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);
		//glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, XDIM, YDIM, ZDIM, 0, GL_RED, GL_UNSIGNED_BYTE, pData);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 200, 200, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, CData);
		GL_CHECK_ERRORS

		//generate mipmaps
		glGenerateMipmap(GL_TEXTURE_3D);
		//delete the volume data allocated on heap
		//delete[] pData;
		//free(Data);
		return true;
	}
	else {
		return false;
	}
}
#pragma endregion

#pragma region 初始化渲染器
//OpenGL initialization
void OnInit() {

	GL_CHECK_ERRORS

	//create a uniform grid of size 20x20 in XZ plane
	grid = new CGrid(20, 20);

	GL_CHECK_ERRORS
	//Load the raycasting shaderraycaster
	shader.LoadFromFile(GL_VERTEX_SHADER, "shaders/raycaster.vert");
	shader.LoadFromFile(GL_FRAGMENT_SHADER, "shaders/raycasterCube.frag");

	//compile and link the shader
	shader.CreateAndLinkProgram();
	shader.Use();
	//add attributes and uniforms
	shader.AddAttribute("vVertex");
	shader.AddUniform("MVP");
	shader.AddUniform("volume");
	shader.AddUniform("camPos");
	shader.AddUniform("step_size");
	shader.AddUniform("model");

	//pass constant uniforms at initialization
	glUniform3f(shader("step_size"), 1.0f / 200, 1.0f / 200, 1.0f / 200);
	glUniform1i(shader("volume"), 0);
	shader.UnUse();

	GL_CHECK_ERRORS

		//load volume data
		if (LoadVolume()) {
			std::cout << "Volume data loaded successfully." << std::endl;
		}
		else {
			std::cout << "Cannot load volume data." << std::endl;
			exit(EXIT_FAILURE);
		}

	

	//setup unit cube vertex array and vertex buffer objects
	glGenVertexArrays(1, &cubeVAOID);
	glGenBuffers(1, &cubeVBOID);
	glGenBuffers(1, &cubeIndicesID);

	//unit cube vertices 
	glm::vec3 vertices[8] = { glm::vec3(-0.5f,-0.5f,-0.5f),
							  glm::vec3(0.5f,-0.5f,-0.5f),
							  glm::vec3(0.5f, 0.5f,-0.5f),
							  glm::vec3(-0.5f, 0.5f,-0.5f),
							  glm::vec3(-0.5f,-0.5f, 0.5f),
							  glm::vec3(0.5f,-0.5f, 0.5f),
							  glm::vec3(0.5f, 0.5f, 0.5f),
							  glm::vec3(-0.5f, 0.5f, 0.5f) };

	//unit cube indices
	GLushort cubeIndices[36] = { 0,5,4,
							     5,0,1,
							     3,7,6,
							     3,6,2,
							     7,4,6,
							     6,4,5,
							     2,1,3,
							     3,1,0,
							     3,0,7,
							     7,0,4,
							     6,5,2,
							     2,5,1 };
	glBindVertexArray(cubeVAOID);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBOID);
	//pass cube vertices to buffer object memory
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &(vertices[0].x), GL_STATIC_DRAW);

	GL_CHECK_ERRORS
	//enable vertex attributre array for position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//pass indices to element array  buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeIndicesID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), &cubeIndices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

	//enable depth test
	glEnable(GL_DEPTH_TEST);

	//set the over blending function
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	cout << "Initialization successfull" << endl;
}
#pragma endregion

#pragma region 窗口回调函数
//release all allocated resources
void OnShutdown(GLFWwindow* window) {
	shader.DeleteShaderProgram();

	glDeleteVertexArrays(1, &cubeVAOID);
	glDeleteBuffers(1, &cubeVBOID);
	glDeleteBuffers(1, &cubeIndicesID);

	glDeleteTextures(1, &textureID);
	delete grid;
	free(Data);
	cout << "Shutdown successfull" << endl;
}
//resize event handler
void OnResize(GLFWwindow* window,int w, int h) {
	//reset the viewport
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	//setup the projection matrix
	P = glm::perspective(45.0f, (float)w/h, 0.001f, 1000.0f);
}
#pragma endregion

#pragma region 绘制函数
//display callback function
void OnRender() {
	GL_CHECK_ERRORS

	//set the camera transform
	glm::mat4 Tr = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, dist));
	glm::mat4 Rx = glm::rotate(Tr, glm::radians(rX), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 MV = glm::rotate(Rx, glm::radians(rY), glm::vec3(0.0f, 1.0f, 0.0f));

	//get the camera position
	glm::vec3 camPos = glm::vec3(glm::inverse(MV) * glm::vec4(0, 0, 0, 1));

	//clear colour and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//get the combined modelview projection matrix
	glm::mat4 MVP = P * MV;

	//render grid
	grid->Render(glm::value_ptr(MVP));

	//enable blending and bind the cube vertex array object
	glEnable(GL_BLEND);
	glBindVertexArray(cubeVAOID);
	//bind the raycasting shader
	shader.Use();
	//pass shader uniforms
	glUniformMatrix4fv(shader("MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
	//glUniformMatrix4fv(shader("model"), 1, GL_FALSE, glm::value_ptr(MVP));
	glUniform3fv(shader("camPos"), 1, &(camPos.x));
	//render the cube
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	//unbind the raycasting shader
	shader.UnUse();
	//disable blending
	glDisable(GL_BLEND);

	//swap front and back buffers to show the rendered result
	//glutSwapBuffers();
}
#pragma endregion

#pragma region  鼠标控制

int flag = 0;
float lastX, lastY;
float anglex, angley;
void glfw_cursor_pos_callback(GLFWwindow* window, double x, double y) {
	ImGui_ImplGlfw_CursorPosCallback( window, x, y);
	if (ImGui::GetIO().WantCaptureMouse) return;
	if (flag == 1)
	{
		float deltaX, deltaY;
		deltaX = x - lastX;
		deltaY = y - lastY;
		rX = 150*deltaY / HEIGHT+anglex;
		rY = 150*deltaX / WIDTH+angley;
	}
}
void glfw_mouse_button_callback(GLFWwindow* window, int button, int action,int mods) {
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	if (ImGui::GetIO().WantCaptureMouse) return;
	if (action == GLFW_PRESS) {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{ 
			double x, y;
			glfwGetCursorPos(window, &x, &y);
			lastX = x;
			lastY = y;
			anglex = rX;
			angley = rY;
			flag=1;
		}
	}
	if(action==GLFW_RELEASE)
	{
		flag = 0;
	}

}
void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	const float speed_fact = 1e-1f;
	if (yoffset < 0)
		dist += speed_fact;
	if (yoffset > 0)
		dist -= speed_fact;
}
#pragma endregion
int main(int argc, char** argv) {

	#pragma region 打开GLFW window
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "VolumeRendering", NULL, NULL);
	if (window == NULL)
	{
		printf("window create failed");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	//初始化GLEW
	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		printf("Init GlEW failed");
		glfwTerminate();
		return -1;
	}
	glViewport(0, 0, WIDTH, HEIGHT);
	glEnable(GL_DEPTH_TEST);

	#pragma endregion
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	auto fonts = ImGui::GetIO().Fonts;
	fonts->AddFontFromFileTTF(
		"c:/windows/fonts/simhei.ttf",
		16.0f,
		NULL,
		fonts->GetGlyphRangesChineseSimplifiedCommon()
	);
	ImGui::StyleColorsLight();
	 // Our state
	//io.Fonts->AddFontFromFileTTF("c:/windows/fonts/simhei.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	OnInit();
	glfwSetWindowCloseCallback(window, OnShutdown);
	//glfwSetScrollCallback(window, OnResize);
	glfwSetFramebufferSizeCallback(window, OnResize);
	glfwSetScrollCallback(window, glfw_scroll_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetCursorPosCallback(window, glfw_cursor_pos_callback);
	
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(bg.r, bg.g, bg.b, bg.a);
		#pragma region imgui控制界面
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		{
			ImGui::Begin("VolumeRendering");                          // Create a window called "Hello, world!" and append into it.
			ImGui::ColorEdit3(u8"背景色", (float*)&bg); // Edit 3 floats representing a color
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::CollapsingHeader("CameraPos")) {
				//ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
				//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
				//ImGui::Checkbox("Another Window", &show_another_window);
				ImGui::SliderFloat("Rx", &rX, 0.0f, 360.0f);
				ImGui::SliderFloat("Ry", &rY, 0.0f, 360.0f);
				ImGui::SliderFloat("dist", &dist, -5.0f, 2.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			}
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::CollapsingHeader(u8"体数据设置")) {
				ImGui::ColorEdit3(u8"小方块颜色", (float*)&smallCubeC);
				ImGui::SliderFloat(u8"小方块密度", &smallCubeD, 0.0f, 0.1f);
				ImGui::ColorEdit3(u8"圆球颜色", (float*)&middleSphereC);
				ImGui::SliderFloat(u8"圆球密度", &middleSphereD, 0.0f, 0.1f);
				ImGui::ColorEdit3(u8"大方块颜色", (float*)&largeCubeC);
				ImGui::SliderFloat(u8"大方块密度", &largeCubeD, 0.0f, 0.1f);
				if (ImGui::Button(u8"更新体数据"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				{
					Classify(CData, Data, Dim);//对体数据分类
					glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, 200, 200, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, CData);
					GL_CHECK_ERRORS
					//generate mipmaps
					glGenerateMipmap(GL_TEXTURE_3D);
				};
				//ImGui::SameLine();
				//ImGui::Text("counter = %d", counter);
			}
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}
		#pragma endregion
		OnRender();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	GLenum glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
		printf("GLWrap : OpenGL error %i\n", glErr);
	}
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}