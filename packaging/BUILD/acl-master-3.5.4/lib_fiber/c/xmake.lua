-- define target: fiber
target("fiber")

    -- set kind: static/shared
    set_kind("$(kind)")

    -- add source files
    add_files("src/**.c")

    -- add include directories
    add_includedirs("src")
    add_includedirs("include", {public = true})

    -- add headers
    add_headerfiles("include/(**.h)")

    -- add flags
    add_cxflags("-std=gnu99")
    add_defines("USE_JMP")

