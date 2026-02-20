/**
 * Unit-тесты парсера команд (cmd_parse).
 * Сборка: pio run -e native
 */
#include "cmd_parse.h"
#include "unity.h"
#include <string.h>

void setUp(void) {}
void tearDown(void) {}

static void test_empty_and_whitespace(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(0, cmd_parse("", &r));
    TEST_ASSERT_EQUAL(CMD_NONE, r.type);

    TEST_ASSERT_EQUAL(0, cmd_parse("  \r\n", &r));
    TEST_ASSERT_EQUAL(CMD_NONE, r.type);
}

static void test_status_commands(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(0, cmd_parse("?", &r));
    TEST_ASSERT_EQUAL(CMD_STATUS, r.type);

    TEST_ASSERT_EQUAL(0, cmd_parse("STATUS", &r));
    TEST_ASSERT_EQUAL(CMD_STATUS, r.type);

    TEST_ASSERT_EQUAL(0, cmd_parse("  ?  ", &r));
    TEST_ASSERT_EQUAL(CMD_STATUS, r.type);
}

static void test_on_off_commands(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(0, cmd_parse("ON", &r));
    TEST_ASSERT_EQUAL(CMD_ON, r.type);

    TEST_ASSERT_EQUAL(0, cmd_parse("START", &r));
    TEST_ASSERT_EQUAL(CMD_ON, r.type);

    TEST_ASSERT_EQUAL(0, cmd_parse("OFF", &r));
    TEST_ASSERT_EQUAL(CMD_OFF, r.type);

    TEST_ASSERT_EQUAL(0, cmd_parse("STOP", &r));
    TEST_ASSERT_EQUAL(CMD_OFF, r.type);
}

static void test_help_command(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(0, cmd_parse("HELP", &r));
    TEST_ASSERT_EQUAL(CMD_HELP, r.type);
}

static void test_freq_valid(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(0, cmd_parse("FREQ 1000", &r));
    TEST_ASSERT_EQUAL(CMD_FREQ, r.type);
    TEST_ASSERT_EQUAL_UINT32(1000, r.freq);

    TEST_ASSERT_EQUAL(0, cmd_parse("FREQ 1", &r));
    TEST_ASSERT_EQUAL(CMD_FREQ, r.type);
    TEST_ASSERT_EQUAL_UINT32(1, r.freq);

    TEST_ASSERT_EQUAL(0, cmd_parse("FREQ 40000000", &r));
    TEST_ASSERT_EQUAL(CMD_FREQ, r.type);
    TEST_ASSERT_EQUAL_UINT32(40000000, r.freq);
}

static void test_freq_invalid(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(-2, cmd_parse("FREQ 0", &r));
    TEST_ASSERT_EQUAL(-2, cmd_parse("FREQ 40000001", &r));
    TEST_ASSERT_EQUAL(-2, cmd_parse("FREQ 99999999", &r));
}

static void test_duty_valid(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(0, cmd_parse("DUTY 0", &r));
    TEST_ASSERT_EQUAL(CMD_DUTY, r.type);
    TEST_ASSERT_EQUAL_UINT8(0, r.duty);

    TEST_ASSERT_EQUAL(0, cmd_parse("DUTY 50", &r));
    TEST_ASSERT_EQUAL(CMD_DUTY, r.type);
    TEST_ASSERT_EQUAL_UINT8(50, r.duty);

    TEST_ASSERT_EQUAL(0, cmd_parse("DUTY 100", &r));
    TEST_ASSERT_EQUAL(CMD_DUTY, r.type);
    TEST_ASSERT_EQUAL_UINT8(100, r.duty);
}

static void test_duty_invalid(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(-2, cmd_parse("DUTY 101", &r));
    TEST_ASSERT_EQUAL(-2, cmd_parse("DUTY 255", &r));
}

static void test_unknown_command(void)
{
    cmd_result_t r;
    TEST_ASSERT_EQUAL(-1, cmd_parse("UNKNOWN", &r));
    TEST_ASSERT_EQUAL(-1, cmd_parse("FREQ", &r));
    TEST_ASSERT_EQUAL(-1, cmd_parse("DUTY", &r));
    TEST_ASSERT_EQUAL(-1, cmd_parse("FREQ 1000 2000", &r));
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_empty_and_whitespace);
    RUN_TEST(test_status_commands);
    RUN_TEST(test_on_off_commands);
    RUN_TEST(test_help_command);
    RUN_TEST(test_freq_valid);
    RUN_TEST(test_freq_invalid);
    RUN_TEST(test_duty_valid);
    RUN_TEST(test_duty_invalid);
    RUN_TEST(test_unknown_command);
    return UNITY_END();
}

