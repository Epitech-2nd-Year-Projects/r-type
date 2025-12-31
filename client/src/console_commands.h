#ifndef CLIENT_CONSOLE_COMMANDS_H_
#define CLIENT_CONSOLE_COMMANDS_H_

namespace engine::ecs {
class Registry;
}

namespace client {

class Application;

void RegisterConsoleCommands(Application& app);

}  // namespace client

#endif  // CLIENT_CONSOLE_COMMANDS_H_

