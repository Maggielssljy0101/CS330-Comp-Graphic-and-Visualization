///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <functional>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;
	bReturn = CreateGLTexture(
		"textures/Lid.png",
		"Lid");

	bReturn = CreateGLTexture(
		"textures/Stone.png",
		"Stone");

	bReturn = CreateGLTexture(
		"textures/Pasta.png",
		"pasta");

	bReturn = CreateGLTexture(
		"textures/glass.png",
		"glass");

	bReturn = CreateGLTexture(
		"textures/jar.png",
		"jar");

	bReturn = CreateGLTexture(
		"textures/beanContainer.png",
		"beancontainer");


	bReturn = CreateGLTexture(
		"textures/beanContainer1.png",
		"beancontainer1");

	bReturn = CreateGLTexture(
		"textures/painting.png",
		"painting");

	bReturn = CreateGLTexture(
		"textures/table.png",
		"table");

	bReturn = CreateGLTexture(
		"textures/wall.png",
		"wall");

	bReturn = CreateGLTexture(
		"textures/plastic.png",
		"plastic");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// Plastic Material
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(0.8f, 0.4f, 0.8f);
	plasticMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plasticMaterial.shininess = 1.0;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);

	// Wood Material
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.2f, 0.2f);
	woodMaterial.shininess = 1.0;
	woodMaterial.tag = "wood";

	// Metal Material
	m_objectMaterials.push_back(woodMaterial);
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.8f);
	metalMaterial.shininess = 8.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// Glass Material
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.8f);
	glassMaterial.shininess = 10.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// Tile Material
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	tileMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	tileMaterial.shininess = 6.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	// Stone Material
	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	stoneMaterial.specularColor = glm::vec3(0.73f, 0.3f, 0.3f);
	stoneMaterial.shininess = 6.0;
	stoneMaterial.tag = "stone";
	m_objectMaterials.push_back(stoneMaterial);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Warm light for direction
	float warmLightX = 1.0f;
	float warmLightY = 0.994f;
	float warmLightZ = 0.75f;

	// Cool light for ambient
	float coolLightX = 0.6f;
	float coolLightY = 0.77f;
	float coolLightZ = 0.9f;

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// A warm directional light
	m_pShaderManager->setVec3Value("pointLights[0].position", -20.5f, 10.0f, -10.0f);
	m_pShaderManager->setVec3Value("pointLights[0].direction", 20.5f, -10.0f, 10.0f);
	m_pShaderManager->setBoolValue("pointLights[0].bUseDirection", true);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", warmLightX * 0.51f, warmLightY * 0.51f, warmLightZ * 0.51f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", warmLightX * 0.56f, warmLightY * 0.56f, warmLightZ * 0.56f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", warmLightX * 0.54f, warmLightY * 0.54f, warmLightZ * 0.54f);
	m_pShaderManager->setFloatValue("pointLights[0].focalStrength", 102.0f);
	m_pShaderManager->setFloatValue("pointLights[0].specularIntensity", 2.1f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// A cool ambient light
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 4.0f, 4.0f);
	m_pShaderManager->setBoolValue("pointLights[1].bUseDirection", false);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", coolLightX * 0.5f, coolLightY * 0.5f, coolLightZ * 0.5f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", coolLightX * 0.2f, coolLightY * 0.2f, coolLightZ * 0.2f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", coolLightX * 0.0f, coolLightY * 0.0f, coolLightZ * 0.0f);
	m_pShaderManager->setFloatValue("pointLights[1].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("pointLights[1].specularIntensity", 0.0f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	LoadSceneTextures();
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{

	// Do we have meshes?
	if (!m_basicMeshes) {
		std::cerr << "ERROR: m_basicMeshes is null!\n";
		return;
	}

	// Set up a function template to handle all our drawing opperations.
	auto drawOne = [&](const DrawCmd& c) {
		SetTransformations(c.scale, c.rotationDeg.x, c.rotationDeg.y, c.rotationDeg.z, c.translation);
		SetShaderMaterial(c.material);
		SetShaderTexture(c.texture);

		switch (c.type) {
		case MeshType::Plane:    m_basicMeshes->DrawPlaneMesh();    break;
		case MeshType::Sphere:   m_basicMeshes->DrawSphereMesh();   break;
		case MeshType::Cylinder: m_basicMeshes->DrawCylinderMesh(); break;
		case MeshType::Box:      m_basicMeshes->DrawBoxMesh();      break;
		}
		};

	// Scene objects as a tidy list of commands
	const std::vector<DrawCmd> cmds = {
		// Ground plane
		{ MeshType::Plane,   {20.0f, 1.0f, 10.0f}, {90.0f, 0.0f,   0.0f}, { 0.0f, 9.0f,  -10.0f}, "stone",   "wall" },

		// Table plane
		{ MeshType::Plane,   {20.0f, 1.0f, 10.0f}, { 0.0f, 0.0f,   0.0f}, { 0.0f, 0.0f,    0.0f}, "wood",    "table" },

		// Stone sphere
		{ MeshType::Sphere,  { 0.3f, 0.3f, 0.3f},  { 0.0f, 0.0f,   0.0f}, {-6.0f, 0.3f,   -3.0f}, "stone",   "Stone" },

		// Cylinders (bean container)
		{ MeshType::Cylinder,{ 1.0f, 2.5f, 1.0f},  { 0.0f, 0.0f,   0.0f}, {-3.0f, 0.3f,    0.0f}, "glass", "glass" },
		{ MeshType::Cylinder,{ 3.0f, 1.0f, 3.0f},  { 0.0f, 0.0f,   0.0f}, { 1.0f, 0.2f,    0.98f},"plastic", "beancontainer1" },
		{ MeshType::Cylinder,{ 3.0f, 3.0f, 3.0f},  { 0.0f, 2.0f,   0.0f}, { 1.0f, 1.2f,    0.98f},"plastic", "beancontainer" },
		{ MeshType::Cylinder,{ 2.8f, 0.5f, 2.8f},  { 0.0f, 0.0f,   0.0f}, { 1.0f, 4.2f,    0.98f},"plastic", "plastic" },

		// Glass jar + lid
		{ MeshType::Cylinder,{ 1.5f, 3.5f, 1.5f},  { 0.0f, 0.0f,   0.0f}, { 6.0f, 0.1f,    0.0f}, "glass",   "jar" },
		{ MeshType::Cylinder,{ 1.0f, 0.3f, 1.0f},  { 0.0f, 0.0f,   0.0f}, { 6.0f, 3.5f,    0.0f}, "plastic", "Lid" },

		// Painting (thin box)
		{ MeshType::Box,     { 3.5f, 0.01f, 5.5f}, {90.0f,180.0f,  0.0f}, { 6.0f, 8.5f,  -10.0f}, "plastic", "painting" },

		// Pasta boxes
		{ MeshType::Box,     { 1.5f, 0.4f, 3.0f},  { 0.0f, 80.0f,  0.0f}, { 3.0f, 0.3f,   5.5f}, "plastic", "pasta" },
		{ MeshType::Box,     { 1.5f, 0.4f, 3.0f},  { 0.0f, 65.0f,  0.0f}, { 3.0f, 0.6f,   5.5f}, "plastic", "pasta" },
		{ MeshType::Box,     { 1.5f, 0.4f, 3.0f},  { 0.0f, 65.0f,  0.0f}, { 3.0f, 1.0f,   5.5f}, "plastic", "pasta" },
	};

	// Draw all our objects
	for (const auto& c : cmds) {
		drawOne(c);
	}
}

