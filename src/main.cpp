// std c++
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>

// glew glut
#include <GL/glew.h>
#include <GL/freeglut.h>

// glm
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// SOIL
#include <SOIL2/SOIL2.h>

// assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// --------------------- end of include --------------------- //

class Mesh
{
public:
    GLuint vao, vbo, ebo;
    GLuint diffuseTexture;  

    std::vector<glm::vec3> vertexPosition;
    std::vector<glm::vec2> vertexTexcoord;
    std::vector<glm::vec3> vertexNormal;

    // index for glDrawElements function
    std::vector<int> index;

    Mesh() {}
    void bindData()
    {
        // Create Vertex Array Object
        glGenVertexArrays(1, &vao); // Assign one Vertex Array Object
        glBindVertexArray(vao);  	

        // Create and Initialize the Vertex Buffer Object
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
            vertexPosition.size() * sizeof(glm::vec3) +
            vertexTexcoord.size() * sizeof(glm::vec2) +
            vertexNormal.size() * sizeof(glm::vec3),
            NULL, GL_STATIC_DRAW);

        // pass Vertex Position
        GLuint offset_position = 0;
        GLuint size_position = vertexPosition.size() * sizeof(glm::vec3);
        glBufferSubData(GL_ARRAY_BUFFER, offset_position, size_position, vertexPosition.data());
        glEnableVertexAttribArray(0);   // input (layout = 0) in shaders represents Vertex Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_position));
        // pass Texture Coordinate
        GLuint offset_texcoord = size_position;
        GLuint size_texcoord = vertexTexcoord.size() * sizeof(glm::vec2);
        glBufferSubData(GL_ARRAY_BUFFER, offset_texcoord, size_texcoord, vertexTexcoord.data());
        glEnableVertexAttribArray(1);   // input (layout = 1) in shaders represents Texture Coordinate
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_texcoord));
        // pass Normal 
        GLuint offset_normal = size_position + size_texcoord;
        GLuint size_normal = vertexNormal.size() * sizeof(glm::vec3);
        glBufferSubData(GL_ARRAY_BUFFER, offset_normal, size_normal, vertexNormal.data());
        glEnableVertexAttribArray(2);   // input (layout = 2)in shaders represents Vertex Normal
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(offset_normal));

        // pass index to ebo
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(GLuint), index.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
    }
    void draw(GLuint program)
    {
        glBindVertexArray(vao);

        // pass texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        glUniform1i(glGetUniformLocation(program, "texture"), 0);

        // draw
        glDrawElements(GL_TRIANGLES, this->index.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

class Model
{
public:
    std::vector<Mesh> meshes;
    std::map<std::string, GLuint> textureMap;
    glm::vec3 translate = glm::vec3(0, 0, 0), rotate = glm::vec3(0, 0, 0), scale = glm::vec3(1, 1, 1);
    Model() {}
    void load(std::string filepath)
    {
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs);
        // check errors
        if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "MODEL READ ERROR: " << import.GetErrorString() << std::endl;
            exit(-1);
        }
        // model file path 
        std::string rootPath = filepath.substr(0, filepath.find_last_of('/'));

        // generate mesh in loops 
        for (int i = 0; i < scene->mNumMeshes; i++)
        {
            meshes.push_back(Mesh());
            Mesh& mesh = meshes.back();

            aiMesh* aimesh = scene->mMeshes[i];

            // pass data to mesh
            for (int j = 0; j < aimesh->mNumVertices; j++)
            {
                // position
                glm::vec3 vvv;
                vvv.x = aimesh->mVertices[j].x;
                vvv.y = aimesh->mVertices[j].y;
                vvv.z = aimesh->mVertices[j].z;
                mesh.vertexPosition.push_back(vvv);

                // normal
                vvv.x = aimesh->mNormals[j].x;
                vvv.y = aimesh->mNormals[j].y;
                vvv.z = aimesh->mNormals[j].z;
                mesh.vertexNormal.push_back(vvv);

                // texture coordinate 
                // assimp could have several texture coord but here 0 is chosen
                glm::vec2 vv(0, 0);
                if (aimesh->mTextureCoords[0])
                {
                    vv.x = aimesh->mTextureCoords[0][j].x;
                    vv.y = aimesh->mTextureCoords[0][j].y;
                }
                mesh.vertexTexcoord.push_back(vv);
            }

            // pass material
            if (aimesh->mMaterialIndex >= 0)
            {
                aiMaterial* material = scene->mMaterials[aimesh->mMaterialIndex];

                // get diffuse texture file path 
                aiString aistr;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &aistr);// 0: only take one diffuse texture
                std::string texpath = aistr.C_Str();
                texpath = rootPath + '/' + texpath;   

                // if not generate texture yet then generate texture
                if (textureMap.find(texpath) == textureMap.end())
                {
                    GLuint tex;
                    glGenTextures(1, &tex);
                    glBindTexture(GL_TEXTURE_2D, tex);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                    int textureWidth, textureHeight;
                    unsigned char* image = SOIL_load_image(texpath.c_str(), &textureWidth, &textureHeight, 0, SOIL_LOAD_RGB);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);   
                    delete[] image;

                    textureMap[texpath] = tex;
                }

                // pass texture
                mesh.diffuseTexture = textureMap[texpath];
            }

            // pass face index
            for (GLuint j = 0; j < aimesh->mNumFaces; j++)
            {
                aiFace face = aimesh->mFaces[j];
                for (GLuint k = 0; k < face.mNumIndices; k++)
                {
                    mesh.index.push_back(face.mIndices[k]);
                }
            }

            mesh.bindData();
        }
    }
    void draw(GLuint program)
    {
        glm::mat4 unit(    
            glm::vec4(1, 0, 0, 0),
            glm::vec4(0, 1, 0, 0),
            glm::vec4(0, 0, 1, 0),
            glm::vec4(0, 0, 0, 1)
        );
        glm::mat4 scale = glm::scale(unit, this->scale);
        glm::mat4 translate = glm::translate(unit, this->translate);

        glm::mat4 rotate = unit;    
        rotate = glm::rotate(rotate, glm::radians(this->rotate.x), glm::vec3(1, 0, 0));
        rotate = glm::rotate(rotate, glm::radians(this->rotate.y), glm::vec3(0, 1, 0));
        rotate = glm::rotate(rotate, glm::radians(this->rotate.z), glm::vec3(0, 0, 1));

        // model matrix
        glm::mat4 model = translate * rotate * scale;
        GLuint mlocation = glGetUniformLocation(program, "model");    
        glUniformMatrix4fv(mlocation, 1, GL_FALSE, glm::value_ptr(model));   

        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i].draw(program);
        }
    }
};

class Camera
{
public:
    // 相机参数
    glm::vec3 position = glm::vec3(0, 0, 0);    // camera position
    glm::vec3 direction = glm::vec3(0, 0, -1);  // view direction
    glm::vec3 up = glm::vec3(0, 1, 0);          // up vector
    float pitch = 0.0f, roll = 0.0f, yaw = 0.0f;    // eular angle
    float fovy = 70.0f, aspect = 1.0, zNear = 0.01, zFar = 100; // perspective projection parameter
    float left = -1.0, right = 1.0, top = 1.0, bottom = -1.0; // ortho projection parameter
    Camera() {}
    
    glm::mat4 getViewMatrix(bool useEulerAngle = true)
    {
        if (useEulerAngle)  
        {
            direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
            direction.y = sin(glm::radians(pitch));
            direction.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw)); // look along -z axis 
        }
        return glm::lookAt(position, position + direction, up);
    }
    
    glm::mat4 getProjectionMatrix(bool usePerspective = true)
    {
        if (usePerspective) 
        {
            return glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
        }
        return glm::ortho(left, right, bottom, top, zNear, zFar);
    }
};

// ---------------------------- end of class definition ---------------------------- //

// model
std::vector<Model> models;  // scene
Model screen;   // render a quad as screen 
Model skybox;   // render a cube as skybox

// shader progrma object
GLuint program;
GLuint debugProgram;   
GLuint shadowProgram;   
GLuint skyboxProgram;   

// texture
GLuint skyboxTexture;   
GLuint shadowTexture;   

// camera
Camera camera;          
Camera shadowCamera;    // render from light source

// light source and shadow parameter
int shadowMapResolution = 1024;     
GLuint shadowMapFBO;                        // FBO of rendering from light source

int windowWidth = 512;  
int windowHeight = 512;
bool keyboardState[1024];   // true means the specific key is pressed

int FrameCounter = 0;

// Deferred Rendering
GLuint gbufferProgram;
GLuint gbufferFBO;  
GLuint gcolor;      // base color texture
GLuint gdepth;      // depth texture
GLuint gworldpos;   // world position texture
GLuint gnormal;     // normal texture
GLuint noisetex;    // nosie texture

// post processing
GLuint composite0;

// --------------- end of global variable definition --------------- //

std::string readShaderFile(std::string filepath)
{
    std::string res, line;
    std::ifstream fin(filepath);
    if (!fin.is_open())
    {
        std::cout << "SHADER FILE " << filepath << " FAIL TO OPEN" << std::endl;
        exit(-1);
    }
    while (std::getline(fin, line))
    {
        res += line + '\n';
    }
    fin.close();
    return res;
}

GLuint getShaderProgram(std::string fshader, std::string vshader)
{
    // read shader source file
    std::string vSource = readShaderFile(vshader);
    std::string fSource = readShaderFile(fshader);
    const char* vpointer = vSource.c_str();
    const char* fpointer = fSource.c_str();

    GLint success;
    GLchar infoLog[512];

    // create and compile vertex shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)(&vpointer), NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);   // check errors
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "VERTEX SHADER " + vshader + " COMPILED ERROR\n" << infoLog << std::endl;
        exit(-1);
    }

    // create and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar**)(&fpointer), NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);   // check errors
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "FRAGMENT " + fshader + " COMPILED ERROR\n" << infoLog << std::endl;
        exit(-1);
    }

    // link two shaders to program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // delete after linking
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void mouseWheel(int wheel, int direction, int x, int y)
{
    // zFar += 1 * direction * 0.1;
    glutPostRedisplay();    // redraw
}

// mouse movement
void mouse(int x, int y)
{
    camera.yaw += 35 * (x - float(windowWidth) / 2.0) / windowWidth;
    camera.yaw = glm::mod(camera.yaw + 180.0f, 360.0f) - 180.0f;    

    camera.pitch += -35 * (y - float(windowHeight) / 2.0) / windowHeight;
    camera.pitch = glm::clamp(camera.pitch, -89.0f, 89.0f);

    glutWarpPointer(windowWidth / 2.0, windowHeight / 2.0);
    glutPostRedisplay();    // redraw
}

void keyboardDown(unsigned char key, int x, int y)
{
    keyboardState[key] = true;
}
void keyboardDownSpecial(int key, int x, int y)
{
    keyboardState[key] = true;
}
void keyboardUp(unsigned char key, int x, int y)
{
    keyboardState[key] = false;
}
void keyboardUpSpecial(int key, int x, int y)
{
    keyboardState[key] = false;
}

void move()
{
    float cameraSpeed = 0.035f;

    // camera control
    if (keyboardState['w']) camera.position += cameraSpeed * camera.direction;
    if (keyboardState['s']) camera.position -= cameraSpeed * camera.direction;
    if (keyboardState['a']) camera.position -= cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
    if (keyboardState['d']) camera.position += cameraSpeed * glm::normalize(glm::cross(camera.direction, camera.up));
    if (keyboardState[GLUT_KEY_CTRL_L]) camera.position.y -= cameraSpeed;
    if (keyboardState[' ']) camera.position.y += cameraSpeed;

    // light position control
    if (keyboardState['j']) shadowCamera.position.x += cameraSpeed;
    if (keyboardState['l']) shadowCamera.position.x -= cameraSpeed;
    if (keyboardState['i']) shadowCamera.position.y += cameraSpeed;
    if (keyboardState['k']) shadowCamera.position.y -= cameraSpeed;

    glutPostRedisplay();    // redraw
}

GLuint loadCubemap(std::vector<const GLchar*> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glActiveTexture(GL_TEXTURE0);

    int width, height;
    unsigned char* image;

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    for (GLuint i = 0; i < faces.size(); i++)
    {
        image = SOIL_load_image(faces[i], &width, &height, 0, SOIL_LOAD_RGB);
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
            GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    return textureID;
}

void init()
{
    // create shader program
    gbufferProgram = getShaderProgram("shaders/gbuffer.fs", "shaders/gbuffer.vs");
    shadowProgram = getShaderProgram("shaders/shadow.fs", "shaders/shadow.vs");
    debugProgram = getShaderProgram("shaders/debug.fs", "shaders/debug.vs");
    skyboxProgram = getShaderProgram("shaders/skybox.fs", "shaders/skybox.vs");
    composite0 = getShaderProgram("shaders/composite0.fs", "shaders/composite0.vs");

    // ------------------------------------------------------------------------ // 

    // read obj model
    Model tree1 = Model();
    tree1.translate = glm::vec3(2.5, 0, 2);
    tree1.scale = glm::vec3(0.0025, 0.0025, 0.0025);
    tree1.load("models/tree/tree02.obj");
    models.push_back(tree1);

    Model tree2 = Model();
    tree2.translate = glm::vec3(10, 0, 7);
    tree2.scale = glm::vec3(0.0015, 0.0015, 0.0015);
    tree2.load("models/tree/tree02.obj");
    models.push_back(tree2);

    Model plane = Model();
    plane.translate = glm::vec3(0, -1.1, 0);
    plane.scale = glm::vec3(10, 10, 10);
    plane.rotate = glm::vec3(0, 0, 0);
    plane.load("models/plane/plane.obj");
    models.push_back(plane);

    // light source sign model
    Model vlight = Model();
    vlight.translate = glm::vec3(1, 0, -1);
    vlight.rotate = glm::vec3(0, 180, 0);
    vlight.scale = glm::vec3(0.008, 0.008, 0.008);
    vlight.load("models/lamp/lampara_escritorio.obj");
    models.push_back(vlight);

    // ------------------------------------------------------------------------ // 

    // generate a square as screen to display texture data
    Mesh msquare;
    msquare.vertexPosition = { glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), glm::vec3(-1, 1, 0), glm::vec3(1, 1, 0) };
    msquare.vertexTexcoord = { glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(0, 1), glm::vec2(1, 1) };
    msquare.index = { 0,1,2,2,1,3 };
    msquare.bindData();
    screen.meshes.push_back(msquare);

    // ------------------------------------------------------------------------ //

    // generate a cube as skybox
    Mesh cube;
    cube.vertexPosition = { //8 vertices of the cube
        glm::vec3(-1, -1, -1),glm::vec3(1, -1, -1),glm::vec3(-1, 1, -1),glm::vec3(1, 1, -1),
        glm::vec3(-1, -1, 1),glm::vec3(1, -1, 1),glm::vec3(-1, 1, 1),glm::vec3(1, 1, 1)
    };
    cube.index = { 0,3,1,0,2,3,1,5,4,1,4,0,4,2,0,4,6,2,5,6,4,5,7,6,2,6,7,2,7,3,1,7,5,1,3,7 };
    cube.bindData();
    skybox.meshes.push_back(cube);

    // load cube texture
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/Box_Right.bmp");
    faces.push_back("skybox/Box_Left.bmp");
    faces.push_back("skybox/Box_Top.bmp");
    faces.push_back("skybox/Box_Bottom.bmp");
    faces.push_back("skybox/Box_Front.bmp");
    faces.push_back("skybox/Box_Back.bmp");

    skyboxTexture = loadCubemap(faces);

    // ------------------------------------------------------------------------ // 

    // Orthogonal projection parameter configuration 
    shadowCamera.left = -30;
    shadowCamera.right = 30;
    shadowCamera.bottom = -30;
    shadowCamera.top = 30;
    shadowCamera.position = glm::vec3(0, 4, 15);

    // create shadow frame buffer object
    glGenFramebuffers(1, &shadowMapFBO);
    // create shadow texture
    glGenTextures(1, &shadowTexture);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapResolution, shadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // bind shadow texture to shadow FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ------------------------------------------------------------------------ // 

    // create gubffer frame buffer object
    glGenFramebuffers(1, &gbufferFBO); 
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);

    // create color texture
    glGenTextures(1, &gcolor);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // bind color texture to color attachment 0 
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gcolor, 0);

    // create normal texture
    glGenTextures(1, &gnormal);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // bind normal texture to color attachment 1 
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gnormal, 0);

    // create world position texture
    glGenTextures(1, &gworldpos);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, windowWidth, windowHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // bind world position to color attachment 2 
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gworldpos, 0);

    // create depth texture
    glGenTextures(1, &gdepth);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, windowWidth, windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // bind depth texture to depth attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gdepth, 0);

    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);	

    // ------------------------------------------------------------------------ // 

    glEnable(GL_DEPTH_TEST);  // enable depth test
    glClearColor(1.0, 1.0, 1.0, 1.0);   // background color
}

void display()
{
    move(); // control camera position

    // the last object will be the light source 
    models.back().translate = shadowCamera.position + glm::vec3(0, 0, 2);

    // ------------------------------------------------------------------------ // 

    // render from the light source direction
    glUseProgram(shadowProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, shadowMapResolution, shadowMapResolution);

    // world position of the origin from the light source 
    shadowCamera.direction = glm::normalize(glm::vec3(0, 0, 0) - shadowCamera.position);
    // pass view matrix 
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "view"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getViewMatrix(false)));
    // pass projection matrix 
    glUniformMatrix4fv(glGetUniformLocation(shadowProgram, "projection"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getProjectionMatrix(false)));

    // draw from light source
    for (auto m : models)
    {
        m.draw(shadowProgram);
    }

    // ------------------------------------------------------------------------ // 

    // skybox drawing
    // pass to three textures of gbuffer
    glUseProgram(skyboxProgram);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, windowWidth, windowHeight);

    // pass view and projection matrix
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));

    // pass cube map texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 1);

    // pass zfar and znear to make coord of skybox at the furthest 
    glUniform1f(glGetUniformLocation(skyboxProgram, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(skyboxProgram, "far"), camera.zFar);

    // the skybox always follow the camera
    skybox.translate = camera.position;

    glDepthMask(GL_FALSE);
    skybox.draw(skyboxProgram);
    glDepthMask(GL_TRUE);

    // ------------------------------------------------------------------------ // 

    // drawing 
    glUseProgram(gbufferProgram);

    // pass view matrix
    glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
    // pass projection matrix
    glUniformMatrix4fv(glGetUniformLocation(gbufferProgram, "projection"), 1, GL_FALSE, glm::value_ptr(camera.getProjectionMatrix()));

    for (auto m : models)
    {
        m.draw(gbufferProgram);
    }

    // ------------------------------------------------------------------------ // 

    // post processing：render with composite0 shader
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(composite0);
    glViewport(0, 0, windowWidth, windowHeight);

    // pass zfar and znear to transfer to linear depth 
    glUniform1f(glGetUniformLocation(composite0, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(composite0, "far"), camera.zFar);

    // pass gcolor texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glUniform1i(glGetUniformLocation(composite0, "gcolor"), 1);
    // pass gnormal texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glUniform1i(glGetUniformLocation(composite0, "gnormal"), 2);
    // pass gworldpos texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glUniform1i(glGetUniformLocation(composite0, "gworldpos"), 3);
    // pass gdepth texture
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glUniform1i(glGetUniformLocation(composite0, "gdepth"), 4);
    // pass shadow depth texture
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glUniform1i(glGetUniformLocation(composite0, "shadowtex"), 5);
    // pass nosie texture
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, noisetex);
    glUniform1i(glGetUniformLocation(composite0, "noisetex"), 6);

    // pass transformation matrix of the light source coord
    glm::mat4 shadowVP = shadowCamera.getProjectionMatrix(false) * shadowCamera.getViewMatrix(false);
    glUniformMatrix4fv(glGetUniformLocation(composite0, "shadowVP"), 1, GL_FALSE, glm::value_ptr(shadowVP));

    // pass light position
    glUniform3fv(glGetUniformLocation(composite0, "lightPos"), 1, glm::value_ptr(shadowCamera.position));
    // pass camera position
    glUniform3fv(glGetUniformLocation(composite0, "cameraPos"), 1, glm::value_ptr(camera.position));

    // pass view matrix
    glUniformMatrix4fv(glGetUniformLocation(composite0, "view"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getViewMatrix(false)));
    // pass projection matrix
    glUniformMatrix4fv(glGetUniformLocation(composite0, "projection"), 1, GL_FALSE, glm::value_ptr(shadowCamera.getProjectionMatrix(false)));

    FrameCounter++;
    if (FrameCounter == INT_MAX)
    {
        FrameCounter = 0;
    }
    glUniform1i(glGetUniformLocation(composite0, "FrameCounter"), FrameCounter);

    screen.draw(composite0);
    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------------------------------ // 

    /*
    // debug shahder output a square to display texture data
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);   // disable depth test tot cover the original screen
    glUseProgram(debugProgram);
    glViewport(0, 0, windowWidth, windowHeight);

    // pass zfar and znear to transform to linera depth
    glUniform1f(glGetUniformLocation(debugProgram, "near"), camera.zNear);
    glUniform1f(glGetUniformLocation(debugProgram, "far"), camera.zFar);

    // pass gcolor texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gcolor);
    glUniform1i(glGetUniformLocation(debugProgram, "gcolor"), 1);
    // pass gnormal texture
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gnormal);
    glUniform1i(glGetUniformLocation(debugProgram, "gnormal"), 2);
    // pass gworldpos texture
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gworldpos);
    glUniform1i(glGetUniformLocation(debugProgram, "gworldpos"), 3);
    // pass gdepth texture
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, gdepth);
    glUniform1i(glGetUniformLocation(debugProgram, "gdepth"), 4);
    // pass shadow depth texture
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glUniform1i(glGetUniformLocation(debugProgram, "shadowtex"), 5);

    screen.draw(debugProgram);
    glEnable(GL_DEPTH_TEST);
    */

    // ------------------------------------------------------------------------ // 

    glutSwapBuffers();               
}

// -------------------------------- main -------------------------------- //

int main(int argc, char** argv)
{
    glutInit(&argc, argv);              
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("rendering window"); 

#ifdef __APPLE__
#else
    glewInit();
#endif

    init();

    // mouse moving function 
    //glutMotionFunc(mouse);  
    glutPassiveMotionFunc(mouse);   // direct moving of mouse 
    //glutMouseWheelFunc(mouseWheel); // zoom in & out with mouse wheel

    // keyboard fucntion
    glutKeyboardFunc(keyboardDown);
    glutSpecialFunc(keyboardDownSpecial);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialUpFunc(keyboardUpSpecial);

    glutDisplayFunc(display);           // display call back function and implement every frame 
    glutMainLoop();                    

    return 0;
}
