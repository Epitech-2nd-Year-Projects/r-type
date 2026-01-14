includes("game_logic", "client", "server")

target("rift")
    set_kind("phony")
    set_default(false)
    add_deps("rift_game_logic", "rift_client", "rift_server")
