
#define NOB_IMPLEMENTATION
#include "nob.h/nob.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#define NOB_EXE_EXT ".exe"
#else
#define NOB_EXE_EXT ""
#endif

static const char *nob_compiler(void) {
    const char *cc = getenv("CC");
    return cc && cc[0] ? cc : "cc";
}

#define BUILD_DIR "build"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};

    if (!nob_mkdir_if_not_exists(BUILD_DIR)) return 1;

    if (argc >= 2 && strcmp(argv[1], "test") == 0) {
        nob_cmd_append(&cmd, nob_compiler(), "-Wall", "-Wextra", "-Os", "-g");
        if (getenv("ALOCTR_USE_MALLOC")) nob_cmd_append(&cmd, "-DALOCTR_USE_MALLOC");
        nob_cmd_append(&cmd, "-o", BUILD_DIR "/test_aloctr" NOB_EXE_EXT, "tests/test_aloctr.c");
        if (!nob_cmd_run(&cmd)) return 1;
        Nob_Cmd run = {0};
        nob_cmd_append(&run, "./" BUILD_DIR "/test_aloctr" NOB_EXE_EXT);
        return nob_cmd_run(&run) ? 0 : 1;
    }

    nob_cmd_append(&cmd, nob_compiler(), "-Wall", "-Wextra", "-Os", "-g");
    if (getenv("ALOCTR_USE_MALLOC")) nob_cmd_append(&cmd, "-DALOCTR_USE_MALLOC");
    nob_cmd_append(&cmd, "-o", BUILD_DIR "/basic" NOB_EXE_EXT, "examples/basic.c");
    if (!nob_cmd_run(&cmd)) return 1;
    return 0;
}

