#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gpiod.h>

#define CHIP        "/dev/gpiochip0"
#define GPIO_OUT    17
#define GPIO_IN     27
#define CONSUMER    "gpio_toggle"
#define ITERATIONS  5

int main(void)
{
    struct gpiod_chip *chip;
    struct gpiod_line_settings *settings;
    struct gpiod_line_config *line_cfg;
    struct gpiod_request_config *req_cfg;
    struct gpiod_line_request *out_req;
    struct gpiod_line_request *in_req;
    unsigned int out_offset = GPIO_OUT;
    unsigned int in_offset  = GPIO_IN;
    enum gpiod_line_value val;
    int i;

    /* 1. Open chip */
    chip = gpiod_chip_open(CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }
    printf("Opened %s\n", CHIP);

    /* ── Configure OUTPUT (GPIO17) ── */
    settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);

    line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &out_offset, 1, settings);

    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, CONSUMER);

    out_req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!out_req) {
        perror("request output");
        gpiod_chip_close(chip);
        return 1;
    }

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    /* ── Configure INPUT (GPIO27) ── */
    settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);

    line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &in_offset, 1, settings);

    req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, CONSUMER);

    in_req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (!in_req) {
        perror("request input");
        gpiod_line_request_release(out_req);
        gpiod_chip_close(chip);
        return 1;
    }

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(line_cfg);
    gpiod_request_config_free(req_cfg);

    printf("GPIO17=output, GPIO27=input (pull-down)\n\n");

    /* ── Toggle loop ── */
    for (i = 0; i < ITERATIONS; i++) {

        /* HIGH */
        gpiod_line_request_set_value(out_req, GPIO_OUT, GPIOD_LINE_VALUE_ACTIVE);
        usleep(100000);
        val = gpiod_line_request_get_value(in_req, GPIO_IN);
        printf("GPIO17=HIGH → GPIO27=%d %s\n", val,
               val == GPIOD_LINE_VALUE_ACTIVE ? "OK" : "FAIL");

        /* LOW */
        gpiod_line_request_set_value(out_req, GPIO_OUT, GPIOD_LINE_VALUE_INACTIVE);
        usleep(100000);
        val = gpiod_line_request_get_value(in_req, GPIO_IN);
        printf("GPIO17=LOW  → GPIO27=%d %s\n", val,
               val == GPIOD_LINE_VALUE_INACTIVE ? "OK" : "FAIL");

        printf("---\n");
    }

    /* ── Cleanup ── */
    gpiod_line_request_release(out_req);
    gpiod_line_request_release(in_req);
    gpiod_chip_close(chip);

    printf("Done.\n");
    return 0;
}
