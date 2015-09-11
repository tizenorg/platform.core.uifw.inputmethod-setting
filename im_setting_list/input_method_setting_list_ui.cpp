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
#include "input_method_setting_list_popup_view.h"
#include <string>
#include <app.h>
#include <efl_extension.h>
#include <vector>
#include <algorithm>
#include "isf_control.h"

#define IM_SETTING_LIST_PACKAGE                      PACKAGE
#define IM_SETTING_LIST_LOCALE_DIR                   ("/usr/apps/"PACKAGE_NAME"/res/locale")
#define IM_SETTING_LIST_TITLE                   dgettext(PACKAGE, "IDS_IME_BODY_KEYBOARD")
#define IM_SETTING_LIST_KEYBOARD_HEADER         dgettext(PACKAGE, "IDS_ST_HEADER_KEYBOARDS")
#define IM_SETTING_LIST_POPUP_TITLE             dgettext(PACKAGE, "IDS_ST_BODY_ATTENTION")
#define IM_SETTING_LIST_POPUP_CANCEL            dgettext(PACKAGE, "IDS_COM_SK_CANCEL_ABB")
#define IM_SETTING_LIST_POPUP_ENABLE            dgettext(PACKAGE, "IDS_ST_BUTTON_ENABLE")
#define IM_SETTING_LIST_VIRTUAL_KEYBOARD        dgettext(PACKAGE, "IDS_ST_HEADER_VIRTUAL_KEYBOARD")
#define IM_SETTING_LIST_DEFAULT_KEYBOARD        dgettext(PACKAGE, "IDS_ST_HEADER_DEFAULT_KEYBOARD_ABB")
#define IM_SETTING_LIST_KEYBOARD_SETTING        dgettext(PACKAGE, "IDS_IME_HEADER_KEYBOARD_SETTINGS_ABB")
#define IM_SETTING_LIST_POPUP_TEXT              dgettext(PACKAGE, "IDS_ST_POP_THIS_INPUT_METHOD_MAY_BE_ABLE_TO_COLLECT_ALL_THE"\
                                                                  "_TEXT_YOU_TYPE_INCLUDING_PERSONAL_DATA_LIKE_PASSWORDS_AND"\
                                                                  "_CREDIT_CARD_NUMBERS_MSG")


typedef struct list_item_text_s
{
    char main_text[255];
    char sub_text[255];
}list_item_text;

typedef struct popup_cb_data_s
{
    Evas_Object *popup;
    void *data;
}popup_cb_data;

typedef struct gen_item_data_s
{
    Elm_Object_Item *gen_item;
    int chk_status;
}gen_item_data;

class ime_info_compare
{
    public:
    bool operator()(const ime_info_s &first, const ime_info_s &sec)
    {
        return (strcasecmp(first.label, sec.label) < 0);
    }
};

typedef void (*popup_ok_cb)(void *data, Evas_Object *obj, void *event_info);
typedef void (*popup_cancel_cb)(void *data, Evas_Object *obj, void *event_info);

static std::vector<ime_info_s>      g_ime_info_list;
static Elm_Genlist_Item_Class       *itc_im_list_keyboard_list = NULL;
static Elm_Genlist_Item_Class       *itc_im_list_group = NULL;
static Elm_Genlist_Item_Class       *itc_im_list_item = NULL;
static Elm_Genlist_Item_Class       *itc_im_list_item_one_line = NULL;
static int                          g_active_ime_index = -1;
static list_item_text               item_text[2];
static std::vector<gen_item_data>   g_gen_item_data;

void im_setting_list_update_window(void *data);

static void im_setting_list_text_domain_set(void)
{
   bindtextdomain(IM_SETTING_LIST_PACKAGE, IM_SETTING_LIST_LOCALE_DIR);
   textdomain(IM_SETTING_LIST_PACKAGE);
}

static Evas_Object *
im_setting_list_main_window_create(const char *name, int app_type)
{
    Evas_Object *eo = NULL;
    eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
    if (eo) {
        elm_win_title_set(eo, name);
        elm_win_borderless_set(eo, EINA_TRUE);
        elm_win_alpha_set(eo, EINA_FALSE);
        elm_win_conformant_set(eo, EINA_TRUE);
        elm_win_autodel_set(eo, EINA_TRUE);

        if (app_type != APP_TYPE_SETTING_NO_ROTATION) {
            int rots[4] = {0, 90, 180, 270};
            elm_win_wm_rotation_available_rotations_set(eo, rots, 4);
        }
    }
    return eo;
}

static Evas_Object* im_setting_list_bg_create(Evas_Object *parent)
{
    Evas_Object *bg;
    if (parent == NULL) return NULL;
    bg = elm_bg_add(parent);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(parent, bg);
    evas_object_show(bg);
    return bg;
}

static void im_setting_list_sort_ime_info(std::vector<ime_info_s> &preinstall, std::vector<ime_info_s> &user)
{
    std::sort(preinstall.begin(), preinstall.end(), ime_info_compare());
    std::sort(user.begin(), user.end(), ime_info_compare());
    for(unsigned int i=0; i<preinstall.size(); ++i)
    {
        g_ime_info_list.push_back(preinstall[i]);
    }
    for(unsigned int i=0; i<user.size(); ++i)
    {
        g_ime_info_list.push_back(user[i]);
    }
}

static void im_setting_list_load_ime_info(void)
{
    std::vector<ime_info_s>      ime_info_list_preinstall;
    std::vector<ime_info_s>      ime_info_list_user;
    ime_info_s *info = NULL;
    g_ime_info_list.clear();
    int cnt = isf_control_get_all_ime_info(&info);
    if (info)
    {
        for(int i=0; i<cnt; ++i)
        {
            SECURE_LOGD("%s %s %d %d %d", info[i].appid, info[i].label, info[i].is_enabled, info[i].is_preinstalled, info[i].has_option);
            if (info[i].is_preinstalled)
            {
                ime_info_list_preinstall.push_back(info[i]);
            }
            else
            {
                ime_info_list_user.push_back(info[i]);
            }
        }
        free(info);
    }
    else
    {
        LOGD("isf_control_get_all_ime_info failed\n");
    }
    im_setting_list_sort_ime_info(ime_info_list_preinstall, ime_info_list_user);
}

static int im_setting_list_get_active_ime_index(void)
{
    char *active_ime_appid = NULL;
    isf_control_get_active_ime(&active_ime_appid);
    std::vector<ime_info_s>::iterator iter = g_ime_info_list.begin();
    std::vector<ime_info_s>::iterator end = g_ime_info_list.end();
    for (; iter != end; ++iter)
    {
        if (active_ime_appid && (!strcmp(active_ime_appid, iter->appid)))
        {
            break;
        }
    }
    if (active_ime_appid)
    {
        free(active_ime_appid);
    }
    return (iter-g_ime_info_list.begin());
}

static void
im_setting_list_app_control_reply_cb(app_control_h request, app_control_h reply, app_control_result_e result, void *user_data)
{
    if (!request || !reply) {
        LOGD("app_control handle is null");
        return;
    }
    if (result == APP_CONTROL_RESULT_SUCCEEDED) {
        im_setting_list_update_window(user_data);
    }
}

static void im_setting_list_show_ime_selector(void *data)
{
    int ret;
    appdata *ad = (appdata *)data;
    app_control_h app_control;
    const char *app_id = "org.tizen.inputmethod-setting-selector";

    if (!ad)
        return;

    ret = app_control_create (&app_control);
    if (ret != APP_CONTROL_ERROR_NONE) {
        LOGD("app_control_create returned %d", ret);
        return;
    }

    ret = app_control_set_operation (app_control, APP_CONTROL_OPERATION_DEFAULT);
    if (ret != APP_CONTROL_ERROR_NONE) {
        LOGD("app_control_set_operation returned %d", ret);
        app_control_destroy(app_control);
        return;
    }

    ret = app_control_set_app_id (app_control, app_id);
    if (ret != APP_CONTROL_ERROR_NONE) {
        LOGD("app_control_set_app_id returned %d", ret);
        app_control_destroy(app_control);
        return;
    }

    if (ad->app_type == APP_TYPE_SETTING_NO_ROTATION)
        app_control_add_extra_data(app_control, "caller", "settings_no_rotation");
    else
        app_control_add_extra_data(app_control, "caller", "settings");

    ret = app_control_send_launch_request(app_control, im_setting_list_app_control_reply_cb, data);
    if (ret != APP_CONTROL_ERROR_NONE) {
        LOGD("app_control_send_launch_request returned %d, %s\n", ret, get_error_message(ret));
        app_control_destroy(app_control);
        return;
    }
    app_control_destroy(app_control);
}

static void
im_setting_list_check_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
    popup_cb_data *cb_data = (popup_cb_data *)data;
    int index = (int)reinterpret_cast<long>(cb_data->data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        delete cb_data;
        return;
    }
    Eina_Bool state = EINA_FALSE;
    state = g_gen_item_data[index].chk_status;
    isf_control_set_enable_ime(g_ime_info_list[index].appid, state);
    evas_object_del(cb_data->popup);
    delete cb_data;
}

static void
im_setting_list_check_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
    popup_cb_data *cb_data = (popup_cb_data *)data;
    int index = (int)reinterpret_cast<long>(cb_data->data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        delete cb_data;
        return;
    }

    Eina_Bool state = g_gen_item_data[index].chk_status;
    isf_control_set_enable_ime(g_ime_info_list[index].appid, !state);
    g_gen_item_data[index].chk_status = !state;
    elm_genlist_item_update(g_gen_item_data[index].gen_item);
    evas_object_del(cb_data->popup);
    delete cb_data;
}

static void _popup_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    eext_object_event_callback_del(obj, EEXT_CALLBACK_BACK, _popup_back_cb);
    popup_cb_data *cb_data = (popup_cb_data *)data;
    int index = (int)(cb_data->data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        delete cb_data;
        return;
    }
    if(g_gen_item_data[index].chk_status){
        im_setting_list_check_popup_cancel_cb(data, NULL, NULL);
    }
    else{
        evas_object_del(cb_data->popup);
        delete cb_data;
    }
}

static void im_setting_list_show_popup(void *data, Evas_Object *obj, popup_ok_cb ime_setting_list_ok_callback, popup_cancel_cb ime_setting_list_cancel_callback)
{
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return;
    }

    Evas_Object *top_widget = elm_object_top_widget_get(obj);
    Evas_Object *popup = elm_popup_add(top_widget);
    elm_popup_align_set (popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    elm_object_part_text_set(popup, "title,text", IM_SETTING_LIST_POPUP_TITLE);

    char chFormatMsg[255] = {'\0'};
    char chPopupMsg[255] = {'\0'};
    snprintf(chFormatMsg, sizeof(chFormatMsg), "%s", IM_SETTING_LIST_POPUP_TEXT);
    snprintf(chPopupMsg, sizeof(chPopupMsg), chFormatMsg, g_ime_info_list[index].label);
    elm_object_text_set(popup, chPopupMsg);

    popup_cb_data *cb_data = new popup_cb_data;
    cb_data->popup = popup;
    cb_data->data = data;

    Evas_Object *btn_cancel = elm_button_add(popup);
    elm_object_text_set(btn_cancel, IM_SETTING_LIST_POPUP_CANCEL);
    elm_object_part_content_set(popup, "button1", btn_cancel);
    evas_object_smart_callback_add(btn_cancel, "clicked", ime_setting_list_cancel_callback, cb_data);

    Evas_Object *btn_ok = elm_button_add(popup);
    elm_object_text_set(btn_ok, IM_SETTING_LIST_POPUP_ENABLE);
    elm_object_part_content_set(popup, "button2", btn_ok);
    evas_object_smart_callback_add(btn_ok, "clicked", ime_setting_list_ok_callback, cb_data);

    eext_object_event_callback_add (popup, EEXT_CALLBACK_BACK, _popup_back_cb, cb_data);
    evas_object_show(popup);
}

static void im_setting_list_check_button_change_cb(void *data, Evas_Object *obj, void *event_info)
{
    /*save the checked ime*/
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return;
    }

    Eina_Bool state = g_gen_item_data[index].chk_status;

    if (!state)
    {
        isf_control_set_enable_ime(g_ime_info_list[index].appid, state);
    }
    else
    {
        im_setting_list_show_popup(data, obj, im_setting_list_check_popup_ok_cb, im_setting_list_check_popup_cancel_cb);
    }
}

static void
im_setting_list_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
    popup_cb_data *cb_data = (popup_cb_data *)data;
    int index = (int)reinterpret_cast<long>(cb_data->data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        delete cb_data;
        return;
    }

    Eina_Bool state = g_gen_item_data[index].chk_status;
    g_gen_item_data[index].chk_status = !state;
    isf_control_set_enable_ime(g_ime_info_list[index].appid, !state);
    elm_genlist_item_update(g_gen_item_data[index].gen_item);
    evas_object_del(cb_data->popup);
    delete cb_data;
}

static void
im_setting_list_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
    popup_cb_data *cb_data = (popup_cb_data *)data;
    evas_object_del(cb_data->popup);
    delete cb_data;
}

static void im_setting_list_item_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    elm_genlist_item_selected_set (item, EINA_FALSE);

    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return;
    }

    if (g_ime_info_list[index].is_preinstalled || (index == g_active_ime_index))
    {
        return;
    }

    Evas_Object *ck = elm_object_item_part_content_get (item, "elm.icon.right");
    if (ck == NULL) {
        ck = elm_object_item_part_content_get (item, "elm.icon");
    }
    Eina_Bool state = g_gen_item_data[index].chk_status;

    if (state)
    {
        g_gen_item_data[index].chk_status = !state;
        isf_control_set_enable_ime(g_ime_info_list[index].appid, !state);
        elm_genlist_item_update(item);
    }
    else
    {
        im_setting_list_show_popup(data, obj, im_setting_list_popup_ok_cb, im_setting_list_popup_cancel_cb);
    }
}

static void im_setting_list_set_default_keyboard_item_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    appdata *ad = (appdata *)data;
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    elm_genlist_item_selected_set (item, EINA_FALSE);
//    im_setting_list_show_ime_selector(data);
    im_setting_list_popup_view_create(ad->win, data);
}

static void im_setting_list_keyboard_setting_item_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    elm_genlist_item_selected_set (item, EINA_FALSE);
    isf_control_open_ime_option_window();
}

static Evas_Object *im_setting_list_conform_create(Evas_Object *parentWin)
{
    Evas_Object *conform = elm_conformant_add(parentWin);
    elm_win_indicator_mode_set(parentWin, ELM_WIN_INDICATOR_SHOW);
    elm_win_indicator_opacity_set(parentWin, ELM_WIN_INDICATOR_OPAQUE);
    evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(conform, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_win_resize_object_add(parentWin, conform);
    evas_object_show(conform);
    return conform;
}

static Evas_Object *im_setting_list_naviframe_create(Evas_Object* parent)
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

static Evas_Object *im_setting_list_genlist_create(Evas_Object* parent)
{
    Evas_Object *genlist = elm_genlist_add(parent);
    evas_object_show(genlist);
    return genlist;
}

static char *im_setting_list_genlist_group_label_get(void *data, Evas_Object *obj, const char *part)
{
    char *text = (char *)data;
#ifdef _MOBILE
    if (!strcmp(part, "elm.text.main")) {
#else
    if (!strcmp(part, "elm.text")) {
#endif
        return strdup(text);
    }
    return NULL;
}

static char *im_setting_list_genlist_keyboard_list_item_label_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return NULL;
    }

    if (!strcmp(part, "elm.text.main.left.top") ||
        !strcmp(part, "elm.text.main.left") ||
        !strcmp(part, "elm.text.main") ||
        !strcmp(part, "elm.text") ||
        !strcmp(part, "elm.text.1")) {
        return strdup(g_ime_info_list[index].label);
    }
    return NULL;
}

static Evas_Object *im_setting_list_genlist_keyboard_list_item_icon_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)reinterpret_cast<long>(data);
    if (index < 0 || index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. index : %d, g_ime_info_list.size() : %d\n", index, g_ime_info_list.size());
        return NULL;
    }

    if (!strcmp(part, "elm.icon.right") || !strcmp(part, "elm.icon")) {
        Evas_Object *ck = elm_check_add(obj);
        elm_object_style_set (ck, "on&off");
        elm_object_disabled_set(ck, g_ime_info_list[index].is_preinstalled || (index == g_active_ime_index));
        elm_check_state_set(ck, g_gen_item_data[index].chk_status);
        evas_object_propagate_events_set (ck, EINA_FALSE);
        evas_object_size_hint_align_set(ck, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_check_state_pointer_set(ck, (Eina_Bool *)(&(g_gen_item_data[index].chk_status)));
        evas_object_pass_events_set(ck, 1);
        evas_object_smart_callback_add(ck, "changed", im_setting_list_check_button_change_cb, (void *)(index));
        evas_object_show(ck);
        return ck;
    }
    return NULL;
}

static char *im_setting_list_genlist_item_label_get(void *data, Evas_Object *obj, const char *part)
{
    list_item_text *item_text = (list_item_text *)data;
    if (!strcmp(part, "elm.text.main.left.top") ||
        !strcmp(part, "elm.text.main.left") ||
        !strcmp(part, "elm.text.main") ||
        !strcmp(part, "elm.text") ||
        !strcmp(part, "elm.text.1")) {
        return strdup(item_text->main_text);
    }

    if (!strcmp(part, "elm.text.sub.left.bottom") ||
        !strcmp(part, "elm.text.multiline") ||
        !strcmp(part, "elm.text.2")) {
        return strdup(item_text->sub_text);
    }
    return NULL;
}

static char *im_setting_list_genlist_item_one_line_label_get(void *data, Evas_Object *obj, const char *part)
{
    list_item_text *item_text = (list_item_text *)data;
    if (!strcmp(part, "elm.text.main.left.top") ||
        !strcmp(part, "elm.text.main.left") ||
        !strcmp(part, "elm.text.main") ||
        !strcmp(part, "elm.text") ||
        !strcmp(part, "elm.text.1")) {
        return strdup(item_text->main_text);
    }
    return NULL;
}

static void im_setting_list_genlist_item_class_create(int app_type)
{
    if (NULL == itc_im_list_group)
    {
        itc_im_list_group = elm_genlist_item_class_new();
        if (itc_im_list_group)
        {
            itc_im_list_group->item_style = "groupindex";
            itc_im_list_group->func.text_get = im_setting_list_genlist_group_label_get;
            itc_im_list_group->func.content_get = NULL;
            itc_im_list_group->func.state_get = NULL;
            itc_im_list_group->func.del = NULL;
        }
    }

    if (NULL == itc_im_list_keyboard_list)
    {
        itc_im_list_keyboard_list = elm_genlist_item_class_new();
        if (itc_im_list_keyboard_list)
        {
#ifdef _MOBILE
            itc_im_list_keyboard_list->item_style = "1line";
#else
            itc_im_list_keyboard_list->item_style = "1text.1icon.1";
#endif
            itc_im_list_keyboard_list->func.text_get = im_setting_list_genlist_keyboard_list_item_label_get;
            itc_im_list_keyboard_list->func.content_get = im_setting_list_genlist_keyboard_list_item_icon_get;
            itc_im_list_keyboard_list->func.state_get = NULL;
            itc_im_list_keyboard_list->func.del = NULL;
        }
    }

    if (app_type == APP_TYPE_SETTING || app_type == APP_TYPE_SETTING_NO_ROTATION)
    {
        if (NULL == itc_im_list_item)
        {
            itc_im_list_item = elm_genlist_item_class_new();
            if (itc_im_list_item)
            {
#ifdef _MOBILE
                itc_im_list_item->item_style = "2line.top";
#else
                itc_im_list_item->item_style = "2text";
#endif
                itc_im_list_item->func.text_get = im_setting_list_genlist_item_label_get;
                itc_im_list_item->func.content_get = NULL;
                itc_im_list_item->func.state_get = NULL;
                itc_im_list_item->func.del = NULL;
            }
        }

        if (NULL == itc_im_list_item_one_line)
        {
            itc_im_list_item_one_line = elm_genlist_item_class_new();
            if (itc_im_list_item_one_line)
            {
#ifdef _MOBILE
                itc_im_list_item_one_line->item_style = "1line";
#else
                itc_im_list_item_one_line->item_style = "1text";
#endif
                itc_im_list_item_one_line->func.text_get = im_setting_list_genlist_item_one_line_label_get;
                itc_im_list_item_one_line->func.content_get = NULL;
                itc_im_list_item_one_line->func.state_get = NULL;
                itc_im_list_item_one_line->func.del = NULL;
            }
        }
    }
}

static void im_setting_list_add_ime(void *data) {
    appdata *ad = (appdata *)data;
    int i = 0;
    im_setting_list_genlist_item_class_create(ad->app_type);

    if (NULL != ad->genlist)
    {
        elm_genlist_clear(ad->genlist);
    }

    g_active_ime_index = im_setting_list_get_active_ime_index();
    if (g_active_ime_index < 0 || g_active_ime_index >= (int)g_ime_info_list.size()) {
        LOGW("Wrong value. g_active_ime_index : %d, g_ime_info_list.size() : %d\n", g_active_ime_index, g_ime_info_list.size());
        return;
    }

    memset(&item_text, 0, sizeof(item_text));
    Elm_Object_Item * group_header_item = NULL;
    if (ad->app_type == APP_TYPE_SETTING || ad->app_type == APP_TYPE_SETTING_NO_ROTATION)
    {
        snprintf(item_text[0].main_text, sizeof(item_text[0].main_text), "%s", IM_SETTING_LIST_DEFAULT_KEYBOARD);
        snprintf(item_text[0].sub_text, sizeof(item_text[0].sub_text), "%s", g_ime_info_list[g_active_ime_index].label);
        elm_genlist_item_append(ad->genlist,
            itc_im_list_item,
            (void *)&item_text[0],
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_set_default_keyboard_item_sel_cb,
            data);

        snprintf(item_text[1].main_text, sizeof(item_text[1].main_text), "%s", IM_SETTING_LIST_KEYBOARD_SETTING);
        Elm_Object_Item *item = elm_genlist_item_append(ad->genlist,
            itc_im_list_item_one_line,
            (void *)&item_text[1],
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_keyboard_setting_item_sel_cb,
            NULL);

        elm_object_item_disabled_set(item, !(g_ime_info_list[g_active_ime_index].has_option));
    }

    group_header_item = elm_genlist_item_append(ad->genlist,
            itc_im_list_group,
            IM_SETTING_LIST_KEYBOARD_HEADER,
            NULL,
            ELM_GENLIST_ITEM_NONE,
            NULL,
            NULL);
    elm_genlist_item_select_mode_set(group_header_item, ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

    /* keyboard list */
    int info_list_size = g_ime_info_list.size();
    for (i = 0; i < info_list_size; i++) {
        gen_item_data item_data;

        if (g_ime_info_list[i].is_preinstalled || (i == g_active_ime_index)) {
            item_data.chk_status = EINA_TRUE;
        }else{
            item_data.chk_status = g_ime_info_list[i].is_enabled;
        }
        g_gen_item_data.push_back(item_data);

        Elm_Object_Item *item = elm_genlist_item_append(ad->genlist,
            itc_im_list_keyboard_list,
            (void *)(i),
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_item_sel_cb,
            (void *)(i));

        if (g_ime_info_list[i].is_preinstalled || (i == g_active_ime_index)) {
            elm_object_item_disabled_set(item, EINA_TRUE);
        }

        g_gen_item_data[i].gen_item = item;
    }
}

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

static void im_setting_list_navi_back_btn_call_cb(void *data, Evas_Object *obj, void *event_info)
{
    evas_object_smart_callback_del(obj, "clicked", im_setting_list_navi_back_btn_call_cb);
    ui_app_exit();
}

Evas_Object *im_setting_list_list_create(void *data)
{
    appdata *ad = (appdata *)data;
    ad->conform = im_setting_list_conform_create(ad->win);
    ad->naviframe = im_setting_list_naviframe_create(ad->conform);
    ad->genlist = im_setting_list_genlist_create(ad->naviframe);
    im_setting_list_add_ime(ad);

    /* Add genlist to naviframe */
    Evas_Object *back_btn = elm_button_add (ad->naviframe);
    elm_object_style_set (back_btn, "naviframe/back_btn/default");
    evas_object_smart_callback_add(back_btn, "clicked", im_setting_list_navi_back_btn_call_cb, NULL);
    Elm_Object_Item *nf_main_item = elm_naviframe_item_push(ad->naviframe,
                IM_SETTING_LIST_TITLE,
                back_btn,
                NULL,
                ad->genlist,
                NULL);
    elm_naviframe_item_title_enabled_set(nf_main_item, EINA_TRUE, EINA_TRUE);
    elm_naviframe_item_pop_cb_set(nf_main_item, im_setting_list_navi_item_pop_cb, ad);
    elm_object_content_set(ad->conform, ad->naviframe);

    return ad->genlist;
}

void im_setting_list_app_terminate(void *data)
{
    g_ime_info_list.clear();
    g_gen_item_data.clear();
    if (NULL != itc_im_list_keyboard_list)
    {
        elm_genlist_item_class_free(itc_im_list_keyboard_list);
        itc_im_list_keyboard_list = NULL;
    }

    if (NULL != itc_im_list_group)
    {
        elm_genlist_item_class_free(itc_im_list_group);
        itc_im_list_group = NULL;
    }

    if (NULL != itc_im_list_item)
    {
        elm_genlist_item_class_free(itc_im_list_item);
        itc_im_list_item = NULL;
    }
}

void im_setting_list_update_window(void *data)
{
    appdata *ad = (appdata *)data;
    im_setting_list_load_ime_info();
    im_setting_list_add_ime(ad);
}

void
im_setting_list_app_create(void *data)
{
    appdata *ad = (appdata *)data;
    if (!ad)
        return;
    im_setting_list_text_domain_set();
    ad->win = im_setting_list_main_window_create(PACKAGE, ad->app_type);
    im_setting_list_bg_create(ad->win);
    im_setting_list_load_ime_info();

    im_setting_list_list_create(ad);
    evas_object_show(ad->win);
}

