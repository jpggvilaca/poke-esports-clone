# Native Build

The Godot UI loads the combat simulation through a C++ GDExtension.

## One-Time Tools

Install Python 3, then install the SCons version used for this project:

```powershell
python -m pip install SCons==4.10.1
```

Initialize the official `godot-cpp` submodule after cloning:

```powershell
git submodule update --init --recursive
```

## Debug Build

From the repository root:

```powershell
scons platform=windows target=template_debug arch=x86_64 compiledb=yes
```

This creates the DLL used by Godot under `poke-clone-esports-ui/bin/` and a
generated `compile_commands.json` file for C++ editor tooling.
