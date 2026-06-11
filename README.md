# Arc Engine - Filament Prototype

Prototype C++ para **Arc Engine** con motor y juego separados.

## Incluye

- `Engine/` separado de `Game/`
- SDL2 para ventana/input
- Google Filament usando backend Vulkan
- Freecam básica
- Skybox simple por color
- Sol/directional light tipo `SUN`
- Cubo en el centro
- Material Filament `.mat` compilado automáticamente con `matc`
- `.vscode/tasks.json` para compilar y ejecutar con `CTRL + SHIFT + B`
- `vcpkg.json` en modo manifest

## Controles

| Acción | Control |
|---|---|
| Mirar alrededor | Mantener click derecho + mover mouse |
| Avanzar | W |
| Retroceder | S |
| Izquierda | A |
| Derecha | D |
| Subir | E |
| Bajar | Q |
| Más rápido | Shift |
| Zoom/FOV | Rueda del mouse |

## Requisitos

- CMake 3.20+
- Ninja
- Compilador C++20
- Vulkan SDK instalado
- vcpkg instalado
- Variable de entorno `VCPKG_ROOT` apuntando a tu carpeta de vcpkg

Ejemplo en PowerShell:

```powershell
$env:VCPKG_ROOT="C:\\dev\\vcpkg"
```

Para dejarlo permanente en Windows:

```powershell
setx VCPKG_ROOT "C:\\dev\\vcpkg"
```

## Uso en VS Code

Abre la carpeta raíz del proyecto en VS Code y presiona:

```txt
CTRL + SHIFT + B
```

La task default hace:

```txt
cmake --preset windows-debug
cmake --build --preset windows-debug
Build/windows-debug/Game/ArcGame.exe
```

## Uso manual

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
cd Build/windows-debug/Game
./ArcGame.exe
```

## Notas importantes

Este prototipo usa Filament directamente dentro de `Engine/Arc/Renderer`. El código de `Game/` no incluye headers de Filament.

El material del cubo está en:

```txt
Assets/Engine/Materials/ArcDefault.mat
```

CMake lo compila con `matc` a:

```txt
Build/windows-debug/Game/Assets/Engine/Materials/ArcDefault.filamat
```

Luego se copia junto al ejecutable para que el runtime lo pueda cargar desde:

```txt
Assets/Engine/Materials/ArcDefault.filamat
```

## Estado actual

```txt
Game/Main.cpp
    ↓
arc::Application
    ↓
arc::Window      SDL2
arc::Renderer    Filament + Vulkan
    ↓
FreeCam + Skybox + Sun + Cube
```

## Limitaciones del prototipo

- El handle nativo para Filament está implementado para Windows.
- En Linux/macOS hay que adaptar `Renderer::GetNativeWindowHandle()` usando X11, Wayland, Cocoa o helpers de plataforma de Filament.
- El cubo usa tangentes placeholder. Para producción, genera tangentes correctas durante importación de assets.
