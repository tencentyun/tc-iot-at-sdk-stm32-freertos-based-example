#ifdef EVENT_POST_ENABLED

#define EVENT_COUNTS     (1)

static TYPE_DEF_TEMPLATE_STRING sg_motor_state_status[64+1]={0};
static DeviceProperty g_propertyEvent_motor_state[] = {

   {.key = "status", .data = sg_motor_state_status, .type = TYPE_TEMPLATE_STRING},
};


static sEvent g_events[]={

    {
     .event_name = "motor_state",
     .type = "alert",
     .timestamp = 0,
     .eventDataNum = sizeof(g_propertyEvent_motor_state)/sizeof(g_propertyEvent_motor_state[0]),
     .pEventData = g_propertyEvent_motor_state,
    },
};

#endif
