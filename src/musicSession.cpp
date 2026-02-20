// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Dimosthenis Krallis


#include "musicSession.h"


MusicSession::MusicSession()
{
    generator.seed(rd());
    sessionId = generateId(5);
    session=
        {
            {"playState", MusicSession::playState::PAUSED},
            {"timeStamp", 0},
            {"playlistPos", 0},
            {"numberOfSongs", 0},
            {"playlistChkSum", 0},
            {"priority" , sessionId}
        };
    playlist.clear();
}

int MusicSession::addIceServer(std::string iceServer)
{
    config.iceServers.emplace_back(iceServer);
    return 0;
}
int MusicSession::connectToSignalingServer(std::string signalingServer)
{
    std::stringstream tmp;
    std::string url;
    ws = std::make_shared<rtc::WebSocket>();
    ws->onMessage([this](rtc::message_variant dat){
        handleSignallingServer(dat);
    });
    tmp << signalingServer << "/" << sessionId;
    url = tmp.str();
    ws->open(url);

    return 0;
}
int MusicSession::connectToPeer(std::string peerId)
{

    if(!ws->isOpen())
    {
        std::cerr << "Hasn't connected to signalling server yet. Waiting\n";
    }
    while(!ws->isOpen());

    auto pc = pcs[peerId] = std::make_shared<rtc::PeerConnection>(config);
    pc->onLocalDescription([this, peerId](rtc::Description desc){
        nlohmann::json msg = {
            {"id",        peerId},
            {"type",      desc.typeString()},
            {"description", std::string(desc)}
        };
        ws->send(msg.dump());
    });

    pc->onLocalCandidate([this, peerId](rtc::Candidate candid){
        nlohmann::json msg = {
            {"id",        peerId},
            {"type",      "candidate"},
            {"candidate", std::string(candid)},
            {"mid",       candid.mid()}
        };
        ws->send(msg.dump());
    });

    auto dc = dcs[peerId] = pc->createDataChannel("music");

    dc->onMessage([this,dc](rtc::message_variant msg){
        if(std::holds_alternative<std::string>(msg))
        {
            std::cout << std::get<std::string>(msg) << std::endl;
            if( interperate(std::get<std::string>(msg),dc))
            {
                std::cerr << "failed to interperate\n";
            }
        }
    });

    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state){
        std::cout << "gathering state: " << state << std::endl;
    });

    pc->onStateChange([](rtc::PeerConnection::State state){
        std::cout << "state: " << state << std::endl;
    });

    return 0;
}
std::string MusicSession::getSessionId()
{
    return sessionId;
}
int MusicSession::forcePullSessionAndPlaylist()
{
    for (auto i : dcs)
    {
        if(i.second->isOpen())
            i.second->send("test");
        else
            return 1;
    }
    return 0;
}
nlohmann::json MusicSession::getPeerSession()
{
    sessionMutex.lock();
    auto sessionTmp = session;
    sessionMutex.unlock();
    return sessionTmp;
}
std::vector<nlohmann::json> MusicSession::getPlaylist()
{
    playListMutex.lock();
    auto playlistTmp = playlist;
    playListMutex.unlock();
    return playlistTmp;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::string MusicSession::generateId(int len)
{
    std::string map ="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::stringstream strm;
    strm.clear();
    for (int i = 0; i < len; i++)
    {
        strm << map[generator()%26];
    }
    return strm.str();
}
void MusicSession::handleSignallingServer(rtc::message_variant data)
{
    if (!std::holds_alternative<std::string>(data))
    {
        std::cerr << "Signaling server returned non string value, ignoring\n";
        return;
    }
    nlohmann::json dat = nlohmann::json::parse(std::get<std::string>(data));
    std::cout << dat.dump(1);
    auto test_id = dat.find("id");
    std::string id = "";
    auto test_type = dat.find("type");
    std::string type = "";
    auto test_description = dat.find("description");
    std::string description = "";
    auto test_candidate = dat.find("candidate");
    std::string candidate = "";
    if (test_id==dat.end())
    {
        return;
    }
    id=test_id->get<std::string>();
    std::shared_ptr<rtc::PeerConnection> pc;
    if (pcs.find(id) == pcs.end())
    {
        pc = pcs[id] = std::make_shared<rtc::PeerConnection>(config); // create if not existant yet
        pc->onLocalDescription([this, id](rtc::Description desc){
            nlohmann::json msg = {
                {"id",        id},
                {"type",      desc.typeString()},
                {"description", std::string(desc)}
            };
            ws->send(msg.dump());
        });

        pc->onLocalCandidate([this, id](rtc::Candidate candid){
            nlohmann::json msg = {
                {"id",        id},
                {"type",      "candidate"},
                {"candidate", std::string(candid)}//,
                //{"mid",       candid.mid()}
            };
            ws->send(msg.dump());
        });
    }
    else
        pc = pcs[id];

    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state){
        std::cout << "gathering state: " << state << std::endl;
    });
    pc->onStateChange([](rtc::PeerConnection::State state){
        std::cout << "state: " << state << std::endl;
    });

    pc->onDataChannel([this,id](std::shared_ptr<rtc::DataChannel> dc){
        //std::cout << "datachannel made!!!!!!!!!!!!!!!!!!!!" << std::endl;
        dcs[id] = dc;
        dc->onMessage([this,dc](rtc::message_variant msg){
            if(std::holds_alternative<std::string>(msg))
            {
                std::cout << std::get<std::string>(msg) << std::endl;

                //while(!dc->isOpen());
                //dc->send("test");
                if( interperate(std::get<std::string>(msg),dc))
                {
                    std::cerr << "failed to interperate\n";
                }
            }
        });
    });


    if(test_type==dat.end())
    {
        return;
    }
    type = test_type->get<std::string>();

    std::cout << "added " << type << std::endl;
    if (type == "answer" || type == "offer") {                  // whether offerer or answerer
        if(test_description != dat.end())
        {
            description = test_description->get<std::string>();
            pc->setRemoteDescription(rtc::Description(description, type));
        }
    }
    if (type == "candidate") {
        if(test_candidate != dat.end())
        {
            candidate = test_candidate->get<std::string>();
            pc->addRemoteCandidate(rtc::Candidate(candidate));
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MusicSession::interperate(std::string message, std::shared_ptr<rtc::DataChannel> dc)
{
    if (!dc->isOpen())
    {
        return 1;
    }

    if(!nlohmann::json::accept(message))
    {
        std::cerr << "Error parsing, ignoring\n";
        return 1;
    }

    auto msg = nlohmann::json::parse(message);



    return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MusicSession::~MusicSession()
{
    for (auto i : dcs)
    {
        i.second->close();
    }
    for (auto i : pcs)
    {
        i.second->close();
    }
    dcs.clear();
    pcs.clear();
}
