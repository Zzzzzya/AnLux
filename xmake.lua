
set_project("AnLux")

set_version("0.1.0")

add_rules("mode.debug")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode"})

add_requires("vcpkg::fmt")
add_requires("vcpkg::directx-headers")

target("AnLux")
    set_kind("binary")
    add_files("src/main.cpp")

    add_packages("vcpkg::fmt")
    add_packages("vcpkg::directx-headers")

    add_links("User32","Gdi32")
    add_links("d3d12","dxgi","d3dcompiler","dxguid")
    
    

    