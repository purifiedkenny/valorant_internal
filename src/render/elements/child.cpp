#include <include/render/settings/functions.h>

bool c_gui::favorite_button(std::string_view label, std::string_view icon, int active)
{
    struct favorite_button_state
    {
        ImVec4 text_col = clr->favorite_button.label;
        float shadow_offset = 0.f;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label.data());

    ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + SCALE(elements->favorite_button.size));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    const bool selected = (active != 0) == var->gui.favorite_childs;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, 0);

    favorite_button_state* state = gui->anim_container(&state, id);
    state->text_col = ImLerp(state->text_col, selected ? clr->widgets.label : clr->favorite_button.label, gui->fixed_speed(8.f));
    state->shadow_offset = ImLerp(state->shadow_offset, selected ? elements->section.shadow_offset : 0.f, gui->fixed_speed(12.f));


    if (state->shadow_offset > 2.f)
        window->DrawList->AddShadowRect(rect.Min, rect.Max, draw->get_clr(clr->accent, 0.2f), SCALE(state->shadow_offset), ImVec2(0, 0), ImDrawFlags_ShadowCutOutShapeBackground, SCALE(elements->section.rounding));

    draw->render_text(window->DrawList, var->font.icons[2], rect.Min, rect.Max, draw->get_clr(state->text_col), icon.data(), NULL, NULL, ImVec2(0.16f, 0.5f));
    draw->render_text(window->DrawList, var->font.inter[0], rect.Min - SCALE(0, 1), rect.Max - SCALE(0, 1), draw->get_clr(state->text_col), label.data(), NULL, NULL, ImVec2(0.73f, 0.5f));

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool begin_child_ex(const char* name, const char* icon, ImGuiID id, const ImVec2& size_arg, ImGuiChildFlags child_flags, ImGuiWindowFlags window_flags)
{
    struct child_state
    {
        float state = 0.f;
        float slow = 0.f;
        ImVec4 favorite_col = clr->child.favorite_inactive;
    };

    child_state* state = gui->anim_container(&state, id);
    bool& favorite_state = var->gui.child_states[id];

    ImGuiContext& g = *GImGui;
    ImGuiWindow* parent_window = g.CurrentWindow;
    IM_ASSERT(id != 0);


    const ImGuiChildFlags ImGuiChildFlags_SupportedMask_ = ImGuiChildFlags_Border | ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY | ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_FrameStyle;
    IM_UNUSED(ImGuiChildFlags_SupportedMask_);
    IM_ASSERT((child_flags & ~ImGuiChildFlags_SupportedMask_) == 0 && "Illegal ImGuiChildFlags value. Did you pass ImGuiWindowFlags values instead of ImGuiChildFlags?");
    IM_ASSERT((window_flags & ImGuiWindowFlags_AlwaysAutoResize) == 0 && "Cannot specify ImGuiWindowFlags_AlwaysAutoResize for BeginChild(). Use ImGuiChildFlags_AlwaysAutoResize!");
    if (child_flags & ImGuiChildFlags_AlwaysAutoResize)
    {
        IM_ASSERT((child_flags & (ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY)) == 0 && "Cannot use ImGuiChildFlags_ResizeX or ImGuiChildFlags_ResizeY with ImGuiChildFlags_AlwaysAutoResize!");
        IM_ASSERT((child_flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) != 0 && "Must use ImGuiChildFlags_AutoResizeX or ImGuiChildFlags_AutoResizeY with ImGuiChildFlags_AlwaysAutoResize!");
    }
#ifndef IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    if (window_flags & ImGuiWindowFlags_AlwaysUseWindowPadding)
        child_flags |= ImGuiChildFlags_AlwaysUseWindowPadding;
#endif
    if (child_flags & ImGuiChildFlags_AutoResizeX)
        child_flags &= ~ImGuiChildFlags_ResizeX;
    if (child_flags & ImGuiChildFlags_AutoResizeY)
        child_flags &= ~ImGuiChildFlags_ResizeY;


    window_flags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar;
    window_flags |= (parent_window->Flags & ImGuiWindowFlags_NoMove);
    if (child_flags & (ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize))
        window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
    if ((child_flags & (ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY)) == 0)
        window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;


    if (child_flags & ImGuiChildFlags_FrameStyle)
    {
        PushStyleColor(ImGuiCol_ChildBg, g.Style.Colors[ImGuiCol_FrameBg]);
        PushStyleVar(ImGuiStyleVar_ChildRounding, g.Style.FrameRounding);
        PushStyleVar(ImGuiStyleVar_ChildBorderSize, g.Style.FrameBorderSize);
        PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.FramePadding);
        child_flags |= ImGuiChildFlags_Border | ImGuiChildFlags_AlwaysUseWindowPadding;
        window_flags |= ImGuiWindowFlags_NoMove;
    }


    g.NextWindowData.Flags |= ImGuiNextWindowDataFlags_HasChildFlags;
    g.NextWindowData.ChildFlags = child_flags;






    ImVec2 size = size_arg;
    if (size.x <= 0)
        size.x = (GetWindowWidth() - SCALE(elements->content.spacing.x) - SCALE(elements->content.padding.x * 2.f)) / 2.f;
    if (size.y <= 0)
        size.y = state->state;

    gui->set_next_window_size(size - SCALE(0, elements->child.titlebar_height));
    gui->set_next_window_pos(parent_window->DC.CursorPos + SCALE(0, elements->child.titlebar_height));




    const char* temp_window_name;
    


    if (name)
        ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%s_%08X", parent_window->Name, name, id);
    else
        ImFormatStringToTempBuffer(&temp_window_name, NULL, "%s/%08X", parent_window->Name, id);


    const float backup_border_size = g.Style.ChildBorderSize;
    if ((child_flags & ImGuiChildFlags_Border) == 0)
        g.Style.ChildBorderSize = 0.0f;

    state->favorite_col = ImLerp(state->favorite_col, favorite_state ? clr->child.favorite_active : clr->child.favorite_inactive, gui->fixed_speed(12.f));

    if (var->gui.content_alpha <= 0.1f)
    {
        state->state = 0.f;
        state->slow = 0.f;
        state->favorite_col = clr->child.favorite_inactive;
    }

    maxi_chamfer_fill(parent_window->DrawList, parent_window->DC.CursorPos,
        parent_window->DC.CursorPos + size, maxi_chamfer(size.y), draw->get_clr(clr->child.background));
    maxi_chamfer_stroke(parent_window->DrawList, parent_window->DC.CursorPos,
        parent_window->DC.CursorPos + size, maxi_chamfer(size.y), draw->get_clr(clr->child.stroke), SCALE(1.f));
    parent_window->DrawList->AddLine(parent_window->DC.CursorPos + SCALE(0, elements->child.titlebar_height - 1), parent_window->DC.CursorPos + ImVec2(size.x, SCALE(elements->child.titlebar_height - 1)), draw->get_clr(clr->child.line), SCALE(1.f));

    parent_window->DrawList->AddRectFilled(
        parent_window->DC.CursorPos + SCALE(14, elements->child.titlebar_height - 1),
        parent_window->DC.CursorPos + SCALE(42, elements->child.titlebar_height + 1), draw->get_clr(clr->accent));
    draw->render_text(parent_window->DrawList, var->font.inter[0], parent_window->DC.CursorPos + SCALE(14, 0), parent_window->DC.CursorPos + ImVec2(size.x, SCALE(elements->child.titlebar_height - 1)), draw->get_clr(clr->child.label), name, NULL, NULL, ImVec2(0.f, 0.5f));


    const bool ret = gui->begin(temp_window_name, NULL, window_flags, 0, 0, 0, 0);


    g.Style.ChildBorderSize = backup_border_size;
    if (child_flags & ImGuiChildFlags_FrameStyle)
    {
        PopStyleVar(3);
        PopStyleColor();
    }

    ImGuiWindow* child_window = g.CurrentWindow;
    child_window->ChildId = id;
    state->slow = ImLerp(state->slow, child_window->ContentSize.y + SCALE(elements->child.titlebar_height) + SCALE(elements->widgets.padding.y) * 2, gui->fixed_speed(12.f));
    state->state = state->slow;


    if (child_window->BeginCount == 1)
        parent_window->DC.CursorPos = child_window->Pos;



    const ImGuiID temp_id_for_activation = ImHashStr("##Child", 0, id);
    if (g.ActiveId == temp_id_for_activation)
        ClearActiveID();
    if (g.NavActivateId == id && !(window_flags & ImGuiWindowFlags_NavFlattened) && (child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY))
    {
        FocusWindow(child_window);
        NavInitWindow(child_window, false);
        SetActiveID(temp_id_for_activation, child_window);
        g.ActiveIdSource = g.NavInputSource;
    }
    return ret;
}

void c_gui::begin_child(std::string_view name, std::string_view icon, const ImVec2& size)
{
    ImGuiID id = GetCurrentWindow()->GetID(name.data());
    bool& favorite_state = var->gui.child_states[id];

    if (!favorite_state && var->gui.favorite_childs) return;

    gui->push_style_var(ImGuiStyleVar_WindowPadding, SCALE(elements->widgets.padding));
    gui->push_style_color(ImGuiCol_ChildBg, draw->get_clr(ImVec4(0.f, 0.f, 0.f, 0.f)));
    begin_child_ex(name.data(), icon.data(), id, size, ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    gui->push_style_var(ImGuiStyleVar_ItemSpacing, SCALE(0, 0));
}

void c_gui::end_child()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* child_window = g.CurrentWindow;

    bool& favorite_state = var->gui.child_states[child_window->ChildId];

    if (!favorite_state && var->gui.favorite_childs) return;

    IM_ASSERT(g.WithinEndChild == false);
    IM_ASSERT(child_window->Flags & ImGuiWindowFlags_ChildWindow);

    gui->pop_style_var();

    g.WithinEndChild = true;
    ImVec2 child_size = child_window->Size;
    gui->end();
    if (child_window->BeginCount == 1)
    {
        ImGuiWindow* parent_window = g.CurrentWindow;
        ImRect bb(parent_window->DC.CursorPos, parent_window->DC.CursorPos + child_size);
        ItemSize(child_size);
        if ((child_window->DC.NavLayersActiveMask != 0 || child_window->DC.NavWindowHasScrollY) && !(child_window->Flags & ImGuiWindowFlags_NavFlattened))
        {
            ItemAdd(bb, child_window->ChildId);
            RenderNavHighlight(bb, child_window->ChildId);


            if (child_window->DC.NavLayersActiveMask == 0 && child_window == g.NavWindow)
                RenderNavHighlight(ImRect(bb.Min - ImVec2(2, 2), bb.Max + ImVec2(2, 2)), g.NavId, ImGuiNavHighlightFlags_Compact);
        }
        else
        {

            ItemAdd(bb, 0);


            if (child_window->Flags & ImGuiWindowFlags_NavFlattened)
                parent_window->DC.NavLayersActiveMaskNext |= child_window->DC.NavLayersActiveMaskNext;
        }
        if (g.HoveredWindow == child_window)
            g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
    }
    gui->pop_style_var();
    gui->pop_style_color();
    g.WithinEndChild = false;
    g.LogLinePosY = -FLT_MAX;
}
