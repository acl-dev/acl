target("qrserver")

    -- set kind: binary
    set_kind("binary")

    -- set default: disable
    set_default(false)

    -- add deps: acl_cpp, acl
    add_deps("acl_cpp", "acl")

    -- add source files
    add_files("**.cpp")

    -- add include directories
    add_includedirs(".")

