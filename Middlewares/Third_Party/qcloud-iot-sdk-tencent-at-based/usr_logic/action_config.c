#ifdef ACTION_ENABLED

#define TOTAL_ACTION_COUNTS     (1)

static TYPE_DEF_TEMPLATE_INT sg_flow_led_in_inv = 0;
static DeviceProperty g_actionInput_flow_led[] = {

   {.key = "inv", .data = &sg_flow_led_in_inv, .type = TYPE_TEMPLATE_INT},
};
static TYPE_DEF_TEMPLATE_BOOL sg_flow_led_out_resault = 0;
static DeviceProperty g_actionOutput_flow_led[] = {

   {.key = "resault", .data = &sg_flow_led_out_resault, .type = TYPE_TEMPLATE_BOOL},
};


static DeviceAction g_actions[]={

    {
     .pActionId = "flow_led",
     .timestamp = 0,
     .input_num = sizeof(g_actionInput_flow_led)/sizeof(g_actionInput_flow_led[0]),
     .output_num = sizeof(g_actionOutput_flow_led)/sizeof(g_actionOutput_flow_led[0]),
     .pInput = g_actionInput_flow_led,
     .pOutput = g_actionOutput_flow_led,
    },
};

#endif
