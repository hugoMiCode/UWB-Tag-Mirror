#include "MessageDecoder.h"

void (*MessageDecoder::_handleCmdResetRace)() = nullptr;
void (*MessageDecoder::_handleCmdStartRace)() = nullptr;

Command MessageDecoder::decodeMessage(const char *message)
{
    // Parse the JSON message
    StaticJsonDocument<200u> doc;
    DeserializationError error = deserializeJson(doc, message);

    // Check for parsing errors
    if (error) {
    #ifdef SERIAL_DEBUG
        Serial.print("Error parsing JSON: ");
        Serial.println(error.c_str());
    #endif
        return Command::NONE;
    }

    // le message doit contenir une clé "cmd" -> si ce n'est pas le cas, on retourne NONE
    if (!doc.containsKey("cmd")) {
        return Command::NONE;
    }
    
    String cmd_str = doc["cmd"];

    // cmd_str doit toujours être un nombre -> si ce n'est pas le cas, on retourne NONE
    if (cmd_str.length() == 0) { //  || !cmd_str.isDigit()
        return Command::NONE;
    }

    Command cmd = (Command)cmd_str.toInt();

    executeCommand(cmd);

    return cmd;
}

void MessageDecoder::executeCommand(Command cmd)
{
    switch (cmd) {
        case Command::RESET_RACE:
            if (_handleCmdResetRace != nullptr) {
                _handleCmdResetRace();
            }
            break;
        case Command::START_RACE:
            if (_handleCmdStartRace != nullptr) {
                _handleCmdStartRace();
            }
            break;
        default:
            break;
    }
}
