#!/usr/bin/env python
import sys

# Build godot-cpp first to get the bindings
env = SConscript("godot-cpp/SConstruct") 

# build the actual module
env.Append(CPPPATH=["addons/src/", "llama.cpp/include", "llama.cpp/ggml/include"])
sources = Glob("addons/src/*.cpp")

platform_ext = {"macos": ".dylib", "linux": ".so", "windows": ".dll"}
ext = platform_ext.get(env["platform"], ".so")

# Output path: bin/<platform>/<target>/libgdllama.<platform>.<target>.<ext>
out_path = "addons/bin/{}/{}/libgdllama.{}.{}{}".format(
    env["platform"],          # macos / linux / windows
    env["target"],            # debug / release
    env["platform"],          # part of filename
    env["target"],            # part of filename
    ext                       # .dylib/.so/.dll
)

# Link Llama.cpp static library
env.Append(LIBPATH=[
    "build-llama/bin",
    "build-llama/src",                  # libllama.a
    "build-llama/ggml/src",             # libggml, libggml-base, libggml-cpu
    "build-llama/ggml/src/ggml-metal",  # libggml-metal.a
    "build-llama/ggml/src/ggml-blas"    # libggml-blas.a
])

env.Append(LIBS=[ "llama", "ggml", "ggml-base", "ggml-cpu", "ggml-metal", "ggml-blas"])# Core llama library + ggml backends
env.Append(CPPDEFINES=["LLAMA_STATIC"]) # Define static linking option if needed

if env["platform"] == "macos":
    env.Append(LINKFLAGS=["-framework", "Accelerate", "-framework", "Metal", "-framework", "Foundation"])
elif env["platform"] == "linux":
    env.Append(LIBS=["pthread", "m", "dl"])
elif env["platform"] == "windows":
    env.Append(LIBS=["user32", "kernel32"])

library = env.SharedLibrary(out_path, source=sources)
Default(library)