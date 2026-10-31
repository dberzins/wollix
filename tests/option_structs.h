// option_structs.h - the option structs, listed once for the registry
// tests. Each row is X(name, type, groups): `name` is the stem of
// wlx_default_<name>_opt and wlx_<name>_opt_defaults, and `groups` the
// field groups the struct embeds, each as G(GROUP) for WLX_<GROUP>_FIELDS.
// A new option struct gets a row here; test_defaults_once,
// the from_defaults marker test and sentinel_group_defaults then cover it.

#ifndef OPTION_STRUCTS_H_
#define OPTION_STRUCTS_H_

#define OPTION_STRUCTS(X) \
    X(slot_style,   WLX_Slot_Style_Opt,   ) \
    X(grid,         WLX_Grid_Opt,         G(LAYOUT_SLOT) G(CONTAINER_DECOR) G(SLOT_DECOR)) \
    X(grid_auto,    WLX_Grid_Auto_Opt,    G(LAYOUT_SLOT) G(CONTAINER_DECOR) G(SLOT_DECOR)) \
    X(layout,       WLX_Layout_Opt,       G(LAYOUT_SLOT) G(CONTAINER_DECOR) G(SLOT_DECOR)) \
    X(overlay,      WLX_Overlay_Opt,      G(CONTENT_PADDING)) \
    X(widget,       WLX_Widget_Opt,       G(LAYOUT_SLOT) G(WIDGET_SIZING) G(BORDER)) \
    X(label,        WLX_Label_Opt,        G(LAYOUT_SLOT) G(WIDGET_SIZING) G(TEXT_TYPOGRAPHY) \
                                          G(TEXT_WRAP) G(TEXT_COLOR) G(BORDER) G(CONTENT_PADDING)) \
    X(button,       WLX_Button_Opt,       G(LAYOUT_SLOT) G(WIDGET_SIZING) G(WIDGET_STATE) \
                                          G(TEXT_TYPOGRAPHY) G(TEXT_WRAP) G(TEXT_COLOR) G(BORDER) \
                                          G(CONTENT_PADDING)) \
    X(dropdown,     WLX_Dropdown_Opt,     G(LAYOUT_SLOT) G(WIDGET_SIZING) G(WIDGET_STATE) \
                                          G(TEXT_TYPOGRAPHY) G(TEXT_COLOR) G(BORDER) G(CONTENT_PADDING)) \
    X(tooltip,      WLX_Tooltip_Opt,      G(TEXT_TYPOGRAPHY)) \
    X(menu,         WLX_Menu_Opt,         G(TEXT_TYPOGRAPHY)) \
    X(menu_item,    WLX_Menu_Item_Opt,    G(WIDGET_STATE)) \
    X(menu_button,  WLX_Menu_Button_Opt,  G(LAYOUT_SLOT) G(WIDGET_SIZING) G(WIDGET_STATE) \
                                          G(TEXT_TYPOGRAPHY) G(TEXT_COLOR) G(BORDER) G(CONTENT_PADDING)) \
    X(checkbox,     WLX_Checkbox_Opt,     G(LAYOUT_SLOT) G(WIDGET_SIZING) G(WIDGET_STATE) \
                                          G(TEXT_TYPOGRAPHY) G(TEXT_WRAP) G(TEXT_COLOR) G(BORDER) \
                                          G(CONTENT_PADDING)) \
    X(inputbox,     WLX_Inputbox_Opt,     G(LAYOUT_SLOT) G(WIDGET_SIZING) G(CONTENT_PADDING) \
                                          G(WIDGET_STATE) G(TEXT_TYPOGRAPHY) G(TEXT_WRAP) G(TEXT_COLOR) \
                                          G(BORDER)) \
    X(slider,       WLX_Slider_Opt,       G(LAYOUT_SLOT) G(WIDGET_SIZING) G(WIDGET_STATE) \
                                          G(TEXT_TYPOGRAPHY) G(BORDER) G(CONTENT_PADDING)) \
    X(separator,    WLX_Separator_Opt,    G(LAYOUT_SLOT) G(WIDGET_SIZING)) \
    X(progress,     WLX_Progress_Opt,     G(LAYOUT_SLOT) G(WIDGET_SIZING) G(BORDER) G(CONTENT_PADDING)) \
    X(image,        WLX_Image_Opt,        G(LAYOUT_SLOT) G(WIDGET_SIZING)) \
    X(toggle,       WLX_Toggle_Opt,       G(LAYOUT_SLOT) G(WIDGET_SIZING) G(WIDGET_STATE) \
                                          G(TEXT_TYPOGRAPHY) G(TEXT_WRAP) G(TEXT_COLOR) G(BORDER) \
                                          G(CONTENT_PADDING)) \
    X(radio,        WLX_Radio_Opt,        G(LAYOUT_SLOT) G(WIDGET_SIZING) G(WIDGET_STATE) \
                                          G(TEXT_TYPOGRAPHY) G(TEXT_WRAP) G(TEXT_COLOR) G(CONTENT_PADDING)) \
    X(scroll_panel, WLX_Scroll_Panel_Opt, G(LAYOUT_SLOT) G(WIDGET_SIZING) G(BORDER)) \
    X(list_clipper, WLX_List_Clipper_Opt, ) \
    X(split,        WLX_Split_Opt,        G(CONTENT_PADDING)) \
    X(split_next,   WLX_Split_Next_Opt,   ) \
    X(panel,        WLX_Panel_Opt,        G(CONTAINER_DECOR) G(SLOT_DECOR) G(CONTENT_PADDING)) \
    X(editor,       WLX_Editor_Opt,       G(LAYOUT_SLOT) G(WIDGET_SIZING) G(CONTENT_PADDING) \
                                          G(WIDGET_STATE) G(TEXT_TYPOGRAPHY) G(TEXT_WRAP) G(TEXT_COLOR) \
                                          G(BORDER))

#endif // OPTION_STRUCTS_H_
