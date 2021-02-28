import os

def normalize_path(path: str):
    return path.replace("\\", "/")

WEB_DIRECTORY = "../bin/Web"

CLI_DIRECTORY = "../Storm-Cli/src"
SOURCE_DIRECTORY = "../Storm-Lib/src"
SOURCE_FILES = []

def find_source_files_in_directory(directory):
    for path in os.listdir(directory):
        full_path = os.path.join(directory, path)
        if os.path.isdir(full_path):
            find_source_files_in_directory(full_path)
        else:
            name, ext = os.path.splitext(path)
            if ext == ".cpp":
                SOURCE_FILES.append(os.path.join(directory, path))

find_source_files_in_directory(SOURCE_DIRECTORY)

if __name__ == "__main__":
    os.makedirs(WEB_DIRECTORY, exist_ok=True)
    current_directory = os.getcwd()

    command_line = "em++ --bind -o {} -std=c++17 -I{} -s ALLOW_MEMORY_GROWTH=1 -s MODULARIZE=1 -s WASM=1 -O3 -DEMSCRIPTEN -DSTORM_DIST -DSTORM_PLATFORM_LINUX -s EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]"
    command_line = command_line.format(normalize_path(os.path.join(WEB_DIRECTORY, "Storm.js")), normalize_path(SOURCE_DIRECTORY))

    for source_file in SOURCE_FILES:
        command_line += " {}".format(normalize_path(source_file))
    command_line += " {}".format(normalize_path("Emscripten.cpp"))

    print(command_line)
    os.system(command_line)
