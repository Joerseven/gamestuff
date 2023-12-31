#include "GameTechRenderer.h"
#include "GameObject.h"
#include "RenderObject.h"
#include "Camera.h"
#include "TextureLoader.h"
#include "MshLoader.h"
#include "Assets.h"
using namespace NCL;
using namespace Rendering;
using namespace CSC8503;

#define SHADOWSIZE 4096

#define MODELPATH "../../Assets/Meshes/"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader.h"

#include <ft2build.h>
#include FT_FREETYPE_H

Matrix4 biasMatrix = Matrix4::Translation(Vector3(0.5f, 0.5f, 0.5f)) * Matrix4::Scale(Vector3(0.5f, 0.5f, 0.5f));

GameTechRenderer::GameTechRenderer(GameWorld& world) : OGLRenderer(*Window::GetWindow()), gameWorld(world)	{
	glEnable(GL_DEPTH_TEST);

	debugShader  = new OGLShader("debug.vert", "debug.frag");
	shadowShader = new OGLShader("shadow.vert", "shadow.frag");
    sceneShader = new OGLShader("scene.vert", "scene.frag");
    textShader = new OGLShader("text.vert", "text.frag");

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			     SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1, 1, 1, 1);

	//Set up the light properties
	lightColour = Vector4(0.8f, 0.8f, 0.5f, 1.0f);
	lightRadius = 1000.0f;
	lightPosition = Vector3(-200.0f, 60.0f, -200.0f);

    cubeMesh = LoadMesh("Cube.msh");
    sphereMesh = LoadMesh("Sphere.msh");

	//Skybox!
	skyboxShader = new OGLShader("skybox.vert", "skybox.frag");
	skyboxMesh = new OGLMesh();
	skyboxMesh->SetVertexPositions({Vector3(-1, 1,-1), Vector3(-1,-1,-1) , Vector3(1,-1,-1) , Vector3(1,1,-1) });
	skyboxMesh->SetVertexIndices({ 0,1,2,2,3,0 });
	skyboxMesh->UploadToGPU();

	LoadSkybox();

	glGenVertexArrays(1, &lineVAO);
	glGenVertexArrays(1, &textVAO);

	glGenBuffers(1, &lineVertVBO);
	glGenBuffers(1, &textVertVBO);
	glGenBuffers(1, &textColourVBO);
	glGenBuffers(1, &textTexVBO);

    LoadFont();

	Debug::CreateDebugFont("PressStart2P.fnt", *LoadTexture("PressStart2P.png"));

	SetDebugStringBufferSizes(10000);
	SetDebugLineBufferSizes(1000);
}

GameTechRenderer::~GameTechRenderer()	{
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
}

void GameTechRenderer::LoadSkybox() {
	std::string filenames[6] = {
		"/Cubemap/skyrender0004.png",
		"/Cubemap/skyrender0001.png",
		"/Cubemap/skyrender0003.png",
		"/Cubemap/skyrender0006.png",
		"/Cubemap/skyrender0002.png",
		"/Cubemap/skyrender0005.png"
	};

	int width[6]	= { 0 };
	int height[6]	= { 0 };
	int channels[6] = { 0 };
	int flags[6]	= { 0 };

	vector<char*> texData(6, nullptr);

	for (int i = 0; i < 6; ++i) {
		TextureLoader::LoadTexture(filenames[i], texData[i], width[i], height[i], channels[i], flags[i]);
		if (i > 0 && (width[i] != width[0] || height[0] != height[0])) {
			std::cout << __FUNCTION__ << " cubemap input textures don't match in size?\n";
			return;
		}
	}
	glGenTextures(1, &skyboxTex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	GLenum type = channels[0] == 4 ? GL_RGBA : GL_RGB;

	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width[i], height[i], 0, type, GL_UNSIGNED_BYTE, texData[i]);
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void GameTechRenderer::RenderFrame() {
	glEnable(GL_CULL_FACE);
	glClearColor(1, 1, 1, 1);
	BuildObjectList();
	SortObjectList();
	RenderShadowMap();
	RenderSkybox();
	RenderCamera();
	glDisable(GL_CULL_FACE); //Todo - text indices are going the wrong way...
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	NewRenderLines();
	NewRenderText();
    for (const auto& s : stringsToRender) {
        RenderText(s.text, s.x, s.y, s.scale, s.color);
    }
    stringsToRender.clear();
    RenderDebugVolumes();
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GameTechRenderer::BuildObjectList() {
	activeObjects.clear();

	gameWorld.OperateOnContents(
		[&](GameObject* o) {
			if (o->IsActive()) {
				const RenderObject* g = o->GetRenderObject();
				if (g) {
					activeObjects.emplace_back(g);
				}
			}
		}
	);
}

void GameTechRenderer::SortObjectList() {

}

void GameTechRenderer::RenderShadowMap() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);

	glCullFace(GL_FRONT);

	BindShader(*shadowShader);
	int mvpLocation = glGetUniformLocation(shadowShader->GetProgramID(), "mvpMatrix");

	Matrix4 shadowViewMatrix = Matrix4::BuildViewMatrix(lightPosition, Vector3(0, 0, 0), Vector3(0,1,0));
	Matrix4 shadowProjMatrix = Matrix4::Perspective(100.0f, 500.0f, 1, 45.0f);

	Matrix4 mvMatrix = shadowProjMatrix * shadowViewMatrix;

	shadowMatrix = biasMatrix * mvMatrix; //we'll use this one later on

	for (const auto&i : activeObjects) {
		Matrix4 modelMatrix = (*i).GetTransform()->GetMatrix();
		Matrix4 mvpMatrix	= mvMatrix * modelMatrix;
		glUniformMatrix4fv(mvpLocation, 1, false, (float*)&mvpMatrix);
		BindMesh((OGLMesh&)*(*i).GetMesh());
		size_t layerCount = (*i).GetMesh()->GetSubMeshCount();
		for (size_t i = 0; i < layerCount; ++i) {
			DrawBoundMesh((uint32_t)i);
		}
	}

	glViewport(0, 0, windowSize.x, windowSize.y);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glCullFace(GL_BACK);
}

void GameTechRenderer::RenderSkybox() {
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	Matrix4 viewMatrix = gameWorld.GetMainCamera().BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera().BuildProjectionMatrix(hostWindow.GetScreenAspect());

	BindShader(*skyboxShader);

	int projLocation = glGetUniformLocation(skyboxShader->GetProgramID(), "projMatrix");
	int viewLocation = glGetUniformLocation(skyboxShader->GetProgramID(), "viewMatrix");
	int texLocation  = glGetUniformLocation(skyboxShader->GetProgramID(), "cubeTex");

	glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
	glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

	glUniform1i(texLocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);

	BindMesh(*skyboxMesh);
	DrawBoundMesh();

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void GameTechRenderer::RenderCamera() {
	Matrix4 viewMatrix = gameWorld.GetMainCamera().BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera().BuildProjectionMatrix(hostWindow.GetScreenAspect());

	OGLShader* activeShader = nullptr;
	int projLocation	= 0;
	int viewLocation	= 0;
	int modelLocation	= 0;
	int colourLocation  = 0;
	int hasVColLocation = 0;
	int hasTexLocation  = 0;
	int shadowLocation  = 0;

	int lightPosLocation	= 0;
	int lightColourLocation = 0;
	int lightRadiusLocation = 0;

	int cameraLocation = 0;

	//TODO - PUT IN FUNCTION
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	for (const auto&i : activeObjects) {
		OGLShader* shader = (OGLShader*)(*i).GetShader();
		BindShader(*shader);

		if ((*i).GetDefaultTexture()) {
			BindTextureToShader(*(OGLTexture*)(*i).GetDefaultTexture(), "mainTex", 0);
		}

		if (activeShader != shader) {
			projLocation	= glGetUniformLocation(shader->GetProgramID(), "projMatrix");
			viewLocation	= glGetUniformLocation(shader->GetProgramID(), "viewMatrix");
			modelLocation	= glGetUniformLocation(shader->GetProgramID(), "modelMatrix");
			shadowLocation  = glGetUniformLocation(shader->GetProgramID(), "shadowMatrix");
			colourLocation  = glGetUniformLocation(shader->GetProgramID(), "objectColour");
			hasVColLocation = glGetUniformLocation(shader->GetProgramID(), "hasVertexColours");
			hasTexLocation  = glGetUniformLocation(shader->GetProgramID(), "hasTexture");

			lightPosLocation	= glGetUniformLocation(shader->GetProgramID(), "lightPos");
			lightColourLocation = glGetUniformLocation(shader->GetProgramID(), "lightColour");
			lightRadiusLocation = glGetUniformLocation(shader->GetProgramID(), "lightRadius");

			cameraLocation = glGetUniformLocation(shader->GetProgramID(), "cameraPos");

			Vector3 camPos = gameWorld.GetMainCamera().GetPosition();
			glUniform3fv(cameraLocation, 1, &camPos.x);

			glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
			glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

			glUniform3fv(lightPosLocation	, 1, (float*)&lightPosition);
			glUniform4fv(lightColourLocation, 1, (float*)&lightColour);
			glUniform1f(lightRadiusLocation , lightRadius);

			int shadowTexLocation = glGetUniformLocation(shader->GetProgramID(), "shadowTex");
			glUniform1i(shadowTexLocation, 1);

			activeShader = shader;
		}

		Matrix4 modelMatrix = (*i).GetTransform()->GetMatrix();
		glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);			
		
		Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
		glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

		Vector4 colour = i->GetColour();
		glUniform4fv(colourLocation, 1, &colour.x);

		glUniform1i(hasVColLocation, !(*i).GetMesh()->GetColourData().empty());

		glUniform1i(hasTexLocation, (OGLTexture*)(*i).GetDefaultTexture() ? 1:0);

		BindMesh((OGLMesh&)*(*i).GetMesh());
		size_t layerCount = (*i).GetMesh()->GetSubMeshCount();
		for (size_t i = 0; i < layerCount; ++i) {
			DrawBoundMesh((uint32_t)i);
		}
	}

    BindShader(*sceneShader);

    projLocation	= glGetUniformLocation(sceneShader->GetProgramID(), "projMatrix");
    viewLocation	= glGetUniformLocation(sceneShader->GetProgramID(), "viewMatrix");
    modelLocation	= glGetUniformLocation(sceneShader->GetProgramID(), "modelMatrix");
    shadowLocation  = glGetUniformLocation(sceneShader->GetProgramID(), "shadowMatrix");
    colourLocation  = glGetUniformLocation(sceneShader->GetProgramID(), "objectColour");
    hasVColLocation = glGetUniformLocation(sceneShader->GetProgramID(), "hasVertexColours");
    hasTexLocation  = glGetUniformLocation(sceneShader->GetProgramID(), "hasTexture");

    lightPosLocation	= glGetUniformLocation(sceneShader->GetProgramID(), "lightPos");
    lightColourLocation = glGetUniformLocation(sceneShader->GetProgramID(), "lightColour");
    lightRadiusLocation = glGetUniformLocation(sceneShader->GetProgramID(), "lightRadius");

    cameraLocation = glGetUniformLocation(sceneShader->GetProgramID(), "cameraPos");

    glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
    glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

    glUniform3fv(lightPosLocation	, 1, (float*)&lightPosition);
    glUniform4fv(lightColourLocation, 1, (float*)&lightColour);
    glUniform1f(lightRadiusLocation , lightRadius);

    int shadowTexLocation = glGetUniformLocation(sceneShader->GetProgramID(), "shadowTex");
    glUniform1i(shadowTexLocation, 1);

    activeShader = sceneShader;

    for (const auto& b : Debug::GetDebugVolumes()) {
        auto prevScale = b.location->GetScale();
        Vector3 newScale;
        if (b.volume->type == VolumeType::OBB) {
            newScale = ((OBBVolume*)b.volume)->GetHalfDimensions() * 2;
        }
        else if (b.volume->type == VolumeType::AABB) {
            newScale = ((AABBVolume*)b.volume)->GetHalfDimensions() * 2;
        } else {
            auto sf = ((SphereVolume*)b.volume)->GetRadius();
            newScale = Vector3(sf, sf, sf);
        }
        auto modelMatrix = b.location->SetScale(newScale).GetMatrix();
        b.location->SetScale(prevScale);
        glUniformMatrix4fv(modelLocation, 1, false, (float*)&modelMatrix);

        Matrix4 fullShadowMat = shadowMatrix * modelMatrix;
        glUniformMatrix4fv(shadowLocation, 1, false, (float*)&fullShadowMat);

        Vector4 colour = b.color;
        glUniform4fv(colourLocation, 1, &colour.x);

        glUniform1i(hasTexLocation, 0);

        size_t layerCount;

        if (b.volume->type == VolumeType::OBB) {
            BindMesh((OGLMesh&)*cubeMesh);
            layerCount = cubeMesh->GetSubMeshCount();
        }
        else if (b.volume->type == VolumeType::AABB) {
            BindMesh((OGLMesh&)*cubeMesh);
            layerCount = cubeMesh->GetSubMeshCount();
        } else {
            BindMesh((OGLMesh&)*sphereMesh);
            layerCount = sphereMesh->GetSubMeshCount();
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        for (size_t i = 0; i < layerCount; ++i) {
            DrawBoundMesh((uint32_t)i);
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

Mesh* GameTechRenderer::LoadMesh(const std::string& name) {
	OGLMesh* mesh = new OGLMesh();
	MshLoader::LoadMesh(name, *mesh);
	mesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	mesh->UploadToGPU();
	return mesh;
}

Mesh *GameTechRenderer::LoadOBJMesh(const std::string &name, Vector3 localOffset) {

    auto mesh = new OGLMesh();
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, std::string(Assets::MESHDIR + name).c_str(), Assets::MESHDIR.c_str());

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        std::cerr << "Unable to load mesh file: " << name << std::endl;
    }

    std::vector<tinyobj::index_t> indices;
    std::vector<int> material_ids;

    for (auto i = 0; i < shapes.size(); i++) {
        indices.insert(indices.end(), shapes[i].mesh.indices.begin(), shapes[i].mesh.indices.end());
        material_ids.insert(material_ids.end(), shapes[i].mesh.material_ids.begin(), shapes[i].mesh.material_ids.end());
    }

    auto vertices = std::vector<Vector3>();
    auto normals = std::vector<Vector3>();
    auto colors = std::vector<Vector4>();
    auto textureCoords = std::vector<Vector2>();
    auto indi = std::vector<GLuint>();

    auto previousIndex = 0;
    auto indexCount = 0;
    auto vertCount = 0;

    for (size_t index = 0; index < material_ids.size(); index++) {
        Vector3 original[] = {
                Vector3(attrib.vertices[indices[3 * index].vertex_index * 3],
                        attrib.vertices[indices[3 * index].vertex_index * 3 + 1],
                        attrib.vertices[indices[3 * index].vertex_index * 3 + 2] * -1) + localOffset,
                Vector3(attrib.vertices[indices[3 * index + 2].vertex_index * 3],
                        attrib.vertices[indices[3 * index + 2].vertex_index * 3 + 1],
                        attrib.vertices[indices[3 * index + 2].vertex_index * 3 + 2] * -1) + localOffset,
                Vector3(attrib.vertices[indices[3 * index + 1].vertex_index * 3],
                        attrib.vertices[indices[3 * index + 1].vertex_index * 3 + 1],
                        attrib.vertices[indices[3 * index + 1].vertex_index * 3 + 2] * -1) + localOffset

        };

        vertices.push_back(original[0]);
        textureCoords.emplace_back(
                attrib.texcoords[indices[3 * index].texcoord_index * 2],
                attrib.texcoords[indices[3 * index].texcoord_index * 2 + 1]
        );
        colors.emplace_back(materials[material_ids[index]].diffuse[0],materials[material_ids[index]].diffuse[1],materials[material_ids[index]].diffuse[2], 1.0);
        indi.push_back(vertices.size() - 1);

        vertices.push_back(original[1]);
        textureCoords.emplace_back(
                attrib.texcoords[indices[3 * index + 1].texcoord_index * 2],
                attrib.texcoords[indices[3 * index + 1].texcoord_index * 2 + 1]
        );
        colors.emplace_back(materials[material_ids[index]].diffuse[0],materials[material_ids[index]].diffuse[1],materials[material_ids[index]].diffuse[2], 1.0);
        indi.push_back(vertices.size() - 1);

        vertices.push_back(original[2]);
        textureCoords.emplace_back(
                attrib.texcoords[indices[3 * index + 2].texcoord_index * 2],
                attrib.texcoords[indices[3 * index + 2].texcoord_index * 2 + 1]
        );
        colors.emplace_back(materials[material_ids[index]].diffuse[0],materials[material_ids[index]].diffuse[1],materials[material_ids[index]].diffuse[2], 1.0);
        indi.push_back(vertices.size() - 1);
    }

    mesh->SetVertexPositions(vertices);
    mesh->SetVertexTextureCoords(textureCoords);
    mesh->SetVertexIndices(indi);
    mesh->SetVertexColours(colors);

    mesh->SetPrimitiveType(GeometryPrimitive::Triangles);

    mesh->AddSubMesh(0, indi.size(), 0, "main");

    mesh->RecalculateNormals();
    mesh->UploadToGPU();

    return mesh;
}

void GameTechRenderer::NewRenderLines() {
	const std::vector<Debug::DebugLineEntry>& lines = Debug::GetDebugLines();
	if (lines.empty()) {
		return;
	}

	Matrix4 viewMatrix = gameWorld.GetMainCamera().BuildViewMatrix();
	Matrix4 projMatrix = gameWorld.GetMainCamera().BuildProjectionMatrix(hostWindow.GetScreenAspect());
	
	Matrix4 viewProj  = projMatrix * viewMatrix;

	BindShader(*debugShader);
	int matSlot = glGetUniformLocation(debugShader->GetProgramID(), "viewProjMatrix");
	GLuint texSlot = glGetUniformLocation(debugShader->GetProgramID(), "useTexture");
	glUniform1i(texSlot, 0);

	glUniformMatrix4fv(matSlot, 1, false, (float*)viewProj.array);

	debugLineData.clear();

	size_t frameLineCount = lines.size() * 2;

	SetDebugLineBufferSizes(frameLineCount);

	glBindBuffer(GL_ARRAY_BUFFER, lineVertVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, lines.size() * sizeof(Debug::DebugLineEntry), lines.data());
	

	glBindVertexArray(lineVAO);
	glDrawArrays(GL_LINES, 0, (GLsizei)frameLineCount);
	glBindVertexArray(0);
}

void GameTechRenderer::RenderDebugVolumes() {

}

void GameTechRenderer::NewRenderText() {
	const std::vector<Debug::DebugStringEntry>& strings = Debug::GetDebugStrings();
	if (strings.empty()) {
		return;
	}

	BindShader(*debugShader);

	OGLTexture* t = (OGLTexture*)Debug::GetDebugFont()->GetTexture();

	if (t) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, t->GetObjectID());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);	
		BindTextureToShader(*t, "mainTex", 0);
	}
	Matrix4 proj = Matrix4::Orthographic(0.0, 100.0f, 100, 0, -1.0f, 1.0f);

	int matSlot = glGetUniformLocation(debugShader->GetProgramID(), "viewProjMatrix");
	glUniformMatrix4fv(matSlot, 1, false, (float*)proj.array);

	GLuint texSlot = glGetUniformLocation(debugShader->GetProgramID(), "useTexture");
	glUniform1i(texSlot, 1);

	debugTextPos.clear();
	debugTextColours.clear();
	debugTextUVs.clear();

	int frameVertCount = 0;
	for (const auto& s : strings) {
		frameVertCount += Debug::GetDebugFont()->GetVertexCountForString(s.data);
	}
	SetDebugStringBufferSizes(frameVertCount);

	for (const auto& s : strings) {
		float size = 20.0f;
		Debug::GetDebugFont()->BuildVerticesForString(s.data, s.position, s.colour, size, debugTextPos, debugTextUVs, debugTextColours);
	}

	glBindBuffer(GL_ARRAY_BUFFER, textVertVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector3), debugTextPos.data());
	glBindBuffer(GL_ARRAY_BUFFER, textColourVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector4), debugTextColours.data());
	glBindBuffer(GL_ARRAY_BUFFER, textTexVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, frameVertCount * sizeof(Vector2), debugTextUVs.data());

	glBindVertexArray(textVAO);
	glDrawArrays(GL_TRIANGLES, 0, frameVertCount);
	glBindVertexArray(0);
}
 
Texture* GameTechRenderer::LoadTexture(const std::string& name) {
	return OGLTexture::TextureFromFile(name).release();
}

Shader* GameTechRenderer::LoadShader(const std::string& vertex, const std::string& fragment) {
	return new OGLShader(vertex, fragment);
}

void GameTechRenderer::SetDebugStringBufferSizes(size_t newVertCount) {
	if (newVertCount > textCount) {
		textCount = newVertCount;

		glBindBuffer(GL_ARRAY_BUFFER, textVertVBO);
		glBufferData(GL_ARRAY_BUFFER, textCount * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, textColourVBO);
		glBufferData(GL_ARRAY_BUFFER, textCount * sizeof(Vector4), nullptr, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, textTexVBO);
		glBufferData(GL_ARRAY_BUFFER, textCount * sizeof(Vector2), nullptr, GL_DYNAMIC_DRAW);

		debugTextPos.reserve(textCount);
		debugTextColours.reserve(textCount);
		debugTextUVs.reserve(textCount);

		glBindVertexArray(textVAO);

		glVertexAttribFormat(0, 3, GL_FLOAT, false, 0);
		glVertexAttribBinding(0, 0);
		glBindVertexBuffer(0, textVertVBO, 0, sizeof(Vector3));

		glVertexAttribFormat(1, 4, GL_FLOAT, false, 0);
		glVertexAttribBinding(1, 1);
		glBindVertexBuffer(1, textColourVBO, 0, sizeof(Vector4));

		glVertexAttribFormat(2, 2, GL_FLOAT, false, 0);
		glVertexAttribBinding(2, 2);
		glBindVertexBuffer(2, textTexVBO, 0, sizeof(Vector2));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}
}

void GameTechRenderer::SetDebugLineBufferSizes(size_t newVertCount) {
	if (newVertCount > lineCount) {
		lineCount = newVertCount;

		glBindBuffer(GL_ARRAY_BUFFER, lineVertVBO);
		glBufferData(GL_ARRAY_BUFFER, lineCount * sizeof(Debug::DebugLineEntry), nullptr, GL_DYNAMIC_DRAW);

		debugLineData.reserve(lineCount);

		glBindVertexArray(lineVAO);

		int realStride = sizeof(Debug::DebugLineEntry) / 2;

		glVertexAttribFormat(0, 3, GL_FLOAT, false, offsetof(Debug::DebugLineEntry, start));
		glVertexAttribBinding(0, 0);
		glBindVertexBuffer(0, lineVertVBO, 0, realStride);

		glVertexAttribFormat(1, 4, GL_FLOAT, false, offsetof(Debug::DebugLineEntry, colourA));
		glVertexAttribBinding(1, 0);
		glBindVertexBuffer(1, lineVertVBO, sizeof(Vector4), realStride);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
	}
}

void GameTechRenderer::LoadFont() {

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    std::string filePath = std::string(Assets::FONTSSDIR) + std::string("CascadiaMono.ttf");

    FT_Face face;
    if (FT_New_Face(ft, filePath.c_str(), 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);


    if (FT_Load_Char(face, 'X', FT_LOAD_RENDER))
    {
        std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
        return;
    }



    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // generate texture
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use
        Character character = {
                texture,
                Vector2i(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                Vector2i(face->glyph->bitmap_left, face->glyph->bitmap_top),
                (unsigned int)face->glyph->advance.x
        };
        Characters.insert(std::pair<char, Character>(c, character));

        glGenVertexArrays(1, &betterTextVAO);
        glGenBuffers(1, &betterTextVBO);
        glBindVertexArray(betterTextVAO);
        glBindBuffer(GL_ARRAY_BUFFER, betterTextVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

}

void GameTechRenderer::RenderText(std::string text, float x, float y, float scale, Vector3 color) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Matrix4 proj = Matrix4::Orthographic(0.0, 1920, 0, 1080, -1.0f, 1.0f);
    BindShader(*textShader);
    glUniform3f(glGetUniformLocation(textShader->GetProgramID(), "textColor"), color.x, color.y, color.z);
    glUniformMatrix4fv(glGetUniformLocation(textShader->GetProgramID(), "projection"), 1, false, (float*)proj.array);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(betterTextVAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = Characters[*c];

        float xpos = x + (float)ch.Bearing.x * scale;
        float ypos = y - (float)(ch.Size.y - ch.Bearing.y) * scale;

        float w = (float)ch.Size.x * scale;
        float h = (float)ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, betterTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (float)(ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GameTechRenderer::AddRenderText(std::string s, float x, float y, float scale, Vector3 color) {
    RenderStrings r = { s, x, y, scale, color };
    stringsToRender.push_back(r);
}


