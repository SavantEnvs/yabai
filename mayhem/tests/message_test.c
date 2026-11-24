/*
 * Functional oracle for the yabai daemon message parser (built by mayhem/build.sh with normal
 * flags). Each case sends `yabai -m` style messages (NUL-separated tokens, as the client packs
 * them) to handle_message() and checks the exact response and the resulting daemon state.
 * Prints one "PASS <name>" / "FAIL <name>" line per case and "SUMMARY <passed> <failed>".
 */
#include "yabai_unity.h"

static int passed, failed;

/* send one message; returns the response (malloc'd) */
static char *send_msg(int argc, const char **argv)
{
    size_t len = 0;
    for (int i = 0; i < argc; ++i) len += strlen(argv[i]) + 1;
    char *message = ts_alloc_unaligned(len + 1);
    char *p = message;
    for (int i = 0; i < argc; ++i) { size_t n = strlen(argv[i]) + 1; memcpy(p, argv[i], n); p += n; }
    *p = '\0';

    char *out = NULL; size_t out_len = 0;
    FILE *rsp = open_memstream(&out, &out_len);
    handle_message(rsp, message);
    fclose(rsp);
    ts_reset();
    return out;
}
#define SEND(...) send_msg(sizeof((const char *[]){__VA_ARGS__}) / sizeof(char *), (const char *[]){__VA_ARGS__})

static void check(const char *name, bool ok, const char *got)
{
    if (ok) { passed++; printf("PASS %s\n", name); }
    else    { failed++; printf("FAIL %s (got: \"%s\")\n", name, got ? got : "(null)"); }
}

static void expect_eq(const char *name, char *got, const char *want)
{
    check(name, got && strcmp(got, want) == 0, got);
    free(got);
}

static void expect_contains(const char *name, char *got, const char *want)
{
    check(name, got && strstr(got, want) != NULL, got);
    free(got);
}

int main(void)
{
    if (!ts_init(MEGABYTES(8))) return 2;
    harness_reset_state();

    expect_eq("config_debug_output_default", SEND("config", "debug_output"), "off\n");
    expect_eq("config_debug_output_set", SEND("config", "debug_output", "on"), "");
    expect_eq("config_debug_output_query", SEND("config", "debug_output"), "on\n");
    expect_eq("config_debug_output_bad_value", SEND("config", "debug_output", "maybe"),
              "\x07unknown value 'maybe' given to command 'debug_output' for domain 'config'\n");
    expect_eq("config_mff_set", SEND("config", "mouse_follows_focus", "on"), "");
    expect_eq("config_mff_query", SEND("config", "mouse_follows_focus"), "on\n");
    expect_eq("unknown_domain", SEND("bogus"), "\x07unknown domain 'bogus'\n");
    expect_eq("unknown_config_command", SEND("config", "no_such_setting"),
              "\x07unknown command 'no_such_setting' for domain 'config'\n");

    expect_eq("rule_add", SEND("rule", "--add", "label=r1", "app=^Foo$", "title=Bar", "manage=off"), "");
    check("rule_add_state", buf_len(g_window_manager.rules) == 1
          && g_window_manager.rules[0].label && strcmp(g_window_manager.rules[0].label, "r1") == 0
          && g_window_manager.rules[0].app && strcmp(g_window_manager.rules[0].app, "^Foo$") == 0
          && g_window_manager.rules[0].title && strcmp(g_window_manager.rules[0].title, "Bar") == 0
          && rule_check_flag(&g_window_manager.rules[0], RULE_APP_VALID)
          && regexec(&g_window_manager.rules[0].app_regex, "Foo", 0, NULL, 0) == 0
          && regexec(&g_window_manager.rules[0].app_regex, "xFoo", 0, NULL, 0) != 0, NULL);
    expect_contains("rule_add_bad_regex", SEND("rule", "--add", "app=[unclosed"), "\x07");
    check("rule_add_bad_regex_state", buf_len(g_window_manager.rules) == 1, NULL);
    expect_eq("rule_remove", SEND("rule", "--remove", "r1"), "");
    check("rule_remove_state", buf_len(g_window_manager.rules) == 0, NULL);
    expect_eq("rule_remove_missing", SEND("rule", "--remove", "r1"), "\x07rule with label 'r1' not found.\n");

    expect_eq("signal_add", SEND("signal", "--add", "event=window_created", "action=echo hi", "label=s1", "app=Term"), "");
    expect_eq("signal_list", SEND("signal", "--list"),
              "[{\n\t\"index\":0,\n\t\"label\":\"s1\",\n\t\"app\":\"Term\",\n\t\"title\":\"\",\n\t\"active\":null,\n\t\"event\":\"window_created\",\n\t\"action\":\"echo hi\"\n}]\n");
    expect_contains("signal_add_bad_event", SEND("signal", "--add", "event=not_an_event", "action=true"), "\x07");
    expect_eq("signal_remove", SEND("signal", "--remove", "s1"), "");
    expect_eq("signal_list_empty", SEND("signal", "--list"), "[]\n");

    printf("SUMMARY %d %d\n", passed, failed);
    return failed ? 1 : 0;
}
