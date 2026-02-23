// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Dimosthenis Krallis

#ifndef MUSICSESSION_H
#define MUSICSESSION_H

#include "rtc/rtc.hpp"

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


class MusicSession : public std::enable_shared_from_this<MusicSession>{
public:
    MusicSession();
    static std::shared_ptr<MusicSession> create();
    int addIceServer(std::string iceServer);
    int connectToSignalingServer(std::string signalingServer);
    int connectToPeer(std::string peerId);
    std::string getSessionId();
    int forcePullSessionAndPlaylist();
    nlohmann::json getPeerSession();
    std::vector<nlohmann::json> getPlaylist();
    int getPlaylistHash();
    int addTrack(nlohmann::json track);
    int addTrackFront(nlohmann::json track);
    int addTrackAt(nlohmann::json track,std::vector<nlohmann::json>::const_iterator pos);
    int removeTrackAt(std::vector<nlohmann::json>::const_iterator pos);
    int removeTrackBack();
    int removeTrackFront();
    int setSession(nlohmann::json sesh);
    int assertState(nlohmann::json msg);
    int getNumberOfConnections();
    bool isConnectedToSignallingServer();
    bool isSafeToSend();
    int updateTime(int time);

    int delay;

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
    std::atomic<bool> playlistInWriteMode;
    std::atomic<int> connections;
    std::atomic<bool> wsConnected;

    enum messageType
    {
        SESSION,
        TRACK,
        OK,
        ASKSYNC,
        ASKPLAYLIST,
        INVALID
    };

    std::string generateId(int len);

    void handleSignallingServer(rtc::message_variant data);
    int interpret(std::string,std::string);

    messageType identify(nlohmann::json msg);
    bool onTime(nlohmann::json msg, int timeDifference);


    static bool getIfFieldIsInteger(nlohmann::json msg, std::string field);
    static bool getIfFieldIsString(nlohmann::json msg, std::string field);

    //int cleanUpConnections();
};

#endif
