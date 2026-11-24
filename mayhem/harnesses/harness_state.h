/* Per-input state reset + socket writer for fuzz_message.c */
static TABLE_HASH_FUNC(harness_hash_u32) { return *(uint32_t *) key; }
static TABLE_COMPARE_FUNC(harness_cmp_u32) { return *(uint32_t *) key_a == *(uint32_t *) key_b; }

static void harness_init_tables(void)
{
    /* mirrors window_manager_init / space_manager_init / process_manager_init table setup */
    table_init(&g_window_manager.application, 150, harness_hash_u32, harness_cmp_u32);
    table_init(&g_window_manager.window, 150, harness_hash_u32, harness_cmp_u32);
    table_init(&g_window_manager.managed_window, 150, harness_hash_u32, harness_cmp_u32);
    table_init(&g_window_manager.window_lost_focused_event, 150, harness_hash_u32, harness_cmp_u32);
    table_init(&g_window_manager.application_lost_front_switched_event, 150, harness_hash_u32, harness_cmp_u32);
    table_init(&g_window_manager.window_animations_table, 150, harness_hash_u32, harness_cmp_u32);
    table_init(&g_window_manager.insert_feedback, 150, harness_hash_u32, harness_cmp_u32);
    table_init(&g_space_manager.view, 23, harness_hash_u32, harness_cmp_u32);
    table_init(&g_process_manager.process, 125, harness_hash_u32, harness_cmp_u32);
}

static struct window_manager harness_wm0;
static struct space_manager harness_sm0;
static struct display_manager harness_dm0;
static struct process_manager harness_pm0;
static struct mouse_state harness_ms0;
static bool harness_initialised;

static void harness_reset_state(void)
{
    if (!harness_initialised) {
        if (!memory_pool_init(&g_signal_storage, KILOBYTES(256))) abort();
        harness_init_tables();
        harness_wm0 = g_window_manager; harness_sm0 = g_space_manager;
        harness_dm0 = g_display_manager; harness_pm0 = g_process_manager;
        harness_ms0 = g_mouse_state;
        harness_initialised = true;
        return;
    }

    for (int i = 0; i < buf_len(g_window_manager.rules); ++i) rule_destroy(&g_window_manager.rules[i]);
    buf_free(g_window_manager.rules);
    for (int i = 0; i < SIGNAL_TYPE_COUNT; ++i) {
        for (int j = 0; j < buf_len(g_signal_event[i]); ++j) event_signal_destroy(&g_signal_event[i][j]);
        buf_free(g_signal_event[i]);
        g_signal_event[i] = NULL;
    }
    g_signal_storage.used = 0;

    g_window_manager = harness_wm0; g_space_manager = harness_sm0;
    g_display_manager = harness_dm0; g_process_manager = harness_pm0;
    g_mouse_state = harness_ms0;
    g_verbose = false;
    g_mission_control_mode = 0;
    g_mission_control_sid = 0;
}

struct harness_writer { int fd; const uint8_t *data; size_t size; int length; };

static void *harness_writer_main(void *arg)
{
    struct harness_writer *w = arg;
    if (write(w->fd, &w->length, sizeof(int)) == sizeof(int)) {
        size_t off = 0;
        while (off < w->size) {
            ssize_t n = write(w->fd, w->data + off, w->size - off);
            if (n <= 0) break;
            off += n;
        }
    }
    shutdown(w->fd, SHUT_WR);
    /* drain the daemon's response so it never blocks on a full socket buffer */
    char buf[4096];
    while (read(w->fd, buf, sizeof(buf)) > 0) {}
    return NULL;
}
