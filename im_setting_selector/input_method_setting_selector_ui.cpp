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
#include "input_method_setting_selector.h"
#include <string>
#include <app.h>
#include <efl_extension.h>
#include <vector>
#include <isf_control.h>
#include <app_control.h>
#include <algorithm>
#include <tzplatform_config.h>
#include <inputmethod_manager.h>

#define IM_SETTING_SELECTOR_PACKAGE        PACKAGE
#define IM_SETTING_SELECTOR_LOCALE_DIR     tzplatform_mkpath(TZ_SYS_RO_APP, PACKAGE_NAME"/res/locale")
#define IM_SETTING_SELECTOR_TITLE          dgettext(PACKAGE, "IDS_ST_HEADER_DEFAULT_KEYBOARD_ABB")
#define IM_SETTING_SELECT_KEYBOARD         dgettext(PACKAGE, "IDS_IME_BODY_SELECT_KEYBOARD")

static std::vector<ime_info_s>      g_ime_info_list;
static Elm_Genlist_Item_Class       *itc_im_selector = NULL;
static Evas_Object                  *group_radio = NULL;
static int                          g_active_ime_id = -1;

typedef struct {
    void *data;
    int index;
} sel_cb_data;

class ime_info_compare
{
    public:
    bool operator()(const ime_info_s &first, const ime_info_s &sec)
    {
        return (strcasecmp(first.label, sec.label) < 0);
    }
};

static void im_setting_selector_text_domain_set(void)
{
    bindtextdomain(IM_SETTING_SELECTOR_PACKAGE, IM_SETTING_SELECTOR_LOCALE_DIR);
    textdomain(IM_SETTING_SELECTOR_PACKAGE);
}

static Evas_Object *
im_setting_selector_main_window_create(const char *name)
{
    Evas_Object *eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
    if (eo) {
        Evas *e;
        Ecore_Evas *ee;
        Evas_Coord w = -1, h = -1;

        elm_win_title_set(eo, name);
        elm_win_borderless_set(eo, EINA_TRUE);
        elm_win_alpha_set(eo, EINA_TRUE);
        elm_win_conformant_set(eo, EINA_TRUE);
        elm_win_autodel_set(eo, EINA_TRUE);
        if (elm_win_wm_rotation_supported_get(eo)) {
            int rots[4] = {0, 90, 180, 270};
            elm_win_wm_rotation_available_rotations_set(eo, rots, 4);
        }
        e = evas_object_evas_get(eo);
        if (e) {
            ee = ecore_evas_ecore_evas_get(e);
            if (ee) {
                ecore_evas_name_class_set(ee, "SYSTEM_POPUP", "SYSTEM_POPUP");
            }
        }

        elm_win_screen_size_get(eo, NULL, NULL, &w, &h);
        if (w > 0 && h > 0) {
            evas_object_resize(eo, w, h);
        }
    }
    return eo;
}

static void im_setting_selector_sort_ime_info(std::vector<ime_info_s> &preinstall, std::vector<ime_info_s> &user)
{
    std::sort(preinstall.begin(), preinstall.end(), ime_info_compare());
    std::sort(user.begin(), user.end(), ime_info_compare());
    for (unsigned int i = 0; i < preinstall.size(); ++i)
    {
        g_ime_info_list.push_back(preinstall[i]);
    }
    for (unsigned int i = 0; i < user.size(); ++i)
    {
        g_ime_info_list.push_back(user[i]);
    }
}

static void im_setting_selector_load_ime_info(void)
{
    std::vector<ime_info_s>      ime_info_list_preinstall;
    std::vector<ime_info_s>      ime_info_list_user;
    g_ime_info_list.clear();
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
            if (info[i].is_enabled && info[i].is_preinstalled)
            {
                ime_info_list_preinstall.push_back(info[i]);
            }
            else if (info[i].is_enabled)
            {
                ime_info_list_user.push_back(info[i]);
            }
        }
        free(info);
    }

    im_setting_selector_sort_ime_info(ime_info_list_preinstall, ime_info_list_user);
    for (unsigned int i = 0; i < g_ime_info_list.size(); ++i)
    {
        if (active_ime_appid && (!strcmp(active_ime_appid, g_ime_info_list[i].appid)))
        {
            g_active_ime_id = i;
        }
    }

    if (active_ime_appid)
    {
        free(active_ime_appid);
    }
}

#ifdef _MOBILE
static void im_setting_selector_show_ime_list(void)
{
     int ret;
     app_control_h app_control;
     const char *app_id = "org.tizen.inputmethod-setting-list";
     ret = app_control_create(&app_control);
     if (ret != APP_CONTROL_ERROR_NONE) {
         LOGD("app_control_create returned %d\n", ret);
         return;
     }

     ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
     if (ret != APP_CONTROL_ERROR_NONE) {
         LOGD("app_control_set_operation returned %d\n", ret);
         app_control_destroy(app_control);
         return;
     }

     ret = app_control_set_app_id(app_control, app_id);
     if (ret != APP_CONTROL_ERROR_NONE) {
         LOGD("app_control_set_app_id returned %d\n", ret);
         app_control_destroy(app_control);
         return;
     }

     ret = app_control_send_launch_request(app_control, NULL, NULL);
     if (ret != APP_CONTROL_ERROR_NONE) {
         LOGD("app_control_send_launch_request returned %d, %s\n", ret, get_error_message(ret));
         app_control_destroy(app_control);
        return;
     }
     app_control_destroy(app_control);
}

static void im_setting_selector_select_keyboard_cb(void *data, Evas_Object *obj, void *event_info)
{
    /* call input method list application*/
    im_setting_selector_show_ime_list();
}
#endif

static void im_setting_selector_radio_change_cb(void *data, Evas_Object *obj, void *event_info)
{
    /*save the checked ime*/
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return;
    }
}

static void im_setting_selector_update_radio_state(Elm_Object_Item *item, Evas_Object *obj, int index)
{
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return;
    }

    if (item && obj) {
        elm_genlist_item_selected_set(item, EINA_FALSE);
        /* Update check button */
        Evas_Object *radio = elm_object_item_part_content_get(item, "elm.swallow.end");
        if (radio == NULL) {
            radio = elm_object_item_part_content_get(item, "elm.icon");
        }
        evas_object_data_set(radio, "parent_genlist", obj);
        elm_radio_value_set(radio, index);
        isf_control_set_active_ime(g_ime_info_list[index].appid);
    }
}

static void im_setting_selector_ime_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    sel_cb_data *cb_data = (sel_cb_data *)data;
    if (!cb_data)
        return;
    int index = cb_data->index;
    appdata *ad = (appdata *)cb_data->data;
    if (!ad)
        return;

    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    if (!item)
        return;
    im_setting_selector_update_radio_state(item, obj, index);

    delete cb_data;
    elm_naviframe_item_pop(ad->naviframe);
    ui_app_exit();
}

static Evas_Object *im_setting_selector_genlist_create(Evas_Object* parent, Evas_Object* conform)
{
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

static char *im_setting_selector_genlist_item_label_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return NULL;
    }

    if (!strcmp(part, "elm.text") ||
        !strcmp(part, "elm.text.main") ||
        !strcmp(part, "elm.text.main.left")) {
        return strdup(g_ime_info_list[index].label);
    }

    return NULL;
}

static Evas_Object *im_setting_selector_genlist_item_icon_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)reinterpret_cast<long>(data);
    if (!strcmp(part, "elm.swallow.end") ||
        !strcmp(part, "elm.icon.2")) {
        Evas_Object *radio = elm_radio_add(obj);
        elm_object_style_set(radio, "list");
        elm_radio_state_value_set(radio, index);
        elm_radio_group_add(radio, group_radio);
        evas_object_show(radio);
        evas_object_smart_callback_add(radio, "changed", im_setting_selector_radio_change_cb, (void *) (index));

        return radio;
    }
    return NULL;
}

static void im_setting_selector_genlist_item_class_create(void)
{
    itc_im_selector = elm_genlist_item_class_new();
    if (itc_im_selector) {
        itc_im_selector->item_style = "type1";
        itc_im_selector->func.text_get = im_setting_selector_genlist_item_label_get;
        itc_im_selector->func.content_get = im_setting_selector_genlist_item_icon_get;
        itc_im_selector->func.state_get = NULL;
        itc_im_selector->func.del = NULL;
    }
}

static void im_setting_selector_add_ime(void *data) {
    appdata *ad = (appdata *)data;
    unsigned int i = 0;
    if (!ad)
        return;
    im_setting_selector_genlist_item_class_create();

    /* keyboard list */
    for (i = 0; i < g_ime_info_list.size(); i++) {
        sel_cb_data *cb_data = new sel_cb_data;
        cb_data->data = data;
        cb_data->index = i;
        elm_genlist_item_append(ad->genlist,
            itc_im_selector,
            (void *)(i),
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_selector_ime_sel_cb,
            (void *)(cb_data));
    }
    elm_radio_value_set(group_radio, g_active_ime_id);
}

Evas_Object *im_setting_selector_list_create(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return NULL;
    ad->genlist = im_setting_selector_genlist_create(ad->popup, ad->conform);
    im_setting_selector_add_ime(ad);
    return ad->genlist;
}

static Evas_Object *im_setting_selector_naviframe_create(Evas_Object* parent)
{
    Evas_Object *naviframe = elm_naviframe_add(parent);
    elm_naviframe_prev_btn_auto_pushed_set(naviframe, EINA_TRUE);
    eext_object_event_callback_add(naviframe, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);
    evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(naviframe, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_object_part_content_set(parent, "elm.swallow.content", naviframe);
    evas_object_show(naviframe);
    return naviframe;
}

#ifndef _WEARABLE
static Eina_Bool im_setting_list_navi_item_pop_cb(void *data, Elm_Object_Item *it)
{
     static bool in_exit = false;
     if (in_exit)
             return EINA_TRUE;
     in_exit = true;
     if (data == NULL)
         return EINA_TRUE;
     ui_app_exit();
     return EINA_TRUE;
}

static void
im_setting_selector_popup_block_clicked_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
    evas_object_del(obj);
    ui_app_exit();
}

Evas_Object *im_setting_selector_popup_create(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return NULL;
    if (NULL == group_radio)
    {
        group_radio = elm_radio_add(ad->win);
        elm_radio_state_value_set(group_radio, -1);
    }
    ad->popup = elm_popup_add(ad->win);
    elm_popup_align_set(ad->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    evas_object_smart_callback_add(ad->popup, "block,clicked", im_setting_selector_popup_block_clicked_cb, NULL);
    elm_object_part_text_set(ad->popup, "title,text", IM_SETTING_SELECTOR_TITLE);

    ad->naviframe = im_setting_selector_naviframe_create(ad->win);
    ad->genlist = im_setting_selector_list_create(data);

    Evas_Object *back_btn = elm_button_add(ad->naviframe);
    elm_object_style_set(back_btn, "naviframe/back_btn/default");
    Elm_Object_Item *nf_main_item = elm_naviframe_item_push(ad->naviframe,
                         NULL,
                         back_btn,
                         NULL,
                         NULL,
                         NULL);
    elm_naviframe_item_pop_cb_set(nf_main_item, im_setting_list_navi_item_pop_cb, ad);

#ifdef _MOBILE
    if (APP_TYPE_NORMAL == ad->app_type) {
        Evas_Object *btn = elm_button_add(ad->popup);
        elm_object_text_set(btn, IM_SETTING_SELECT_KEYBOARD);
        elm_object_part_content_set(ad->popup, "button1", btn);
        evas_object_smart_callback_add(btn, "clicked", im_setting_selector_select_keyboard_cb, ad);
    }
#endif

    elm_object_content_set(ad->popup, ad->genlist);
    evas_object_show(ad->popup);
    return ad->popup;
}
#endif

#ifdef _WEARABLE
static char *
im_setting_selector_title_text_get(void *data, Evas_Object *obj, const char *part)
{
    char buf[1024];
    snprintf(buf, 1023, "%s", IM_SETTING_SELECTOR_TITLE);
    return strdup(buf);
}

static Evas_Object *im_setting_selector_conform_create(Evas_Object *parentWin)
{
    Evas_Object *conform = elm_conformant_add(parentWin);
    elm_win_indicator_mode_set(parentWin, ELM_WIN_INDICATOR_SHOW);
    elm_win_indicator_opacity_set(parentWin, ELM_WIN_INDICATOR_OPAQUE);
    evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(conform, EVAS_HINT_FILL, EVAS_HINT_FILL);

    Evas_Object *bg = elm_bg_add(conform);
    elm_object_style_set(bg, "indicator/headerbg");
    elm_object_part_content_set(conform, "elm.swallow.indicator_bg", bg);
    evas_object_show(bg);

    Evas_Coord w = -1, h = -1;
    elm_win_screen_size_get(parentWin, NULL, NULL, &w, &h);
    if (w > 0 && h > 0) {
        evas_object_resize(conform, w, h);
    }
    evas_object_event_callback_add(parentWin, EVAS_CALLBACK_RESIZE, NULL, conform);
    evas_object_show(conform);
    return conform;
}

static void im_setting_selector_screen_create(void *data)
{
    appdata *ad = NULL;
    Evas_Object *genlist = NULL;
    Elm_Genlist_Item_Class *ttc = elm_genlist_item_class_new();

    ad = (appdata *) data;
    if (ad == NULL) return;

    ttc->item_style = "title";
    ttc->func.text_get = im_setting_selector_title_text_get;

    ad->conform = im_setting_selector_conform_create(ad->win);
    ad->naviframe = im_setting_selector_naviframe_create(ad->conform);
    genlist = im_setting_selector_genlist_create(ad->win, ad->conform);
    evas_object_show(ad->win);

    im_setting_selector_genlist_item_class_create();

    elm_genlist_mode_set(genlist, ELM_LIST_SCROLL);
    elm_genlist_item_append(genlist, ttc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);

    if (NULL == group_radio) {
        group_radio = elm_radio_add(genlist);
        elm_radio_state_value_set(group_radio, g_active_ime_id);
    }

    /* keyboard list */
    for (unsigned int i = 0; i < g_ime_info_list.size(); i++) {
        sel_cb_data *cb_data = new sel_cb_data;
        cb_data->data = data;
        cb_data->index = i;
        elm_genlist_item_append(genlist,
            itc_im_selector,
            (void *)(i),
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_selector_ime_sel_cb,
            (void *)(cb_data));
    }

    elm_radio_state_value_set(group_radio, g_active_ime_id);
    elm_radio_value_set(group_radio, g_active_ime_id);
    elm_genlist_item_class_free(ttc);
    elm_naviframe_item_push(ad->naviframe, NULL, NULL, NULL, genlist, "empty");
}
#endif

void
im_setting_selector_app_create(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return;
    im_setting_selector_text_domain_set();
    ad->win = im_setting_selector_main_window_create(PACKAGE);
    im_setting_selector_load_ime_info();
#ifdef _WEARABLE
    im_setting_selector_screen_create(ad);
#else
    im_setting_selector_popup_create(ad);
    evas_object_show(ad->win);
#endif
}

void
im_setting_selector_app_pause(void *data)
{
    ui_app_exit();
}

void im_setting_selector_app_terminate(void *data)
{
    g_ime_info_list.clear();
    if (NULL != itc_im_selector)
    {
        elm_genlist_item_class_free(itc_im_selector);
        itc_im_selector = NULL;
    }
}
