import argparse

def create_cmake_lists(project_name, version, cpp_std, sources):
    template = f"""cmake_minimum_required(VERSION 3.10)
project({project_name} VERSION {version})

# Set C++ standard
set(CMAKE_CXX_STANDARD {cpp_std})

# Add source files
set(SOURCE_FILES
    {" ".join(sources)}
)

# Add executable
add_executable({project_name} ${{SOURCE_FILES}})
"""

    with open("CMakeLists.txt", "w") as file:
        file.write(template)
    print("CMakeLists.txt has been created successfully.")

def main():
    parser = argparse.ArgumentParser(description="Generate a CMakeLists.txt file for a C++ project.")
    parser.add_argument("project_name", type=str, help="The name of the project.")
    parser.add_argument("--version", type=str, default="1.0", help="Version of the project.")
    parser.add_argument("--cpp_std", type=str, default="c++17", help="C++ standard version.")
    parser.add_argument("--sources", type=str, nargs='+', default=["main.cpp"], help="List of source files.")

    args = parser.parse_args()

    create_cmake_lists(args.project_name, args.version, args.cpp_std, args.sources)

if __name__ == "__main__":
    main()
