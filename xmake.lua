
set_project("AnLux")

set_version("0.1.0")

add_rules("mode.debug", "mode.release")

add_requires("vcpkg::fmt")

target("AnLux")
    set_kind("binary")
    add_files("src/main.cpp")
    add_packages("vcpkg::fmt")
    

    