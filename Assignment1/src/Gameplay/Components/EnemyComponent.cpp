#include "Gameplay/Components/EnemyComponent.h"
#include "Gameplay/GameObject.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/JsonGlmHelpers.h"
void EnemyComponent::Awake() {
	counter = 0;
	direction = 0;
	RotationSpeed = glm::vec3(90, 0, 0);
}

void EnemyComponent::Update(float deltaTime) {
	if (counter == 150) {
		if (direction == 0) {
			direction = 1;
			counter = 0;
		}
		else if (direction == 1) {
			direction = 0;
			counter = 0;
		}
	}
	if (direction == 0) {
		GetGameObject()->SetPostion(glm::vec3(GetGameObject()->GetPosition().x + 0.015, 0, 0.7));
		GetGameObject()->SetRotation(GetGameObject()->GetRotationEuler() + RotationSpeed * deltaTime);
		counter++;
	}
	if (direction == 1) {
		GetGameObject()->SetPostion(glm::vec3(GetGameObject()->GetPosition().x - 0.015, 0, 0.7));
		GetGameObject()->SetRotation(GetGameObject()->GetRotationEuler() - RotationSpeed * deltaTime);
		counter++;
	}
}
void EnemyComponent::RenderImGui() {
	LABEL_LEFT(ImGui::DragFloat3, "Counter", &counter);
}

nlohmann::json EnemyComponent::ToJson() const {
	return {
		{ "counter", counter }
	};
}

EnemyComponent::Sptr EnemyComponent::FromJson(const nlohmann::json& data) {
	EnemyComponent::Sptr result = std::make_shared<EnemyComponent>();
	
	return result;
}
