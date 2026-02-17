// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Dimosthenis Krallis

#ifndef MUSICSESSION_H
#define MUSICSESSION_H

#include "rtc/rtc.hpp"

#include <nlohmann/json_fwd.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <rtc/configuration.hpp>
#include <stdexcept>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <atomic>


class MusicSession{
public:
    MusicSession();
    int connectToSignalingServer(std::string signalingServer);
    int connectToPeer(std::string peerId);
    std::string getSessionId();
    int forcePullSessionAndPlaylist();
    nlohmann::json getPeerSession();
    std::vector<nlohmann::json> getPlaylist();

    enum playState{
        PLAYING,
        PAUSED
    };

    ~MusicSession();
private:
    std::string sessionId;
    std::mutex playListMutex;
    std::mutex sessionMutex;
    nlohmann::json session;
    std::vector<nlohmann::json> playlist;

    std::string generateId();
};

#endif
