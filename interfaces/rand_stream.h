#ifndef INTERFACES_RAND_STREAM_H
#define INTERFACES_RAND_STREAM_H

// Major version of the rand_stream interface, changed when an enumerator is removed or renumbered.
#define RAND_STREAM_VERSION_MAJOR 1
// Minor version of the rand_stream interface, changed when an enumerator is added.
#define RAND_STREAM_VERSION_MINOR 0

// The random stream a draw of the game logic comes from; 3.1c keeps one per thread.
enum RandStream {
    // The stream of the main thread, seeded in GameMain.
    RAND_STREAM_MAIN = 0,
    // The stream of the load thread, seeded in LoadMatch.
    RAND_STREAM_LOAD = 1
};

#endif
