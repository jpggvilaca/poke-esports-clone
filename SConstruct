#!/usr/bin/env python

import os

# godot-cpp supplies the platform-specific compiler flags and links its static
# bindings library into our GDExtension DLL.
env = SConscript("godot-cpp/SConstruct")
env.Append(CPPPATH=[".", "native"])

core_sources = [
    "BattleRules.cpp",
    "BattleSession.cpp",
    "OpponentAI.cpp",
    "PlayerProfileSystem.cpp",
    "RatingSystem.cpp",
    "ProgressionSystem.cpp",
    "ScoutSystem.cpp",
    "SimulationData.cpp",
    "SkillSystem.cpp",
    "TrainerProfile.cpp",
]
extension_sources = core_sources + Glob("native/*.cpp")

library = env.SharedLibrary(
    "poke-clone-esports-ui/bin/libpoke_esports{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=extension_sources,
)

def RunBackendTests(target, source, env):
    import subprocess

    return subprocess.call([source[0].abspath])

test_env = env.Clone()
if env.get("platform") == "windows":
    test_env.Append(CXXFLAGS=["/EHsc"])
test_core_objects = [
    test_env.Object(
        "tests/obj/{}".format(os.path.splitext(os.path.basename(source))[0]),
        source,
    )
    for source in core_sources
]
test_main_object = test_env.Object(
    "tests/obj/test_main",
    "tests/test_main.cpp",
)
test_program = test_env.Program(
    "tests/bin/backend_core_tests",
    source=[test_main_object] + test_core_objects,
)
run_backend_tests = test_env.Alias(
    "run_backend_tests",
    test_program,
    Action(RunBackendTests, "Running backend C++ tests"),
)
test_env.AlwaysBuild(run_backend_tests)
test_env.Alias("tests", run_backend_tests)
test_env.Alias("test", run_backend_tests)

env.NoCache(library)
Default(library)
