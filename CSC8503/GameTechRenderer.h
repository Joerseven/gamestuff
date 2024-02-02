#pragma once
#include "OGLRenderer.h"
#include "OGLShader.h"
#include "OGLTexture.h"
#include "OGLMesh.h"

#include "GameWorld.h"

namespace NCL {
	class Maths::Vector3;
	class Maths::Vector4;
	namespace CSC8503 {
		class RenderObject;

        struct Character {
            unsigned int TextureID;  // ID handle of the glyph texture
            Vector2i   Size;       // Size of glyph
            Vector2i  Bearing;    // Offset from baseline to left/top of glyph
            unsigned int Advance;    // Offset to advance to next glyph
        };

        struct RenderStrings {
            std::string text;
            float x;
            float y;
            float scale;
            Vector3 color;
        };

		class GameTechRenderer : public OGLRenderer	{
		public:
			GameTechRenderer(GameWorld& world);
			~GameTechRenderer();

			Mesh*		LoadMesh(const std::string& name);
            Mesh*       LoadOBJMesh(const std::string& name, Vector3 localOffset = {0,0,0});
			Texture*	LoadTexture(const std::string& name);
			Shader*		LoadShader(const std::string& vertex, const std::string& fragment);

            void AddRenderText(std::string s, float x, float y, float scale, Vector3 color);


		protected:
			void NewRenderLines();
			void NewRenderText();

			void RenderFrame()	override;

			OGLShader*		defaultShader;

			GameWorld&	gameWorld;

			void BuildObjectList();
			void SortObjectList();
			void RenderShadowMap();
			void RenderCamera(); 
			void RenderSkybox();
            void RenderText(std::string text, float x, float y, float scale, Vector3 color);

            void LoadFont();

            void RenderDebugVolumes();

			void LoadSkybox();

			void SetDebugStringBufferSizes(size_t newVertCount);
			void SetDebugLineBufferSizes(size_t newVertCount);

			vector<const RenderObject*> activeObjects;

			OGLShader*  debugShader;
			OGLShader*  skyboxShader;
			OGLMesh*	skyboxMesh;
			GLuint		skyboxTex;
            OGLShader*  textShader;

            OGLShader* sceneShader;

            Mesh* cubeMesh;
            Mesh* sphereMesh;

            std::vector<RenderStrings> stringsToRender;

			//shadow mapping things
			OGLShader*	shadowShader;
			GLuint		shadowTex;
			GLuint		shadowFBO;
			Matrix4     shadowMatrix;

			Vector4		lightColour;
			float		lightRadius;
			Vector3		lightPosition;

            std::map<char, Character> Characters;

			//Debug data storage things
			vector<Vector3> debugLineData;

			vector<Vector3> debugTextPos;
			vector<Vector4> debugTextColours;
			vector<Vector2> debugTextUVs;

			GLuint lineVAO;
			GLuint lineVertVBO;
			size_t lineCount;

			GLuint textVAO;
			GLuint textVertVBO;
			GLuint textColourVBO;
			GLuint textTexVBO;
			size_t textCount;

            GLuint betterTextVAO;
            GLuint betterTextVBO;
		};
	}
}

