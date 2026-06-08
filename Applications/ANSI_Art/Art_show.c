/**
 * @file Art_show.c
 * @author Pcb-yun (pcbyinyun@163.com)
 * @brief ASCII Art渲染源码
 */

#include "Art_show.h"
#include "shell.h"
#include <string.h>
#include "log.h"

/**
 * @brief ASCII Art渲染函数
 * @param name ASCII Art图片名称
 * @return 渲染状态
 */
bool Art_show(const char *name) {
    const char *data = NULL;
    if (!find_art(name, &data)) {
        return false;
    }

    Shell *shell = shellGetCurrent();
    ART_OUT(shell, data, strlen(data));
    return true;
}

static void Show_shell(int argc, char *argv[]) {
    if (argc < 2) {
        logPrintln("Usage: show <name>\r\n"
                   "Available names:");
        Art_list();
        return;
    }
    if (!Art_show(argv[1])) {
        logPrintln("show failed");
    }
}
SHELL_EXPORT_CMD(
SHELL_CMD_PERMISSION(0)|SHELL_CMD_TYPE(SHELL_TYPE_CMD_MAIN)|SHELL_CMD_DISABLE_RETURN,
Art, Show_shell, Show ASCII Art image);
