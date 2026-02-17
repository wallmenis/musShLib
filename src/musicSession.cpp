// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Dimosthenis Krallis


#include "musicSession.h"


MusicSession::MusicSession()
{
    sessionId = generateId();
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

std::string MusicSession::generateId()
{
    std::hash<int> hash;
    return "";
}
