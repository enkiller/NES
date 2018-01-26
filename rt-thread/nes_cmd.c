#include <rtthread.h>

#if defined(RT_USING_GUIENGINE) && defined(RT_USING_FINSH)
#include <shell.h>

extern rt_err_t nes_runing(int argc, char** argv);

static int nes(int argc, char** argv)
{
    return nes_runing(argc, argv);

}
FINSH_FUNCTION_EXPORT_ALIAS(nes, __cmd_nes, run nes game simulator);

#endif