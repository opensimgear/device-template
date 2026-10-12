/* All eight axes, 24 direct contacts and one optional 16-contact bank at 0x20.
 * No encoder on I2C: fast quadrature edges require direct MCU acquisition. */
#define CONTACT(n) {n,false,0}
static const osg_profile_button_t profile_buttons[]={
    CONTACT(0),CONTACT(1),CONTACT(2),CONTACT(3),CONTACT(4),CONTACT(5),CONTACT(6),CONTACT(7),
    CONTACT(8),CONTACT(9),CONTACT(10),CONTACT(11),CONTACT(12),CONTACT(13),CONTACT(14),CONTACT(15),
    CONTACT(16),CONTACT(17),CONTACT(18),CONTACT(19),CONTACT(20),CONTACT(21),CONTACT(22),CONTACT(23),
    CONTACT(24),CONTACT(25),CONTACT(26),CONTACT(27),CONTACT(28),CONTACT(29),CONTACT(30),CONTACT(31),
    CONTACT(32),CONTACT(33),CONTACT(34),CONTACT(35),CONTACT(36),CONTACT(37),CONTACT(38),CONTACT(39)
};
#undef CONTACT
static const osg_profile_axis_t profile_axes[]={
    {0,0x30,0,4095,32768},{1,0x31,0,4095,32768},{2,0x32,0,4095,32768},{3,0x33,0,4095,32768},
    {4,0x34,0,4095,32768},{5,0x35,0,4095,32768},{6,0x36,0,4095,32768},{7,0x37,0,4095,32768}
};
static const osg_profile_t selected_profile={
    .name="expanded",.buttons=profile_buttons,.button_count=40,
    .axes=profile_axes,.axis_count=8,.expander_mask=1
};
