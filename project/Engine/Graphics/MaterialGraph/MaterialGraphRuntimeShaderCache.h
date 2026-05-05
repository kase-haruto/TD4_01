#pragma once

#include <Engine/Assets/DataAsset/MaterialAsset.h>
#include <Engine/Graphics/MaterialGraph/ShaderGraphCodeGenerator.h>
#include <Engine/Graphics/Pipeline/Shader/ShaderCompiler.h>

#include <dxcapi.h>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <wrl.h>

namespace CalyxEngine {
	struct MaterialGraphRuntimeShader {
		Microsoft::WRL::ComPtr<IDxcBlob> pixelShader;
		std::string hlsl;
		std::size_t hash = 0;
		bool cacheHit = false;
		bool usesObjectTexture = false;
	};

	class MaterialGraphRuntimeShaderCache {
	public:
		MaterialGraphRuntimeShader GetOrCompileObject3DPixelShader(MaterialAsset& material) {
			const GeneratedShaderGraphCode generated = ShaderGraphCodeGenerator::GenerateObject3DRuntimePixelShaderSource(material);
			const std::size_t hash = HashGeneratedShader(generated.hlsl);
			const std::string key = material.GetGuid().ToString() + ":object3d:" + std::to_string(hash);

			auto it = shaders_.find(key);
			if(it != shaders_.end()) {
				MaterialGraphRuntimeShader result = it->second;
				result.cacheHit = true;
				return result;
			}

			ShaderCompiler compiler;
			compiler.InitializeDXC();
			MaterialGraphRuntimeShader shader;
			shader.pixelShader = compiler.CompileSource(L"MaterialGraphRuntimeObject3D.PS.hlsl", generated.hlsl, L"ps_6_5");
			shader.hlsl = generated.hlsl;
			shader.hash = hash;
			shader.cacheHit = false;
			shader.usesObjectTexture = generated.usesObjectTexture;
			shaders_[key] = shader;
			return shader;
		}

		MaterialGraphRuntimeShader GetOrCompilePreviewPixelShader(MaterialAsset& material) {
			const GeneratedShaderGraphCode generated = ShaderGraphCodeGenerator::GeneratePreviewPixelShaderSource(material);
			const std::size_t hash = HashGeneratedShader(generated.hlsl);
			const std::string key = material.GetGuid().ToString() + ":" + std::to_string(hash);

			auto it = shaders_.find(key);
			if(it != shaders_.end()) {
				MaterialGraphRuntimeShader result = it->second;
				result.cacheHit = true;
				return result;
			}

			ShaderCompiler compiler;
			compiler.InitializeDXC();
			MaterialGraphRuntimeShader shader;
			shader.pixelShader = compiler.CompileSource(L"MaterialGraphRuntimePreview.PS.hlsl", generated.hlsl, L"ps_6_5");
			shader.hlsl = generated.hlsl;
			shader.hash = hash;
			shader.cacheHit = false;
			shader.usesObjectTexture = generated.usesObjectTexture;
			shaders_[key] = shader;
			return shader;
		}

		void Clear() {
			shaders_.clear();
		}

		std::size_t Size() const {
			return shaders_.size();
		}

	private:
		static std::size_t HashGeneratedShader(const std::string& hlsl) {
			return std::hash<std::string>{}(hlsl);
		}

		std::unordered_map<std::string, MaterialGraphRuntimeShader> shaders_;
	};
}
