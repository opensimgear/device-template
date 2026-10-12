/* Rev B wiring demonstration: one of each supported input type. */
static const osg_profile_button_t profile_buttons[]={ {0,false,0},{1,true,0},{4,false,0} };
static const osg_profile_axis_t profile_axes[]={ {0,0x30,0,4095,32768} };
static const osg_profile_encoder_t profile_encoders[]={ {2,3} };
static const osg_profile_hat_t profile_hats[]={ {{5,6,7,8}} };
static const osg_profile_selector_t profile_selectors[]={ {9,10} };
static const osg_profile_t selected_profile={
    .name="example",.buttons=profile_buttons,.button_count=3,
    .axes=profile_axes,.axis_count=1,.encoders=profile_encoders,.encoder_count=1,
    .hats=profile_hats,.hat_count=1,.selectors=profile_selectors,.selector_count=1
};
