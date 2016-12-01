#ifndef BUILD_LK
#include <linux/string.h>
#endif


#include "lcm_drv.h"
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                         (720)
#define FRAME_HEIGHT                                        (1280)

#define REGFLAG_DELAY                                       0xFE
#define REGFLAG_END_OF_TABLE                                0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                  lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)              lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()


static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51, 1, {0xFF}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static int transform[] = {
    255,253,252,250,249,247,246,244,243,241,
    240,238,237,236,234,233,231,230,228,227,
    225,224,222,221,219,218,217,215,214,212,
    211,209,208,206,205,203,202,200,199,198,
    196,195,193,192,190,189,187,186,184,183,
    181,180,179,177,176,174,173,171,170,168,
    167,165,164,163,161,160,158,157,155,154,
    152,151,149,148,146,145,144,142,141,139,
    138,136,135,133,132,130,129,127,126,125,
    124,123,122,121,120,119,118,117,116,115,
    114,113,112,111,109,107,105,103,101,99,
    97, 95, 93, 91, 89, 87, 85, 83, 81, 79,
    77, 75, 73, 71, 69, 67, 65, 63, 61, 59,
    57, 56, 55, 54, 53, 52, 51, 50, 49, 47,
    46, 45, 44, 43, 42, 41, 40, 39, 38, 36,
    35, 34, 33, 33, 33, 32, 32, 32, 32, 31, //105
    31, 31, 30, 30, 30, 30, 29, 29, 29, 28,
    28, 28, 27, 27, 27, 27, 26, 26, 26, 25,
    25, 25, 25, 24, 24, 24, 23, 23, 23, 23,
    22, 22, 22, 21, 21, 21, 21, 20, 20, 20,
    19, 19, 19, 19, 18, 18, 18, 17, 17, 17, //55
    17, 16, 16, 16, 15, 15, 15, 15, 14, 14,
    14, 13, 13, 13, 13, 12, 12, 12, 11, 11,
    11, 11, 10, 10, 10, 9, 9, 8, 8, 7,
    6, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 0
};
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd)
        {
            case REGFLAG_DELAY :
            {
                MDELAY(table[i].count);
                break;
            }
            case REGFLAG_END_OF_TABLE :
            {
                break;
            }
            default:
            {
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
            }
        }
    }
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    #if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
    #else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    #endif

    // DSI
    /* Command mode setting */
    //1 Three lane or Four lane
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Video mode setting
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active                = 8;
    params->dsi.vertical_backporch                  = 40;
    params->dsi.vertical_frontporch                 = 40;
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active              = 20;
    params->dsi.horizontal_backporch                = 80;
    params->dsi.horizontal_frontporch               = 80;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->physical_width = 68;
    params->physical_height = 121;

    params->dsi.PLL_CLOCK=230;
    params->dsi.ssc_disable = 1;

    params->dsi.noncont_clock = 1;
    params->dsi.noncont_clock_period = 2;

    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1;
    params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
    params->dsi.lcm_esd_check_table[0].count        = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

static void init_lcm_registers(void)
{
    unsigned int data_array[33];

    data_array[0] = 0x00042902;
    data_array[1] = 0x038198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x73032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0C062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x19092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x010a2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x010b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0b0c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x010d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x010e2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x260f2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x26102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001a2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x441e2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xc01f2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0a202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0a222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8c242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8c252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3B282300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002a2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002e2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002f2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003a2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003e2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003f2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00412300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00422300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00432300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00442300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x23512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x67532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xab552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x23572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x67592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x895a2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xab5b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xcd5c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xef5d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x115e2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x025f2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0c612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0d622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0e632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0f642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026a2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x026d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x056e2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x056f2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x06732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x07742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0c772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0d782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0e792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0f7a2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x027b2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x027c2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x027d2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x027e2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x027f2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02802300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02812300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02822300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x02832300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05842300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05852300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05862300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x05872300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01882300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x06892300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x078A2300;
    dsi_set_cmdq(data_array, 1, 1);

    //Page 4
    data_array[0] = 0x00042902;
    data_array[1] = 0x048198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x156C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1a6E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x256F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xA43A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x208D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xBA872300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x76262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xD1B22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x018198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x0A222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89532300; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8a552300; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x75502300; 
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x71512300; 
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1b602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x01612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0c622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x058198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x01802300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x018198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x01342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x04A02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0CA12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x17A22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x10A32300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x10A42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x20A52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0FA62300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x19A72300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45A82300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1BA92300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x27AA2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x38AB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x16AC2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x13AD2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x46AE2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1BAF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x20B02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x46B12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x6eB22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3fB32300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x03C02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0bC12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x16C22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x11C32300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x11C42300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x21C52300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x0fC62300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1aC72300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x44C82300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1bC92300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x27CA2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x37CB2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x16CC2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x13CD2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x47CE2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x1bCF2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x21D02300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x45D12300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x6dD22300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x3fD32300;
    dsi_set_cmdq(data_array, 1, 1);
    

    data_array[0] = 0x00042902;
    data_array[1] = 0x058198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x82082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x82092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x830D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x830E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x830F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x85182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x85192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x851A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x851B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x851C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x861D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x861E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x861F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88282300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x882A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x882B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x882C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x892D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x892E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x892F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8a322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8a332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8a342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8a352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8a362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8b372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8b382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8b392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8b3A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8b3B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8b3C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8c3D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8c3E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8c3F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8c402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8c412300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8d422300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8d432300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8d442300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8d452300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8d462300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8e472300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8e482300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8e492300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8e4A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8e4B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8f4C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8f4D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8f4E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8f4F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8f502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x91572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x91582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x91592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x915A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x915B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x925C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x925D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x925E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x925F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x92602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x94662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x94672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x94682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x94692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x946A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x946B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x956C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x956D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x956E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x956F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x95702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x97762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x97772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x97782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x97792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x977A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x987B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x987C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x987D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x987E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x987F2300;
    dsi_set_cmdq(data_array, 1, 1);

     

    data_array[0] = 0x00042902;
    data_array[1] = 0x068198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x98002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x99012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x99022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x99032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x99042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x99052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a0A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b0B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b0C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b0D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b0E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b0F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d1A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e1B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e1C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e1D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e1E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e1F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0282300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa12A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa12B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa12C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa12D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa12E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa12F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa3352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa3362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa3372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa3382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa3392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa43A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa43B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa43C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa43D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa43E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa53F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5412300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5422300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5432300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa6442300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa6452300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa6462300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa6472300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa6492300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa74A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa74B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa74C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa74D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa74E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa84F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa9542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa9552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa9562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa9572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa9582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaa592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaa5A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaa5B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaa5C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaa5D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaa5E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xab5F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xab602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xab612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xab622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xab632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xac642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xac652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xac662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xac672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xac682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xad692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xad6A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xad6B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xad6C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xad6D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xae6E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xae6F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xae702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xae712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xae722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaf732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaf742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaf752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaf762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaf772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xaf782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xb0792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xb07A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xb07B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xb07C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xb07D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xb17E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xb17F2300;
    dsi_set_cmdq(data_array, 1, 1);
     

 
    data_array[0] = 0x00042902;
    data_array[1] = 0x078198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00282300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00412300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00422300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00432300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00442300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00452300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00462300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00472300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00482300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00492300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007F2300;
    dsi_set_cmdq(data_array, 1, 1);
   
    data_array[0] = 0x00042902;
    data_array[1] = 0x088198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x000F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x001F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00282300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x002F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x003F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00412300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00422300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00432300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00442300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00452300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00462300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00472300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00482300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00492300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x004F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x005F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x006F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x007F2300;
    dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00042902;
    data_array[1] = 0x098198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x00032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x81092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x820F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x83152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x84192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x841A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x841B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x841C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x851D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x851E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x851F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x85202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x85212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x85222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x86282300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x87292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x872A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x872B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x872C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x872D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x872E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x882F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x88352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x89392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x893A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x893B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8A3C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8A3D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8A3E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8A3F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8A402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8A412300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8B422300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8B432300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8B442300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8B452300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8B462300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8B472300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8C482300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8C492300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8C4A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8C4B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8C4C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8C4D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8C4E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8D4F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8D502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8D512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8D522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8D532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8D542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8E552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8E562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8E572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8E582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8E592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8E5A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8F5B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8F5C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8F5D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8F5E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8F5F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x8F602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x90672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x91682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x91692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x916A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x916B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x916C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x916D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x926E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x926F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x92702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x92712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x92722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x92732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x93792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x947A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x947B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x947C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x947D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x947E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x947F2300;
    dsi_set_cmdq(data_array, 1, 1);



    data_array[0] = 0x00042902;
    data_array[1] = 0x0A8198FF;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x94002300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x95012300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x95022300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x95032300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x95042300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x95052300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x95062300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96072300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96082300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x96092300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x960A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x960B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x960C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x970D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x970E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x970F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x97102300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x97112300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x97122300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x98132300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x98142300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x98152300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x98162300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x98172300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x98182300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x99192300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x991A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x991B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x991C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x991D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x991E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x991F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a202300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a212300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a222300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a232300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a242300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9a252300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b262300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b272300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b282300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b292300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b2A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9b2B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c2C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c2D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c2E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c2F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c302300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9c312300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d322300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d332300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d342300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d352300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d362300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d372300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9d382300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e392300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e3A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e3B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e3C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e3D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9e3E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f3F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f402300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f412300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f422300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f432300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0x9f442300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0452300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0462300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0472300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0482300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa0492300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa04A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa14B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa14C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa14D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa14E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa14F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa1502300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa1512300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2522300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2532300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2542300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2552300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2562300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa2572300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa3582300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa3592300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa35A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa35B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa35C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa35D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa45E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa45F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa4602300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa4612300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa4622300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa4632300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5642300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5652300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5662300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5672300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5682300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa5692300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa56A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa66B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa66C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa66D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa66E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa66F2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa6702300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa7712300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa7722300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa7732300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa7742300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa7752300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa7762300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8772300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8782300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa8792300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa87A2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa87B2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa87C2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa97D2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa97E2300;
    dsi_set_cmdq(data_array, 1, 1);
    data_array[0] = 0xa97F2300;
    dsi_set_cmdq(data_array, 1, 1);


    //Page 0
    data_array[0] = 0x00042902;
    data_array[1] = 0x008198FF;
    dsi_set_cmdq(data_array, 2, 1);

    //open TE
    //data_array[0] = 0x00352300;
    //dsi_set_cmdq(data_array, 1, 1);
    //CABC
    //data_array[0] = 0x00511500;
    //dsi_set_cmdq(data_array, 1, 1);
    //data_array[0] = 0x24531500;
    //dsi_set_cmdq(data_array, 1, 1);
    //data_array[0] = 0x01551500;
    //dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00110500;      //Sleep Out
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;     //Display On
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);
}


static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 200, {}},

    // Display ON
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_in_setting[] = {
    // Display off sequence
    {0x28, 20, {0x00}},

    // Sleep Mode On
    {0x10, 100, {0x00}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_init(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(20);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);
    init_lcm_registers();
}



static void lcm_suspend(void)
{
    push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,0);
    MDELAY(10);
}


static void lcm_resume(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(20);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);
    init_lcm_registers();
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    data_array[3]= 0x00053902;
    data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[5]= (y1_LSB);
    data_array[6]= 0x002c3909;

    dsi_set_cmdq(data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
    //unsigned int default_level = 0;//145;
    unsigned int mapped_level = 0;

    //for LGE backlight IC mapping table
    if(level > 255)
            level = 255;

    if(level >0)
            //mapped_level = default_level+(level)*(255-default_level)/(255);
            mapped_level = (unsigned char)transform[255-level];
    else
            mapped_level=0;

    // Refresh value of backlight level.
    lcm_backlight_level_setting[0].para_list[0] = mapped_level;

    push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define LCM_COMPARE_ID_YS_CPT 0x41

static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[5];
    unsigned int array[16];

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(11);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(11);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);

    array[0] = 0x00013700;// read id return two byte,version and id
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0xDA, buffer, 1);
    id = buffer[0];
    #ifdef BUILD_LK
        printf("lk debug: YS_CPT id = 0x%x\n", id);
    #endif

    if(LCM_COMPARE_ID_YS_CPT == id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void lcm_vddi_power_on(void)
{
    #ifdef BUILD_LK
        printf("lk  lcm_vddi_power_on\n");
    #else
        printk("kernel lcm_vddi_power_on\n");
    #endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ONE);
    MDELAY(10);
}
static void lcm_vddi_power_off(void)
{
    #ifdef BUILD_LK
        printf("lk  lcm_vddi_power_off\n");
    #else
        printk("kernel lcm_vddi_power_off\n");
    #endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ZERO);
    MDELAY(10);
}
static void lcm_vddi_power_init(void)
{
    #ifdef BUILD_LK
        printf("lk  lcm_vddi_power_init\n");
    #else
        printk("kernel lcm_vddi_power_init\n");
    #endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ONE);
}
LCM_DRIVER ili9881c_dsi_ys_cpt_id41_lcm_drv =
{
    .name           = "ili9881c_dsi_dj_boe",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
    .set_backlight  = lcm_setbacklight,
    .vddi_power_on  = lcm_vddi_power_on,
    .vddi_power_off  = lcm_vddi_power_off,
    .vddi_power_init = lcm_vddi_power_init,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update
#endif
};
