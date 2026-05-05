#pragma once

#include <Engine/Editor/NodeEditor/NodeGraph.h>

#include <string>
#include <vector>

namespace CalyxEngine {
	enum class ShaderGraphPinRole : int32_t {
		Input,
		Output,
	};

	struct ShaderGraphPinSchema {
		std::string name;
		NodeValueType valueType = NodeValueType::None;
		ShaderGraphPinRole role = ShaderGraphPinRole::Input;
	};

	struct ShaderGraphNodeSchema {
		std::string type;
		std::string title;
		std::vector<ShaderGraphPinSchema> pins;
	};

	struct ShaderGraphSchema {
		std::string name;
		std::vector<ShaderGraphNodeSchema> nodes;
	};

	class ShaderGraphSchemas {
	public:
		static ShaderGraphSchema Object3DMaterial() {
			ShaderGraphSchema schema;
			schema.name = "Object3DMaterial";
			schema.nodes = {
				{"Output",
				 "Output",
				 {{"Surface", NodeValueType::Material, ShaderGraphPinRole::Input}}},
				{"ToonMaster",
				 "Toon Master",
				 {{"Base Color", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Highlight", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"1st Shade", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"2nd Shade", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Base Step", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Base Feather", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Shade Step", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Shade Feather", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Spec Threshold", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Spec Softness", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Spec Intensity", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Surface", NodeValueType::Material, ShaderGraphPinRole::Output}}},
				{"LitMaster",
				 "Lit Master",
				 {{"Base Color", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Shininess", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Roughness", NodeValueType::Float, ShaderGraphPinRole::Input},
				  {"Reflect", NodeValueType::Bool, ShaderGraphPinRole::Input},
				  {"Surface", NodeValueType::Material, ShaderGraphPinRole::Output}}},
				{"UnlitMaster",
				 "Unlit Master",
				 {{"Base Color", NodeValueType::Color, ShaderGraphPinRole::Input},
				  {"Surface", NodeValueType::Material, ShaderGraphPinRole::Output}}},
				{"ObjectTexture",
				 "Object Texture",
				 {{"Texture", NodeValueType::Texture2D, ShaderGraphPinRole::Output}}},
				{"TextureSample",
				 "Texture Sample",
				 {{"Texture", NodeValueType::Texture2D, ShaderGraphPinRole::Input},
				  {"Color", NodeValueType::Color, ShaderGraphPinRole::Output}}},
			};
			return schema;
		}
	};
}
