/*-----------------data config start  -------------------*/ 

#define TOTAL_PROPERTY_COUNT 5

static sDataPoint    sg_DataTemplate[TOTAL_PROPERTY_COUNT];

typedef struct _ProductDataDefine {
    TYPE_DEF_TEMPLATE_ENUM m_led;
    TYPE_DEF_TEMPLATE_BOOL m_power_switch;
    TYPE_DEF_TEMPLATE_FLOAT m_tempreture;
    TYPE_DEF_TEMPLATE_FLOAT m_humidity;
    TYPE_DEF_TEMPLATE_INT m_brightness;
} ProductDataDefine;

static   ProductDataDefine     sg_ProductData;

static void _init_data_template(void)
{
    sg_ProductData.m_led = 1;
    sg_DataTemplate[0].data_property.data = &sg_ProductData.m_led;
    sg_DataTemplate[0].data_property.key  = "led";
    sg_DataTemplate[0].data_property.type = TYPE_TEMPLATE_ENUM;

    sg_ProductData.m_power_switch = 0;
    sg_DataTemplate[1].data_property.data = &sg_ProductData.m_power_switch;
    sg_DataTemplate[1].data_property.key  = "power_switch";
    sg_DataTemplate[1].data_property.type = TYPE_TEMPLATE_BOOL;

    sg_ProductData.m_tempreture = 0;
    sg_DataTemplate[2].data_property.data = &sg_ProductData.m_tempreture;
    sg_DataTemplate[2].data_property.key  = "tempreture";
    sg_DataTemplate[2].data_property.type = TYPE_TEMPLATE_FLOAT;

    sg_ProductData.m_humidity = 0;
    sg_DataTemplate[3].data_property.data = &sg_ProductData.m_humidity;
    sg_DataTemplate[3].data_property.key  = "humidity";
    sg_DataTemplate[3].data_property.type = TYPE_TEMPLATE_FLOAT;

    sg_ProductData.m_brightness = 0;
    sg_DataTemplate[4].data_property.data = &sg_ProductData.m_brightness;
    sg_DataTemplate[4].data_property.key  = "brightness";
    sg_DataTemplate[4].data_property.type = TYPE_TEMPLATE_INT;

};
