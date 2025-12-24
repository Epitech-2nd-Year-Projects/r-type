target("protocol")
    set_kind("static")
    set_default(false)
    add_files("src/*.cpp")
    add_includedirs("include", {public = true})
    add_headerfiles("include/protocol/**.h|**.hpp")
    add_deps("engine_net", "engine_core")

