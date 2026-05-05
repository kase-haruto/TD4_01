#pragma once

#include <Engine/Graphics/MaterialGraph/ShaderReflectionInfo.h>

#include <algorithm>
#include <string>
#include <vector>

namespace CalyxEngine {
	struct ShaderGraphValidationResult {
		bool ok = true;
		std::vector<std::string> messages;

		void Error(std::string message) {
			ok = false;
			messages.push_back(std::move(message));
		}
	};

	class ShaderGraphValidator {
	public:
		static ShaderGraphValidationResult ValidateObject3DMaterialShader(const ShaderReflectionInfo& reflection) {
			ShaderGraphValidationResult result;
			RequireResource(reflection, result, "MaterialConstants", ShaderResourceKind::CBuffer, 0);
			RequireResource(reflection, result, "gTexture", ShaderResourceKind::Texture, 0);
			RequireResource(reflection, result, "gSampler", ShaderResourceKind::Sampler, 0);

			const ShaderCBufferLayout* materialConstants = FindCBuffer(reflection, "MaterialConstants");
			if(!materialConstants) {
				result.Error("Missing MaterialConstants cbuffer layout.");
				return result;
			}

			RequireCBufferVariable(*materialConstants, result, "color");
			RequireCBufferVariable(*materialConstants, result, "enableLighting");
			RequireCBufferVariable(*materialConstants, result, "toonBaseStep");
			RequireCBufferVariable(*materialConstants, result, "toonShadeStep");
			RequireCBufferVariable(*materialConstants, result, "toonSpecularIntensity");
			return result;
		}

	private:
		static const ShaderResourceBinding* FindResource(
			const ShaderReflectionInfo& reflection,
			const std::string& name) {
			auto it = std::find_if(reflection.resources.begin(), reflection.resources.end(), [&name](const ShaderResourceBinding& binding) {
				return binding.name == name;
			});
			return it == reflection.resources.end() ? nullptr : &(*it);
		}

		static const ShaderCBufferLayout* FindCBuffer(
			const ShaderReflectionInfo& reflection,
			const std::string& name) {
			auto it = std::find_if(reflection.cbuffers.begin(), reflection.cbuffers.end(), [&name](const ShaderCBufferLayout& layout) {
				return layout.name == name;
			});
			return it == reflection.cbuffers.end() ? nullptr : &(*it);
		}

		static void RequireResource(
			const ShaderReflectionInfo& reflection,
			ShaderGraphValidationResult& result,
			const std::string& name,
			ShaderResourceKind kind,
			uint32_t bindPoint) {
			const ShaderResourceBinding* binding = FindResource(reflection, name);
			if(!binding) {
				result.Error("Missing shader resource: " + name);
				return;
			}
			if(binding->kind != kind) {
				result.Error("Unexpected resource kind: " + name);
			}
			if(binding->bindPoint != bindPoint) {
				result.Error("Unexpected bind point for " + name);
			}
		}

		static void RequireCBufferVariable(
			const ShaderCBufferLayout& layout,
			ShaderGraphValidationResult& result,
			const std::string& name) {
			const auto it = std::find_if(layout.variables.begin(), layout.variables.end(), [&name](const ShaderCBufferVariable& variable) {
				return variable.name == name;
			});
			if(it == layout.variables.end()) {
				result.Error("Missing cbuffer variable: " + layout.name + "." + name);
			}
		}
	};
}
