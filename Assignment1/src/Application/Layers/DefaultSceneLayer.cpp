#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/EnemyComponent.h"
#include "Gameplay/Components/CharacterMovement.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		// This time we'll have 2 different shaders, and share data between both of them using the UBO
		// This shader will handle reflective materials 
		ShaderProgram::Sptr reflectiveShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_environment_reflective.glsl" }
		});
		reflectiveShader->SetDebugName("Reflective");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr basicShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_blinn_phong_textured.glsl" }
		});
		basicShader->SetDebugName("Blinn-phong");

		// This shader handles our basic materials without reflections (cause they expensive)
		ShaderProgram::Sptr specShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/textured_specular.glsl" }
		});
		specShader->SetDebugName("Textured-Specular");

		// This shader handles our foliage vertex shader example
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/screendoor_transparency.glsl" }
		});
		foliageShader->SetDebugName("Foliage");

		// This shader handles our cel shading example
		ShaderProgram::Sptr toonShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/toon_shading.glsl" }
		});
		toonShader->SetDebugName("Toon Shader");

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping"); 

		// This shader handles our tangent space normal mapping
		ShaderProgram::Sptr tangentSpaceMapping = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_tangentspace_normal_maps.glsl" }
		});
		tangentSpaceMapping->SetDebugName("Tangent Space Mapping");

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing");
		    
		//Milena's Homemade shaders
		ShaderProgram::Sptr Ambient = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/Ambient.glsl" }
		});    
		Ambient->SetDebugName("Milena's Ambient");

		ShaderProgram::Sptr Specular = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/Specular.glsl" }
		});
		Specular->SetDebugName("Milena's Specular");


		// Load in the meshes

		MeshResource::Sptr groundMesh = ResourceManager::CreateAsset<MeshResource>("ground.obj");
		MeshResource::Sptr plantMesh = ResourceManager::CreateAsset<MeshResource>("Plant.obj");
		MeshResource::Sptr plant2Mesh = ResourceManager::CreateAsset<MeshResource>("plant2.obj");
		MeshResource::Sptr plant3Mesh = ResourceManager::CreateAsset<MeshResource>("Plant3.obj");
		MeshResource::Sptr logMesh = ResourceManager::CreateAsset<MeshResource>("Log.obj");
		// Load in some textures
		Texture2D::Sptr    groundTex    = ResourceManager::CreateAsset<Texture2D>("textures/grassTex.png");
		Texture2D::Sptr    leafTex = ResourceManager::CreateAsset<Texture2D>("textures/leaf1.png");
		Texture2D::Sptr    leaf2Tex = ResourceManager::CreateAsset<Texture2D>("textures/leaf2.png");
		Texture2D::Sptr    logTex = ResourceManager::CreateAsset<Texture2D>("textures/logUV.png");



		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" }
		});

		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>(); 

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lutCool = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");  
		Texture3D::Sptr lutWarm = ResourceManager::CreateAsset<Texture3D>("luts/warm.CUBE");
		Texture3D::Sptr lutCustom = ResourceManager::CreateAsset<Texture3D>("luts/custom.CUBE");

		// Configure the color correction LUT
		scene->SetColorLUT(lutCool);

		// Create our materials
		// This will be our box material, with no environment reflections
		
		Material::Sptr groundMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			groundMaterial->Name = "Ground"; 
			groundMaterial->Set("u_Material.Diffuse", groundTex);
			groundMaterial->Set("u_Material.Shininess", 0.1f);
		}	
		Material::Sptr leafMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			leafMaterial->Name = "leaf1";
			leafMaterial->Set("u_Material.Diffuse", leafTex);
			leafMaterial->Set("u_Material.Shininess", 0.5f);
		}
		Material::Sptr leaf2Material = ResourceManager::CreateAsset<Material>(basicShader);
		{
			leaf2Material->Name = "leaf2";
			leaf2Material->Set("u_Material.Diffuse", leaf2Tex);
			leaf2Material->Set("u_Material.Shininess", 0.5f);
		} 
		Material::Sptr logMaterial = ResourceManager::CreateAsset<Material>(basicShader);
		{
			logMaterial->Name = "log";
			logMaterial->Set("u_Material.Diffuse", logTex);
			logMaterial->Set("u_Material.Shininess", 0.5f);
		}
		// Create some lights for our scene
		scene->Lights.resize(3);
		scene->Lights[0].Position = glm::vec3(-4.84f, -4.0f, 7.2f);
		scene->Lights[0].Color = glm::vec3(1.0f, 1.0f, 1.0f);
		scene->Lights[0].Range = 1000.0f;
	
		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ -4.84, -4.24, 2.24 });
			camera->SetRotation({ 68, 0.0, -53.3 });
		}

		// Set up all our sample objects
		GameObject::Sptr log1 = scene->CreateGameObject("log");
		{
			// Set position in the scene 
			log1->SetPostion(glm::vec3(-2.75f, 2.27f, 0.73f));
			log1->SetRotation(glm::vec3(90.0f, 0.0f, 90.0f));
			log1->SetScale(glm::vec3(2));
			RenderComponent::Sptr renderer = log1->Add<RenderComponent>();
			renderer->SetMesh(logMesh);
			renderer->SetMaterial(logMaterial);
			log1->Add<EnemyComponent>();
		}
		GameObject::Sptr log2 = scene->CreateGameObject("log2");
		{
			// Set position in the scene
			log2->SetPostion(glm::vec3(-1.66f, -3.827, 0.64f));
			log2->SetRotation(glm::vec3(90.0f, 0.0f, -47.0f));
			log2->SetScale(glm::vec3(0.75));
			RenderComponent::Sptr renderer = log2->Add<RenderComponent>();
			renderer->SetMesh(logMesh);
			renderer->SetMaterial(logMaterial);
		}
		GameObject::Sptr plant2 = scene->CreateGameObject("Plant 2");
		{
			// Set position in the scene
			plant2->SetPostion(glm::vec3(-4.62f, -2.27f, 1.0f));
			plant2->SetRotation(glm::vec3(90.0f, 0.0f, 0.0f));
			plant2->SetScale(glm::vec3(3.0f));
			RenderComponent::Sptr renderer = plant2->Add<RenderComponent>();
			renderer->SetMesh(plant3Mesh);
			renderer->SetMaterial(leaf2Material);
			plant2->Add<RotatingBehaviour>();
		}
		GameObject::Sptr plant3 = scene->CreateGameObject("Plant 3");
		{
			plant3->SetPostion(glm::vec3(-2.21f, -4.3f, 0.15f));
			plant3->SetRotation(glm::vec3(90.0f, 0.0f, 150.0f));
			plant3->SetScale(glm::vec3(3.0f));
			RenderComponent::Sptr renderer = plant3->Add<RenderComponent>();
			renderer->SetMesh(plant3Mesh);
			renderer->SetMaterial(leafMaterial);
		}
		GameObject::Sptr plant4 = scene->CreateGameObject("Plant 4");
		{
			plant4->SetPostion(glm::vec3(0.65f, -10.27f, 1.63f));
			plant4->SetRotation(glm::vec3(90.0f, 0.0f, 30.0f));
			plant4->SetScale(glm::vec3(6.5));
			RenderComponent::Sptr renderer = plant4->Add<RenderComponent>();
			renderer->SetMesh(plant2Mesh);
			renderer->SetMaterial(leaf2Material);
		}
		GameObject::Sptr plant5 = scene->CreateGameObject("Plant 5");
		{
			plant5->SetPostion(glm::vec3(-1.65f, -0.64f, 0.69f));
			plant5->SetRotation(glm::vec3(90.0f, 0.0f, -24.0f));
			plant5->SetScale(glm::vec3(2));
			RenderComponent::Sptr renderer = plant5->Add<RenderComponent>();
			renderer->SetMesh(plantMesh);
			renderer->SetMaterial(leafMaterial);
		}
		GameObject::Sptr plant6 = scene->CreateGameObject("Plant 6");
		{
			plant6->SetPostion(glm::vec3(-0.5f, -2.24f, 0.75f));
			plant6->SetRotation(glm::vec3(90.0f, 0.0f, -20.0f));
			plant6->SetScale(glm::vec3(2));
			RenderComponent::Sptr renderer = plant6->Add<RenderComponent>();
			renderer->SetMesh(plantMesh);
			renderer->SetMaterial(leaf2Material);
			plant6->Add<RotatingBehaviour>();
		}
		
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();
			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(groundMaterial);
			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}
