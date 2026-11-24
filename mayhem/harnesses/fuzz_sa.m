/*
 * fuzz_sa: the scripting-addition socket (/tmp/yabai-sa_$USER.socket) served by the payload yabai
 * injects into Dock (src/osax/payload.m).
 *
 * The daemon sends an int16 length, an opcode byte (SA_OPCODE_*, src/osax/common.h) and packed
 * binary arguments. payload.m's handle_connection() reads each connection into a
 * SA_SOCKET_BUFF_LEN stack buffer with read_message() and dispatches it with handle_message()
 * to the do_* unpackers. The fuzz input is the raw byte stream a client writes to that socket;
 * the harness writes it to a socketpair and runs the body of handle_connection() on the other end.
 *
 * Linux build: payload.m is compiled as Objective-C (GNUstep-2 ABI) against stand-in headers
 * (mayhem/stubs/sa); there is no Objective-C runtime or SkyLight, so every message send and
 * SkyLight call returns nil/0. The payload's load-time constructor (which binds the real socket
 * and scans Dock's memory for private symbols) is turned into a plain function and never run.
 *
 * Opcode SA_OPCODE_WINDOW_OPACITY_FADE is rejected (-1): it starts a detached animation thread
 * that keeps running (and sleeping) after the input is done, so later inputs would depend on it.
 * State kept across inputs: window_fade_table/lock (initialised once as start_daemon() does;
 * only the rejected fade opcode writes them).
 */
#include <signal.h>
#include <fcntl.h>
#define constructor used
#include "../../src/osax/payload.m"
#undef constructor

static struct { void *isa; } harness_dock_object;
static void harness_dock_function(void) {}  /* add/move space: called via inline asm */
static void harness_remove_space(id space, id display_space, id spaces, uint64_t sid1, uint64_t sid2)
{ (void)space; (void)display_space; (void)spaces; (void)sid1; (void)sid2; }
static void harness_focus_window(ProcessSerialNumber psn, uint32_t wid) { (void)psn; (void)wid; }

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    signal(SIGPIPE, SIG_IGN);
    pthread_mutex_init(&window_fade_lock, NULL);
    table_init(&window_fade_table, 150, hash_wid, compare_wid);
    /* What init_instances() finds by scanning Dock in the real payload: the dock.spaces and
     * desktop-picture-manager instances and Dock's private space/focus functions. Here they are
     * a placeholder object and a no-op function, so the handlers get past their "not found"
     * guards and unpack their full argument lists. */
    dock_spaces = (id) &harness_dock_object;
    dp_desktop_picture_manager = (id) &harness_dock_object;
    add_space_fp = move_space_fp = (uint64_t) &harness_dock_function;
    remove_space_fp = (uint64_t) &harness_remove_space;
    set_front_window_fp = (uint64_t) &harness_focus_window;
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) abort();

    /* the client: inputs above the socket buffer size are cut short by the peer, like a real
     * client whose connection the payload closes after one message */
    int flags = fcntl(fds[0], F_GETFL, 0);
    fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);
    size_t off = 0;
    while (off < size) {
        ssize_t n = write(fds[0], data + off, size - off);
        if (n <= 0) break;
        off += n;
    }
    shutdown(fds[0], SHUT_WR);

    /* the payload: body of handle_connection() */
    int sockfd = fds[1];
    int result = 0;
    char message[SA_SOCKET_BUFF_LEN];
    if (read_message(sockfd, message)) {
        if (message[0] == SA_OPCODE_WINDOW_OPACITY_FADE) {
            result = -1;
        } else {
            handle_message(sockfd, message);
        }
    }
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    close(fds[0]);
    return result;
}
