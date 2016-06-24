/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "input_method_setting_list.h"
#include "input_method_setting_list_ui.h"
#include "input_method_setting_list_popup_view.h"
#include <string>
#include <efl_extension.h>
#include <vector>
#include <isf_control.h>
#include <algorithm>
#include <inputmethod_manager.h>

#define IM_SETTING_LIST_POPUP_VIEW_TITLE          dgettext(PACKAGE, "IDS_ST_HEADER_DEFAULT_KEYBOARD_ABB")
#define IM_SETTING_SELECT_KEYBOARD                dgettext(PACKAGE, "IDS_IME_BODY_SELECT_KEYBOARD")

static std::vector<ime_info_s>      g_active_ime_info_list;
static Elm_Genlist_Item_Class       *itc_im_list = NULL;
static Evas_Object                  *group_radio = NULL;
static int                          g_active_ime_id = -1;

typedef struct {
    void        *data;
    int         index;
} sel_cb_data;

class ime_info_compare
{
    public:
    bool operator()(const ime_info_s &first, const ime_info_s &sec)
    {
        return (strcasecmp(first.label, sec.label) < 0);
    }
};

static void im_setting_list_sort_ime_info(std::vector<ime_info_s> &preinstall, std::vector<ime_info_s> &user)
{
    std::sort(preinstall.begin(), preinstall.end(), ime_info_compare());
    std::sort(user.begin(), user.end(), ime_info_compare());
    for (unsigned int i = 0; i < preinstall.size(); ++i)
    {
        g_active_ime_info_list.push_back(preinstall[i]);
    }
    for (unsigned int i = 0; i < user.size(); ++i)
    {
        g_active_ime_info_list.push_back(user[i]);
    }
}

static void im_setting_list_load_active_ime_info(void)
{
    std::vector<ime_info_s>      active_ime_info_list_preinstall;
    std::vector<ime_info_s>      active_ime_info_list_user;
    g_active_ime_info_list.clear();
    char *active_ime_appid = NULL;

    int ret = ime_manager_get_active_ime(&active_ime_appid);
    if (ret == IME_MANAGER_ERROR_NONE)
        LOGD("get active ime : %s\n", active_ime_appid);
    else
        LOGW("Failed to get active ime. error : %d\n", ret);

    ime_info_s *info = NULL;
    int cnt = isf_control_get_all_ime_info(&info);
    if (info)
    {
        for (int i = 0; i < cnt; ++i)
        {
            SECURE_LOGD("%s %s %d %d %d\n", info[i].appid, info[i].label, info[i].is_enabled, info[i].is_preinstalled, info[i].has_option);
            if (info[i].is_enabled && info[i].is_preinstalled) {
                active_ime_info_list_preinstall.push_back(info[i]);
            } else if (info[i].is_enabled) {
                active_ime_info_list_user.push_back(info[i]);
            }
        }
        free(info);
    }

    im_setting_list_sort_ime_info(active_ime_info_list_preinstall, active_ime_info_list_user);
    for (unsigned int i = 0; i < g_active_ime_info_list.size(); ++i)
    {
        if (active_ime_appid && (!strcmp(active_ime_appid, g_active_ime_info_list[i].appid)))
        {
            g_active_ime_id = i;
        }
    }

    if (active_ime_appid)
    {
        free(active_ime_appid);
    }
}

static void im_setting_list_radio_change_cb(void *data, Evas_Object *obj, void *event_info)
{
    /*save the checked ime*/
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_active_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_active_ime_info_list.size() : %d\n", index, g_active_ime_info_list.size());
        return;
    }
}

static void im_setting_list_update_radio_state(Elm_Object_Item *item, Evas_Object *obj, int index)
{
    if (index < 0 || index >= (int)g_active_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_active_ime_info_list.size() : %d\n", index, g_active_ime_info_list.size());
        return;
    }

    if (item && obj) {
        elm_genlist_item_selected_set(item, EINA_FALSE);
        /* Update check button */
        Evas_Object *radio = elm_object_item_part_content_get(item, "elm.icon.right");
        if (radio == NULL) {
            radio = elm_object_item_part_content_get(item, "elm.icon");
        }
        evas_object_data_set(radio, "parent_genlist", obj);
        elm_radio_value_set(radio, index);
        isf_control_set_active_ime(g_active_ime_info_list[index].appid);
    }
}

void im_setting_list_update_window_selector(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return;
    im_setting_list_load_active_ime_info();
    im_setting_list_update_window(ad);
}

static void im_setting_list_ime_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    sel_cb_data * cb_data = (sel_cb_data *)data;
    if (!cb_data)
        return;
    appdata *ad = (appdata *)cb_data->data;
    if (!ad)
        return;
    int index = cb_data->index;

    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    if (!item) {
        delete cb_data;
        return;
    }
    im_setting_list_update_radio_state(item, obj, index);
    im_setting_list_update_window_selector(ad);

    if (ad->popup) {
        evas_object_del(ad->popup);
    }
    ad->popup = NULL;
    delete cb_data;
    elm_naviframe_item_pop(ad->naviframe);
}

static Evas_Object *im_setting_list_genlist_create(Evas_Object* parent, Evas_Object* conform)
{
    if (!parent)
        return NULL;
    Evas_Object *genlist = elm_genlist_add(parent);
    elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
#ifdef _CIRCLE
    /* Circle Surface Creation */
    Eext_Circle_Surface *circle_surface = eext_circle_surface_conformant_add(conform);
    Evas_Object *circle_genlist = eext_circle_object_genlist_add(genlist, circle_surface);
    eext_rotary_object_event_activated_set(circle_genlist, EINA_TRUE);
#endif
    evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);
    evas_object_show(genlist);
    return genlist;
}

static char *im_setting_list_genlist_item_label_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_active_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_active_ime_info_list.size() : %d\n", index, g_active_ime_info_list.size());
        return NULL;
    }

    if (!strcmp(part, "elm.text") ||
        !strcmp(part, "elm.text.main") ||
        !strcmp(part, "elm.text.main.left")) {
        return strdup(g_active_ime_info_list[index].label);
    }

    return NULL;
}

static Evas_Object *im_setting_list_genlist_item_icon_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)reinterpret_cast<long>(data);
    if (!strcmp(part, "elm.swallow.end") ||
        !strcmp(part, "elm.icon.2")) {
        Evas_Object *radio = elm_radio_add(obj);
        elm_object_style_set(radio, "list");
        elm_radio_state_value_set(radio, index);
        elm_radio_group_add(radio, group_radio);
        evas_object_show(radio);
        evas_object_smart_callback_add(radio, "changed", im_setting_list_radio_change_cb, (void *) (index));

        return radio;
    }
    return NULL;
}

static void im_setting_list_genlist_item_class_create(void)
{
    itc_im_list = elm_genlist_item_class_new();
    if (itc_im_list) {
        itc_im_list->item_style = "type1";
        itc_im_list->func.text_get = im_setting_list_genlist_item_label_get;
        itc_im_list->func.content_get = im_setting_list_genlist_item_icon_get;
        itc_im_list->func.state_get = NULL;
        itc_im_list->func.del = NULL;
    }
}

#ifndef _WEARABLE
static Evas_Object *im_setting_list_list_create(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return NULL;
    im_setting_list_genlist_item_class_create();
    Evas_Object *genlist = NULL;
    genlist = im_setting_list_genlist_create(ad->popup, ad->conform);
    unsigned int i = 0;

    /* keyboard list */
    for (i = 0; i < g_active_ime_info_list.size(); i++) {
        sel_cb_data *cb_data = new sel_cb_data;
        cb_data->data = data;
        cb_data->index = i;
        elm_genlist_item_append(genlist,
            itc_im_list,
            (void *)(i),
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_ime_sel_cb,
            (void *)(cb_data));
    }
    elm_radio_value_set(group_radio, g_active_ime_id);

    return genlist;
}

static void
im_setting_list_popup_block_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return;
    if (ad->popup) {
        evas_object_del(ad->popup);
    }
    ad->popup = NULL;
}

static void im_setting_list_popup_view_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return;
    eext_object_event_callback_del(obj, EEXT_CALLBACK_BACK, im_setting_list_popup_view_back_cb);
    if (ad->popup) {
        evas_object_del(ad->popup);
    }
    ad->popup = NULL;
}

static Evas_Object *im_setting_list_popup_create(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad || !ad->win)
        return NULL;
    Evas_Object *parentWin = ad->win;
    if (NULL == group_radio)
    {
        group_radio = elm_radio_add(parentWin);
        elm_radio_state_value_set(group_radio, -1);
    }

    Evas_Object *popup = elm_popup_add(parentWin);
    elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    evas_object_smart_callback_add(popup, "block,clicked", im_setting_list_popup_block_clicked_cb, data);
    elm_object_part_text_set(popup, "title,text", IM_SETTING_LIST_POPUP_VIEW_TITLE);
    elm_object_style_set(popup, "theme_bg");
    eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, im_setting_list_popup_view_back_cb, data);
    ad->popup = popup;

    Evas_Object *genlist = im_setting_list_list_create(data);
    elm_object_content_set(popup, genlist);
    evas_object_show(popup);
    return popup;
}
#endif

#ifdef _WEARABLE
static char *
im_setting_list_default_keyboard_title_text_get(void *data, Evas_Object *obj, const char *part)
{
    return strdup(IM_SETTING_LIST_POPUP_VIEW_TITLE);
}

static Eina_Bool _pop_cb(void *data, Elm_Object_Item *it)
{
#ifdef _CIRCLE
    appdata *ad = (appdata *)data;
    if (ad && ad->main_circle_genlist)
        eext_rotary_object_event_activated_set(ad->main_circle_genlist, EINA_TRUE);
#endif

    return EINA_TRUE;
}

static void im_setting_list_screen_create(void *data)
{
    appdata *ad = NULL;
    Evas_Object *genlist = NULL;
    Elm_Genlist_Item_Class *ttc = elm_genlist_item_class_new();

    ad = (appdata *) data;
    if (ad == NULL) return;

    ttc->item_style = "title";
    ttc->func.text_get = im_setting_list_default_keyboard_title_text_get;

    im_setting_list_genlist_item_class_create();
    genlist = im_setting_list_genlist_create(ad->win, ad->conform);

    elm_genlist_mode_set(genlist, ELM_LIST_SCROLL);
    elm_genlist_item_append(genlist, ttc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

    if (NULL == group_radio) {
        group_radio = elm_radio_add(ad->win);
        elm_radio_state_value_set(group_radio, g_active_ime_id);
    }

    /* keyboard list */
    for (unsigned int i = 0; i < g_active_ime_info_list.size(); i++) {
        sel_cb_data *cb_data = new sel_cb_data;
        cb_data->data = data;
        cb_data->index = i;
        elm_genlist_item_append(genlist,
            itc_im_list,
            (void *)(i),
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_ime_sel_cb,
            (void *)(cb_data));
    }

    elm_radio_state_value_set(group_radio, g_active_ime_id);
    elm_radio_value_set(group_radio, g_active_ime_id);
    elm_genlist_item_class_free(ttc);

#ifdef _CIRCLE
    im_setting_list_add_padding(genlist);
#endif

    Elm_Object_Item *navi_it = elm_naviframe_item_push(ad->naviframe, NULL, NULL, NULL, genlist, "empty");
    elm_naviframe_item_pop_cb_set(navi_it, _pop_cb, ad);
}
#endif

void
im_setting_list_popup_view_create(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad || !ad->win)
        return;

    im_setting_list_load_active_ime_info();
#ifdef _WEARABLE
    im_setting_list_screen_create(data);
#else
    im_setting_list_popup_create(data);
#endif
}
