target("gid_server")

    -- set kind: binary
    set_kind("binary")

    -- set default: disable
    set_default(false)

    -- add deps: acl_cpp, acl
    add_deps("acl_cpp", "acl")

    -- add source files
    add_files("src/**.c")
    if is_plat("windows") then
        add_files("win32/*.c")
    else
        add_files("unix/*.c")
    end

    -- add include directories
    add_includedirs(".", "include")

