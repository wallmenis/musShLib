# musShLib

A music session sharing library based on WebRTC.

### Note:
No actual music transfer for copyright reasons.

-----------------------------------------------

## Usage:
Just drag and include the source files in your project.

The signalling server communication is designed to be used with the signalling server
of the examples of `libdatachannel`.

## Dependencies:
- [`libdatachannel`](https://github.com/paullouisageneau/libdatachannel)
- [`nlohmann_json`](https://github.com/nlohmann/json)

You may install them with your favorite package manager.
Personally, I use Fedora linux and used 
```
sudo dnf install libdatachannel-devel json-devel
```

If you want, you may help with the project by showing me how to compile with those
libraries from source.

## Documentation:
- `static std::shared_ptr<MusicSession> create();`
Called like `MusicSession::create()`, this is a factory of music sessions.
It is how you are meant to start a session.
- `int addIceServer(std::string iceServer);`
The library uses WebRTC. In WebRTC you need 2 types of servers, a signalling and an
ice server. The signalling server is for coordination of who connects to where and
the ice server is for *how* connects to where via the [ICE](https://en.wikipedia.org/wiki/Interactive_Connectivity_Establishment)
protocol. Remember to add one otherwise, other than localhost, you won't be able
to connect. Google has some you can use. Also [Coturn](https://github.com/coturn/coturn/)
is agood alternative.
- `int connectToSignalingServer(std::string signalingServer);` Signaling or Signalling
(I looked it up, both are correct) server is the server that decides who connects to
where. The library is designed to work with `libdatachannel`'s example signalling
servers. I recommend the python version since that is with what I am testing with.
- `int connectToPeer(std::string peerId);`
Connects to another instance for synchronization. Doesn't synchronize though. You
have to choose when to start synchronizing. with either `int forcePullSessionAndPlaylist();`
or `int assertState(nlohmann::json msg);`.
- `std::string getSessionId();` returns the session ID (5 letter string telling who
you are in the connection).
- `int forcePullSessionAndPlaylist();`
Sends a packet to get the session of the other peers we are connected to.
Doesn't do anything for playlist but it may trigger playlist regeneration if the
hash of the playlist and number of songs in it are different.
- `nlohmann::json getPeerSession();`
With this, you get the state of your session in json form which you can mutate for
later usage.
Here is an example state:
```
{
    "playState": 1,
    "timeStamp": 0,
    "playlistPos": 0,
    "numberOfSongs": 0,
    "playlistChkSum": 0,
    "priority" : "AABBC"
}
```
- `std::vector<nlohmann::json> getPlaylist();`
With this method you get the playlist in a form of a vector of json entries.
These json entries look like this:
```
{ 
    "track" : "trackName",
    "url"   : "trackUrl",
    "hash"  : "trackHash"
}
```
- `int getPlaylistHash();` This calculates the hash of the playlist. Now why you
would want that, I don't know. Feel free to hack together different stuff : )
It is used internally for the calculation.
- `int addTrack(nlohmann::json track);`
`int addTrackFront(nlohmann::json track);`
`int addTrackAt(nlohmann::json track,std::vector<nlohmann::json>::const_iterator pos);`
These methods are used to add tracks. The format for a track is the same as in
`std::vector<nlohmann::json> getPlaylist();`. The position in the vector where the
track is placed is implied by the name of the function. That is except of `addTrack`
where it is just added at the last position. We also use iterators for ease of placement.
- `int removeTrackAt(std::vector<nlohmann::json>::const_iterator pos);` 
`int removeTrackBack();`
`int removeTrackFront();`
Just like in the `addTrack` commands we just saw, the `removeTrack` commands remove
tracks from the vector. The name implies from where to remove from.
- `int setSession(nlohmann::json sesh);`
This sets the session details according to the json object provided. Sessions look
exactly like as shown in `getPeerSession`.
- `int assertState(nlohmann::json msg);`
This sets the session details according to the json object provided with 
the field "priority" set as our own session id. Then it sends the state to every
connection.
- `int getNumberOfConnections();`
This returns the number of connections (datachannels) that are active.
- `bool isConnectedToSignallingServer();`
This returns `true` if we are connected to the signalling server.
Otherwise, it returns `false`.
- `bool isSafeToSend();`
This returns `true` if there are more than 0 connections enabled.
Otherwise, `false`.
- `int updateTime(int time);`
This updates the field "timeStamp" in our current session. It is there to avoid
boilerplate since it is expected that that field is going to be updated constantly.

- `int delay;`
This property (default value: 100) defines the +- window of leeway we give to when
to update based on time. The time interval is arbitrary as long as you are fine with
integer percision.

- `enum playState{
    PLAYING,
    PAUSED
};
` This is the enumerator type of the states that represent playing a track or not.
They are used to fill the field "playState" in our session.
