#include <include/render/settings/functions.h>

bool c_gui::section(std::string_view icon, int section_id, int& section_variable)
{
    struct section_state
    {
        ImVec4 label_color = clr->section.icon;
        float selected_alpha = 0.f;
    };

    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID((std::stringstream{} << "section_" << icon).str().c_str());
    const bool selected = section_id == section_variable;

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect rect(pos, pos + SCALE(elements->section.size));
    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held);

    if (pressed)
        section_variable = section_id;


    section_state* state = gui->anim_container(&state, id);
    state->label_color = ImLerp(state->label_color,
        selected ? clr->widgets.label : (hovered ? clr->widgets.label_inactive : clr->section.icon),
        gui->fixed_speed(10.f));
    state->selected_alpha = ImLerp(state->selected_alpha, selected ? 1.0f : 0.0f, gui->fixed_speed(10.f));

    if (state->selected_alpha > 0.01f) {
        maxi_chamfer_gradient(window->DrawList, rect.Min, rect.Max, maxi_chamfer(rect.GetHeight()),
            draw->get_clr(clr->accent_from, state->selected_alpha),
            draw->get_clr(clr->accent_to, state->selected_alpha));
    }
    else if (hovered) {
        maxi_chamfer_fill(window->DrawList, rect.Min, rect.Max, maxi_chamfer(rect.GetHeight()),
            draw->get_clr(clr->child.background));
    }

    draw->render_text(window->DrawList, var->font.inter[0], rect.Min + SCALE(16, 0), rect.Max,
        draw->get_clr(state->label_color), icon.data(), NULL, NULL, ImVec2(0.0f, 0.5f));

    return pressed;
}
