#!/usr/bin/env python

# godot-cpp supplies the platform-specific compiler flags and links its static
# bindings library into our GDExtension DLL.
env = SConscript("godot-cpp/SConstruct")
env.Append(CPPPATH=[".", "native"])

sources = [
    "BattleRules.cpp",
    "BattleSession.cpp",
    "OpponentAI.cpp",
    "PlayerProfileSystem.cpp",
    "RatingSystem.cpp",
    "ProgressionSystem.cpp",
    "SimulationData.cpp",
    "SkillSystem.cpp",
    "TrainerProfile.cpp",
]
sources += Glob("native/*.cpp")

library = env.SharedLibrary(
    "poke-clone-esports-ui/bin/libpoke_esports{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

env.NoCache(library)
Default(library)
