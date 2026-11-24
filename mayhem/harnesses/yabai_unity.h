/* Unity build of the yabai daemon message parser for Linux (see fuzz_message.c). */
#ifndef YABAI_UNITY_H
#define YABAI_UNITY_H
#include "apple_stubs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <spawn.h>
#include <poll.h>

#include "workspace_decls.h"
#include "../../src/misc/extern.h"
#include "../../src/misc/macros.h"
#include "../../src/misc/memory_pool.h"
#include "../../src/misc/ts.h"
#include "../../src/misc/log.h"
#include "../../src/misc/helpers.h"
#include "../../src/misc/timer.h"
#include "../../src/misc/sbuffer.h"
#define HASHTABLE_IMPLEMENTATION
#include "../../src/misc/hashtable.h"
#undef HASHTABLE_IMPLEMENTATION
#include "../../src/osax/common.h"
#include "../../src/view.h"
#include "../../src/sa.h"
#include "../../src/event_loop.h"
#include "../../src/event_signal.h"
#include "../../src/rule.h"
#include "../../src/message.h"
#include "../../src/display.h"
#include "../../src/space.h"
#include "../../src/window.h"
#include "../../src/process_manager.h"
#include "../../src/application.h"
#include "../../src/display_manager.h"
#include "../../src/space_manager.h"
#include "../../src/window_manager.h"
#include "../../src/mouse_handler.h"

#include "../../src/mission_control.c"
#include "../../src/event_signal.c"
#include "../../src/rule.c"
#include "../../src/message.c"

/* globals normally defined in src/yabai.c */
struct signal *g_signal_event[SIGNAL_TYPE_COUNT];
struct process_manager g_process_manager;
struct display_manager g_display_manager;
struct window_manager g_window_manager;
struct space_manager g_space_manager;
struct memory_pool g_signal_storage;
struct mouse_state g_mouse_state;
struct event_loop g_event_loop;
void *g_workspace_context;
enum mission_control_mode g_mission_control_mode;
uint64_t g_mission_control_sid;
double g_cv_host_clock_frequency;
int g_layer_normal_window_level;
int g_layer_below_window_level;
int g_layer_above_window_level;
uint8_t *g_event_bytes;
char g_sa_socket_file[MAXLEN];
char g_socket_file[MAXLEN];
char g_config_file[4096];
char g_lock_file[MAXLEN];
mach_port_t g_bs_port;
int g_connection;
bool g_verbose;
pid_t g_pid;

#include "harness_state.h"
#endif
