#include <stdio.h>
#include "structs.h"
#include "sish.h"

int main() {
    Error program_result = sish();
    if (program_result.is_ok) {

    } else {
        printf("Error (%d): %s\n", program_result.error_code, program_result.error_string);
    }
    return 0;
}