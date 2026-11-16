#include <include/render/settings/functions.h>
#include <windows.h>


ImGuiKey vk_to_imguikey(int vk)
{
    switch (vk)
    {

    case 0x08: return ImGuiKey_Backspace;
    case 0x09: return ImGuiKey_Tab;
    case 0x0D: return ImGuiKey_Enter;
    case 0x1B: return ImGuiKey_Escape;
    case 0x20: return ImGuiKey_Space;
    case 0x21: return ImGuiKey_PageUp;
    case 0x22: return ImGuiKey_PageDown;
    case 0x23: return ImGuiKey_End;
    case 0x24: return ImGuiKey_Home;
    case 0x25: return ImGuiKey_LeftArrow;
    case 0x26: return ImGuiKey_UpArrow;
    case 0x27: return ImGuiKey_RightArrow;
    case 0x28: return ImGuiKey_DownArrow;
    case 0x2D: return ImGuiKey_Insert;
    case 0x2E: return ImGuiKey_Delete;


    case 0x30: return ImGuiKey_0;
    case 0x31: return ImGuiKey_1;
    case 0x32: return ImGuiKey_2;
    case 0x33: return ImGuiKey_3;
    case 0x34: return ImGuiKey_4;
    case 0x35: return ImGuiKey_5;
    case 0x36: return ImGuiKey_6;
    case 0x37: return ImGuiKey_7;
    case 0x38: return ImGuiKey_8;
    case 0x39: return ImGuiKey_9;


    case 0x41: return ImGuiKey_A;
    case 0x42: return ImGuiKey_B;
    case 0x43: return ImGuiKey_C;
    case 0x44: return ImGuiKey_D;
    case 0x45: return ImGuiKey_E;
    case 0x46: return ImGuiKey_F;
    case 0x47: return ImGuiKey_G;
    case 0x48: return ImGuiKey_H;
    case 0x49: return ImGuiKey_I;
    case 0x4A: return ImGuiKey_J;
    case 0x4B: return ImGuiKey_K;
    case 0x4C: return ImGuiKey_L;
    case 0x4D: return ImGuiKey_M;
    case 0x4E: return ImGuiKey_N;
    case 0x4F: return ImGuiKey_O;
    case 0x50: return ImGuiKey_P;
    case 0x51: return ImGuiKey_Q;
    case 0x52: return ImGuiKey_R;
    case 0x53: return ImGuiKey_S;
    case 0x54: return ImGuiKey_T;
    case 0x55: return ImGuiKey_U;
    case 0x56: return ImGuiKey_V;
    case 0x57: return ImGuiKey_W;
    case 0x58: return ImGuiKey_X;
    case 0x59: return ImGuiKey_Y;
    case 0x5A: return ImGuiKey_Z;


    case 0x60: return ImGuiKey_Keypad0;
    case 0x61: return ImGuiKey_Keypad1;
    case 0x62: return ImGuiKey_Keypad2;
    case 0x63: return ImGuiKey_Keypad3;
    case 0x64: return ImGuiKey_Keypad4;
    case 0x65: return ImGuiKey_Keypad5;
    case 0x66: return ImGuiKey_Keypad6;
    case 0x67: return ImGuiKey_Keypad7;
    case 0x68: return ImGuiKey_Keypad8;
    case 0x69: return ImGuiKey_Keypad9;


    case 0x70: return ImGuiKey_F1;
    case 0x71: return ImGuiKey_F2;
    case 0x72: return ImGuiKey_F3;
    case 0x73: return ImGuiKey_F4;
    case 0x74: return ImGuiKey_F5;
    case 0x75: return ImGuiKey_F6;
    case 0x76: return ImGuiKey_F7;
    case 0x77: return ImGuiKey_F8;
    case 0x78: return ImGuiKey_F9;
    case 0x79: return ImGuiKey_F10;
    case 0x7A: return ImGuiKey_F11;
    case 0x7B: return ImGuiKey_F12;


    case 0x10: return ImGuiKey_LeftShift;
    case 0x11: return ImGuiKey_LeftCtrl;
    case 0x12: return ImGuiKey_LeftAlt;


    case 0xA0: return ImGuiKey_LeftShift;
    case 0xA1: return ImGuiKey_RightShift;
    case 0xA2: return ImGuiKey_LeftCtrl;
    case 0xA3: return ImGuiKey_RightCtrl;
    case 0xA4: return ImGuiKey_LeftAlt;
    case 0xA5: return ImGuiKey_RightAlt;


    case 0x13: return ImGuiKey_Pause;
    case 0x14: return ImGuiKey_CapsLock;
    case 0x90: return ImGuiKey_NumLock;
    case 0x91: return ImGuiKey_ScrollLock;

    default: return ImGuiKey_None;
    }
}

const char* keys[] =
{
    "None",
    "Mouse 1",
    "Mouse 2",
    "CN",
    "Mouse 3",
    "Mouse 4",
    "Mouse 5",
    "-",
    "Back",
    "Tab",
    "-",
    "-",
    "CLR",
    "Enter",
    "-",
    "-",
    "Shift",
    "CTL",
    "Menu",
    "Pause",
    "Caps",
    "KAN",
    "-",
    "JUN",
    "FIN",
    "KAN",
    "-",
    "Escape",
    "CON",
    "NCO",
    "ACC",
    "MAD",
    "Space",
    "PGU",
    "PGD",
    "End",
    "Home",
    "Left",
    "Up",
    "Right",
    "Down",
    "SEL",
    "PRI",
    "EXE",
    "PRI",
    "INS",
    "Delete",
    "HEL",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "WIN",
    "WIN",
    "APP",
    "-",
    "SLE",
    "Num 0",
    "Num 1",
    "Num 2",
    "Num 3",
    "Num 4",
    "Num 5",
    "Num 6",
    "Num 7",
    "Num 8",
    "Num 9",
    "MUL",
    "ADD",
    "SEP",
    "MIN",
    "Delete",
    "DIV",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "NUM",
    "SCR",
    "EQU",
    "MAS",
    "TOY",
    "OYA",
    "OYA",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "Shift",
    "Shift",
    "Ctrl",
    "Ctrl",
    "Alt",
    "Alt"
};



















































static uint64_t activation_time = 0;

bool key_select(const char* k_id, int* key)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(k_id);

    const ImVec2 pos = window->DC.CursorPos;
    const float width = GetContentRegionAvail().x;

    const ImRect rect(pos, pos + ImVec2(width, SCALE(25)));
    ImGui::ItemSize(rect, style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    char buf_display[64] = "...";

    bool value_changed = false;
    int k = *key;

    std::string active_key = "";
    active_key += keys[*key];

    if (*key != 0 && g.ActiveId != id) {
        strcpy_s(buf_display, active_key.c_str());
    }
    else if (g.ActiveId == id) {
        strcpy_s(buf_display, "...");
    }

    const bool hovered = ItemHoverable(rect, id, 0);

    if (hovered && io.MouseClicked[0])
    {
        if (g.ActiveId != id) {

            memset(io.MouseDown, 0, sizeof(io.MouseDown));
            *key = 0;
            activation_time = GetTickCount64();
        }
        ImGui::SetActiveID(id, window);
        ImGui::FocusWindow(window);
    }
    else if (io.MouseClicked[0]) {

        if (g.ActiveId == id)
            ImGui::ClearActiveID();
    }

    if (g.ActiveId == id) {

        if (GetTickCount64() > activation_time + 100) {
            for (auto i = 0; i < 5; i++) {
                if (io.MouseDown[i]) {
                    switch (i) {
                    case 0:
                        k = 0x01;
                        break;
                    case 1:
                        k = 0x02;
                        break;
                    case 2:
                        k = 0x04;
                        break;
                    case 3:
                        k = 0x05;
                        break;
                    case 4:
                        k = 0x06;
                        break;
                    }
                    value_changed = true;
                    activation_time = GetTickCount64();
                    ImGui::ClearActiveID();
                }
            }
            if (!value_changed) {

                for (auto i = 0x0; i <= 0x100; i++) {
                    ImGuiKey imgui_key = vk_to_imguikey(i);

                    if (imgui_key != ImGuiKey_None && ImGui::IsNamedKey(imgui_key) && ImGui::IsKeyDown(imgui_key)) {
                        k = i;
                        value_changed = true;
                        activation_time = GetTickCount64();
                        ImGui::ClearActiveID();
                        break;
                    }
                }


                if (!value_changed) {

                    int problematic_keys[] = {
                        0xA4,
                        0xA5,
                        0x70,
                        0x71,
                        0x72,
                        0x73,
                        0x74,
                        0x75,
                        0x76,
                        0x77,
                        0x78,
                        0x79,
                        0x7A,
                        0x7B,
                        0x41,
                        0x42,
                        0x43,
                        0x44,
                        0x45,
                        0x46,
                        0x47,
                        0x48,
                        0x49,
                        0x4A,
                        0x4B,
                        0x4C,
                        0x4D,
                        0x4E,
                        0x4F,
                        0x50,
                        0x51,
                        0x52,
                        0x53,
                        0x54,
                        0x55,
                        0x56,
                        0x57,
                        0x58,
                        0x59,
                        0x5A
                    };

                    for (int vk : problematic_keys) {
                        if (GetAsyncKeyState(vk) & 0x8000) {
                            k = vk;
                            value_changed = true;
                            activation_time = GetTickCount64();
                            ImGui::ClearActiveID();
                            break;
                        }
                    }
                }
            }
        }

        if (Shortcut(ImGuiKey_Escape, 0, id)) {
            *key = 0;
            ImGui::ClearActiveID();
        }
        else {
            *key = k;
        }
    }

    window->DrawList->AddRectFilled(rect.Min, rect.Max, draw->get_clr(clr->keybind.button_bg), SCALE(elements->widgets.rounding));
    draw->render_text(window->DrawList, var->font.inter[2], rect.Min - SCALE(0, 1), rect.Max - SCALE(0, 1), draw->get_clr(clr->widgets.label), buf_display, NULL, NULL, ImVec2(0.5f, 0.5f));

    return value_changed;
}

bool hold_button(std::string_view label, std::string_view icon, bool reset = false)
{
    struct button_state
    {
        float alpha = 0.f;
        bool active = false;
    };

    ImGuiWindow* window = GetCurrentWindow();

    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const float width = GetContentRegionAvail().x;
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, pos + ImVec2(width, SCALE(25.f)));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);

    button_state* state = gui->anim_container(&state, id);

    if (pressed)
        state->active = true;

    state->alpha = ImClamp(state->alpha + (gui->fixed_speed(8.f) * (state->active ? 1.f : -1.f)), 0.f, 1.f);

    if (state->alpha >= 0.9f)
        state->active = false;

    window->DrawList->AddRectFilled(rect.Min, rect.Max, draw->get_clr(clr->keybind.button_bg, state->alpha), SCALE(elements->widgets.rounding));

    draw->render_text(window->DrawList, var->font.icons[6], rect.Min, ImVec2(rect.Min.x + SCALE(30), rect.Max.y), reset ? draw->get_clr(ImColor(255, 158, 158)) : draw->get_clr(clr->widgets.label), icon.data(), NULL, NULL, ImVec2(0.5f, 0.5f));
    draw->render_text(window->DrawList, var->font.inter[2], rect.Min + SCALE(30, -1), rect.Max - SCALE(0, 1), reset ? draw->get_clr(ImColor(255, 158, 158)) : draw->get_clr(clr->widgets.label), label.data(), NULL, NULL, ImVec2(0.0f, 0.5f));

    return pressed;
}

bool toggle_button(std::string_view label, std::string_view icon, bool* callback)
{
    ImGuiWindow* window = GetCurrentWindow();

    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const float width = GetContentRegionAvail().x;
    const ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, pos + ImVec2(width, SCALE(25.f)));

    ItemSize(rect, 0);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        *callback = !(*callback);

    float* state = gui->anim_container(&state, id);

    *state = ImClamp(*state + (gui->fixed_speed(8.f) * (*callback ? 1.f : -1.f)), 0.f, 1.f);

    window->DrawList->AddRectFilled(rect.Min, rect.Max, draw->get_clr(clr->keybind.button_bg, *state), SCALE(elements->widgets.rounding));

    draw->render_text(window->DrawList, var->font.icons[6], rect.Min, ImVec2(rect.Min.x + SCALE(30), rect.Max.y), draw->get_clr(clr->widgets.label), icon.data(), NULL, NULL, ImVec2(0.5f, 0.5f));
    draw->render_text(window->DrawList, var->font.inter[2], rect.Min + SCALE(30, -1), rect.Max - SCALE(0, 1), draw->get_clr(clr->widgets.label), label.data(), NULL, NULL, ImVec2(0.0f, 0.5f));
    draw->render_text(window->DrawList, var->font.icons[7], ImVec2(rect.Max.x - SCALE(30), rect.Min.y), rect.Max, draw->get_clr(clr->widgets.label, *state), "D", NULL, NULL, ImVec2(0.5f, 0.5f));

    return pressed;
}

bool mode_button_toggle(std::string_view label, bool active)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    ImVec2 pos = window->DC.CursorPos;

    const ImRect rect(pos, pos + SCALE(58, 20));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);



    float* state = gui->anim_container(&state, id);

    *state = ImClamp(*state + (gui->fixed_speed(8.f) * (active ? 1.f : -1.f)), 0.f, 1.f);

    window->DrawList->AddRectFilled(rect.Min, rect.Max, draw->get_clr(clr->keybind.background), SCALE(elements->widgets.rounding));
    window->DrawList->AddRectFilled(rect.Min, rect.Max, draw->get_clr(clr->accent, *state), SCALE(elements->widgets.rounding));

    draw->render_text(window->DrawList, var->font.inter[2], rect.Min - SCALE(0, 1), rect.Max - SCALE(0, 1), draw->get_clr(clr->widgets.label), label.data(), NULL, NULL, ImVec2(0.5f, 0.5f));

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

int mode_button(int* mode)
{
    PushStyleVar(ImGuiStyleVar_ItemSpacing, SCALE(4, 12));

    BeginGroup();
    {
        if (mode_button_toggle("Toggle", *mode == 0))
            *mode = 0;

        gui->sameline();

        if (mode_button_toggle("Hold", *mode == 1))
            *mode = 1;
    }
    EndGroup();
    PopStyleVar();

    return *mode;
}

void key_render_checkmark(ImDrawList* draw_list, ImVec2 pos, ImU32 col, float sz)
{
    float thickness = SCALE(1.f);
    pos += ImVec2(thickness * 0.25f, thickness * 0.25f);

    float third = sz / 3.0f;
    float bx = pos.x + third;
    float by = pos.y + sz - third * 0.5f;
    draw_list->PathLineTo(ImVec2(bx - third, by - third));
    draw_list->PathLineTo(ImVec2(bx, by));
    draw_list->PathLineTo(ImVec2(bx + third * 2.0f, by - third * 2.0f));
    draw_list->PathStroke(col, 0, thickness);
}

bool key_checkbox(std::string_view label, bool* callback)
{
    struct checkbox_state
    {
        ImVec4 label_clr = clr->widgets.label_inactive;
        float radius = 0.f;
        float rounding = 0.f;
        float alpha[2] = { 0.f, 0.f };
    };

    ImGuiWindow* window = GetCurrentWindow();
    bool& favorite_state = var->gui.child_states[window->ChildId];

    if (!favorite_state && var->gui.favorite_childs) return false;

    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    const ImRect total = gui->widget_rect();
    const ImRect rect(ImVec2(total.Min.x, total.GetCenter().y - SCALE(elements->checkbox.height) / 2), ImVec2(total.Max.x, total.GetCenter().y + SCALE(elements->checkbox.height) / 2));
    const ImRect clickable(ImVec2(rect.Max.x - SCALE(elements->checkbox.height), rect.Min.y), rect.Max);

    ItemSize(total, 0);
    if (!ItemAdd(total, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);
    if (pressed)
        *callback = !(*callback);

    checkbox_state* state = gui->anim_container(&state, id);
    state->label_clr = ImLerp(state->label_clr, *callback ? clr->widgets.label : clr->widgets.label_inactive, gui->fixed_speed(10.f));
    state->alpha[0] = ImClamp(state->alpha[0] + (gui->fixed_speed(6.f) * (*callback ? 1.f : -1.f)), 0.f, 1.f);
    state->alpha[1] = ImClamp(state->alpha[1] + (gui->fixed_speed(10.f) * (state->rounding <= SCALE(elements->widgets.rounding + 0.5f) ? 1.f : -1.f)), 0.f, 1.f);
    state->radius = ImLerp(state->radius, *callback ? clickable.GetWidth() / 2.f : 0.f, gui->fixed_speed(12.f));
    state->rounding = ImLerp(state->rounding, *callback ? SCALE(elements->widgets.rounding) : SCALE(30.f), gui->fixed_speed(12.f));

    window->DrawList->AddRectFilled(clickable.Min, clickable.Max, draw->get_clr(clr->widgets.background), SCALE(elements->widgets.rounding));

    const int vtx_start = window->DrawList->VtxBuffer.Size;
    window->DrawList->AddRectFilled(clickable.GetCenter() - ImVec2(state->radius, state->radius), clickable.GetCenter() + ImVec2(state->radius, state->radius), draw->get_clr(ImVec4(1.f, 1.f, 1.f, state->alpha[0])), state->rounding);
    const int vtx_end = window->DrawList->VtxBuffer.Size;
    ShadeVertsLinearColorGradientKeepAlpha(window->DrawList, vtx_start, vtx_end, clickable.Min, clickable.Max, draw->get_clr(clr->accent), draw->get_clr(ImVec4(clr->accent.x * 0.2f, clr->accent.y * 0.2f, clr->accent.z * 0.2f, 1.f)));

    window->DrawList->PushClipRect(clickable.GetCenter() - ImVec2(state->radius, state->radius), clickable.GetCenter() + ImVec2(state->radius, state->radius), true);
    key_render_checkmark(window->DrawList, clickable.GetCenter() - SCALE(3.5f, 4.f), draw->get_clr(clr->widgets.label, state->alpha[1]), SCALE(8.f));
    window->DrawList->PopClipRect();

    draw->render_text(window->DrawList, var->font.inter[2], total.Min - SCALE(elements->widgets.label_padding), total.Max - SCALE(elements->widgets.label_padding), draw->get_clr(state->label_clr), label.data(), NULL, NULL, ImVec2(0.f, 0.5f));

    return pressed;
}

bool c_gui::keybind(std::string_view label, keybind_state* state)
{
    struct keybind_statee
    {
        bool active[2] = { false, false };
        bool hovered = false;
        float alpha[2] = { 0.f, 0.f };
    };

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;



    const ImGuiID id = window->GetID(label.data());

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + SCALE(11, 11));

    ImGui::ItemSize(rect, style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id))
        return false;

    const bool hovered = ItemHoverable(rect, id, 0);

    keybind_statee* astate = gui->anim_container(&astate, id);

    if (hovered && g.IO.MouseClicked[0] || (astate->active[0] && g.IO.MouseClicked[0] && !astate->hovered))
        astate->active[0] = !astate->active[0];

    astate->alpha[0] = ImClamp(astate->alpha[0] + (8.f * g.IO.DeltaTime * (astate->active[0] ? 1.f : -1.f)), 0.f, 1.f);
    astate->alpha[1] = ImClamp(astate->alpha[1] + (8.f * g.IO.DeltaTime * (astate->active[1] ? 1.f : -1.f)), 0.f, 1.f);

    draw->render_text(window->DrawList, var->font.icons[5], rect.Min, rect.Max, draw->get_clr(clr->widgets.label_inactive), "C", NULL, NULL, ImVec2(0.5f, 0.5f));

    if (!IsRectVisible(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max + ImVec2(0, 2)))
    {
        astate->active[0] = false;
        astate->active[1] = false;
        astate->alpha[0] = 0.f;
        astate->alpha[1] = 0.f;
    }

    if (astate->alpha[0] <= 0.01f)
        astate->active[1] = false;

    gui->push_style_var(ImGuiStyleVar_WindowPadding, SCALE(10, 10));
    gui->push_style_var(ImGuiStyleVar_ItemSpacing, SCALE(10, 10));
    gui->push_style_color(ImGuiCol_PopupBg, draw->get_clr(clr->keybind.background));
    gui->push_style_var(ImGuiStyleVar_PopupBorderSize, 0.f);
    if (astate->alpha[0] >= 0.01f)
    {
        astate->hovered = (g.HoveredWindow && strstr(g.HoveredWindow->Name, (std::stringstream{} << "keybind_window" << id).str().c_str()) && IsMouseHoveringRect(g.HoveredWindow->Pos, g.HoveredWindow->Pos + g.HoveredWindow->Size, false)) ||
                (g.HoveredWindow && strstr(g.HoveredWindow->Name, (std::stringstream{} << "key_window" << id).str().c_str()) && IsMouseHoveringRect(g.HoveredWindow->Pos, g.HoveredWindow->Pos + g.HoveredWindow->Size, false));

        SetNextWindowSize(SCALE(140, 115));
        SetNextWindowPos(rect.Min);
        gui->push_style_var(ImGuiStyleVar_Alpha, astate->alpha[0]);
        gui->begin((std::stringstream{} << "keybind_window" << id).str(), NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse, true, draw->get_clr(clr->accent, 0.3f), SCALE(30.f), 0);
        {
            if (astate->active[0] && IsMouseHoveringRect(GetWindowPos(), GetWindowPos() + GetWindowSize()))
                SetWindowFocus();






            toggle_button("New Bind", "B", &astate->active[1]);

            if (hold_button("Reset", "C", true))
            {

                state->key = 0;
                state->mode = 0;
                state->value = 0;
                astate->active[1] = true;
            }

        }
        gui->end();
        gui->pop_style_var();

        if (astate->alpha[1] >= 0.01f)
        {
            SetNextWindowSize(SCALE(140, 100));
            SetNextWindowPos(rect.Min + SCALE(150, 7));
            gui->push_style_var(ImGuiStyleVar_Alpha, astate->alpha[1] * astate->alpha[0]);
            gui->begin((std::stringstream{} << "key_window" << id).str(), NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollWithMouse, true, draw->get_clr(clr->accent, 0.3f), SCALE(30.f), 0);
            {
                if (astate->active[1] && IsMouseHoveringRect(GetWindowPos(), GetWindowPos() + GetWindowSize()))
                    SetWindowFocus();

                key_select((std::stringstream{} << GetCurrentWindow()->ID << "key_select").str().c_str(), &state->key);



                SetCursorPosY(GetCursorPosY() - SCALE(9));


            }
            gui->end();
            gui->pop_style_var();
        }
    }
    gui->pop_style_color();
    gui->pop_style_var(3);

    return false;

}
