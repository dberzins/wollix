// test_defaults_once.c - every option default macro sets each designator
// exactly once, and every option struct has a by-value defaults function.
//
// Compiled as its own translation unit (it is NOT part of the single-TU
// test_runner) with the initializer-override warning promoted to an error,
// the inverse of the project's normal build flags: a default macro that
// repeats a designator (a field group's default followed by a per-widget
// override of the same field) fails to compile here. This TU deliberately
// holds only the declarations - the implementation, whose own internal
// macro calls override defaults on purpose, is linked from
// test_defaults_once_impl.c built with the normal flags. The same TU calls
// every wlx_*_opt_defaults function, so a struct whose function is missing
// fails to link.

#include "wollix.h"
#include "wollix_editor.h"

#include <stdio.h>

#define EXPAND_ONCE(name, type) \
    do { type m_ = wlx_default_##name##_opt(); type f_ = wlx_##name##_opt_defaults(); \
         (void)m_; (void)f_; count++; } while (0)

int main(void) {
    int count = 0;
    EXPAND_ONCE(slot_style,   WLX_Slot_Style_Opt);
    EXPAND_ONCE(grid,         WLX_Grid_Opt);
    EXPAND_ONCE(grid_auto,    WLX_Grid_Auto_Opt);
    EXPAND_ONCE(layout,       WLX_Layout_Opt);
    EXPAND_ONCE(overlay,      WLX_Overlay_Opt);
    EXPAND_ONCE(widget,       WLX_Widget_Opt);
    EXPAND_ONCE(label,        WLX_Label_Opt);
    EXPAND_ONCE(button,       WLX_Button_Opt);
    EXPAND_ONCE(dropdown,     WLX_Dropdown_Opt);
    EXPAND_ONCE(tooltip,      WLX_Tooltip_Opt);
    EXPAND_ONCE(menu,         WLX_Menu_Opt);
    EXPAND_ONCE(menu_item,    WLX_Menu_Item_Opt);
    EXPAND_ONCE(menu_button,  WLX_Menu_Button_Opt);
    EXPAND_ONCE(checkbox,     WLX_Checkbox_Opt);
    EXPAND_ONCE(inputbox,     WLX_Inputbox_Opt);
    EXPAND_ONCE(slider,       WLX_Slider_Opt);
    EXPAND_ONCE(separator,    WLX_Separator_Opt);
    EXPAND_ONCE(progress,     WLX_Progress_Opt);
    EXPAND_ONCE(image,        WLX_Image_Opt);
    EXPAND_ONCE(toggle,       WLX_Toggle_Opt);
    EXPAND_ONCE(radio,        WLX_Radio_Opt);
    EXPAND_ONCE(scroll_panel, WLX_Scroll_Panel_Opt);
    EXPAND_ONCE(list_clipper, WLX_List_Clipper_Opt);
    EXPAND_ONCE(split,        WLX_Split_Opt);
    EXPAND_ONCE(split_next,   WLX_Split_Next_Opt);
    EXPAND_ONCE(panel,        WLX_Panel_Opt);
    EXPAND_ONCE(editor,       WLX_Editor_Opt);

    // A spot check that the function really is the macro: the defaults a
    // caller is most likely to lean on.
    WLX_Button_Opt b = wlx_button_opt_defaults();
    if (b.pos != WLX_UNSET || b.span != 1 || b.border_width != (float)WLX_UNSET || !b.wrap) {
        fprintf(stderr, "test_defaults_once: wlx_button_opt_defaults does not match the macro\n");
        return 1;
    }
    WLX_Editor_Opt e = wlx_editor_opt_defaults();
    if (e.tab_columns != 4 || !e.show_scrollbar) {
        fprintf(stderr, "test_defaults_once: wlx_editor_opt_defaults does not match the macro\n");
        return 1;
    }

    printf("test_defaults_once: OK (%d option macros set each designator once; %d defaults functions)\n",
           count, count);
    return 0;
}
