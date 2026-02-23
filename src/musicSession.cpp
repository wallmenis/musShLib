// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Dimosthenis Krallis


#include "musicSession.h"


MusicSession::MusicSession()
{
    delay = 100;
    wsConnected = false;
    connections = 0;
    generator.seed(rd());
    sessionId = generateId(5);
    playlistInWriteMode = false;
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

std::shared_ptr<MusicSession> MusicSession::create()
{
    return std::make_shared<MusicSession>();
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
    auto self = shared_from_this();
    ws->onOpen([self](){self->wsConnected=true;});
    ws->onClosed([self](){self->wsConnected=false;});
    ws->onMessage([self](rtc::message_variant dat){
        self->handleSignallingServer(dat);
    });
    tmp << signalingServer << "/" << sessionId;
    url = tmp.str();
    ws->open(url);

    return 0;
}
int MusicSession::connectToPeer(std::string peerId)
{

    if(!wsConnected)
    {
        std::cerr << "Hasn't connected to signalling server yet. Try again later.\n";
        return 1;
    }
    pcsMutex.lock();
    auto pc = pcs[peerId] = std::make_shared<rtc::PeerConnection>(config);
    pcsMutex.unlock();
    auto self = shared_from_this();
    pc->onLocalDescription([self, peerId](rtc::Description desc){
        nlohmann::json msg = {
            {"id",        peerId},
            {"type",      desc.typeString()},
            {"description", std::string(desc)}
        };
        self->ws->send(msg.dump());
    });

    pc->onLocalCandidate([self, peerId](rtc::Candidate candid){
        nlohmann::json msg = {
            {"id",        peerId},
            {"type",      "candidate"},
            {"candidate", std::string(candid)}//,
            //{"mid",       candid.mid()}           //commented out because we just use datachannels for now
        };
        self->ws->send(msg.dump());
    });


    dcsMutex.lock();
    auto dc = dcs[peerId] = pc->createDataChannel("music");
    dcsMutex.unlock();
    dc->onOpen([self](){self->connections++;});
    dc->onClosed([self, peerId](){
        self->dcsMutex.lock();
        self->dcs.erase(peerId);
        self->dcsMutex.unlock();
        self->connections--;
    });
    dc->onMessage([self,peerId](rtc::message_variant msg){
        if(std::holds_alternative<std::string>(msg))
        {
            std::cout << std::get<std::string>(msg) << std::endl;
            if( self->interpret(std::get<std::string>(msg),peerId))
            {
                std::cerr << "failed to interperate\n";
            }
        }
    });

    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state){
        std::cout << "gathering state: " << state << std::endl;
    });
    pc->onStateChange([self, peerId](rtc::PeerConnection::State state){
        std::cout << "state: " << state << std::endl;
        if (//false
            rtc::PeerConnection::State::Closed == state ||
            rtc::PeerConnection::State::Disconnected == state ||
            rtc::PeerConnection::State::Failed == state
            )
        {
            std::cout << peerId << " disconnected\n";
            std::lock_guard<std::mutex> mtxpc(self->pcsMutex);
            self->pcs.erase(peerId);
        }
    });

    return 0;
}
std::string MusicSession::getSessionId()
{
    return sessionId;
}
int MusicSession::forcePullSessionAndPlaylist()
{
    nlohmann::json msg = {{"askSync", getSessionId()}};
    std::lock_guard<std::mutex>mtxdc(dcsMutex);
    for (auto i : dcs)
    {
        if(i.second->isOpen())
            i.second->send(msg.dump());
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
int MusicSession::getPlaylistHash()
{
    auto plist = getPlaylist();
    std::hash<std::string> hasher;
    int hash = 0;
    std::stringstream strm;
    for (auto i : plist)
    {
        strm << i.dump();
    }
    hash = hasher(strm.str())%0x7FFFFFFF;
    return hash;
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
    if(!nlohmann::json::accept(std::get<std::string>(data)))
    {
        std::cerr << "Signalling server returned bad text, ignoring\n";
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
    auto self = shared_from_this();
    pcsMutex.lock();
    if (pcs.find(id) == pcs.end())
    {
        pc = pcs[id] = std::make_shared<rtc::PeerConnection>(config); // create if not existant yet
        pcsMutex.unlock();
        pc->onLocalDescription([self, id](rtc::Description desc){
            nlohmann::json msg = {
                {"id",        id},
                {"type",      desc.typeString()},
                {"description", std::string(desc)}
            };
            self->ws->send(msg.dump());
        });

        pc->onLocalCandidate([self, id](rtc::Candidate candid){
            nlohmann::json msg = {
                {"id",        id},
                {"type",      "candidate"},
                {"candidate", std::string(candid)}//,
                //{"mid",       candid.mid()}
            };
            self->ws->send(msg.dump());
        });

        pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state){
            std::cout << "gathering state: " << state << std::endl;
        });
        pc->onStateChange([self, id](rtc::PeerConnection::State state){
            std::cout << "state: " << state << std::endl;
            if (//false
                rtc::PeerConnection::State::Closed == state ||
                rtc::PeerConnection::State::Disconnected == state ||
                rtc::PeerConnection::State::Failed == state
                )
            {
                std::cout << id << " disconnected\n";
                std::lock_guard<std::mutex> mtxpc(self->pcsMutex);
                self->pcs.erase(id);
            }
        });
    }
    else
    {
        pc = pcs[id];
        pcsMutex.unlock();
    }



    pc->onDataChannel([self,id](std::shared_ptr<rtc::DataChannel> dc){
        //std::cout << "datachannel made!!!!!!!!!!!!!!!!!!!!" << std::endl;
        self->dcsMutex.lock();
        self->dcs[id] = dc;
        self->dcsMutex.unlock();
        dc->onOpen([self](){self->connections++;});
        dc->onClosed([self,id](){
            self->connections--;
            self->dcsMutex.lock();
            self->dcs.erase(id);
            self->dcsMutex.unlock();
        });
        dc->onMessage([self,id](rtc::message_variant msg){
            if(std::holds_alternative<std::string>(msg))    // May use dcs[id] inside instead and pass the ID. I find it cleaner that way instead of weak pointers.
            {
                std::cout << std::get<std::string>(msg) << std::endl;

                if( self->interpret(std::get<std::string>(msg),id))
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
int MusicSession::addTrack(nlohmann::json track)
{
    playListMutex.lock();
    playlist.emplace_back(track);
    playListMutex.unlock();
    std::lock_guard<std::mutex> mtxsc(sessionMutex);
    session["playlistChkSum"] = getPlaylistHash();
    std::lock_guard<std::mutex> mtxpl(playListMutex);   //it lives until addTrack dies;
    return playlist.size();
}
int MusicSession::setSession(nlohmann::json sesh)
{
    std::lock_guard<std::mutex> mtxsc(sessionMutex);
    session = sesh;
    return 0;
}
bool MusicSession::onTime(nlohmann::json msg, int timeDifference)
{
    nlohmann::json sesh = getPeerSession();
    if (sesh["priority"] != msg["priority"])
        return false;
    if (sesh["playlistPos"] != msg["playlistPos"])
        return false;
    if (sesh["numberOfSongs"] != msg["numberOfSongs"])
        return false;
    if (sesh["playlistChkSum"] != msg["playlistChkSum"])
        return false;
    if (sesh["playState"] != msg["playState"])
        return false;
    if (std::abs(msg["timeStamp"].get<int>() - sesh["timeStamp"].get<int>() ) >timeDifference)
        return false;
    return true;
}
int MusicSession::assertState(nlohmann::json msg)
{
    auto toSend = msg;
    toSend["priority"] = getSessionId();
    std::lock_guard<std::mutex> mtxdc(dcsMutex);
    for (auto i : dcs)
    {
        i.second->send(toSend.dump());
    }
    return dcs.size();
}
int MusicSession::getNumberOfConnections()
{
    return connections;
}
bool MusicSession::isConnectedToSignallingServer()
{
    return wsConnected;
}
bool MusicSession::isSafeToSend()
{
    return getNumberOfConnections() > 0 || isConnectedToSignallingServer();
}
int MusicSession::updateTime(int time)
{
    int oldTime = 0;
    std::lock_guard<std::mutex> mtxsc(sessionMutex);
    oldTime = session["timeStamp"];
    session["timeStamp"] = time;
    return oldTime;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MusicSession::interpret(std::string message, std::string id)
{
    dcsMutex.lock();
    auto test_dc = dcs.find(id);
    if (test_dc == dcs.end())
    {
        dcsMutex.unlock();
        std::cerr << "Invalid id: "<< id <<" ignoring\n";
        return 1;
    }
    auto dc = test_dc->second;
    dcsMutex.unlock();
    if (!dc->isOpen())
    {
        std::cerr << "Datachannel not open yet, ignoring\n";
        return 1;
    }

    if(!nlohmann::json::accept(message))
    {
        std::cerr << "Error parsing, ignoring\n";
        return 1;
    }

    auto msg = nlohmann::json::parse(message);

    MusicSession::messageType type = identify(msg);
    int phash = getPlaylistHash();
    int pnum = getPlaylist().size();

    std::cout << type << std::endl;

    if(type == messageType::SESSION)
    {
        auto sesh = getPeerSession();
        if(msg["priority"]==getSessionId())
            dc->send(sesh.dump());
        else if(!onTime(msg,delay))
            setSession(msg);
        if (phash != msg["playlistChkSum"] || pnum != msg["numberOfSongs"])
        {
            nlohmann::json toSend = {{"sendPlaylist", phash}};
            dc->send(toSend.dump());
            playlistInWriteMode = true;
        }
        return 0;
    }

    if(type == messageType::ASKPLAYLIST)
    {
        auto pl = getPlaylist();
        for (auto i : pl)
        {
            dc->send(i.dump());
        }
        return 0;
    }

    if(type == messageType::ASKSYNC)
    {
        auto sesh = getPeerSession();
        dc->send(sesh.dump());
        return 0;
    }

    if(type == messageType::OK)
    {
        nlohmann::json toSend = {{"sendPlaylist", phash}};
        if (msg["ok"] == phash)
            playlistInWriteMode = false;
        else
            dc->send(toSend.dump());
        return 0;
    }

    if(type == messageType::TRACK && playlistInWriteMode)
    {
        addTrack(msg);
        return 0;
    }

    return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MusicSession::messageType MusicSession::identify(nlohmann::json msg)
{
    bool isSession =
        getIfFieldIsString(msg,"priority") &&
        getIfFieldIsInteger(msg, "timeStamp") &&
        getIfFieldIsInteger(msg, "playlistPos") &&
        getIfFieldIsInteger(msg, "numberOfSongs") &&
        getIfFieldIsInteger(msg, "playlistChkSum") &&
        getIfFieldIsInteger(msg, "playState") &&
        msg.size() == 6;
    bool isTrack =
        getIfFieldIsString(msg, "track") &&
        getIfFieldIsString(msg, "hash") &&
        getIfFieldIsString(msg, "url") &&
        msg.size() == 3;
    bool isOk =
        getIfFieldIsInteger(msg, "ok") &&
        msg.size() == 1;
    bool isAskSync =
        getIfFieldIsString(msg, "askSync") &&
        msg.size() == 1;
    bool isAskPlaylist =
        getIfFieldIsInteger(msg, "sendPlaylist") &&
        msg.size() == 1;
    if(isSession)
        return messageType::SESSION;
    if(isTrack)
        return messageType::TRACK;
    if(isOk)
        return messageType::OK;
    if(isAskPlaylist)
        return messageType::ASKPLAYLIST;
    if(isAskSync)
        return messageType::ASKSYNC;

    return messageType::INVALID;
}

bool MusicSession::getIfFieldIsInteger(nlohmann::json msg, std::string field)
{
    if(msg.empty())
    {
        return false;
    }
    auto it = msg.find(field);
    if(it == msg.end())
    {
        return false;
    }
    if(!msg.find(field).value().is_number_integer())
    {
        return false;
    }
    return true;
}

bool MusicSession::getIfFieldIsString(nlohmann::json msg, std::string field)
{
    if(msg.empty())
    {
        return false;
    }
    auto it = msg.find(field);
    if(it == msg.end())
    {
        return false;
    }
    if(!msg.find(field).value().is_string())
    {
        return false;
    }
    return true;
}

MusicSession::~MusicSession()
{
    std::lock_guard<std::mutex>mtxdc(dcsMutex);     //this may be useless but I don't want to risk stuff.
    std::lock_guard<std::mutex>mtxpc(pcsMutex);
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
