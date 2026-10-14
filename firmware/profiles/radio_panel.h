/* Existing starter inputs; displays require a separately fitted output port. */
static const osg_profile_button_t profile_buttons[]={
    {0,true,0},{1,true,0},{2,false,0}
};
static const osg_profile_encoder_t profile_encoders[]={ {3,4},{5,6} };
static const osg_profile_axis_t profile_axes[]={ {0,0x30,100,3900,0} };
static const osg_profile_t selected_profile={.name="Radio panel",.buttons=profile_buttons,.button_count=3,
    .encoders=profile_encoders,.encoder_count=2,.axes=profile_axes,.axis_count=1};
