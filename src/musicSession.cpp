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
    return 0;
}
int MusicSession::connectToPeer(std::string peerId)
{
    return 0;
}
std::string MusicSession::getSessionId()
{
    return sessionId;
}
int MusicSession::forcePullSessionAndPlaylist()
{
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

MusicSession::~MusicSession()
{

}
