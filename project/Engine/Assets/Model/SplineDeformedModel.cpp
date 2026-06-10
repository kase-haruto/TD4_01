#include "SplineDeformedModel.h"

#include <Engine/Assets/Manager/AssetManager.h>
#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <limits>

SplineDeformedModel::SplineDeformedModel() {
	modelData_ = nullptr;
	Initialize();
}

SplineDeformedModel::SplineDeformedModel(const std::string& sourceFile) {
	modelData_ = nullptr;
	Initialize();
	SetSourceModel(sourceFile);
}

void SplineDeformedModel::Initialize() {
	CreateMaterialBuffer();
	Map();
}

bool SplineDeformedModel::SetSourceModel(const std::string& sourceFile) {
	if(fileName_ == sourceFile && sourceReady_) {
		return true;
	}
	if(fileName_ == sourceFile) {
		return LoadSourceIfReady();
	}

	fileName_ = sourceFile;
	sourceReady_ = false;
	buffersReady_ = false;
	ownedModelData_.reset();
	modelData_ = nullptr;
	originalVertices_.clear();
	originalIndices_.clear();
	originalSubMeshes_.clear();

	return LoadSourceIfReady();
}

bool SplineDeformedModel::LoadSourceIfReady() {
	if(sourceReady_) return true;
	if(fileName_.empty()) return false;

	auto* assetManager = CalyxEngine::AssetManager::GetInstance();
	if(!assetManager || !assetManager->GetModelManager()) return false;

	auto* modelManager = assetManager->GetModelManager();
	if(!modelManager->IsModelLoaded(fileName_)) {
		modelManager->LoadModel(fileName_);
		return false;
	}

	ModelData& source = modelManager->GetModelData(fileName_);
	if(source.meshResource.Vertices().empty() || source.meshResource.Indices().empty()) {
		return false;
	}

	ownedModelData_ = std::make_unique<ModelData>();
	ownedModelData_->meshResource.data.vertices = source.meshResource.Vertices();
	ownedModelData_->meshResource.data.indices = source.meshResource.Indices();
	ownedModelData_->meshResource.data.material = source.meshResource.Material();
	ownedModelData_->meshResource.data.materials = source.meshResource.Materials();
	ownedModelData_->meshResource.data.subMeshes = source.meshResource.SubMeshes();
	ownedModelData_->meshResource.topology = source.meshResource.topology;
	ownedModelData_->localAABB = source.localAABB;

	originalVertices_ = ownedModelData_->meshResource.Vertices();
	originalIndices_ = ownedModelData_->meshResource.Indices();
	originalSubMeshes_ = ownedModelData_->meshResource.SubMeshes();
	modelData_ = ownedModelData_.get();

	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	modelData_->meshResource.VertexBuffer().Initialize(device, UINT(modelData_->meshResource.Vertices().size()));
	modelData_->meshResource.VertexBuffer().TransferVectorData(modelData_->meshResource.Vertices());
	modelData_->meshResource.IndexBuffer().Initialize(device, UINT(modelData_->meshResource.Indices().size()));
	modelData_->meshResource.IndexBuffer().TransferVectorData(modelData_->meshResource.Indices());
	buffersReady_ = true;

	OnModelLoaded();
	sourceReady_ = true;
	return true;
}

bool SplineDeformedModel::Rebuild(const SplineData& spline, Axis axis, float radiusScale, float distanceOffset) {
	if(!LoadSourceIfReady()) return false;
	if(!modelData_ || originalVertices_.empty() || spline.TotalLength() <= 0.0f || spline.SegmentCount() <= 0) {
		return false;
	}

	const int along = AxisIndex(axis);
	const int perpA = PerpIndexA(axis);
	const int perpB = PerpIndexB(axis);

	float minAlong = FLT_MAX;
	float maxAlong = -FLT_MAX;
	float centerA = 0.0f;
	float centerB = 0.0f;
	for(const auto& vertex : originalVertices_) {
		const CalyxEngine::Vector3 local{vertex.position.x, vertex.position.y, vertex.position.z};
		minAlong = (std::min)(minAlong, local[along]);
		maxAlong = (std::max)(maxAlong, local[along]);
		centerA += local[perpA];
		centerB += local[perpB];
	}
	centerA /= static_cast<float>(originalVertices_.size());
	centerB /= static_cast<float>(originalVertices_.size());

	const float length = (std::max)(0.0001f, maxAlong - minAlong);
	auto& vertices = modelData_->meshResource.Vertices();
	vertices = originalVertices_;

	CalyxEngine::Vector3 prevRight = CalyxEngine::Vector3::Zero();
	for(size_t i = 0; i < vertices.size(); ++i) {
		const VertexPosUvN& src = originalVertices_[i];
		const CalyxEngine::Vector3 local{src.position.x, src.position.y, src.position.z};

		const float normalized = (local[along] - minAlong) / length;
		const float distance = normalized * spline.TotalLength() + distanceOffset;
		const float t = spline.DistanceToT(distance);

		CalyxEngine::Vector3 forward = spline.Tangent(t);
		if(forward.LengthSquared() <= 1e-8f) {
			forward = CalyxEngine::Vector3::Forward();
		}

		CalyxEngine::Vector3 upSeed = CalyxEngine::Vector3::Up();
		if(std::abs(CalyxEngine::Vector3::Dot(forward, upSeed)) > 0.95f) {
			upSeed = {1.0f, 0.0f, 0.0f};
		}

		CalyxEngine::Vector3 right = CalyxEngine::Vector3::Cross(upSeed, forward).Normalize();
		if(prevRight.LengthSquared() > 1e-8f && CalyxEngine::Vector3::Dot(prevRight, right) < 0.0f) {
			right = -right;
		}
		prevRight = right;

		CalyxEngine::Vector3 up = CalyxEngine::Vector3::Cross(forward, right).Normalize();
		const CalyxEngine::Vector3 center = spline.Evaluate(t);

		const float offsetA = (local[perpA] - centerA) * radiusScale;
		const float offsetB = (local[perpB] - centerB) * radiusScale;
		const CalyxEngine::Vector3 deformed = center + right * offsetA + up * offsetB;
		vertices[i].position = {deformed.x, deformed.y, deformed.z, src.position.w};

		const CalyxEngine::Vector3 nLocal = src.normal;
		const CalyxEngine::Vector3 n = (forward * nLocal[along] + right * nLocal[perpA] + up * nLocal[perpB]);
		vertices[i].normal = n.LengthSquared() > 1e-8f ? n.Normalize() : src.normal;
	}

	RecalculateLocalAABB();
	UploadDeformedVertices();
	blasBuilt_ = false;
	return true;
}

bool SplineDeformedModel::RebuildTiled(
	const SplineData& spline,
	Axis axis,
	float radiusScale,
	float distanceOffset,
	float tileLength,
	int maxTiles) {
	if(!LoadSourceIfReady()) return false;
	if(!modelData_ || originalVertices_.empty() || spline.TotalLength() <= 0.0f || spline.SegmentCount() <= 0) {
		return false;
	}

	const int along = AxisIndex(axis);
	const int perpA = PerpIndexA(axis);
	const int perpB = PerpIndexB(axis);

	float minAlong = FLT_MAX;
	float maxAlong = -FLT_MAX;
	float centerA = 0.0f;
	float centerB = 0.0f;
	for(const auto& vertex : originalVertices_) {
		const CalyxEngine::Vector3 local{vertex.position.x, vertex.position.y, vertex.position.z};
		minAlong = (std::min)(minAlong, local[along]);
		maxAlong = (std::max)(maxAlong, local[along]);
		centerA += local[perpA];
		centerB += local[perpB];
	}
	centerA /= static_cast<float>(originalVertices_.size());
	centerB /= static_cast<float>(originalVertices_.size());

	const float sourceLength = (std::max)(0.0001f, maxAlong - minAlong);
	const float stepLength = tileLength > 0.0f ? tileLength : sourceLength;
	int tileCount = static_cast<int>(std::ceil(spline.TotalLength() / stepLength));
	if(maxTiles > 0) {
		tileCount = (std::min)(tileCount, maxTiles);
	}
	tileCount = (std::max)(tileCount, 1);

	if(originalVertices_.size() > static_cast<size_t>((std::numeric_limits<uint32_t>::max)()) / static_cast<size_t>(tileCount)) {
		return false;
	}

	auto& vertices = modelData_->meshResource.Vertices();
	auto& indices = modelData_->meshResource.Indices();
	auto& subMeshes = modelData_->meshResource.SubMeshes();

	vertices.clear();
	indices.clear();
	subMeshes.clear();
	vertices.reserve(originalVertices_.size() * static_cast<size_t>(tileCount));
	indices.reserve(originalIndices_.size() * static_cast<size_t>(tileCount));

	CalyxEngine::Vector3 prevRight = CalyxEngine::Vector3::Zero();
	for(int tile = 0; tile < tileCount; ++tile) {
		const float tileStartDistance = distanceOffset + static_cast<float>(tile) * stepLength;

		for(const auto& src : originalVertices_) {
			const CalyxEngine::Vector3 local{src.position.x, src.position.y, src.position.z};
			const float localDistance = ((local[along] - minAlong) / sourceLength) * stepLength;
			const float distance = tileStartDistance + localDistance;
			const float t = spline.DistanceToT(distance);

			CalyxEngine::Vector3 forward = spline.Tangent(t);
			if(forward.LengthSquared() <= 1e-8f) {
				forward = CalyxEngine::Vector3::Forward();
			}

			CalyxEngine::Vector3 upSeed = CalyxEngine::Vector3::Up();
			if(std::abs(CalyxEngine::Vector3::Dot(forward, upSeed)) > 0.95f) {
				upSeed = {1.0f, 0.0f, 0.0f};
			}

			CalyxEngine::Vector3 right = CalyxEngine::Vector3::Cross(upSeed, forward).Normalize();
			if(prevRight.LengthSquared() > 1e-8f && CalyxEngine::Vector3::Dot(prevRight, right) < 0.0f) {
				right = -right;
			}
			prevRight = right;

			CalyxEngine::Vector3 up = CalyxEngine::Vector3::Cross(forward, right).Normalize();
			const CalyxEngine::Vector3 center = spline.Evaluate(t);

			const float offsetA = (local[perpA] - centerA) * radiusScale;
			const float offsetB = (local[perpB] - centerB) * radiusScale;
			const CalyxEngine::Vector3 deformed = center + right * offsetA + up * offsetB;

			VertexPosUvN dst = src;
			dst.position = {deformed.x, deformed.y, deformed.z, src.position.w};

			const CalyxEngine::Vector3 nLocal = src.normal;
			const CalyxEngine::Vector3 n = (forward * nLocal[along] + right * nLocal[perpA] + up * nLocal[perpB]);
			dst.normal = n.LengthSquared() > 1e-8f ? n.Normalize() : src.normal;
			vertices.push_back(dst);
		}

	}

	if(originalSubMeshes_.empty()) {
		MeshData::SubMesh subMesh{};
		subMesh.indexStart = 0;
		subMesh.materialIndex = 0;
		for(int tile = 0; tile < tileCount; ++tile) {
			const uint32_t baseVertex = static_cast<uint32_t>(tile * originalVertices_.size());
			for(uint32_t index : originalIndices_) {
				indices.push_back(baseVertex + index);
			}
		}
		subMesh.indexCount = static_cast<uint32_t>(indices.size());
		subMeshes.push_back(subMesh);
	} else {
		for(const auto& sourceSubMesh : originalSubMeshes_) {
			MeshData::SubMesh subMesh{};
			subMesh.indexStart = static_cast<uint32_t>(indices.size());
			subMesh.materialIndex = sourceSubMesh.materialIndex;
			for(int tile = 0; tile < tileCount; ++tile) {
				const uint32_t baseVertex = static_cast<uint32_t>(tile * originalVertices_.size());
				for(uint32_t i = 0; i < sourceSubMesh.indexCount; ++i) {
					indices.push_back(baseVertex + originalIndices_[sourceSubMesh.indexStart + i]);
				}
			}
			subMesh.indexCount = static_cast<uint32_t>(indices.size()) - subMesh.indexStart;
			if(subMesh.indexCount > 0) {
				subMeshes.push_back(subMesh);
			}
		}
	}

	RecalculateLocalAABB();
	modelData_->meshResource.VertexBuffer().Initialize(GraphicsGroup::GetInstance()->GetDevice().Get(), UINT(vertices.size()));
	modelData_->meshResource.VertexBuffer().TransferVectorData(vertices);
	modelData_->meshResource.IndexBuffer().Initialize(GraphicsGroup::GetInstance()->GetDevice().Get(), UINT(indices.size()));
	modelData_->meshResource.IndexBuffer().TransferVectorData(indices);
	buffersReady_ = true;
	blasBuilt_ = false;
	return true;
}

void SplineDeformedModel::UploadDeformedVertices() {
	if(!buffersReady_ || !modelData_ || modelData_->meshResource.Vertices().empty()) return;
	modelData_->meshResource.VertexBuffer().TransferVectorData(modelData_->meshResource.Vertices());
}

void SplineDeformedModel::RecalculateLocalAABB() {
	if(!modelData_ || modelData_->meshResource.Vertices().empty()) return;

	CalyxEngine::Vector3 minPos{FLT_MAX, FLT_MAX, FLT_MAX};
	CalyxEngine::Vector3 maxPos{-FLT_MAX, -FLT_MAX, -FLT_MAX};
	for(const auto& vertex : modelData_->meshResource.Vertices()) {
		const CalyxEngine::Vector3 p{vertex.position.x, vertex.position.y, vertex.position.z};
		minPos = CalyxEngine::Vector3::Min(minPos, p);
		maxPos = CalyxEngine::Vector3::Max(maxPos, p);
	}
	modelData_->localAABB.Initialize(minPos, maxPos);
}

void SplineDeformedModel::Draw(const WorldTransform& transform) {
	if(!modelData_) return;
	Model::Draw(transform);
}

void SplineDeformedModel::ShowImGuiInterface() {
	BaseModel::ShowImGuiInterface();
}

void SplineDeformedModel::CreateMaterialBuffer() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	materialBuffer_.Initialize(device);
}

void SplineDeformedModel::MaterialBufferMap() {
	TransferMaterial();
}

void SplineDeformedModel::Map() {
	MaterialBufferMap();
}

int SplineDeformedModel::AxisIndex(Axis axis) {
	return static_cast<int>(axis);
}

int SplineDeformedModel::PerpIndexA(Axis axis) {
	switch(axis) {
	case Axis::X:
		return 2;
	case Axis::Y:
		return 0;
	case Axis::Z:
	default:
		return 0;
	}
}

int SplineDeformedModel::PerpIndexB(Axis axis) {
	switch(axis) {
	case Axis::X:
		return 1;
	case Axis::Y:
		return 2;
	case Axis::Z:
	default:
		return 1;
	}
}
