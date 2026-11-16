#pragma once
#include "imgui.h"

class c_colors
{
public:

    ImVec4 accent = ImColor(242, 7, 7);
    ImVec4 accent_from = ImColor(255, 8, 8);
    ImVec4 accent_to = ImColor(120, 2, 2);

    struct
    {
        ImVec4 background = ImColor(10, 7, 8);
        ImVec4 bar = ImColor(6, 4, 5);
        ImVec4 border = ImColor(58, 38, 40);
        ImVec4 name = ImColor(255, 255, 255);
        ImVec4 year = ImColor(110, 99, 101);
    } window;

    struct
    {
        ImVec4 label = ImColor(255, 255, 255);
        ImVec4 background = ImColor(18, 13, 14);
        ImVec4 stroke = ImColor(39, 27, 29);
        ImVec4 line = ImColor(39, 27, 29);
        ImVec4 favorite_inactive = ImColor(110, 99, 101);
        ImVec4 favorite_active = ImColor(242, 7, 7);
    } child;

    struct
    {
        ImVec4 label = ImColor(255, 255, 255);
        ImVec4 label_inactive = ImColor(180, 172, 173);
        ImVec4 background = ImColor(9, 6, 7);
    } widgets;

    struct
    {
        ImVec4 icon = ImColor(110, 99, 101);
    } section;

    struct
    {
        ImVec4 label = ImColor(180, 172, 173);
    } favorite_button;

    struct
    {
        ImVec4 background = ImColor(9, 6, 7);
    } dropdown;

    struct
    {
        ImVec4 background = ImColor(9, 6, 7);
    } colorpicker;

    struct
    {
        ImVec4 background = ImColor(9, 6, 7);
        ImVec4 button_bg = ImColor(31, 22, 24);
    } keybind;

    struct
    {
        ImVec4 background = ImColor(18, 13, 14);
        ImVec4 label = ImColor(255, 255, 255);
    } notify;

    struct
    {
        ImVec4 background = ImColor(6, 4, 5);
        ImVec4 text = ImColor(255, 255, 255);
        ImVec4 line = ImColor(242, 7, 7);
    } watermark;

};

inline c_colors* clr = new c_colors();
