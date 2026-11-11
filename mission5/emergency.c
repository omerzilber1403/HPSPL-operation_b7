#include "emergency.h"

#include <stdlib.h>
#include <string.h>

/* 1 if s has a '\0' inside its first capacity chars (bounded strlen check). */
static int fits_in(const char *s, size_t capacity)
{
    for (size_t i = 0; i < capacity; i++) {
        if (s[i] == '\0')
            return 1;
    }
    return 0;
}

static void clear_options(LaunchOptions *out)
{
    out->device[0] = '\0';
    out->mode[0] = '\0';
    out->action[0] = '\0';
}

/* Copies the three pairs into out; returns 0 on the first bad argument. */
static int fill_options(int argc, char *const argv[], LaunchOptions *out)
{
    for (int i = 1; i < argc; i += 2) {
        const char *option = argv[i];
        const char *value = argv[i + 1];
        char *field;
        const char *expected;

        if (option == NULL || value == NULL)
            return 0;
        if (strcmp(option, "--device") == 0) {
            field = out->device;
            expected = "BEERSHEVANATOR";
        } else if (strcmp(option, "--mode") == 0) {
            field = out->mode;
            expected = "emergency";
        } else if (strcmp(option, "--action") == 0) {
            field = out->action;
            expected = "self-destruct";
        } else {
            return 0;
        }
        /* a field that is already filled means the option appeared twice */
        if (field[0] != '\0' || strcmp(value, expected) != 0)
            return 0;
        strcpy(field, value); /* value equals expected, which fits its field */
    }
    return 1;
}

int parse_launch_options(int argc, char *const argv[], LaunchOptions *out)
{
    if (out == NULL)
        return 0;
    clear_options(out);

    /* program name + exactly three option/value pairs */
    if (argc != 7 || argv == NULL || !fill_options(argc, argv, out)) {
        clear_options(out);
        return 0;
    }
    return 1;
}

const AuthorizationRecord *find_authorization(const AuthorizationRecord *records, size_t count,
                                              int request_id, int unit_id, int required_clearance)
{
    if (records == NULL || request_id <= 0 || unit_id <= 0 ||
        required_clearance < 0 || required_clearance > 5)
        return NULL;

    for (size_t i = 0; i < count; i++) {
        const AuthorizationRecord *record = &records[i];
        /* matching positive ids, approved == 0 and clearance in
         * [required, 5] also cover the "valid record" rules */
        if (record->request_id == request_id && record->unit_id == unit_id &&
            record->approver_id > 0 && record->approved == 0 &&
            record->clearance >= required_clearance && record->clearance <= 5)
            return record;
    }
    return NULL;
}

int emergency_request_init(EmergencyRequest *dest,
                           const LaunchOptions *options,
                           const AuthorizationRecord *authorization,
                           const int *steps, size_t step_count)
{
    /* never overwrite a request that still owns memory */
    if (dest == NULL || dest->device != NULL || dest->steps != NULL)
        return 0;
    if (options == NULL || authorization == NULL || steps == NULL ||
        step_count < 1 || step_count > 8)
        return 0;
    if (!fits_in(options->device, M5_DEVICE_CAPACITY) || options->device[0] == '\0' ||
        !fits_in(options->mode, M5_MODE_CAPACITY) ||
        !fits_in(options->action, M5_ACTION_CAPACITY))
        return 0;

    /* sizes are bounded (device < 32, step_count <= 8), so no overflow */
    char *device = malloc(strlen(options->device) + 1);
    int *steps_copy = malloc(step_count * sizeof(*steps_copy));
    if (device == NULL || steps_copy == NULL) {
        free(device);
        free(steps_copy);
        return 0;
    }
    strcpy(device, options->device);
    for (size_t i = 0; i < step_count; i++)
        steps_copy[i] = steps[i];

    dest->request_id = authorization->request_id;
    dest->unit_id = authorization->unit_id;
    dest->authorization = *authorization; /* struct copy by value */
    dest->device = device;
    strcpy(dest->mode, options->mode);
    strcpy(dest->action, options->action);
    dest->steps = steps_copy;
    dest->step_count = step_count;
    return 1;
}

void emergency_request_free(EmergencyRequest *request)
{
    EmergencyRequest empty = {0};

    if (request == NULL)
        return;
    free(request->device);
    free(request->steps);
    *request = empty;
}

int validate_emergency_request(const EmergencyRequest *request)
{
    if (request == NULL || request->device == NULL || request->steps == NULL ||
        request->step_count != 3)
        return 0;
    /* comparing against a shorter literal stops before the array ends,
     * so mode/action are never read out of bounds */
    if (strcmp(request->device, "BEERSHEVANATOR") != 0 ||
        strcmp(request->mode, "emergency") != 0 ||
        strcmp(request->action, "self-destruct") != 0)
        return 0;

    const AuthorizationRecord *auth = &request->authorization;
    if (request->request_id <= 0 || request->unit_id <= 0 || auth->approver_id <= 0 ||
        auth->request_id != request->request_id || auth->unit_id != request->unit_id ||
        auth->approved != 1 ||
        auth->clearance < M5_ADMIN_CLEARANCE || auth->clearance > 5)
        return 0;

    return request->steps[0] == STEP_PREPARE_EMERGENCY &&
           request->steps[1] == STEP_REQUEST_AUTHORIZATION &&
           request->steps[2] == STEP_EXECUTE_COMMAND;
}
