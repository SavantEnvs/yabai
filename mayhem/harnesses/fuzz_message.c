/*
 * fuzz_message: the yabai daemon command socket (/tmp/yabai_$USER.socket).
 *
 * `yabai -m ...` connects to the daemon and sends an int32 length followed by that many bytes of
 * NUL-separated tokens. The daemon's DAEMON_MESSAGE handler (src/event_loop.c) reads the frame
 * into temporary storage (misc/ts.h) and passes it to handle_message() (src/message.c), which
 * tokenizes it and dispatches to the config/display/space/window/query/rule/signal domains
 * (rule parsing + regcomp in src/rule.c, signals in src/event_signal.c).
 *
 * The fuzz input is the message body. The harness frames it exactly like the client does
 * (int32 length = input size), sends it over a socketpair, and runs the same read loop as the
 * DAEMON_MESSAGE handler before calling handle_message() with the response stream on the socket.
 * The length is set from the input size because a declared length larger than the 8 MiB
 * temporary storage makes ts_assert_within_bounds() exit() by design (upstream 297156c028).
 *
 * yabai is macOS-only: Apple framework calls are stubbed (mayhem/stubs) to report that no
 * displays, spaces or windows exist, and the other yabai modules are not linked (their
 * functions resolve to zero-returning stubs, generated at build time).
 *
 * State kept across inputs: the 8 MiB temporary storage (reset and zeroed after every input);
 * the window manager / rule / signal globals are re-zeroed and re-initialised per input.
 */
#include "yabai_unity.h"

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc; (void)argv;
    signal(SIGPIPE, SIG_IGN);
    if (!ts_init(MEGABYTES(8))) abort();
    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size > MEGABYTES(4)) return -1;  /* keep the frame well inside temporary storage */

    harness_reset_state();

    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) abort();

    /* client side: writer thread, so frames larger than the socket buffer don't deadlock */
    int length = (int) size;
    struct harness_writer w = { fds[0], data, size, length };
    pthread_t thread;
    pthread_create(&thread, NULL, harness_writer_main, &w);

    /* daemon side: same read loop as EVENT_HANDLER(DAEMON_MESSAGE) in src/event_loop.c */
    int param1 = fds[1];
    int bytes_read = 0, bytes_to_read = 0;
    if (read(param1, &bytes_to_read, sizeof(int)) == sizeof(int)) {
        char *message = ts_alloc_unaligned(bytes_to_read);
        do {
            int cur_read = read(param1, message+bytes_read, bytes_to_read-bytes_read);
            if (cur_read <= 0) break;
            bytes_read += cur_read;
        } while (bytes_read < bytes_to_read);

        FILE *rsp;
        if ((bytes_read == bytes_to_read) && (rsp = fdopen(param1, "w"))) {
            handle_message(rsp, message);
            fflush(rsp);
            fclose(rsp);
            param1 = -1;
        }
    }
    if (param1 != -1) close(param1);

    pthread_join(thread, NULL);
    close(fds[0]);

    memset(g_temp_storage.memory, 0, g_temp_storage.used < g_temp_storage.size ? g_temp_storage.used : g_temp_storage.size);
    ts_reset();
    return 0;
}
