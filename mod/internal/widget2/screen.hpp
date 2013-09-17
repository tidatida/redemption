/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Product name: redemption, a FLOSS RDP proxy
 *   Copyright (C) Wallix 2010-2012
 *   Author(s): Christophe Grosjean, Dominique Lafages, Jonathan Poelen,
 *              Meng Tan
 */

#if !defined(REDEMPTION_MOD_INTERNAL_WIDGET2_SCREEN_HPP)
#define REDEMPTION_MOD_INTERNAL_WIDGET2_SCREEN_HPP

#include "composite.hpp"
#include "tooltip.hpp"

#include <typeinfo>

class WidgetScreen : public WidgetParent
{
    CompositeInterface * impl;
public:
    WidgetTooltip * tooltip;

    WidgetScreen(DrawApi& drawable, uint16_t width, uint16_t height, NotifyApi * notifier = NULL)
        : WidgetParent(drawable, Rect(0, 0, width, height), *this, notifier)
        , tooltip(NULL)
    {
        this->impl = new CompositeVector;
        this->tab_flag = IGNORE_TAB;
    }

    virtual ~WidgetScreen()
    {
        if (this->tooltip) {
            delete this->tooltip;
            this->tooltip = NULL;
        }
        if (this->impl) {
            delete this->impl;
            this->impl = NULL;
        }
    }


    virtual bool tooltip_exist(int = 10) {
        return (this->tooltip);
    }
    virtual void show_tooltip(Widget2 * widget, const char * text, int x, int y, int = 10) {
        if (text == NULL) {
            if (this->tooltip) {
                this->remove_widget(this->tooltip);
                this->refresh(this->tooltip->rect);
                delete this->tooltip;
                this->tooltip = NULL;
            }
        }
        else if (this->tooltip == NULL) {
            int w = 0;
            int h = 0;
            this->drawable.text_metrics(text, w, h);
            int sw = this->rect.cx;
            int posx = ((x + w) > sw)?(sw - w):x;
            int posy = (y > h)?(y - h):0;
            this->tooltip = new WidgetTooltip(this->drawable,
                                              posx,
                                              posy,
                                              *this,
                                              widget,
                                              text);
            this->add_widget(this->tooltip);
            this->refresh(this->tooltip->rect);
        }
    }

    virtual void rdp_input_mouse(int device_flags, int x, int y, Keymap2* keymap)
    {
        if (this->tooltip) {
            if (device_flags & MOUSE_FLAG_MOVE) {
                Widget2 * w = this->last_widget_at_pos(x, y);
                if (w != this->tooltip->notifier) {
                    this->hide_tooltip();
                }
            }
            if (device_flags & (MOUSE_FLAG_BUTTON1)) {
                this->hide_tooltip();
            }
        }
        WidgetParent::rdp_input_mouse(device_flags, x, y, keymap);
    }

    virtual void rdp_input_scancode(long int param1, long int param2, long int param3, long int param4, Keymap2* keymap)
    {
        this->hide_tooltip();
        if (this->tab_flag != IGNORE_TAB) {
            WidgetParent::rdp_input_scancode(param1, param2, param3, param4, keymap);
        }
        else if (this->current_focus) {
            this->current_focus->rdp_input_scancode(param1, param2, param3, param4, keymap);
        }

        for (uint32_t n = keymap->nb_kevent_available(); n ; --n) {
            keymap->get_kevent();
        }
    }

    virtual void draw(const Rect& clip)
    {
        Rect new_clip = clip.intersect(this->rect);
        this->impl->draw(new_clip);
        this->draw_inner_free(clip, BLACK);
    }

    virtual void add_widget(Widget2 * w) {
        this->impl->add_widget(w);
    }
    virtual void remove_widget(Widget2 * w) {
        this->impl->remove_widget(w);
    }
    virtual void clear() {
        this->impl->clear();
    }

    virtual void set_xy(int16_t x, int16_t y) {
        int16_t xx = x - this->dx();
        int16_t yy = y - this->dy();
        this->impl->set_xy(xx, yy);
        WidgetParent::set_xy(x, y);
    }

    virtual Widget2 * widget_at_pos(int16_t x, int16_t y) {
        if (!this->rect.contains_pt(x, y))
            return 0;
        if (this->current_focus) {
            if (this->current_focus->rect.contains_pt(x, y)) {
                return this->current_focus;
            }
        }
        return this->impl->widget_at_pos(x, y);
    }

    virtual bool next_focus() {
        return this->impl->next_focus(this);
    }

    virtual bool previous_focus() {
        return this->impl->previous_focus(this);
    }

    virtual void draw_inner_free(const Rect& clip, int bg_color) {
        Region region;
        region.rects.push_back(clip);

        this->impl->draw_inner_free(clip, bg_color, region);

        for (std::size_t i = 0, size = region.rects.size(); i < size; ++i) {
            this->drawable.draw(RDPOpaqueRect(region.rects[i], bg_color), region.rects[i]);
        }
    }

};

#endif
