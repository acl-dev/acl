-- project
set_project("acl")

-- version
--set_version("3.5.0")
--set_xmakever("2.1.6")

-- set warning all as error
set_warnings("all", "error")

-- the debug or release mode
add_rules("mode.debug", "mode.release")
if is_mode("release") and is_plat("android", "iphoneos") then
    set_optimize("smallest")
end

-- add common flags and macros
add_defines("ACL_WRITEABLE_CHECK", "ACL_PREPARE_COMPILE")

-- for the windows platform (msvc)
if is_plat("windows") then 
    add_ldflags("-nodefaultlib:\"msvcrt.lib\"")
end
-- for the windows platform (msvc)
if is_plat("windows") then 
    if is_mode("release") then
        add_cxflags("-MT") 
    elseif is_mode("debug") then
        add_cxflags("-MTd") 
    end
    add_cxxflags("-EHsc")
	add_syslinks("ws2_32", "IPHlpApi", "kernel32", "user32", "gdi32")
end

if is_mode("release") then
    set_symbols("debug")
end

-- for the android platform
if is_plat("android") then
    add_defines("ANDROID")
end

-- for macosx
if is_plat("macosx") then
    add_defines("MACOSX")
    add_links("pthread", "z")
end

-- for linux
if is_plat("linux") then
    add_defines("LINUX2")
    add_links("pthread", "z", "dl")
end

-- for mingw
if is_plat("mingw") then
    add_defines("LINUX2", "MINGW")
end

-- for freebsd
if is_plat("freebsd") then
    add_defines("FREEBSD")
end

-- for Solaris (x86)
if is_plat("sunos5") then
    add_defines("SUNOS5")
end

-- for all non-windows platforms
if not is_plat("windows") then
    add_cflags("-Wshadow",
            "-Wpointer-arith",
            "-Waggregate-return",
            "-Wmissing-prototypes",
            "-Wno-long-long",
            "-Wuninitialized",
            "-Wstrict-prototypes",
            "-fdata-sections",
            "-ffunction-sections",
            "-fPIC",
            "-fno-rtti",
            "-fno-exceptions",
            "-fomit-frame-pointer"
            )
    add_cxxflags("-Wshadow",
            "-Wpointer-arith",
            "-Wno-long-long",
            "-Wuninitialized",
            "-fdata-sections",
            "-ffunction-sections",
            "-fPIC",
            "-fno-rtti",
            "-fno-exceptions",
            "-fomit-frame-pointer"
            )

    if is_kind("static") then
    	add_cxflags("-fvisibility-inlines-hidden")

        --add_cflags("-flto")
        --add_cxxflags("-flto")

        if not is_plat("android") then
            add_cflags("-flto")
            add_cxxflags("-flto")
        end
    end
    add_defines("_REENTRANT", "_USE_FAST_MACRO", "_POSIX_PTHREAD_SEMANTICS", "_GNU_SOURCE=1")
    if is_plat("android") then
        add_defines("ACL_CLIENT_ONLY")
    end
    add_defines("ACL_PREPARE_COMPILE")
    add_defines("ANDROID")
    add_defines("NDEBUG")
    add_defines("acl_cpp_EXPORTS")
    add_cflags("fno-addrsig")
    add_cxxflags("fno-addrsig")
    --add_cflags("-MD", "-MT", "-MF")
    --add_cxxflags("-MD", "-MT", "-MF")
    add_cflags("-no-canonical-prefixes")
    add_cxxflags("-no-canonical-prefixes")
    add_cflags("-fno-addrsig")
    add_cxxflags("-fno-addrsig")
end

-- include project sources
includes("app/**/xmake.lua", "lib_acl", "lib_protocol", "lib_acl_cpp") 
if is_plat("linux") and not is_plat("android") then
    includes("lib_fiber/c", "lib_fiber/cpp")
end

-- Build project (static library/release by default)
--
-- linux/macosx/windows:
-- $ xmake
--
-- iphoneos:
-- $ xmake f -p iphoneos
-- $ xmake
--
-- android:
-- $ xmake f -p android --ndk=/home/xxx/android-ndk-r10e
-- $ xmake
--
-- freebsd:
-- $ xmake f -p freebsd
-- $ xmake
--
-- mingw:
-- $ xmake f -p mingw
-- $ xmake
--
-- Build for share library with debug mode
--
-- $ xmake f -k shared -m debug; xmake
--
-- Build and run app example
--
-- $ xmake run [gson|iconv|wizard|master_ctl|master_daemon|...]
--
-- Configuration 
--
-- $ xmake f -p [windows|linux|iphoneos|android|macosx|freebsd|sunos5|cross] -m [debug|release] -a [x86|x64|x86_64|armv7|arm64|armv8-a] -k [static|shared]
--
-- Generate IDE project
--
-- $ xmake project -k vs2008
-- $ xmake project -k vs2017 -m "debug,release"
-- $ xmake project -k makefile
--
-- If you want to known more usage about xmake, please see: http://xmake.io/#/home?id=configuration
--
