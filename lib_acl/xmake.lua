-- define target: acl
target("acl")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add source files
    add_files("src/**.c|*/bak/*.c")

    -- add include directories
    add_includedirs(".")
    add_includedirs("include", {public = true})

    -- add headers
    add_headerfiles("include/(**.h)")

    -- add defines
    add_defines("USE_REUSEPORT")
