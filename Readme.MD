# SML Injected

## Credits
- [SML](https://github.com/lukas0x1/sml-pc) - The original project
- [UniversalHookX](https://github.com/bruhmoment21/UniversalHookX) - The graphical hook that renders ImGui

# Dependancies
- [ImGui](https://github.com/ocornut/imgui)
- [MinHook](https://github.com/TsudaKageyu/minhook)
- [libmem](https://github.com/rdbo/libmem)

# How to build
- Clone the repository
- Open the project in Visual Studio and create a x64 Release with toolset msvc_x64
- Build the project

# How to use
- Copy the `sml_config.json` file and `mods` folder to the game directory
- Inject canvas.dll into the game using your favorite injector
- Need a injector? Use [FateInjector](https://github.com/fligger/FateInjector)

# Make your own mods
- Download `mod-demo` folder and open it in Visual Studio
- Do whatever and build the same way as the main project