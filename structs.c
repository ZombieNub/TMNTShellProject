#include "structs.h"

Option new_some(void* value_ptr) {
    Option opt;
    opt.is_some = 1;
    opt.value_ptr = value_ptr;
    return opt;
}

Option new_none() {
    Option opt;
    opt.is_some = 0;
    return opt;
}

Error new_ok(void* value_ptr) {
    Error err;
    err.is_ok = 1;
    err.value_ptr = value_ptr;
    return err;
}

Error new_err(int error_code, char* error_string) {
    Error err;
    err.is_ok = 0;
    err.error_code = error_code;
    err.error_string = error_string;
    return err;
}