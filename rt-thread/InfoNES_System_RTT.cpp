#include <rtthread.h>
#include <string.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME  "[NES]"
//#define DBG_LEVEL         DBG_LOG
#define DBG_LEVEL         DBG_INFO
//#define DBG_LEVEL         DBG_WARNING
//#define DBG_LEVEL         DBG_ERROR
//#define DBG_COLOR
#include <rtdbg.h>

#if defined(PKG_USING_GUIENGINE)

#define NES_THREAD_STACK_SIZE     (2048)
#define NES_THREAD_PRIORITY       (RT_THREAD_PRIORITY_MAX * 4 / 5)
#define NES_THREAD_TICK           (10)

#define NES_APP_THREAD_STACK_SIZE (2048)
#define NES_APP_THREAD_PRIORITY   (NES_THREAD_PRIORITY - 1)
#define NES_APP_THREAD_TICK       (20)         


#define NES_STATUS_RUN            (0x44)
#define NES_STATUS_PAUSE          (0x22)
#define NES_STATUS_EXIT           (0x11)
#define NES_STATUS_STOP           (0x00)

#define NES_FRAME_RATE            (60)


#define NES_KEY_RIGHT             (RTGUIK_d)
#define NES_KEY_LEFT              (RTGUIK_a)
#define NES_KEY_DOWN              (RTGUIK_s)
#define NES_KEY_UP                (RTGUIK_w)
#define NES_KEY_START             (RTGUIK_i)
#define NES_KEY_SELECT            (RTGUIK_o)
#define NES_KEY_A                 (RTGUIK_k)
#define NES_KEY_B                 (RTGUIK_l)

//#define NES_KEY_RIGHT             (RTGUIK_RIGHT)
//#define NES_KEY_LEFT              (RTGUIK_LEFT)
//#define NES_KEY_DOWN              (RTGUIK_DOWN)
//#define NES_KEY_UP                (RTGUIK_UP)
//#define NES_KEY_START             (RTGUIK_s)
//#define NES_KEY_SELECT            (RTGUIK_a)
//#define NES_KEY_A                 (RTGUIK_z)
//#define NES_KEY_B                 (RTGUIK_x)


#include <rtgui/rtgui.h>
#include <rtgui/rtgui_system.h>
#include <rtgui/rtgui_app.h>
#include <rtgui/image.h>
#include <rtgui/widgets/window.h>
#include <rtgui/dc.h>

struct _nes_des
{
    struct rtgui_app *app;
    struct rtgui_win *win;
    struct rtgui_dc *frame_dc;
    rt_thread_t app_tid;
    rt_thread_t nes_tid;
    void *rom_data;
    rt_base_t status;
    rt_uint32_t dwKeyPad1;
    rt_uint32_t dwKeyPad2;
    rt_uint32_t dwKeySystem;
};

static struct _nes_des nes_des;

static rt_err_t rtgui_cmd_send(rt_uint32_t type, rt_uint32_t command);
static void nes_app_delete(void);
/*-------------------------------------------------------------------*/
/*  InfoNES_system.c                                                 */
/*-------------------------------------------------------------------*/
#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_pAPU.h"
#include "super_marie_nes.c"
/*-------------------------------------------------------------------*/
/*  Palette data                                                     */
/*-------------------------------------------------------------------*/
WORD NesPalette[64] =
{
  0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
  0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
  0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
  0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
  0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
  0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
  0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
};

/*-------------------------------------------------------------------*/
/*  Function prototypes                                              */
/*-------------------------------------------------------------------*/
/* Menu screen */
int InfoNES_Menu()
{
    rt_kprintf("\n");
    rt_kprintf("---------------------------\n");
    rt_kprintf("|           NES            |\n");
    rt_kprintf("|     W            I  O    |\n");
    rt_kprintf("|  A     D                 |\n");
    rt_kprintf("|     S          K  L      |\n");
    rt_kprintf("|                          |\n");
    rt_kprintf("---------------------------\n");
    rt_kprintf("\n");

    return 0;
}

/* Read ROM image file */
int InfoNES_ReadRom(const char *pszFileName)
{
    char *p_data = (char *)(nes_des.rom_data);

    memcpy(&NesHeader, p_data, 1 * sizeof(NesHeader));
    p_data += 1 * sizeof(NesHeader);
    if (memcmp(NesHeader.byID, "NES\x1a", 4) != 0)
    {
        /* not .nes file */
        return -1;
    }
    memset(SRAM, 0, SRAM_SIZE);

    if (NesHeader.byInfo1 & 4)
    {
        memcpy(&SRAM[0x1000], p_data, 1 * 512);
        p_data += 1 * 512;
    }

    ROM = (BYTE *)rt_malloc(NesHeader.byRomSize * 0x4000);
    memcpy(ROM, p_data, NesHeader.byRomSize * 0x4000);
    p_data += NesHeader.byRomSize * 0x4000;

    if (NesHeader.byVRomSize > 0)
    {
        /* Allocate Memory for VROM Image */
        VROM = (BYTE *)rt_malloc(NesHeader.byVRomSize * 0x2000);
        memcpy(VROM, p_data, NesHeader.byVRomSize * 0x2000);
        p_data += NesHeader.byVRomSize * 0x2000;
    }

    if (nes_des.rom_data)
    {
        rt_free(nes_des.rom_data);
        nes_des.rom_data = RT_NULL;
    }

    return 0;
}

/* Release a memory for ROM */
void InfoNES_ReleaseRom()
{
    if (ROM)
    {
        rt_free(ROM);
        ROM = RT_NULL;
    }

    if (VROM)
    {
        rt_free(VROM);
        VROM = RT_NULL;
    }
}

/* Transfer the contents of work frame on the screen */
void InfoNES_LoadFrame()
{
    static int delay;
    static rt_tick_t old_tick;

    rtgui_cmd_send(nes_des.status, (rt_uint32_t)WorkFrame);

    dbg_log(DBG_LOG, "delay:%d\n", delay);

    delay = (RT_TICK_PER_SECOND / NES_FRAME_RATE) - (rt_tick_get() - old_tick);
    if (delay < 1) delay = 1;
    rt_thread_delay(delay);
    old_tick = rt_tick_get();
}

/* Get a joypad state */
void InfoNES_PadState(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem)
{
	*pdwPad1 = nes_des.dwKeyPad1;
    *pdwPad2 = nes_des.dwKeyPad2;
    *pdwSystem = nes_des.dwKeySystem;
}

/* memcpy */
void *InfoNES_MemoryCopy(void *dest, const void *src, int count)
{
    memcpy(dest, src, count);
    return dest;
}

/* memset */
void *InfoNES_MemorySet(void *dest, int c, int count)
{
    memset(dest, c, count);
    return dest;
}

/* Print debug message */
void InfoNES_DebugPrint(char *pszMsg)
{
    rt_kprintf(pszMsg);
}

/* Wait */
void InfoNES_Wait()
{
}

/* Sound Initialize */
void InfoNES_SoundInit(void)
{
}

/* Sound Open */
int InfoNES_SoundOpen(int samples_per_sync, int sample_rate)
{
    return 0;
}

/* Sound Close */
void InfoNES_SoundClose(void)
{
}

/* Sound Output 5 Waves - 2 Pulse, 1 Triangle, 1 Noise, 1 DPCM */
void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5)
{
}

/* Print system message */
void InfoNES_MessageBox(char *pszMsg, ...)
{
}



static rt_err_t rtgui_cmd_send(rt_uint32_t type, rt_uint32_t command)
{
    struct rtgui_event_command cmd;

    if (nes_des.app)
    {
        RTGUI_EVENT_COMMAND_INIT(&cmd);

        cmd.wid = RT_NULL;
        cmd.type = type;
        cmd.command_id = command;

        return rtgui_send(nes_des.app, &(cmd.parent), sizeof(struct rtgui_event_command));
    }

    return RT_ERROR;
}

static void key_down(struct rtgui_event_kbd *key_event)
{
    switch (key_event->key)
    {
    case NES_KEY_RIGHT:
        nes_des.dwKeyPad1 |= (1 << 7);
        break;

    case NES_KEY_LEFT:
        nes_des.dwKeyPad1 |= (1 << 6);
        break;

    case NES_KEY_DOWN:
        nes_des.dwKeyPad1 |= (1 << 5);
        break;

    case NES_KEY_UP:
        nes_des.dwKeyPad1 |= (1 << 4);
        break;

    case NES_KEY_START:
        /* Start */
        nes_des.dwKeyPad1 |= (1 << 3);
        break;

    case NES_KEY_SELECT:
        /* Select */
        nes_des.dwKeyPad1 |= (1 << 2);
        break;

    case NES_KEY_A:
        /* 'A' */
        nes_des.dwKeyPad1 |= (1 << 1);
        break;

    case NES_KEY_B:
        /* 'B' */
        nes_des.dwKeyPad1 |= (1 << 0);
        break;
    }
}

void key_up(struct rtgui_event_kbd *key_event)
{
    switch (key_event->key)
    {
    case NES_KEY_RIGHT:
        nes_des.dwKeyPad1 &= ~(1 << 7);
        break;

    case NES_KEY_LEFT:
        nes_des.dwKeyPad1 &= ~(1 << 6);
        break;

    case NES_KEY_DOWN:
        nes_des.dwKeyPad1 &= ~(1 << 5);
        break;

    case NES_KEY_UP:
        nes_des.dwKeyPad1 &= ~(1 << 4);
        break;

    case NES_KEY_START:
        /* Start */
        nes_des.dwKeyPad1 &= ~(1 << 3);
        break;

    case NES_KEY_SELECT:
        /* Select */
        nes_des.dwKeyPad1 &= ~(1 << 2);
        break;

    case NES_KEY_A:
        /* 'A' */
        nes_des.dwKeyPad1 &= ~(1 << 1);
        break;

    case NES_KEY_B:
        /* 'B' */
        nes_des.dwKeyPad1 &= ~(1 << 0);
        break;
    }
}

static rt_bool_t draw_frame_buffer(struct rtgui_win *win, rt_uint16_t *p_data)
{
    struct rtgui_dc *dc;
    rtgui_rect_t rect;
    register int i;
    struct rtgui_point dc_point = { 0 };
    register rt_uint16_t* pBuf = (rt_uint16_t*)(((struct rtgui_dc_buffer *)nes_des.frame_dc)->pixel);

    dc = rtgui_dc_begin_drawing(RTGUI_WIDGET(win));
    if (dc == RT_NULL)
    {
        rt_kprintf("dc is null \n");
        return RT_FALSE;
    }
    rtgui_dc_get_rect(dc, &rect);
    rect.x1 = ((rect.x2 - rect.x1 - NES_DISP_WIDTH) >> 1) + rect.x1;
    rect.y1 = ((rect.y2 - rect.y1 - NES_DISP_HEIGHT) >> 1) + rect.y1;
    for (i = 0; i < NES_DISP_HEIGHT * NES_DISP_WIDTH; i++)
    {
        pBuf[i] = ((p_data[i] << 1) & 0xffe0) | (p_data[i] & 0x1f);
    }

    rtgui_dc_blit(nes_des.frame_dc, &dc_point, dc, &rect);

    rtgui_dc_end_drawing(dc, RT_TRUE);

    return RT_TRUE;
}


static rt_bool_t win_event_handler(struct rtgui_object *object, rtgui_event_t *event)
{
    rt_bool_t err;
    struct rtgui_win *win = RTGUI_WIN(object);

    err = rtgui_win_event_handler(RTGUI_OBJECT(win), event);
    if (event->type == RTGUI_EVENT_COMMAND)
    {
        struct rtgui_event_command *cmd = (struct rtgui_event_command *)event;

        if (cmd->type == NES_STATUS_RUN)
        {
            draw_frame_buffer(win, (rt_uint16_t *)cmd->command_id);
        }
        else if (cmd->type == NES_STATUS_PAUSE)
        {
            //TODO
        }
        else if (cmd->type == NES_STATUS_EXIT)
        {
            nes_des.app->state_flag = RTGUI_APP_FLAG_EXITED;
            nes_app_delete();
        }
        else
        {
            RT_ASSERT(RT_NULL);
        }
    }
    else if (event->type == RTGUI_EVENT_KBD)
    {
        struct rtgui_event_kbd *key_event = (struct rtgui_event_kbd *)event;

        if (key_event->type == RTGUI_KEYDOWN)
        {
            dbg_log(DBG_LOG, "KEYDOWN event key:%d\r\n", key_event->key);
            key_down(key_event);
        }
        else if (key_event->type == RTGUI_KEYUP)
        {
            dbg_log(DBG_LOG, "KEYUP event key:%d\r\n", key_event->key);
            key_up(key_event);
        }
    }

    return err;
}

static void nes_app_delete(void)
{
    if (nes_des.frame_dc)
    {
        rtgui_dc_destory(nes_des.frame_dc);
    }
    if (nes_des.win)
    {
        rtgui_win_destroy(nes_des.win);
    }
    if (nes_des.app)
    {
        rtgui_app_close(nes_des.app);
        rtgui_app_destroy(nes_des.app);
    }

    InfoNES_ReleaseRom();

    if (nes_des.nes_tid)
    {
        rt_thread_delete(nes_des.nes_tid);
    }

    if (nes_des.app_tid)
    {
        rt_thread_delete(nes_des.app_tid);
    }

    memset(&nes_des, 0, sizeof(struct _nes_des));

    rt_schedule();
}

static void nes_app_create(void *parameter)
{
    nes_des.app = rtgui_app_create("nes_app");
    if (nes_des.app == RT_NULL)
    {
        dbg_log(DBG_ERROR, "create nes app fail!!\r\n");
        goto out;
    }

    nes_des.win = rtgui_mainwin_create(RT_NULL, "NES", RTGUI_WIN_STYLE_NO_TITLE);
    if (nes_des.win == RT_NULL)
    {
        dbg_log(DBG_ERROR, "create windows fail!!\n");
        goto out;
    }

    nes_des.frame_dc = rtgui_dc_buffer_create_pixformat(RTGRAPHIC_PIXEL_FORMAT_RGB565, NES_DISP_WIDTH, NES_DISP_HEIGHT);
    if (nes_des.frame_dc == RT_NULL)
    {
        dbg_log(DBG_ERROR, "create dc buffer fail!!\r\n");
        goto out;
    }

    if (InfoNES_Load((char *)parameter) != RT_EOK)
    {
        dbg_log(DBG_ERROR, "NES load game fail!!\r\n");
        goto out;
    }

    nes_des.nes_tid = rt_thread_create("nes",(void(*)(void *))InfoNES_Main, \
        RT_NULL, NES_THREAD_STACK_SIZE, NES_THREAD_PRIORITY, NES_THREAD_TICK);

    if (nes_des.nes_tid == RT_NULL)
    {
        dbg_log(DBG_ERROR, "create NES thread fail!!\r\n");
        goto out;
    }

    rt_thread_startup(nes_des.nes_tid);

    rtgui_object_set_event_handler(RTGUI_OBJECT(nes_des.win), win_event_handler);
    rtgui_win_show(nes_des.win, RT_FALSE);
    rtgui_app_run(nes_des.app);
    //never run here
out:
    nes_app_delete();
}

void show_help(void)
{
    rt_kprintf("Usage      : nes <ops> [file name]\n");
    rt_kprintf("   -r      : run nes game\n");
    rt_kprintf("   -s      : stop nes game\n");
    rt_kprintf("   -p      : pause nes game\n");
}

extern "C" rt_err_t nes_runing(int argc, char** argv);
rt_err_t nes_runing(int argc, char** argv)
{
    if (argc < 2)
    {
        //TODO print help info
        show_help();
        return RT_EOK;
    }

    if ((strcmp("-r", argv[1]) == 0) && (nes_des.status == NES_STATUS_STOP))
    {
        if (argc == 3)
        {
#ifdef RT_USING_DFS
            int fd;
            unsigned long long size;

            fd = open(argv[2], O_RDONLY, 0);
            if (fd < 0)
            {
                dbg_log(DBG_ERROR, "open file %s fail!!!\r\n", argv[2]);
            }

            size = lseek(fd, 0, SEEK_END);
            nes_des.rom_data = rt_malloc(size);
            if (nes_des.rom_data == RT_NULL)
            {
                dbg_log(DBG_ERROR, "malloc rom data fail!!!\r\n");
                return RT_ERROR;
            }
            lseek(fd, 0, SEEK_SET);
            read(fd, nes_des.rom_data, size);
            close(fd);
#endif
        }
        else
        {
            nes_des.rom_data = rt_malloc(sizeof(_Super_Marie_nes));
            if (nes_des.rom_data == RT_NULL)
            {
                dbg_log(DBG_ERROR, "malloc rom data fail!!!\r\n");
                return RT_ERROR;
            }
            memcpy(nes_des.rom_data, _Super_Marie_nes, sizeof(_Super_Marie_nes));
        }

        nes_des.app_tid = rt_thread_create("nes_app", nes_app_create, RT_NULL, \
            NES_APP_THREAD_STACK_SIZE, NES_APP_THREAD_PRIORITY, NES_APP_THREAD_TICK);

        if (nes_des.app_tid == RT_NULL)
        {
            dbg_log(DBG_ERROR, "create NES app thread fail!!!\r\n");
            return RT_ERROR;
        }

        rt_thread_startup(nes_des.app_tid);

        nes_des.status = NES_STATUS_RUN;

        return RT_EOK;
    }
    else if ((strcmp("-s", argv[1]) == 0) && (nes_des.status == NES_STATUS_RUN))
    {
        rtgui_cmd_send(NES_STATUS_EXIT, 0);
        nes_des.status = NES_STATUS_STOP;
        return 0;
    }
    else if ((strcmp("-p", argv[1]) == 0) && (nes_des.status == NES_STATUS_RUN))
    {
        //TODO 
    }

    show_help();
    return RT_EOK;
}


#endif

