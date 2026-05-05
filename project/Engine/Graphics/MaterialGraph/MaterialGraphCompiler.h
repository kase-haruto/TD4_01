#pragma once

#include <Engine\Assets\DataAsset\MaterialAsset.h>

#include <algorithm>
#include <string>

namespace CalyxEngine {
	class MaterialGraphCompiler {
	public:
		static void Compile(MaterialAsset& material) {
			const Node* output = FindOutput(material.graph);
			if(!output) return;

			const NodePin* surfacePin = FindInput(*output, "Surface");
			if(surfacePin) {
				const Node* master = FindLinkedNode(material.graph, surfacePin->id);
				if(master) {
					if(master->type == "ToonMaster") {
						CompileToonMaster(material, *master);
						return;
					}
					if(master->type == "LitMaster") {
						CompileLitMaster(material, *master);
						return;
					}
					if(master->type == "UnlitMaster") {
						CompileUnlitMaster(material, *master);
						return;
					}
				}
			}

			CompileLegacyOutput(material, *output);
		}

	private:
		static const Node* FindOutput(const NodeGraph& graph) {
			for(const auto& node : graph.nodes) {
				if(node.type == "Output") return &node;
			}
			return nullptr;
		}

		static const NodePin* FindInput(const Node& node, const std::string& name) {
			for(const auto& pin : node.inputs) {
				if(pin.name == name) return &pin;
			}
			return nullptr;
		}

		static const Node* FindLinkedNode(const NodeGraph& graph, int32_t inputPinId) {
			for(const auto& link : graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				graph.FindPin(link.fromPinId, &fromNode);
				return fromNode;
			}
			return nullptr;
		}

		static float GetFloatProperty(const Node& node, const char* key, float fallback) {
			if(!node.properties.contains(key)) return fallback;
			return node.properties.value(key, fallback);
		}

		static Vector4 GetColorProperty(const Node& node, const char* key, const Vector4& fallback) {
			auto it = node.properties.find(key);
			if(it == node.properties.end() || !it->is_array() || it->size() != 4) return fallback;
			return {it->at(0).get<float>(), it->at(1).get<float>(), it->at(2).get<float>(), it->at(3).get<float>()};
		}

		static Vector4 EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Color) return fallback;
				if(fromNode->type == "Color") return fromNode->colorValue;
				if(fromNode->type == "MultiplyColor") {
					return EvaluateColor(material, fromNode->inputs[0].id, {1, 1, 1, 1}) *
						   EvaluateColor(material, fromNode->inputs[1].id, {1, 1, 1, 1});
				}
				if(fromNode->type == "LerpColor") {
					const Vector4 a = EvaluateColor(material, fromNode->inputs[0].id, fallback);
					const Vector4 b = EvaluateColor(material, fromNode->inputs[1].id, fallback);
					const float t = std::clamp(EvaluateFloat(material, fromNode->inputs[2].id, 0.0f), 0.0f, 1.0f);
					return {
						a.x + (b.x - a.x) * t,
						a.y + (b.y - a.y) * t,
						a.z + (b.z - a.z) * t,
						a.w + (b.w - a.w) * t};
				}
			}
			return fallback;
		}

		static float EvaluateFloat(const MaterialAsset& material, int32_t inputPinId, float fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Float) return fallback;
				if(fromNode->type == "Float" || fromNode->type == "Shininess" || fromNode->type == "Roughness") return fromNode->floatValue;
				if(fromNode->type == "MultiplyFloat") {
					return EvaluateFloat(material, fromNode->inputs[0].id, 1.0f) *
						   EvaluateFloat(material, fromNode->inputs[1].id, 1.0f);
				}
				if(fromNode->type == "LerpFloat") {
					const float a = EvaluateFloat(material, fromNode->inputs[0].id, fallback);
					const float b = EvaluateFloat(material, fromNode->inputs[1].id, fallback);
					const float t = std::clamp(EvaluateFloat(material, fromNode->inputs[2].id, 0.0f), 0.0f, 1.0f);
					return a + (b - a) * t;
				}
				if(fromNode->type == "SaturateFloat") {
					return std::clamp(EvaluateFloat(material, fromNode->inputs[0].id, fallback), 0.0f, 1.0f);
				}
				if(fromNode->type == "OneMinusFloat") {
					return 1.0f - EvaluateFloat(material, fromNode->inputs[0].id, fallback);
				}
			}
			return fallback;
		}

		static bool EvaluateBool(const MaterialAsset& material, int32_t inputPinId, bool fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Bool) return fallback;
				if(fromNode->type == "Reflect") return fromNode->boolValue;
			}
			return fallback;
		}

		static int32_t EvaluateLightingMode(MaterialAsset& material, int32_t inputPinId, int32_t fallback) {
			for(const auto& link : material.graph.links) {
				if(link.toPinId != inputPinId) continue;
				const Node* fromNode = nullptr;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId, &fromNode);
				if(!fromNode || !fromPin || fromPin->valueType != NodeValueType::Int) return fallback;
				if(fromNode->type == "LightingMode") return std::clamp(fromNode->intValue, 0, 4);
				if(fromNode->type == "HalfLambertLighting") return 0;
				if(fromNode->type == "LambertLighting") return 1;
				if(fromNode->type == "ToonLighting") {
					CompileToonLightingNode(material, *fromNode);
					return 2;
				}
				if(fromNode->type == "NoLighting") return 3;
				if(fromNode->type == "UnlitColorLighting") return 4;
			}
			return fallback;
		}

		static void CompileLegacyOutput(MaterialAsset& material, const Node& output) {
			for(const auto& pin : output.inputs) {
				if(pin.name == "BaseColor") material.color = EvaluateColor(material, pin.id, material.color);
				if(pin.name == "Shininess") material.shininess = EvaluateFloat(material, pin.id, material.shininess);
				if(pin.name == "Roughness") material.roughness = EvaluateFloat(material, pin.id, material.roughness);
				if(pin.name == "Reflect") material.isReflect = EvaluateBool(material, pin.id, material.isReflect);
				if(pin.name == "Lighting Mode") material.lightingMode = EvaluateLightingMode(material, pin.id, material.lightingMode);
			}
		}

		static void CompileLitMaster(MaterialAsset& material, const Node& node) {
			material.lightingMode = static_cast<int32_t>(GetFloatProperty(node, "lightingMode", 0.0f));
			if(const NodePin* pin = FindInput(node, "Base Color")) material.color = EvaluateColor(material, pin->id, GetColorProperty(node, "baseColor", material.color));
			if(const NodePin* pin = FindInput(node, "Shininess")) material.shininess = EvaluateFloat(material, pin->id, GetFloatProperty(node, "shininess", material.shininess));
			if(const NodePin* pin = FindInput(node, "Roughness")) material.roughness = EvaluateFloat(material, pin->id, GetFloatProperty(node, "roughness", material.roughness));
			if(const NodePin* pin = FindInput(node, "Reflect")) material.isReflect = EvaluateBool(material, pin->id, material.isReflect);
		}

		static void CompileUnlitMaster(MaterialAsset& material, const Node& node) {
			material.lightingMode = 4;
			if(const NodePin* pin = FindInput(node, "Base Color")) material.color = EvaluateColor(material, pin->id, GetColorProperty(node, "baseColor", material.color));
		}

		static void CompileToonMaster(MaterialAsset& material, const Node& node) {
			material.lightingMode = 2;

			const Vector4 baseColor = GetColorProperty(node, "baseColor", {1, 1, 1, 1});
			const Vector4 highlightColor = GetColorProperty(node, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
			const Vector4 firstShade = GetColorProperty(node, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
			const Vector4 secondShade = GetColorProperty(node, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});

			if(const NodePin* pin = FindInput(node, "Base Color")) material.color = EvaluateColor(material, pin->id, baseColor);
			else material.color = baseColor;

			material.toonBaseColor = {1, 1, 1, 1};
			material.toonHighlightColor = highlightColor;
			material.toonMidShadowColor = firstShade;
			material.toonShadowColor = secondShade;

			float shadeStep = GetFloatProperty(node, "shadeStep", -0.15f);
			float baseStep = GetFloatProperty(node, "baseStep", 0.25f);
			float shadeFeather = GetFloatProperty(node, "shadeFeather", 0.03f);
			float baseFeather = GetFloatProperty(node, "baseFeather", 0.03f);

			float specularThreshold = GetFloatProperty(node, "specularThreshold", 0.96f);
			float specularSoftness = GetFloatProperty(node, "specularSoftness", 0.02f);
			float specularIntensity = GetFloatProperty(node, "specularIntensity", 0.35f);

			if(const NodePin* pin = FindInput(node, "Highlight")) material.toonHighlightColor = EvaluateColor(material, pin->id, highlightColor);
			if(const NodePin* pin = FindInput(node, "1st Shade")) material.toonMidShadowColor = EvaluateColor(material, pin->id, firstShade);
			if(const NodePin* pin = FindInput(node, "2nd Shade")) material.toonShadowColor = EvaluateColor(material, pin->id, secondShade);
			if(const NodePin* pin = FindInput(node, "Shade Step")) shadeStep = EvaluateFloat(material, pin->id, shadeStep);
			if(const NodePin* pin = FindInput(node, "Base Step")) baseStep = EvaluateFloat(material, pin->id, baseStep);
			if(const NodePin* pin = FindInput(node, "Shade Feather")) shadeFeather = EvaluateFloat(material, pin->id, shadeFeather);
			if(const NodePin* pin = FindInput(node, "Base Feather")) baseFeather = EvaluateFloat(material, pin->id, baseFeather);
			if(const NodePin* pin = FindInput(node, "Spec Threshold")) specularThreshold = EvaluateFloat(material, pin->id, specularThreshold);
			if(const NodePin* pin = FindInput(node, "Spec Softness")) specularSoftness = EvaluateFloat(material, pin->id, specularSoftness);
			if(const NodePin* pin = FindInput(node, "Spec Intensity")) specularIntensity = EvaluateFloat(material, pin->id, specularIntensity);

			material.toonShadeStep = shadeStep;
			material.toonShadeFeather = shadeFeather;
			material.toonBaseStep = baseStep;
			material.toonBaseFeather = baseFeather;
			material.toonSpecularThreshold = specularThreshold;
			material.toonSpecularSoftness = specularSoftness;
			material.toonSpecularIntensity = specularIntensity;
		}

		static void CompileToonLightingNode(MaterialAsset& material, const Node& node) {
			material.toonHighlightColor = GetColorProperty(node, "toonHighlightColor", material.toonHighlightColor);
			material.toonBaseColor = GetColorProperty(node, "toonBaseColor", material.toonBaseColor);
			material.toonMidShadowColor = GetColorProperty(node, "toonMidShadowColor", material.toonMidShadowColor);
			material.toonShadowColor = GetColorProperty(node, "toonShadowColor", material.toonShadowColor);
			material.toonThreshold1 = GetFloatProperty(node, "toonThreshold1", material.toonThreshold1);
			material.toonThreshold2 = GetFloatProperty(node, "toonThreshold2", material.toonThreshold2);
			material.toonThreshold3 = GetFloatProperty(node, "toonThreshold3", material.toonThreshold3);
			material.toonEdgeSoftness = GetFloatProperty(node, "toonEdgeSoftness", material.toonEdgeSoftness);
			material.toonShadeStep = material.toonThreshold1;
			material.toonShadeFeather = material.toonEdgeSoftness;
			material.toonBaseStep = material.toonThreshold2;
			material.toonBaseFeather = material.toonEdgeSoftness;
			material.toonSpecularThreshold = GetFloatProperty(node, "toonSpecularThreshold", material.toonSpecularThreshold);
			material.toonSpecularSoftness = GetFloatProperty(node, "toonSpecularSoftness", material.toonSpecularSoftness);
			material.toonSpecularIntensity = GetFloatProperty(node, "toonSpecularIntensity", material.toonSpecularIntensity);
		}
	};
}
