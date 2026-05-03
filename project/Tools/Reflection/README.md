# Calyx Reflection / Object Collection

This folder contains the build-time collection step for editor-placeable scene objects.
It is designed to give the engine a small Unreal-style workflow without relying on C++ runtime reflection.

## Goal

Add a marker before a class declaration, then let the editor discover it automatically:

```cpp
#include <Engine/Foundation/Reflection/CalyxReflection.h>

CALYX_OBJECT(Category = Event, DisplayName = "Camera Event")
class CameraEventObject : public BaseEventObject {
public:
	CameraEventObject();
};
```

After the next build, the object is registered into `SceneObjectRegistry` and appears in the editor placement panel.

## Build Flow

1. MSBuild runs `Tools/Reflection/generate_reflection.ps1` as a pre-build event.
2. The script scans header files under `Engine/**/*.h` and `Game/**/*.h`.
3. It finds `CALYX_OBJECT(...)` markers followed by `class ClassName :`.
4. It writes generated registration code to:
   - `Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h`
   - `Engine/Foundation/Reflection/CalyxObjectRegistry.generated.cpp`
5. `main.cpp` calls `CalyxEngine::RegisterGeneratedSceneObjects()` during startup.
6. `PlaceToolPanel` asks `SceneObjectRegistry` for placeable objects and builds the Events list from that registry data.

## Important Files

- `Engine/Foundation/Reflection/CalyxReflection.h`
  Defines marker macros. These compile to nothing and exist for the generator.

- `Tools/Reflection/generate_reflection.ps1`
  The active generator used by MSBuild. It is PowerShell-based so it works in the Visual Studio build environment.

- `Tools/Reflection/generate_reflection.py`
  Python version of the generator. It is not wired into MSBuild because Python may not be installed.

- `Engine/Foundation/Reflection/CalyxObjectRegistry.generated.cpp`
  Generated registration output. Do not edit by hand.

- `Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.*`
  Runtime registry used by scene loading and editor placement.

- `Engine/Application/UI/Panels/PlaceToolPanel.cpp`
  Reads placeable registry entries and creates editor placement items.

## CALYX_OBJECT

`CALYX_OBJECT` is the currently active marker.

Supported fields:

```cpp
CALYX_OBJECT(
	Category = Event,
	DisplayName = "Camera Event"
)
```

- `Category`
  Maps to `ObjectType`. Currently `Event` is used by the placement panel.

- `DisplayName`
  The name shown in the editor.

- `Icon`
  Asset path passed to `TextureManager::LoadTexture`. It is relative to `Resources/Assets`.
  Default: `UI/Tool/event.png`.

- `Placeable`
  If `true`, the class is exposed to the placement panel.
  Default: `true`.

If `TypeName` is omitted, the C++ class name is used as the serialized type name.
If `DisplayName` is omitted, `TypeName` is used.

## CALYX_GENERATED_BODY

`CALYX_GENERATED_BODY()` is an optional placeholder marker.

Current role:

- It compiles to nothing.
- It documents that the class participates in the generated reflection/object collection system.

Planned role:

- This is where generated per-class glue can be attached later if needed.
- Examples: static class descriptor access, property table binding, editor metadata hooks, version migration hooks.

Keeping it in the class now makes future expansion less disruptive.

## CALYX_PROPERTY

`CALYX_PROPERTY(...)` is also a placeholder marker right now.

Current role:

- It compiles to nothing.
- The generator does not yet collect properties.

Planned role:

- Mark fields for automatic inspector display.
- Add metadata such as display name, range, category, serialization behavior, and editor-only flags.

Example target syntax:

```cpp
CALYX_PROPERTY(Edit, DisplayName = "Trigger Radius", Min = 0.0f)
float triggerRadius_ = 1.0f;
```

The current implementation intentionally stops at object collection. Property reflection should be added as a second step so it can be designed around the existing `IConfigurable` and JSON serialization flow.

## Adding A New Placeable Event

1. Create the event class as usual.
2. Include `CalyxReflection.h` in the header.
3. Add `CALYX_OBJECT(...)` immediately before the class declaration.
4. Build the project.
5. The generated registry file is updated by the pre-build step.
6. The event appears under the Events category in the placement panel.

## Minimal Event Example

```cpp
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/Event/BaseEventObject.h>

CALYX_OBJECT(Category = Event, DisplayName = "Tutorial Event")
class TutorialEvent : public BaseEventObject {
public:
	TutorialEvent();
	std::string_view GetObjectClassName() const override { return "TutorialEvent"; }
};
```

This uses the default event icon and is placeable by default.

## Current Limitations

- The generator is regex-based. Keep `CALYX_OBJECT(...)` directly before the class declaration.
- The class must be in a header under `Engine` or `Game`.
- The class must have a default constructor compatible with `std::make_shared<ClassName>()`.
- Only object collection is implemented. Property collection is reserved for the next stage.
