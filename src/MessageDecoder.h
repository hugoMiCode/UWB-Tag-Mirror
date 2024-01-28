#include <ArduinoJson.h>

/*
    Commande:
    reset la course.
    démarrer une course.
    calibrer les Anchors.
*/

enum class Command {
    NONE = 0,
    RESET_RACE = 1,
    START_RACE = 2
    // TODO: ajouter les autres commandes pour calibrer les Anchors
};

// TODO: mettre dans la class les fonctions a appeler en fonction de la commande
// comme dans IRReceiver.h
// TODO: améliorer la robustesse du décodeur

class MessageDecoder {
public:
    Command decodeMessage(const char* message);
    void executeCommand(Command cmd);

    static void attachCmdResetRace(void (*handleCmdResetRace)()) {
        _handleCmdResetRace = handleCmdResetRace;
    };

    static void attachCmdStartRace(void (*handleCmdStartRace)()) {
        _handleCmdStartRace = handleCmdStartRace;
    };


private:
    static void (*_handleCmdResetRace)();
    static void (*_handleCmdStartRace)();
};
