#include "SceneObjectLibrary.h"

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/Event/Destroying/ObjectDestroying.h>
#include <Engine/System/Event/EventBus.h>
#include <cctype>
#include <iostream>
#include <limits>

uint32_t SceneObjectLibrary::nextPickingID_ = 1;

SceneObjectLibrary::SceneObjectLibrary() {
	connDestroy_ = EventBus::Subscribe<ObjectDestroying>(
		[this](const ObjectDestroying& ev) {
			if(suppressDestroySync_ || !ev.object) return;

			const Guid id = ev.object->GetGuid();
			auto	   it = objects_.find(id);
			if(it == objects_.end()) return;

			EventBus::Publish(ObjectRemoved{ev.object, owner_});
			objects_.erase(it);
		});
}
SceneObjectLibrary::~SceneObjectLibrary() = default;

namespace {
	std::string TrimName(const std::string& name) {
		const auto first = name.find_first_not_of(" \t\r\n");
		if(first == std::string::npos) return {};

		const auto last = name.find_last_not_of(" \t\r\n");
		return name.substr(first, last - first + 1);
	}

	std::string RemoveTrailingNumberSuffix(const std::string& name) {
		if(name.size() < 3 || name.back() != ')') return name;

		const auto open = name.find_last_of('(');
		if(open == std::string::npos || open + 1 >= name.size() - 1) return name;

		for(size_t i = open + 1; i < name.size() - 1; ++i) {
			if(!std::isdigit(static_cast<unsigned char>(name[i]))) {
				return name;
			}
		}

		return TrimName(name.substr(0, open));
	}

	bool TryParseNumberedName(const std::string& name,
							  const std::string& expectedBaseName,
							  uint32_t& outIndex) {
		if(name.size() < expectedBaseName.size() + 3 || name.back() != ')') return false;

		const auto open = name.find_last_of('(');
		if(open == std::string::npos || open + 1 >= name.size() - 1) return false;

		const std::string baseName = TrimName(name.substr(0, open));
		if(baseName != expectedBaseName) return false;

		uint64_t parsed = 0;
		for(size_t i = open + 1; i < name.size() - 1; ++i) {
			if(!std::isdigit(static_cast<unsigned char>(name[i]))) {
				return false;
			}

			parsed = parsed * 10 + static_cast<uint64_t>(name[i] - '0');
			if(parsed > (std::numeric_limits<uint32_t>::max)()) {
				return false;
			}
		}

		if(parsed == 0) return false;
		outIndex = static_cast<uint32_t>(parsed);
		return true;
	}

	bool IsNameUsed(const std::unordered_map<Guid, std::shared_ptr<SceneObject>>& objects,
					const std::string&											 name,
					const SceneObject*											 ignore) {
		for(const auto& [id, sp] : objects) {
			(void)id;
			if(!sp || sp.get() == ignore) continue;
			if(sp->GetName() == name) return true;
		}
		return false;
	}
} // namespace

void SceneObjectLibrary::CompactNumberedNames(const std::string& baseName) {
	const std::string trimmedBaseName = TrimName(baseName);
	if(trimmedBaseName.empty()) return;

	struct NumberedObject {
		uint32_t						currentIndex = 0;
		std::shared_ptr<SceneObject>	object;
	};

	std::vector<NumberedObject> numberedObjects;
	numberedObjects.reserve(objects_.size());

	for(const auto& [id, sp] : objects_) {
		(void)id;
		if(!sp) continue;

		uint32_t currentIndex = 0;
		if(TryParseNumberedName(sp->GetName(), trimmedBaseName, currentIndex)) {
			numberedObjects.push_back({currentIndex, sp});
		}
	}

	if(numberedObjects.empty()) return;

	std::sort(numberedObjects.begin(), numberedObjects.end(),
			  [](const NumberedObject& lhs, const NumberedObject& rhs) {
				  if(lhs.currentIndex != rhs.currentIndex) return lhs.currentIndex < rhs.currentIndex;
				  return lhs.object->GetGuid().ToString() < rhs.object->GetGuid().ToString();
			  });

	for(size_t i = 0; i < numberedObjects.size(); ++i) {
		SceneObject* object = numberedObjects[i].object.get();
		if(!object) continue;

		const uint32_t	newIndex = static_cast<uint32_t>(i + 1);
		const std::string newName = trimmedBaseName + "(" + std::to_string(newIndex) + ")";
		if(object->GetName() != newName) {
			object->SetName(newName, object->GetObjectType());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの追加
//////////////////////////////////////////////////////////////////////////////////
void SceneObjectLibrary::AddObject(const std::shared_ptr<SceneObject>& object) {
	if(!object) return;

	const Guid	id		  = object->GetGuid();
	if(objects_.contains(id)) return;

	std::string finalName = MakeUniqueName(object->GetName(), object.get());
	object->SetName(finalName, object->GetObjectType());

	// Picking ID 割り当て
	if(object->GetPickingID() == 0) {
		object->SetPickingID(nextPickingID_++);
	}

	// shared_ptr で登録
	objects_[id] = object;

	// イベント発火
	EventBus::Publish(ObjectAdded{object, owner_});
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクト名の変更
//////////////////////////////////////////////////////////////////////////////////
std::string SceneObjectLibrary::RenameObject(const std::shared_ptr<SceneObject>& object,
											 const std::string&					requestedName) {
	if(!object) return {};

	const std::string finalName = MakeUniqueName(requestedName, object.get());
	object->SetName(finalName, object->GetObjectType());
	return finalName;
}

//////////////////////////////////////////////////////////////////////////////////
///     現在存在するオブジェクト集合から一意な名前を生成
//////////////////////////////////////////////////////////////////////////////////
std::string SceneObjectLibrary::MakeUniqueName(const std::string& requestedName,
											   const SceneObject* ignore) const {
	std::string baseName = TrimName(requestedName);
	if(baseName.empty()) {
		baseName = ignore ? std::string(ignore->GetObjectClassName()) : "SceneObject";
		if(baseName.empty()) baseName = "SceneObject";
	}

	if(!IsNameUsed(objects_, baseName, ignore)) {
		return baseName;
	}

	baseName = RemoveTrailingNumberSuffix(baseName);
	if(baseName.empty()) {
		baseName = ignore ? std::string(ignore->GetObjectClassName()) : "SceneObject";
		if(baseName.empty()) baseName = "SceneObject";
	}

	if(!IsNameUsed(objects_, baseName, ignore)) {
		return baseName;
	}

	for(uint32_t index = 1;; ++index) {
		const std::string candidate = baseName + "(" + std::to_string(index) + ")";
		if(!IsNameUsed(objects_, candidate, ignore)) {
			return candidate;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの削除(shared_ptr指定)
//////////////////////////////////////////////////////////////////////////////////
bool SceneObjectLibrary::RemoveObject(const std::shared_ptr<SceneObject>& object) {
	if(!object) return false;
	Guid id = object->GetGuid();
	const std::string removedBaseName = RemoveTrailingNumberSuffix(object->GetName());
	std::cout << "[REMOVE] " << object->GetName()
			  << " GUID=" << id.ToString()
			  << " use_count=" << object.use_count() << std::endl;

	// 子を完全に削除（再帰）
	auto children = object->GetChildren();
	for(auto& child : children) {
		if(child) {
			RemoveObject(child);
		}
	}

	// 先に削除イベントを発火（FxSystem が emitter を消す）
	EventBus::Publish(ObjectRemoved{object, owner_});

	// DestroyRecursive で階層を断つ
	suppressDestroySync_ = true;
	object->Destroy();
	suppressDestroySync_ = false;

	// 最後にライブラリから除外
	objects_.erase(id);
	CompactNumberedNames(removedBaseName);
	std::cout << "[AFTER ERASE]"
			  << " use_count=" << object.use_count()
			  << std::endl;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
///     idをもとに削除
//////////////////////////////////////////////////////////////////////////////////
bool SceneObjectLibrary::RemoveObject(Guid id) {
	auto it = objects_.find(id);
	if(it == objects_.end()) return false;

	std::string removedBaseName;
	if(auto sp = it->second) {
		removedBaseName = RemoveTrailingNumberSuffix(sp->GetName());

		// 子リストをコピーしてから再帰削除
		auto children = sp->GetChildren();
		for(auto& child : children) {
			if(child) {
				RemoveObject(child);
			}
		}

		suppressDestroySync_ = true;
		sp->Destroy();
		suppressDestroySync_ = false;
		EventBus::Publish(ObjectRemoved{sp, owner_});
	}

	objects_.erase(it);
	CompactNumberedNames(removedBaseName);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////
///     リストのクリア
//////////////////////////////////////////////////////////////////////////////////
void SceneObjectLibrary::Clear() {
	// Destroy → イベント → クリア
	for(auto& [id, sp] : objects_) {
		if(!sp) continue;
		suppressDestroySync_ = true;
		sp->Destroy();
		suppressDestroySync_ = false;
		EventBus::Publish(ObjectRemoved{sp, owner_});
	}

	objects_.clear();
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの検索(idから)
//////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<SceneObject> SceneObjectLibrary::Find(Guid id) const {
	auto it = objects_.find(id);
	if(it == objects_.end()) return nullptr;
	return it->second;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの検索(名前から)
//////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<SceneObject> SceneObjectLibrary::FindByName(const std::string& name) const {
	for(const auto& [id, sp] : objects_) {
		if(sp && sp->GetName() == name) {
			return sp;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの一覧取得(raw)
//////////////////////////////////////////////////////////////////////////////////
std::vector<SceneObject*> SceneObjectLibrary::GetAllObjectsRaw() const {
	std::vector<SceneObject*> result;
	result.reserve(objects_.size());

	for(const auto& [id, sp] : objects_) {
		if(sp) {
			result.push_back(sp.get());
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの一覧取得(shared_ptr版)
//////////////////////////////////////////////////////////////////////////////////
std::vector<std::shared_ptr<SceneObject>> SceneObjectLibrary::GetAllObjectsShared() const {
	std::vector<std::shared_ptr<SceneObject>> result;
	result.reserve(objects_.size());

	for(const auto& [id, sp] : objects_) {
		if(sp) {
			result.push_back(sp);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////
///     オブジェクトの所属判定
//////////////////////////////////////////////////////////////////////////////////
bool SceneObjectLibrary::Contains(const std::shared_ptr<SceneObject>& obj) const {
	if(!obj) return false;
	return objects_.contains(obj->GetGuid());
}

namespace {
	// シェーダー(picking.ps)と同一のハッシュ関数
	// ピッキング結果の可視化のために色を分散させているため、検索時もこれを通す必要がある
	uint32_t Hash(uint32_t x) {
		x ^= x >> 17;
		x *= 0xed5ad4bb;
		x ^= x >> 11;
		x *= 0xac4c1b51;
		x ^= x >> 15;
		x *= 0x31848bab;
		x ^= x >> 14;
		return x;
	}
} // namespace

std::shared_ptr<SceneObject> SceneObjectLibrary::FindSharedByPickingID(uint32_t hashedPickingID) const {
	for(const auto& [id, sp] : objects_) {
		// シェーダーがRGB(24bit)に書き出しているため、検索時も下位24bitのみで比較する
		if(sp && (Hash(sp->GetPickingID()) & 0x00FFFFFF) == hashedPickingID) {
			return sp;
		}
	}
	return nullptr;
}
