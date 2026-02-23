#include "musicSession.h"
#include <iostream>
//#include <memory>
#include <unistd.h>


int main(int argc, char *argv[])
{
    //std::shared_ptr<MusicSession> mu = std::make_shared<MusicSession>();
    auto mu = MusicSession::create();
    std::cout << mu->getSessionId() << std::endl;
    mu->addIceServer("127.0.0.1:3479");
    mu->connectToSignalingServer("127.0.0.1:8000");
    std::string otherPear;
    std::stringstream inpStrm;

    if(argc > 1)
    {
        for (int i = 0; i < 5; i++)
        {
            inpStrm << argv[1][i];
        }
        otherPear = inpStrm.str();
        while(!mu->isConnectedToSignallingServer());
        std::cout << "sending\n";

        mu->connectToPeer(otherPear);
        while (mu->getNumberOfConnections() == 0);
        mu->forcePullSessionAndPlaylist();
    }
    else
    {
        nlohmann::json track = {
            {"track", "gamer"},
            {"hash", "gaming"},
            {"url", "the game"}
        };
        mu->addTrack(track);
    }
    while(1)
    {
        std::cout << mu->getPeerSession().dump(4);
        sleep(1);
    }
    return 0;
}
