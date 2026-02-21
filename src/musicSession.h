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
#include <random>


class MusicSession{
public:
    MusicSession();
    int addIceServer(std::string iceServer);
    int connectToSignalingServer(std::string signalingServer);
    int connectToPeer(std::string peerId);
    std::string getSessionId();
    int forcePullSessionAndPlaylist();
    nlohmann::json getPeerSession();
    std::vector<nlohmann::json> getPlaylist();
    std::string getPlaylistHash();

    enum playState{
        PLAYING,
        PAUSED
    };

    ~MusicSession();
private:
    std::shared_ptr<rtc::WebSocket> ws;
    std::unordered_map<std::string,std::shared_ptr<rtc::DataChannel>> dcs;
    std::unordered_map<std::string,std::shared_ptr<rtc::PeerConnection>> pcs;

    std::string sessionId;
    std::mutex playListMutex;
    std::mutex sessionMutex;
    std::mutex dcsMutex;
    std::mutex pcsMutex;
    nlohmann::json session;
    std::vector<nlohmann::json> playlist;
    std::random_device rd;
    std::mt19937 generator;
    rtc::Configuration config;

    std::string generateId(int len);

    void handleSignallingServer(rtc::message_variant data);
    int interpret(std::string,std::string);
    //int cleanUpConnections();
};

#endif
