#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <fstream>
#include <vector>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glad/glad.h>
#include <FTGL/ftgl.h>
#include <GLFW/glfw3.h>
#include <SOIL/SOIL.h>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;
    GLuint TextureBuffer;
    GLuint TextureID;

    GLenum PrimitiveMode; // GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_LINE_STRIP_ADJACENCY, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY and GL_TRIANGLES_ADJACENCY
    GLenum FillMode; // GL_FILL, GL_LINE
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
    glm::mat4 projection;
    glm::mat4 model;
    glm::mat4 view;
    GLuint MatrixID; // For use with normal shader
    GLuint TexMatrixID; // For use with texture shader
} Matrices;

struct FTGLFont {
    FTFont* font;
    GLuint fontMatrixID;
    GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path)
{

    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    // Read the Vertex Shader code from the file
    std::string VertexShaderCode;
    std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
    if (VertexShaderStream.is_open()) {
        std::string Line = "";
        while (getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }

    // Read the Fragment Shader code from the file
    std::string FragmentShaderCode;
    std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
    if (FragmentShaderStream.is_open()) {
        std::string Line = "";
        while (getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }

    GLint Result = GL_FALSE;
    int InfoLogLength;

    // Compile Vertex Shader
    //	cout << "Compiling shader : " <<  vertex_file_path << endl;
    char const* VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> VertexShaderErrorMessage(max(InfoLogLength, int(1)));
    glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    //	cout << VertexShaderErrorMessage.data() << endl;

    // Compile Fragment Shader
    //	cout << "Compiling shader : " << fragment_file_path << endl;
    char const* FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> FragmentShaderErrorMessage(max(InfoLogLength, int(1)));
    glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
    //	cout << FragmentShaderErrorMessage.data() << endl;
    // Link the program
    //	cout << "Linking program" << endl;
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);

    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    std::vector<char> ProgramErrorMessage(max(InfoLogLength, int(1)));
    glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
    //	cout << ProgramErrorMessage.data() << endl;

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

static void error_callback(int error, const char* description)
{
    cout << "Error: " << description << endl;
}

void quit(GLFWwindow* window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue(int hue)
{
    float intp;
    float fracp = modff(hue / 60.0, &intp);
    float x = 1.0 - abs((float)((int)intp % 2) + fracp - 1.0);

    if (hue < 60)
        return glm::vec3(1, x, 0);
    else if (hue < 120)
        return glm::vec3(x, 1, 0);
    else if (hue < 180)
        return glm::vec3(0, 1, x);
    else if (hue < 240)
        return glm::vec3(0, x, 1);
    else if (hue < 300)
        return glm::vec3(x, 0, 1);
    else
        return glm::vec3(1, 0, x);
}

/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject(GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode = GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers(1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers(1, &(vao->ColorBuffer)); // VBO - colors

    glBindVertexArray(vao->VertexArrayID); // Bind the VAO
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData(GL_ARRAY_BUFFER, 3 * numVertices * sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
        0, // attribute 0. Vertices
        3, // size (x,y,z)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
        );

    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData(GL_ARRAY_BUFFER, 3 * numVertices * sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW); // Copy the vertex colors
    glVertexAttribPointer(
        1, // attribute 1. Color
        3, // size (r,g,b)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
        );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject(GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode = GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat[3 * numVertices];
    for (int i = 0; i < numVertices; i++) {
        color_buffer_data[3 * i] = red;
        color_buffer_data[3 * i + 1] = green;
        color_buffer_data[3 * i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject(GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode = GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;
    vao->TextureID = textureID;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers(1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers(1, &(vao->TextureBuffer)); // VBO - textures

    glBindVertexArray(vao->VertexArrayID); // Bind the VAO
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData(GL_ARRAY_BUFFER, 3 * numVertices * sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
        0, // attribute 0. Vertices
        3, // size (x,y,z)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
        );

    glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
    glBufferData(GL_ARRAY_BUFFER, 2 * numVertices * sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW); // Copy the vertex colors
    glVertexAttribPointer(
        2, // attribute 2. Textures
        2, // size (s,t)
        GL_FLOAT, // type
        GL_FALSE, // normalized?
        0, // stride
        (void*)0 // array buffer offset
        );

    return vao;
}

/* Render the VBOs handled by VAO */
void draw3DObject(struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode(GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray(vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject(struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode(GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray(vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Bind Textures using texture units
    glBindTexture(GL_TEXTURE_2D, vao->TextureID);

    // Enable Vertex Attribute 2 - Texture
    glEnableVertexAttribArray(2);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

    // Unbind Textures to be safe
    glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image */
GLuint createTexture(const char* filename)
{
    GLuint TextureID;
    // Generate Texture Buffer
    glGenTextures(1, &TextureID);
    // All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
    glBindTexture(GL_TEXTURE_2D, TextureID);
    // Set our texture parameters
    // Set texture wrapping to GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set texture filtering (interpolation)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load image and create OpenGL texture
    int twidth, theight;
    unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
    SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

    return TextureID;
}

/**************************
 * Customizable functions *
 **************************/

int hover_flag = 0;
int sc_flag = 0;
float hover_y = 0;
int score = 0;
int temp_score = score;
char score_string[3];
char lives_string[1];
char time_string[2];
char level_string[1];
int score_display_flag = 0;
int lives = 3;
int level = 3;
float loading_time = 0;
int init_flag = 0;
bool pause = false;
bool jump = false;
int dir = 1;
int px = 0;
int pz = 9;
float rx = 0;
float ry = 0;
int hole[5];
int tile[5];
float health = 15;
float cy = 0;
int b_m = 0;
bool on_tile = false;
int coin_count=0;
int timer=15;
int time_c=timer;
bool tower_view=false;
bool top_view=false;
bool follow_view=false;	
bool helicopter_view=false;
bool adventure_view=false;
int iteration=0,c_i=0,turn=0;
float camera_rotation_angle = 90;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Function is called first on GLFW_PRESS.

    if (action == GLFW_PRESS) {
        switch (key) {
	case GLFW_KEY_KP_7:
		tower_view=!tower_view;
	break;
	case GLFW_KEY_KP_9:
		top_view=!top_view;
	break;
	case GLFW_KEY_KP_1:
		follow_view=!follow_view;
	break;
	case GLFW_KEY_KP_3:
		adventure_view=!adventure_view;
	break;	
	case GLFW_KEY_KP_4 :
		helicopter_view=true;
		turn=1;
		camera_rotation_angle=90;
	break;
	case GLFW_KEY_KP_6 :
		helicopter_view=true;
		turn=-1;
		camera_rotation_angle=90;
	break;
	case GLFW_KEY_F :
		if(iteration<3){
			iteration++;
			c_i=iteration;
		}
	break;
	case GLFW_KEY_G :
		if(iteration>1){
			iteration--;
			c_i=iteration;
		}
	break;
        case GLFW_KEY_UP:
            if (sc_flag == 0) {
                hover_flag--;
                if (hover_flag < 0)
                    hover_flag = 2;
            }
            else if (sc_flag == 4) {
                hover_flag--;
                if (hover_flag < 5)
                    hover_flag = 6;
            }
            else if (sc_flag == 3 && loading_time > 20 && !jump) {
                pz--;
		if(on_tile && cy<=ry)
			pz++;
                dir = 1;
            }
            break;
        case GLFW_KEY_DOWN:
            if (sc_flag == 0) {
                hover_flag++;
                if (hover_flag > 2)
                    hover_flag = 0;
            }
            else if (sc_flag == 4) {
                hover_flag++;
                if (hover_flag > 6) {
                    hover_flag = 5;
                }
            }
            else if (sc_flag == 3 && loading_time > 20 && !jump ) {
                pz++;
		if(on_tile && cy<=ry)
			pz--;
                dir = 4;
            }
            break;
        case GLFW_KEY_LEFT:
            if (sc_flag == 3 && loading_time > 20 && !jump) {
                px--;
		if(on_tile && cy<=ry)
			px++;
                dir = 2;
            }
            break;
        case GLFW_KEY_RIGHT:
            if (sc_flag == 3 && loading_time > 20 && !jump) {
                px++;
		if(on_tile && cy<=ry)
			px--;
                dir = 3;
            }
            break;
        case GLFW_KEY_ENTER:
            if (sc_flag == 0) {
                if (hover_flag == 2)
                    quit(window);
                else if (hover_flag == 0) {
                    lives = 3;
                    level = 1;
                    loading_time = 0;
                    sc_flag = 3;
                    px = 0;
                    pz = 9;
                    score = 0;
               //     pause = false;
                    jump = false;
                    dir = 1;
                }
                else if (hover_flag == 1)
                    sc_flag = 1;
            }
            else if (sc_flag == 4) {
                if (hover_flag == 6) {
                    quit(window);
                }
                else if (hover_flag == 5)
                    lives = 3;
                level = 1;
                loading_time = 0;
                sc_flag = 0;
                hover_flag = 0;
            }
            break;
        case GLFW_KEY_SPACE:
            jump = true;
            break;
        case GLFW_KEY_BACKSPACE:
            if (hover_flag == 1)
                if (sc_flag == 1)
                    sc_flag = 0;
            break;
        case GLFW_KEY_P:
                pause = !pause;
            break;
        case GLFW_KEY_W:
            if (sc_flag == 3 && loading_time > 20 && !jump)
                dir = 1;
            break;
        case GLFW_KEY_S:
            if (sc_flag == 3 && loading_time > 20 && !jump)
                dir = 4;
            break;
        case GLFW_KEY_A:
            if (sc_flag == 3 && loading_time > 20 && !jump)
                dir = 2;
            break;
        case GLFW_KEY_D:
            if (sc_flag == 3 && loading_time > 20 && !jump)
                dir = 3;
            break;
        default:
            break;
        }
    }
    else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_ESCAPE:
            quit(window);
            break;
	case GLFW_KEY_KP_4 :
		helicopter_view=false;
	break;
	case GLFW_KEY_KP_6 :
		helicopter_view=false;
	break;
        default:
            break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar(GLFWwindow* window, unsigned int key)
{
    switch (key) {
    case 'Q':
    case 'q':
        quit(window);
        break;
    default:
        break;
    }
}

/* Executed when a mouse button is pressed/released */
void mouseButton(GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
        if (action == GLFW_RELEASE)
            if (sc_flag == 0) {
                if (hover_flag == 2)
                    quit(window);
                if (hover_flag == 1)
                    sc_flag = 1;
                if (hover_flag == 0) {
                    lives = 3;
                    level = 1;
                    loading_time = 0;
                    sc_flag = 3;
                    px = 0;
                    pz = 9;
                    score = 0;
                    pause = false;
                    jump = false;
                    dir = 1;
                }
            }
            else if (sc_flag == 1) {
                if (hover_flag == 4)
                    sc_flag = 0;
            }
            else if (sc_flag == 4) {
                if (hover_flag == 6)
                    quit(window);
                if (hover_flag == 5) {
                    lives = 3;
                    level = 1;
                    loading_time = 0;
                    sc_flag = 0;
                    hover_flag = 0;
                }
            }
	    else if(sc_flag == 3 && loading_time > 20 && !jump){
		if(dir==1)
			pz+=1;
		else if(dir==4){
			pz-=1;
		}
		else if(dir==2)
			px-=1;
		else if(dir==3)
			px+=1;
	}
        break;
    case GLFW_MOUSE_BUTTON_RIGHT:
        if (action == GLFW_PRESS) {
            jump=true;
        }
        break;
    default:
        break;
    }
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow(GLFWwindow* window, int width, int height)
{
    int fbwidth = width, fbheight = height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
	 is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

    GLfloat fov = 90.0f;

    // sets the viewport of openGL renderer
    glViewport(0, 0, (GLsizei)fbwidth, (GLsizei)fbheight);

    // set the projection matrix as perspective
    /* glMatrixMode (GL_PROJECTION);
	 glLoadIdentity ();
	 gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
    // Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *cube[100], *rectangle, *hover, *dot, *loading_bar, *life[3], *player, *coins[5], *fire[5], *health_bar;
int coins_x[5], coins_z[5];
int fire_x[5], fire_z[5];

// Creates the triangle object used in this sample code
void createDot()
{
    static const GLfloat vertex_buffer_data[] = {
        0.1, 0.1, 0,
        0.1, -0.1, 0,
        -0.1, -0.1, 0,

        0.1, 0.1, 0,
        -0.1, 0.1, 0,
        -0.1, -0.1, 0
    };
    static const GLfloat color_buffer_data[] = {
        0.5, 0.5, 0,
        0.5, 0.5, 0,
        0.5, 0.5, 0,

        0.5, 0.5, 0,
        0.5, 0.5, 0,
        0.5, 0.5, 0,
    };
    dot = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createFire(int n)
{
    static const GLfloat vertex_buffer_data[] = {
        1.0f, 0, 1.0f,
        1.0f, 0, -1.0f,
        -1.0f, 0, -1.0f,

        1.0f, 0, 1.0f,
        -1.0f, 0, 1.0f,
        -1.0f, 0, -1.0f
    };
    static const GLfloat color_buffer_data[] = {
        1, 0, 0,
        1, 0, 0,
        1, 0, 0,

        1, 0, 0,
        1, 0, 0,
        1, 0, 0
    };
    int i;
    for (i = 0; i < n; i++)
        fire[i] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createLoadBar()
{
    static const GLfloat vertex_buffer_data[] = {
        0.1, 0.1, 0,
        0.1, -0.1, 0,
        -0.1, -0.1, 0,

        0.1, 0.1, 0,
        -0.1, 0.1, 0,
        -0.1, -0.1, 0
    };
    static const GLfloat color_buffer_data[] = {
        0.7, 0.9, 0.7,
        0.6, 0.8, 0.7,
        0.7, 0.9, 0.7,

        0.7, 0.9, 0.7,
        0.6, 0.8, 0.7,
        0.7, 0.9, 0.7,
    };
    loading_bar = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createHealthBar()
{
    static const GLfloat vertex_buffer_data[] = {
        0.1, 0.1, 0,
        0.1, -0.1, 0,
        -0.1, -0.1, 0,

        0.1, 0.1, 0,
        -0.1, 0.1, 0,
        -0.1, -0.1, 0
    };
    static const GLfloat color_buffer_data[] = {
        1.0, 0, 0,
        1.0, 0, 0,
        1.0, 0, 0,

        1.0, 0.0, 0.0,
        1.0, 0.0, 0.0,
        1.0, 0.0, 0.0,
    };
    health_bar = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCube(int n)
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array as used in glBegin (GL_TRIANGLES) */
    static const GLfloat vertex_buffer_data[] = {

        -1.0f, -1.0f, -1.0f, // triangle 1 : begin

        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, 1.0f, // triangle 1 : end

        1.0f, 1.0f, -1.0f, // triangle 2 : begin

        -1.0f, -1.0f, -1.0f,

        -1.0f, 1.0f, -1.0f, // triangle 2 : end

        1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f, -1.0f,

        1.0f, -1.0f, -1.0f,

        1.0f, 1.0f, -1.0f,

        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,

        -1.0f, 1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,

        1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f, -1.0f,

        -1.0f, 1.0f, 1.0f,

        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,

        1.0f, 1.0f, -1.0f,

        1.0f, -1.0f, -1.0f,

        1.0f, 1.0f, 1.0f,

        1.0f, -1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,

        1.0f, 1.0f, -1.0f,

        -1.0f, 1.0f, -1.0f,

        1.0f, 1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,

        -1.0f, 1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,

        -1.0f, 1.0f, 1.0f,

        1.0f, -1.0f, 1.0f

    };

    static const GLfloat color_buffer_data[] = {

        0.583f, 0.771f, 0.014f,

        0.609f, 0.115f, 0.436f,

        0.327f, 0.483f, 0.844f,

        0.822f, 0.569f, 0.201f,

        0.435f, 0.602f, 0.223f,

        0.310f, 0.747f, 0.185f,

        0.597f, 0.770f, 0.761f,

        0.559f, 0.436f, 0.730f,

        0.359f, 0.583f, 0.152f,

        0.483f, 0.596f, 0.789f,

        0.559f, 0.861f, 0.639f,

        0.195f, 0.548f, 0.859f,

        0.014f, 0.184f, 0.576f,

        0.771f, 0.328f, 0.970f,

        0.406f, 0.615f, 0.116f,

        0.676f, 0.977f, 0.133f,

        0.971f, 0.572f, 0.833f,

        0.140f, 0.616f, 0.489f,

        0.997f, 0.513f, 0.064f,

        0.945f, 0.719f, 0.592f,

        0.543f, 0.021f, 0.978f,

        0.279f, 0.317f, 0.505f,

        0.167f, 0.620f, 0.077f,

        0.347f, 0.857f, 0.137f,

        0.055f, 0.953f, 0.042f,

        0.714f, 0.505f, 0.345f,

        0.783f, 0.290f, 0.734f,

        0.722f, 0.645f, 0.174f,

        0.302f, 0.455f, 0.848f,

        0.225f, 0.587f, 0.040f,

        0.517f, 0.713f, 0.338f,

        0.053f, 0.959f, 0.120f,

        0.393f, 0.621f, 0.362f,

        0.673f, 0.211f, 0.457f,

        0.820f, 0.883f, 0.371f,

        0.982f, 0.099f, 0.879f

    };
    // create3DObject creates and returns a handle to a VAO that can be used later
    int i;
    for (i = 0; i < n; i++)
        cube[i] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCoins(int n)
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array as used in glBegin (GL_TRIANGLES) */
    static const GLfloat vertex_buffer_data[] = {

        -0.3f, -0.3f, -0.3f, // triangle 1 : begin

        -0.3f, -0.3f, 0.3f,

        -0.3f, 0.3f, 0.3f, // triangle 1 : end

        0.3f, 0.3f, -0.3f, // triangle 2 : begin

        -0.3f, -0.3f, -0.3f,

        -0.3f, 0.3f, -0.3f, // triangle 2 : end

        0.3f, -0.3f, 0.3f,

        -0.3f, -0.3f, -0.3f,

        0.3f, -0.3f, -0.3f,

        0.3f, 0.3f, -0.3f,

        0.3f, -0.3f, -0.3f,

        -0.3f, -0.3f, -0.3f,

        -0.3f, -0.3f, -0.3f,

        -0.3f, 0.3f, 0.3f,

        -0.3f, 0.3f, -0.3f,

        0.3f, -0.3f, 0.3f,

        -0.3f, -0.3f, 0.3f,

        -0.3f, -0.3f, -0.3f,

        -0.3f, 0.3f, 0.3f,

        -0.3f, -0.3f, 0.3f,

        0.3f, -0.3f, 0.3f,

        0.3f, 0.3f, 0.3f,

        0.3f, -0.3f, -0.3f,

        0.3f, 0.3f, -0.3f,

        0.3f, -0.3f, -0.3f,

        0.3f, 0.3f, 0.3f,

        0.3f, -0.3f, 0.3f,

        0.3f, 0.3f, 0.3f,

        0.3f, 0.3f, -0.3f,

        -0.3f, 0.3f, -0.3f,

        0.3f, 0.3f, 0.3f,

        -0.3f, 0.3f, -0.3f,

        -0.3f, 0.3f, 0.3f,

        0.3f, 0.3f, 0.3f,

        -0.3f, 0.3f, 0.3f,

        0.3f, -0.3f, 0.3f

    };

    static const GLfloat color_buffer_data[] = {

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f,

        1.f, 1.f, 0.f

    };
    // create3DObject creates and returns a handle to a VAO that can be used later
    int i;
    for (i = 0; i < n; i++)
        coins[i] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createPlayer()
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array as used in glBegin (GL_TRIANGLES) */
    static const GLfloat vertex_buffer_data[] = {

        -1.0f, -1.0f, -1.0f, // triangle 1 : begin

        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, 1.0f, // triangle 1 : end

        1.0f, 1.0f, -1.0f, // triangle 2 : begin

        -1.0f, -1.0f, -1.0f,

        -1.0f, 1.0f, -1.0f, // triangle 2 : end

        1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f, -1.0f,

        1.0f, -1.0f, -1.0f,

        1.0f, 1.0f, -1.0f,

        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,

        -1.0f, 1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,

        1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f, 1.0f,

        -1.0f, -1.0f, -1.0f,

        -1.0f, 1.0f, 1.0f,

        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,

        1.0f, 1.0f, -1.0f,

        1.0f, -1.0f, -1.0f,

        1.0f, 1.0f, 1.0f,

        1.0f, -1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,

        1.0f, 1.0f, -1.0f,

        -1.0f, 1.0f, -1.0f,

        1.0f, 1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,

        -1.0f, 1.0f, 1.0f,

        1.0f, 1.0f, 1.0f,

        -1.0f, 1.0f, 1.0f,

        1.0f, -1.0f, 1.0f

    };

    static const GLfloat color_buffer_data[] = {

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f,

        0.5f, 0.f, 0.5f

    };
    player = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createHover()
{
    /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

    /* Define vertex array as used in glBegin (GL_TRIANGLES) */
    static const GLfloat vertex_buffer_data[] = {
        -0.25, -0.25, 0, // vertex 1
        0.25, -0.25, 0, // vertex 2
        0.25, 0.25, 0, // vertex 3

        0.25, 0.25, 0, // vertex 3
        -0.25, 0.25, 0, // vertex 4
        -0.25, -0.25, 0 // vertex 1
    };

    static const GLfloat color_buffer_data[] = {
        1, 1, 0, // color 1
        1, 1, 0, // color 2
        1, 1, 0, // color 3

        1, 1, 0, // color 3
        1, 1, 0, // color 4
        1, 1, 0 // color 1
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    hover = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

// Creates the rectangle object used in this sample code
void createRectangle(GLuint textureID)
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data[] = {
        -4, -4, 0, // vertex 1
        4, -4, 0, // vertex 2
        4, 4, 0, // vertex 3

        4, 4, 0, // vertex 3
        -4, 4, 0, // vertex 4
        -4, -4, 0 // vertex 1
    };

    static const GLfloat color_buffer_data[] = {
        1, 0, 0, // color 1
        0, 0, 1, // color 2
        0, 1, 0, // color 3

        0, 1, 0, // color 3
        0.3, 0.3, 0.3, // color 4
        1, 0, 0 // color 1
    };

    // Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
    static const GLfloat texture_buffer_data[] = {
        0, 1, // TexCoord 1 - bot left
        1, 1, // TexCoord 2 - bot right
        1, 0, // TexCoord 3 - top right

        1, 0, // TexCoord 3 - top right
        0, 0, // TexCoord 4 - top left
        0, 1 // TexCoord 1 - bot left
    };

    // create3DTexturedObject creates and returns a handle to a VAO that can be used later
    rectangle = create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
}

void createLives(GLuint textureID)
{
    // GL3 accepts only Triangles. Quads are not supported
    static const GLfloat vertex_buffer_data[] = {
        -4, -4, 0, // vertex 1
        4, -4, 0, // vertex 2
        4, 4, 0, // vertex 3

        4, 4, 0, // vertex 3
        -4, 4, 0, // vertex 4
        -4, -4, 0 // vertex 1
    };

    static const GLfloat color_buffer_data[] = {
        1, 0, 0, // color 1
        0, 0, 1, // color 2
        0, 1, 0, // color 3

        0, 1, 0, // color 3
        0.3, 0.3, 0.3, // color 4
        1, 0, 0 // color 1
    };

    // Texture coordinates start with (0,0) at top left of the image to (1,1) at bot right
    static const GLfloat texture_buffer_data[] = {
        0, 1, // TexCoord 1 - bot left
        1, 1, // TexCoord 2 - bot right
        1, 0, // TexCoord 3 - top right

        1, 0, // TexCoord 3 - top right
        0, 0, // TexCoord 4 - top left
        0, 1 // TexCoord 1 - bot left
    };

    // create3DTexturedObject creates and returns a handle to a VAO that can be used later
    int i;
    for (i = 0; i < 3; i++)
        life[i] = create3DTexturedObject(GL_TRIANGLES, 6, vertex_buffer_data, texture_buffer_data, textureID, GL_FILL);
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void startscreen()
{
    // clear the color and depth in the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    static float c = 0;
    c++;
    //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(sinf(c*M_PI/180.0),3*cosf(c*M_PI/180.0),0)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP; // MVP = Projection * View * Model

    // Render with texture shaders now
    glUseProgram(textureProgramID);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    Matrices.model = glm::mat4(1.0f);

    //glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
    //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    //Matrices.model *= (translateRectangle * rotateRectangle);
    MVP = VP * Matrices.model;

    // Copy MVP to texture shaders
    glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Set the texture sampler to access Texture0 memory
    glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DTexturedObject(rectangle);

    if (hover_flag == 0)
        hover_y = 0;
    else if (hover_flag == 1)
        hover_y = -1;
    else if (hover_flag == 2)
        hover_y = -2;
    // Load identity to model matrix
    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 translateHover = glm::translate(glm::vec3(-0.1f, 0.15f + hover_y, 0.0f)); // glTranslatef
    glm::mat4 scaleHover = glm::scale(glm::vec3(4.f, 1.f, 0.f));
    //glm::mat4 HoverTransform = translateTriangle * rotateTriangle;
    Matrices.model *= (translateHover * scaleHover);
    MVP = VP * Matrices.model; // MVP = p * V * M

    //  Don't change unless you are sure!!
    // Copy MVP to normal shaders
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(hover);

    // Increment angles
    float increments = 1;

    // Render font on screen
    glm::vec3 fontColor = getRGBfromHue(0);

    // Use font Shaders for next part of code
    glUseProgram(fontProgramID);
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane

    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTitle = glm::translate(glm::vec3(-2, 2, 0));
    Matrices.model *= translateTitle;
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

    // Render font
    GL3Font.font->Render("G r a v i t y");

    glm::vec3 fontColor1 = getRGBfromHue(100);

    // Use font Shaders for next part of code
    glUseProgram(fontProgramID);
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane

    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateNewGame = glm::translate(glm::vec3(-1, 0, 0));
    glm::mat4 scaleNewGame = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateNewGame * scaleNewGame);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor1[0]);
    // Render font
    GL3Font.font->Render("New Game");

    glm::vec3 fontColor2 = getRGBfromHue(50);
    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateControls = glm::translate(glm::vec3(-1, -1, 0));
    glm::mat4 scaleControls = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateControls * scaleControls);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor2[0]);
    // Render font
    GL3Font.font->Render("Controls");

    glm::vec3 fontColor3 = getRGBfromHue(200);
    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateQuit = glm::translate(glm::vec3(-1, -2, 0));
    glm::mat4 scaleQuit = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateQuit * scaleQuit);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render("Quit");
}

void controlsscreen()
{

    // clear the color and depth in the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    static float c = 0;
    c++;
    //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(sinf(c*M_PI/180.0),3*cosf(c*M_PI/180.0),0)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP; // MVP = Projection * View * Model

    // Render with texture shaders now
    glUseProgram(textureProgramID);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    Matrices.model = glm::mat4(1.0f);

    //glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
    //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    //Matrices.model *= (translateRectangle * rotateRectangle);
    MVP = VP * Matrices.model;

    // Copy MVP to texture shaders
    glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Set the texture sampler to access Texture0 memory
    glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DTexturedObject(rectangle);

    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 translateHover = glm::translate(glm::vec3(-3.4f, 3.6f, 0.0f)); // glTranslatef
    glm::mat4 scaleHover = glm::scale(glm::vec3(1.5f, 0.75f, 0.f));
    //glm::mat4 HoverTransform = translateTriangle * rotateTriangle;
    Matrices.model *= (translateHover * scaleHover);
    MVP = VP * Matrices.model; // MVP = p * V * M

    //  Don't change unless you are sure!!
    // Copy MVP to normal shaders
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    if (hover_flag == 4)
        draw3DObject(hover);

    // Increment angles
    float increments = 1;

    // Render font on screen
    glm::vec3 fontColor = getRGBfromHue(0);

    // Use font Shaders for next part of code
    glUseProgram(fontProgramID);
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane

    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTitle = glm::translate(glm::vec3(-1.5, 3, 0));
    Matrices.model *= translateTitle;
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

    // Render font
    GL3Font.font->Render("CONTROLS");

    glm::vec3 fontColor1 = getRGBfromHue(100);

    // Use font Shaders for next part of code
    glUseProgram(fontProgramID);
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane

    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateNewGame = glm::translate(glm::vec3(-3.5, 2, 0));
    glm::mat4 scaleNewGame = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateNewGame * scaleNewGame);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor1[0]);
    // Render font
    GL3Font.font->Render("Keyboard");

    glm::vec3 fontColor2 = getRGBfromHue(50);
    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateControls = glm::translate(glm::vec3(2.5, 2, 0));
    glm::mat4 scaleControls = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateControls * scaleControls);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor2[0]);
    // Render font
    GL3Font.font->Render("Mouse");

    glm::vec3 fontColor3 = getRGBfromHue(200);
    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateQuit = glm::translate(glm::vec3(-3.75, 3.5, 0));
    glm::mat4 scaleQuit = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateQuit * scaleQuit);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render("Back");
}

void loading_effect()
{

    // clear the color and depth in the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    static float c = 0;
    c++;
    //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(sinf(c*M_PI/180.0),3*cosf(c*M_PI/180.0),0)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP; // MVP = Projection * View * Model

    // Render with texture shaders now
    glUseProgram(textureProgramID);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    Matrices.model = glm::mat4(1.0f);

    //glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
    //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    //Matrices.model *= (translateRectangle * rotateRectangle);
    MVP = VP * Matrices.model;

    // Copy MVP to texture shaders
    glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Set the texture sampler to access Texture0 memory
    glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DTexturedObject(rectangle);

    float scx = 1.3;
    float check_float_x;
    int check_int_x;
    check_float_x = loading_time / 0.6;
    check_int_x = check_float_x;
    check_float_x = check_float_x - check_int_x;
    if (check_float_x >= 0 && check_float_x <= 0.17)
        scx = 1.3;
    else if (check_float_x > 0.3 && check_float_x < 0.6)
        scx = 1.55;
    else if (check_float_x > 0.6 && check_float_x < 0.9)
        scx = 1.8;
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateDot = glm::translate(glm::vec3(scx, -0.12, 0)); // glTranslatef
    Matrices.model *= (translateDot);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(dot);
    glUseProgram(programID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateLoadBar = glm::translate(glm::vec3(-2.96 + loading_time * 0.15, -2.48, 0)); // glTranslatef
    Matrices.model *= (translateLoadBar);
    glm::mat4 scaleLoadBar = glm::scale(glm::vec3(1 + (loading_time * 1.5), 2.5, 1));
    Matrices.model *= (scaleLoadBar);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(loading_bar);
}
float ttime = 0;
void gamescreen()
{

    if(c_i!=0){
	if(dir==1)
		pz-=1;
	else if(dir==4)
		pz+=1;
	else if(dir==2)
		px-=1;
	else if(dir==3)
		px+=1;
	c_i--;
    }
    if(helicopter_view){
    	if(turn==1)
		camera_rotation_angle-=0.5;
    	else if(turn==-1)
		camera_rotation_angle+=0.5;
    }
    // clear the color and depth in the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    static float c = 0;
    c++;
    //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(sinf(c*M_PI/180.0),3*cosf(c*M_PI/180.0),0)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP; // MVP = Projection * View * Model

    // Render with texture shaders now
    glUseProgram(textureProgramID);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();

    Matrices.model = glm::mat4(1.0f);

    //glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
    //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
    //Matrices.model *= (translateRectangle * rotateRectangle);
    MVP = VP * Matrices.model;

    // Copy MVP to texture shaders
    glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Set the texture sampler to access Texture0 memory
    glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

    // draw3DObject draws the VAO given to it using current MVP matrix
    //	if(!pause)
    //		draw3DTexturedObject(rectangle);

    glUseProgram(textureProgramID);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    int i = 0;
    for (i = 0; i < lives; i++) {
        Matrices.model = glm::mat4(1.0f);

        glm::mat4 translateLife = glm::translate(glm::vec3(-2.4 + 0.4 * i, 3.6, 0));
        glm::mat4 scaleLife = glm::scale(glm::vec3(0.05, 0.05, 0.05));
        Matrices.model *= (translateLife * scaleLife);
        MVP = VP * Matrices.model;

        // Copy MVP to texture shaders
        glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

        // Set the texture sampler to access Texture0 memory
        glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

        // draw3DObject draws the VAO given to it using current MVP matrix
        draw3DTexturedObject(life[i]);
    }
    // Increment angles
    float increments = 1;

    // Render font on screen
    glm::vec3 fontColor = getRGBfromHue(0);

    if (temp_score != score || score == 0) {
        temp_score = score;
        int i, r, length = 0;
        for (i = 0; i < 3; i++) {
            r = temp_score % 10;
            if (temp_score != 0)
                length++;
            temp_score = temp_score / 10;
            score_string[i] = (char)(r + 48);
        }
        for (i = 0; i < 3; i++)
            if (score_string[i] == '0' && i >= length)
                score_string[i] = ' ';
        if (score == 0) {
            score_string[0] = '0';
        }
        char c;
        for (i = 0; i < length / 2; i++) {
            c = score_string[i];
            score_string[i] = score_string[length - 1 - i];
            score_string[length - 1 - i] = c;
        }
        temp_score = score;
    }
    time_c=timer;
    int r;
    for(i=0;i<2;i++){
	r=time_c%10;
	time_c/=10;
	time_string[1-i]= (char)(r + 48);
	}
    glm::vec3 fontColor3 = getRGBfromHue(200);

    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateScore_text = glm::translate(glm::vec3(2, 3.5, 0));
    glm::mat4 scaleScore_text = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateScore_text * scaleScore_text);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render("Score :");

    // Transform the text
    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateScore = glm::translate(glm::vec3(3.5, 3.5, 0));
    glm::mat4 scaleScore = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateScore * scaleScore);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render(score_string);

    // Transform the text
    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateLevel_text = glm::translate(glm::vec3(-0.75, 3.5, 0));
    glm::mat4 scaleLevel_text = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateLevel_text * scaleLevel_text);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render("Level :");

    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateLevel = glm::translate(glm::vec3(0.75, 3.5, 0));
    glm::mat4 scaleLevel = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateLevel * scaleLevel);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    level_string[0] = (char)(level + 48);
    GL3Font.font->Render(level_string);

    // Transform the text
    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateLives_text = glm::translate(glm::vec3(-3.75, 3.5, 0));
    glm::mat4 scaleLives_text = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateLives_text * scaleLives_text);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render("Lives :");

    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateLives = glm::translate(glm::vec3(-2.25, 3.5, 0));
    glm::mat4 scaleLives = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateLives * scaleLives);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    //lives_string[0]=(char)(lives+48);
    GL3Font.font->Render(lives_string);

    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTime_text = glm::translate(glm::vec3(2, -3.5, 0));
    glm::mat4 scaleTime_text = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateTime_text * scaleTime_text);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render("Timer :");

    glUseProgram(fontProgramID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTime = glm::translate(glm::vec3(3.25, -3.5, 0));
    glm::mat4 scaleTime = glm::scale(glm::vec3(0.5, 0.5, 1));
    Matrices.model *= (translateTime * scaleTime);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    //lives_string[0]=(char)(lives+48);
    GL3Font.font->Render(time_string,2);

    glUseProgram(programID);

    glUseProgram(programID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateLoadBar = glm::translate(glm::vec3(3.5, -1.96 + 15 * 0.15, 0)); // glTranslatef
    Matrices.model *= (translateLoadBar);
    glm::mat4 scaleLoadBar = glm::scale(glm::vec3(1.2, 1 + (15 * 1.5), 1));
    Matrices.model *= (scaleLoadBar);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(loading_bar);

    glUseProgram(programID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateHealthBar = glm::translate(glm::vec3(3.5, -1.96 + health * 0.15, 0)); // glTranslatef
    Matrices.model *= (translateHealthBar);
    glm::mat4 scaleHealthBar = glm::scale(glm::vec3(1, 1 + (health * 1.5), 1));
    Matrices.model *= (scaleHealthBar);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(health_bar);

    int cx = 0, cz = 0;
    for (i = 0; i < 100; i++) {
        Matrices.model = glm::mat4(1.0f);
        /* Render your scene */
        if (i == 97 && level==3) {
            if (cy >= 0.5) {
                b_m = 1;
            }
            if (cy <= -0.5) {
                b_m = 0;
            }
            if (b_m == 0) {
                cy += 0.01;
            }
            else if (b_m == 1) {
                cy -= 0.01;
            }
	}
        glm::mat4 translateCube;
        if (i == tile[0] || i == tile[1] || i == tile[2] || i == tile[3] || i == tile[4])
            translateCube = glm::translate(glm::vec3(-3 + 0.6f * cx, 0.f + cy, 0.6f * cz)); // glTranslatef
        else
            translateCube = glm::translate(glm::vec3(-3 + 0.6f * cx, 0.f, 0.6f * cz)); // glTranslatef
        glm::mat4 scaleCube = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
        //glm::mat4 HoverTransform = translateTriangle * rotateTriangle;
        Matrices.model *= (translateCube * scaleCube);
        GLfloat ex,ey,ez,tx,ty,tz,ux,uy,uz;
        ex =  0.5;
	ey = 2;
        ez = 7;
	tx=0;
	ty=0;
	tz=0;
	ux=0;
	uy=1;
	uz=0;
	if(tower_view){
		//fprintf(stderr,"In\n");
		ex = 0;
		ey = 10;
        	ez = 10;
		tx=0;
		ty=0;
		tz=0;
		ux=0;
		uy=1;
		uz=0;
	}
	else if(top_view){
		ex = 0;
		ey = 10;
        	ez = 0;
		tx=0;
		ty=0;
		tz=3;
		ux=0;
		uy=-1;
		uz=0;
	}
	else if(follow_view){
		ex =  0.6f * px -3;
		ey = 5;
        	ez = 7+(0.6*pz);
		tx=-3 + 0.6f * px;
		ty=0.5f + ry + cy;
		tz=0.6f * pz;
		ux=0;
		uy=1;
		uz=0;
	}
	else if(helicopter_view){
		ex = sin(camera_rotation_angle * M_PI / 180.0f) * 10;
		ey = 2;
        	ez = cos(camera_rotation_angle * M_PI / 180.0f) * 10;
		tx=0;
		ty=0;
		tz=0;
		ux=0;
		uy=1;
		uz=0;
	}
	else if(adventure_view){
		ex = -3 + 0.6f * px;
		ey = 0.5 + ry + cy+1;
        	ez = 0.6*pz-2;
		tx= -3 + 0.6f * px;
		ty= 0.5f + ry + cy;
		tz= 0.6f * pz-2;
		if(dir==1)
			tz= 0.6f * pz - 5;
		else if(dir==4)
			tz= 0.6f * pz + 5;
		else if(dir==2)
			tx= -3 + 0.6f * px - 5;
		else if(dir==3)
			tx= -3 + 0.6f * px + 5;
		ux=0;
		uy=1;
		uz=0;
		
	}
        Matrices.view = glm::lookAt(glm::vec3(ex, ey, ez), glm::vec3(tx, ty, tz), glm::vec3(ux, uy, uz)); // Fixed camera for 2D (ortho) in XY plane
        VP = Matrices.projection * Matrices.view;
        MVP = VP * Matrices.model; // MVP = p * V * M
        //  Don't change unless you are sure!!
        // Copy MVP to normal shaders
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        if (i == hole[0] || i == hole[1] || i == hole[2] || i == hole[3] || i == hole[4])
            ;
        else
            draw3DObject(cube[i]);
        cx++;
        if (cx > 9) {
            cx = 0;
            cz++;
        }
    }
    if (jump) {
        rx = (0.6 * ttime);
        ry = (0.4 * ttime) - (0.2 * ttime * ttime);
        ttime += 0.1;
        if (ttime > 2.1) {
            jump = false;
            if (dir == 1)
                pz -= 2;
            else if (dir == 4)
                pz += 2;
            else if (dir == 2)
                px -= 2;
            else if (dir == 3)
                px += 2;
            rx = 0;
            ttime = 0;
        }
    }
    if (px < 0 || px > 9 || pz > 9 || pz < 0 || (hole[0] == (pz * 10 + px)) || (hole[1] == (pz * 10 + px)) || (hole[2] == (pz * 10 + px)) || (hole[3] == (pz * 10 + px)) || (hole[4] == (pz * 10 + px)) || health <= 0) {
        lives--;
        px = 0;
        pz = 9;
        health = 15;
    }
	
    for (i = 0; i < 5; i++) {
        Matrices.model = glm::mat4(1.0f);
        /* Render your scene */
        glm::mat4 translateCoins = glm::translate(glm::vec3(-3 + 0.6f * coins_x[i], 0.5f, 0.6f * coins_z[i])); // glTranslatef
        glm::mat4 scaleCoins = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
        //glm::mat4 HoverTransform = translateTriangle * rotateTriangle;
        Matrices.model *= (translateCoins * scaleCoins);
        //Matrices.view = glm::lookAt(glm::vec3(0.5, 2, 7), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane
        VP = Matrices.projection * Matrices.view;
        MVP = VP * Matrices.model; // MVP = p * V * M
        //  Don't change unless you are sure!!
        // Copy MVP to normal shaders
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(coins[i]);
    }
    if(level==2 || level==3){
    for (i = 0; i < 5; i++) {
        Matrices.model = glm::mat4(1.0f);
        glm::mat4 translateFire = glm::translate(glm::vec3(-3 + 0.6 * fire_x[i], 0.3f, 0.6 * fire_z[i]));
        glm::mat4 scaleFire = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
        Matrices.model *= (translateFire * scaleFire);
        VP = Matrices.projection * Matrices.view;
        MVP = VP * Matrices.model;
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(fire[i]);
    }
	}

    if ((10 * pz + px == tile[0]) || (10 * pz + px == tile[1]) || (10 * pz + px == tile[2]) || (10 * pz + px == tile[3]) || (10 * pz + px == tile[4])) {
        if (cy + 0.5 > 0.5 + ry && !on_tile) {
            health -= 5;
            if (dir == 1)
                pz += 1;
            else if (dir == 4)
                pz -= 1;
            else if (dir == 2)
                px += 1;
            else if (dir == 3)
                px -= 1;
        }
        else if (cy + 0.5 <= 0.5 + ry){
	    if(!on_tile && cy+0.75<=0.5+ry)
		health-=5;
            on_tile = true;
	}
    }
    else
        on_tile = false;
    if (dir == 1) {
        if ((10 * (pz - 1) + px == tile[0]) || (10 * (pz - 1) + px == tile[1]) || (10 * (pz - 1) + px == tile[2]) || (10 * (pz - 1) + px == tile[3]) || (10 * (pz - 1) + px == tile[4])) {
            if (cy + 0.5 > 0.5 + ry && !on_tile && jump) {
                health -= 5;
                jump = false;
                ry = 0;
            }
        }
    }
    if (dir == 4) {
        if ((10 * (pz + 1) + px == tile[0]) || (10 * (pz + 1) + px == tile[1]) || (10 * (pz + 1) + px == tile[2]) || (10 * (pz + 1) + px == tile[3]) || (10 * (pz + 1) + px == tile[4])) {
            if (cy + 0.5 > 0.5 + ry && !on_tile && jump) {
                health -= 5;
                jump = false;
                ry = 0;
            }
        }
    }
    if (dir == 2) {
        if ((10 * pz + px - 1 == tile[0]) || (10 * pz + px - 1 == tile[1]) || (10 * pz + px - 1 == tile[2]) || (10 * pz + px - 1 == tile[3]) || (10 * pz + px - 1 == tile[4])) {
            if (cy + 0.5 > 0.5 + ry && !on_tile && jump) {
                health -= 5;
                jump = false;
                ry = 0;
            }
        }
    }
    if (dir == 3) {
        if ((10 * pz + px + 1 == tile[0]) || (10 * pz + px + 1 == tile[1]) || (10 * pz + px + 1 == tile[2]) || (10 * pz + px + 1 == tile[3]) || (10 * pz + px + 1 == tile[4])) {
            if (cy + 0.5 > 0.5 + ry && !on_tile && jump) {
                health -= 5;
                jump = false;
                ry = 0;
            }
        }
    }
    glUseProgram(programID);
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translatePlayer;
    /* Render your scene */
    if (dir == 1) {
        if (on_tile)
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px, 0.5f + ry + cy, 0.6f * pz - rx)); // glTranslatef
        else
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px, 0.5f + ry, 0.6f * pz - rx));
    }
    else if (dir == 4) {
        if (on_tile)
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px, 0.5f + ry + cy, 0.6f * pz + rx)); // glTranslatef
        else
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px, 0.5f + ry, 0.6f * pz + rx));
    }
    else if (dir == 2) {
        if (on_tile)
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px - rx, 0.5f + ry + cy, 0.6f * pz)); // glTranslatef
        else
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px - rx, 0.5f + ry, 0.6f * pz));
    }
    else if (dir == 3) {
        if (on_tile)
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px + rx, 0.5f + ry + cy, 0.6f * pz)); // glTranslatef
        else
            translatePlayer = glm::translate(glm::vec3(-3 + 0.6f * px + rx, 0.5f + ry, 0.6f * pz));
    }
    glm::mat4 scalePlayer = glm::scale(glm::vec3(0.2f, 0.2f, 0.2f));
    Matrices.model *= (translatePlayer * scalePlayer);
    //	 GLfloat x=2,z=30;
    //  	x = sin(glfwGetTime());
    //  	z = cos(glfwGetTime());
    MVP = VP * Matrices.model; // MVP = p * V * M
    //  Don't change unless you are sure!!
    // Copy MVP to normal shaders
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    draw3DObject(player);
}

void endscreen()
{
    // clear the color and depth in the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the loaded shader program
    // Don't change unless you know what you are doing
    glUseProgram(programID);

    static float c = 0;
    c++;
    //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(sinf(c*M_PI/180.0),3*cosf(c*M_PI/180.0),0)); // Fixed camera for 2D (ortho) in XY plane

    // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
    //  Don't change unless you are sure!!
    glm::mat4 VP = Matrices.projection * Matrices.view;

    // Send our transformation to the currently bound shader, in the "MVP" uniform
    // For each model you render, since the MVP will be different (at least the M part)
    //  Don't change unless you are sure!!
    glm::mat4 MVP; // MVP = Projection * View * Model

    // Render with texture shaders now
    glUseProgram(textureProgramID);

    // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
    // glPopMatrix ();
    Matrices.model = glm::mat4(1.0f);
    MVP = VP * Matrices.model;

    // Copy MVP to texture shaders
    glUniformMatrix4fv(Matrices.TexMatrixID, 1, GL_FALSE, &MVP[0][0]);

    // Set the texture sampler to access Texture0 memory
    glUniform1i(glGetUniformLocation(textureProgramID, "texSampler"), 0);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DTexturedObject(rectangle);

    if (hover_flag == 5)
        hover_y = 0;
    else if (hover_flag == 6) {
        hover_y = -1;
    }
    // Load identity to model matrix
    Matrices.model = glm::mat4(1.0f);

    /* Render your scene */
    glm::mat4 translateHover = glm::translate(glm::vec3(-0.1f, 0.15f + hover_y, 0.0f)); // glTranslatef
    glm::mat4 scaleHover = glm::scale(glm::vec3(4.f, 1.f, 0.f));
    //glm::mat4 HoverTransform = translateTriangle * rotateTriangle;
    Matrices.model *= (translateHover * scaleHover);
    MVP = VP * Matrices.model; // MVP = p * V * M

    //  Don't change unless you are sure!!
    // Copy MVP to normal shaders
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(hover);

    // Increment angles
    float increments = 1;

    // Render font on screen
    glm::vec3 fontColor = getRGBfromHue(0);

    // Use font Shaders for next part of code
    glUseProgram(fontProgramID);
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane

    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateTitle = glm::translate(glm::vec3(-1.5, 3, 0));
    Matrices.model *= translateTitle;
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

    // Render font
    GL3Font.font->Render("Your Score");

    glm::vec3 fontColor4 = getRGBfromHue(170);

    // Use font Shaders for next part of code
    glUseProgram(fontProgramID);
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane

    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateScore = glm::translate(glm::vec3(-0.3, 2, 0));
    glm::mat4 scaleScore = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateScore * scaleScore);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor4[0]);
    // Render font
    if (score_display_flag == 1)
        GL3Font.font->Render(score_string);

    glm::vec3 fontColor1 = getRGBfromHue(100);

    // Use font Shaders for next part of code
    glUseProgram(fontProgramID);
    Matrices.view = glm::lookAt(glm::vec3(0, 0, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)); // Fixed camera for 2D (ortho) in XY plane

    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateNewGame = glm::translate(glm::vec3(-1, 0, 0));
    glm::mat4 scaleNewGame = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateNewGame * scaleNewGame);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor1[0]);
    // Render font
    GL3Font.font->Render("Menu");

    glm::vec3 fontColor3 = getRGBfromHue(200);
    // Transform the text
    Matrices.model = glm::mat4(1.0f);
    glm::mat4 translateQuit = glm::translate(glm::vec3(-1, -1, 0));
    glm::mat4 scaleQuit = glm::scale(glm::vec3(0.75, 0.75, 1));
    Matrices.model *= (translateQuit * scaleQuit);
    MVP = Matrices.projection * Matrices.view * Matrices.model;
    // send font's MVP and font color to fond shaders
    glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniform3fv(GL3Font.fontColorID, 1, &fontColor3[0]);
    // Render font
    GL3Font.font->Render("Quit");
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW(int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "G R A V I T Y", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(1);

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
	 is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard); // general keyboard input
    glfwSetCharCallback(window, keyboardChar); // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton); // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL(GLFWwindow* window, int width, int height)
{
    // Load Textures
    // Enable Texture0 as current texture memory
    glActiveTexture(GL_TEXTURE0);
    // load an image file directly as a new OpenGL texture
    // GLuint texID = SOIL_load_OGL_texture ("beach.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_TEXTURE_REPEATS); // Buggy for OpenGL3
    GLuint textureID, textureID1;
    if (sc_flag == 0)
        textureID = createTexture("space1.jpg");
    else if (sc_flag == 1)
        textureID = createTexture("space2.jpg");
    else if (sc_flag == 3) {
        if (init_flag == 1)
            textureID = createTexture("loading.jpg");
        else
            textureID = createTexture("space3.jpg");
    }
    else if (sc_flag == 4)
        textureID = createTexture("space4.jpg");
    // check for an error during the load process
    if (textureID == 0)
        cout << "SOIL loading error: '" << SOIL_last_result() << "'" << endl;
    textureID1 = createTexture("lives.jpg");
    // Create and compile our GLSL program from the texture shaders
    textureProgramID = LoadShaders("TextureRender.vert", "TextureRender.frag");
    // Get a handle for our "MVP" uniform
    Matrices.TexMatrixID = glGetUniformLocation(textureProgramID, "MVP");

    /* Objects should be created before any other gl function and shaders */
    // Create the models
    createHover();
    createDot();
    createLoadBar();
    createHealthBar();
    createCube(100);
    createCoins(5);
    createFire(5);
    createPlayer(); // Generate the VAO, VBOs, vertices data & copy into the array buffer
    createRectangle(textureID);
    createLives(textureID1);

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders("Sample_GL3.vert", "Sample_GL3.frag");
    // Get a handle for our "MVP" uniform
    Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

    reshapeWindow(window, width, height);

    // Background color of the scene
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClearDepth(1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialise FTGL stuff
    const char* fontfile = "arial.ttf";
    GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

    if (GL3Font.font->Error()) {
        //		cout << "Error: Could not load font `" << fontfile << "'" << endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    // Create and compile our GLSL program from the font shaders
    fontProgramID = LoadShaders("fontrender.vert", "fontrender.frag");
    GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
    fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
    fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
    fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
    GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
    GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

    GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
    GL3Font.font->FaceSize(1);
    GL3Font.font->Depth(0);
    GL3Font.font->Outset(0, 0);
    GL3Font.font->CharMap(ft_encoding_unicode);

    //	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    //	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    //	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    //	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main(int argc, char** argv)
{
    int width = 800;
    int height = 600;

    GLFWwindow* window = initGLFW(width, height);

    //initGL (window, width, height);

    //PlaySound("starwars.mp3", NULL, SND_ASYNC|SND_FILENAME|SND_LOOP);

    double last_update_time = glfwGetTime(), current_time;
    double last_timer_update = glfwGetTime(), timer_update;
    double xpos, ypos;
    double xpos_o,ypos_o;
    glfwGetCursorPos(window, &xpos_o, &ypos_o);
    double widthc = width, heightc = height;
    int level_c = 0;
    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {
	if(!pause){
        glfwGetCursorPos(window, &xpos, &ypos);
        // OpenGL Draw commands
        if (sc_flag == 0) {
            if (xpos >= 215 * (widthc / 600) && xpos <= 365 * (widthc / 600) && ypos <= 305 * (heightc / 600) && ypos >= 270 * (heightc / 600))
                hover_flag = 0;
            else if (xpos >= 215 * (widthc / 600) && xpos <= 365 * (widthc / 600) && ypos <= 380 * (heightc / 600) && ypos >= 345 * (heightc / 600))
                hover_flag = 1;
            else if (xpos >= 215 * (widthc / 600) && xpos <= 365 * (widthc / 600) && ypos <= 455 * (heightc / 600) && ypos >= 420 * (heightc / 600))
                hover_flag = 2;
            if (init_flag == 0) {
                initGL(window, width, height);
                init_flag = 1;
            }
            startscreen();
        }
        else if (sc_flag == 1) {
            if (xpos >= 20 * (widthc / 600) && xpos <= 95 * (widthc / 600) && ypos <= 45 * (heightc / 600) && ypos >= 20 * (heightc / 600))
                hover_flag = 4;
            else
                hover_flag = 1;
            if (init_flag == 1) {
                initGL(window, width, height);
                init_flag = 0;
            }
            controlsscreen();
        }
        else if (sc_flag == 3) {
            if (loading_time >= 0 && loading_time <= 20) {
                init_flag = 1;
                loading_time += 0.1;
            }
            if (init_flag == 4) {
                gamescreen();
                if (lives == 0 || level==4 || timer==0) {
                    sc_flag = 4;
                    hover_flag = 5;
                    lives = 3;
		    health=15;
		    tower_view=false;
		    top_view=false;
		    adventure_view=false;
		    follow_view=false; 
		    cy=0;
                }
		if (ypos-ypos_o > 0)
			dir=1;
		if (ypos-ypos_o < 0)
			dir=4;
		if (xpos-xpos_o>0)
			dir=3;
		if (xpos-xpos_o<0)
			dir=2;
		xpos_o=xpos;
		ypos_o=ypos;
		if(10*pz+px==9 && coin_count==5)
		{
			level++;
			px=0;
			pz=9;
			coin_count=0;
			if(level==2)
				timer=30;
			else if(level==3)
				timer=45;
			else if(level==4){
				sc_flag = 4;
                    		hover_flag = 5;
                    		lives = 3;
			}
                }
                if (px == coins_x[0] && pz == coins_z[0]) {
                    score += 10;
                    coins_x[0] = 100;
                    coins_z[0] = 100;
		    coin_count++;
                }
                else if (px == coins_x[1] && pz == coins_z[1]) {
                    score += 10;
                    coins_x[1] = 100;
                    coins_z[1] = 100;
		    coin_count++;
                }
                else if (px == coins_x[2] && pz == coins_z[2]) {
                    score += 10;
                    coins_x[2] = 100;
                    coins_z[2] = 100;
                }
                else if (px == coins_x[3] && pz == coins_z[3]) {
                    score += 10;
                    coins_x[3] = 100;
                    coins_z[3] = 100;
		    coin_count++;
		    coin_count++;
                }
                else if (px == coins_x[4] && pz == coins_z[4]) {
                    score += 10;
                    coins_x[4] = 100;
                    coins_z[4] = 100;
		    coin_count++;
                }
                if (px == fire_x[0] && pz == fire_z[0]) {
                    health -= 0.1;
                }
                else if (px == fire_x[1] && pz == fire_z[1]) {
                    health -= 0.1;
                }
                else if (px == fire_x[2] && pz == fire_z[2]) {
                    health -= 0.1;
                }
                else if (px == fire_x[3] && pz == fire_z[3]) {
                    health -= 0.1;
                }
                else if (px == fire_x[4] && pz == fire_z[4]) {
                    health -= 0.1;
                }
            }
            else if (init_flag != 4){
                loading_effect();
		level_c=0;
		level=1;
		timer=15;
		health=15;
		cy=0;
	    }
            if (init_flag == 1 || init_flag == 3) {
                initGL(window, width, height);
                init_flag = 4;
                if (loading_time > 20)
                    initGL(window, width, height);
            }
            if (level_c != level) {
                int i;
                for (i = 0; i < 5; i++) {
                    coins_x[i] = rand() % 10;
                    coins_z[i] = rand() % 10;
                }
                level_c = level;
            }
        }
        else if (sc_flag == 4) {
            if (xpos >= 215 * (widthc / 600) && xpos <= 365 * (widthc / 600) && ypos <= 305 * (heightc / 600) && ypos >= 270 * (heightc / 600))
                hover_flag = 5;
            else if (xpos >= 215 * (widthc / 600) && xpos <= 365 * (widthc / 600) && ypos <= 380 * (heightc / 600) && ypos >= 345 * (heightc / 600))
                hover_flag = 6;
            if (xpos >= 265 * (widthc / 600) && xpos <= 315 * (widthc / 600) && ypos <= 150 * (heightc / 600) && ypos >= 130 * (heightc / 600))
                score_display_flag = 1;
            else
                score_display_flag = 0;
            if (init_flag == 4) {
                initGL(window, width, height);
                init_flag = 0;
            }
            endscreen();
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
        current_time = glfwGetTime();
	timer_update = glfwGetTime();
        if ((current_time - last_update_time) >= 5) {
            int i;
            for (i = 0; i < 5; i++) {
                hole[i] = rand() % 100;
                if (hole[i] == (pz * 10 + px) || hole[i] == 90)
                    hole[i] = 37;
		if(level==3){
                tile[i] = rand() % 100;
		}
		if(level==2 || level==3){
                fire_x[i] = rand() % 10;
                fire_z[i] = rand() % 10;
                if (fire_z[i] * 10 + fire_x[i] == tile[i])
                    tile[i] == 100;
                if (fire_x[i] == px && fire_z[i] == pz) {
                    fire_x[i] = 8;
                    fire_z[i] = 7;
		}
                }
            }
            last_update_time = current_time;
        }
	if((timer_update - last_timer_update) >=1){
		timer-=1;
		last_timer_update=timer_update;
	}
	}
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
