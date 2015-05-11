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
#include <string>
#include <app.h>
#include <efl_extension.h>
#include <vector>
#include "isf_control.h"

#define IM_SETTING_LIST_TITLE                   dgettext(PACKAGE, "IDS_IME_BODY_KEYBOARD")
#define IM_SETTING_LIST_KEYBOARD_HEADER         dgettext(PACKAGE, "IDS_ST_HEADER_KEYBOARDS")
#define IM_SETTING_LIST_POPUP_TITLE             dgettext(PACKAGE, "IDS_ST_BODY_ATTENTION")
#define IM_SETTING_LIST_POPUP_CANCEL            dgettext(PACKAGE, "IDS_COM_SK_CANCEL_ABB")
#define IM_SETTING_LIST_POPUP_OK                dgettext(PACKAGE, "IDS_HELP_SK2_OK_ABB")
#define IM_SETTING_LIST_POPUP_CANCEL            dgettext(PACKAGE, "IDS_COM_SK_CANCEL_ABB")
#define IM_SETTING_LIST_POPUP_OK                dgettext(PACKAGE, "IDS_HELP_SK2_OK_ABB")
#define IM_SETTING_LIST_VIRTUAL_KEYBOARD        dgettext(PACKAGE, "IDS_ST_HEADER_VIRTUAL_KEYBOARD")
#define IM_SETTING_LIST_DEFAULT_KEYBOARD        dgettext(PACKAGE, "IDS_ST_HEADER_DEFAULT_KEYBOARD_ABB")
#define IM_SETTING_LIST_KEYBOARD_SETTING        dgettext(PACKAGE, "IDS_IME_HEADER_KEYBOARD_SETTINGS_ABB")
#define IM_SETTING_LIST_POPUP_TEXT              dgettext(PACKAGE, "IDS_ST_POP_THIS_INPUT_METHOD_MAY_BE_ABLE_TO_COLLECT_ALL_THE_TEXT                                                                  _YOU_TYPE_INCLUDING_PERSONAL_DATA_LIKE_PASSWORDS_AND_CREDIT_CARD_                                                                   NUMBERS_MSG")

#define IM_SETTING_PACKAGE             PACKAGE
#define IM_SETTING_LOCALE_DIR           ("/usr/apps/"PACKAGE_NAME"/res/locale")

using namespace std;

static std::vector<ime_info_s>  g_ime_info_list;
static Elm_Genlist_Item_Class  *itc_im_list_keyboard_list = NULL;
static Elm_Genlist_Item_Class  *itc_im_list_group = NULL;
static Elm_Genlist_Item_Class  *itc_im_list_item = NULL;
static Elm_Genlist_Item_Class  *itc_im_list_item_one_line = NULL;

typedef struct list_item_text_s
{
    char main_text[255];
    char sub_text[255];
}list_item_text;
list_item_text item_text[2];

typedef struct popup_cb_data_s
{
    Evas_Object *popup;
    Evas_Object *parent;
    void *event_info;
    void *data;
}popup_cb_data;

static void im_setting_list_text_domain_set(void)
{
   bindtextdomain(IM_SETTING_PACKAGE, IM_SETTING_LOCALE_DIR);
   textdomain(IM_SETTING_PACKAGE);
}

static Evas_Object *
im_setting_list_main_window_create(char *name)
{
    Evas_Object *eo = NULL;
    int w = -1, h = -1;
    eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
    if (eo) {
        elm_win_title_set(eo, name);
        elm_win_borderless_set(eo, EINA_TRUE);
        elm_win_alpha_set(eo, EINA_FALSE);
        elm_win_conformant_set(eo, EINA_TRUE);
        elm_win_autodel_set(eo, EINA_TRUE);
        elm_win_screen_size_get(eo, NULL, NULL, &w, &h);
        if(w == -1 || h == -1){
           LOGD("elm_win_screen_size_get() is failed\n");
           return NULL;
        }
        evas_object_resize(eo, w, h);
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

static void im_setting_list_load_ise_info(void)
{
    ime_info_s *info = NULL;
    g_ime_info_list.clear();
    int cnt = isf_control_get_all_ime_info(&info);
    if(info)
    {
        for(int i=0; i<cnt; ++i)
        {
            g_ime_info_list.push_back(info[i]);
        }
        free(info);
    }
    else
    {
        LOGD("isf_control_get_all_ime_info failed\n");
    }
}

static void im_setting_list_show_ise_selector(void)
{
    int ret;
    app_control_h app_control;
    const char *app_id = "org.tizen.inputmethod-setting-selector"; // This is temporary. AppId can be got using pkgmgr-info later.
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

      app_control_add_extra_data(app_control, "caller", "settings");
      ret = app_control_send_launch_request(app_control, NULL, NULL);
      if (ret != APP_CONTROL_ERROR_NONE) {
         LOGD("app_control_send_launch_request returned %d, %s\n", ret, get_error_message(ret));
         app_control_destroy(app_control);
         return;
      }
      app_control_destroy(app_control);
}

static void im_setting_list_check_button_change_cb(void *data, Evas_Object *obj, void *event_info)
{
    /*save the checked ise*/
    int index = (int)data;
    Eina_Bool state = elm_check_state_get (obj);
    isf_control_set_enable_ime(g_ime_info_list[index].appid, state);
}

static void im_setting_list_update_check_button_state(Elm_Object_Item *item, Evas_Object *obj, int index)
{
    Eina_Bool state = EINA_FALSE;
    if (item && obj) {
        elm_genlist_item_selected_set (item, EINA_FALSE);
        /* Update check button */
        Evas_Object *ck = elm_object_item_part_content_get (item, "elm.icon.right");
        if (ck == NULL){
            ck = elm_object_item_part_content_get (item, "elm.icon");
        }
        evas_object_data_set (ck, "parent_genlist", obj);
        state = !elm_check_state_get (ck);
        elm_check_state_set (ck, state);
        isf_control_set_enable_ime(g_ime_info_list[index].appid, state);
    }
}

static void im_setting_list_ise_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    int index = (int)data;
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    if (!item)
        return;
    im_setting_list_update_check_button_state(item, obj, index);
}

static void
im_setting_list_popup_ok_cb(void *data, Evas_Object *obj, void *event_info)
{
    popup_cb_data *cb_data = (popup_cb_data *)data;
    im_setting_list_ise_sel_cb(cb_data->data, cb_data->parent, cb_data->event_info);
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

    Evas_Object *popup = elm_popup_add(obj);
    elm_object_part_text_set(popup, "title,text", IM_SETTING_LIST_POPUP_TITLE);
    elm_object_text_set(popup, IM_SETTING_LIST_POPUP_TEXT);

    popup_cb_data *cb_data = new popup_cb_data;
    cb_data->popup = popup;
    cb_data->parent = obj;
    cb_data->event_info = event_info;
    cb_data->data = data;

    Evas_Object *btn_cancel = elm_button_add(popup);
    elm_object_text_set(btn_cancel, IM_SETTING_LIST_POPUP_CANCEL);
    elm_object_part_content_set(popup, "button1", btn_cancel);
    evas_object_smart_callback_add(btn_cancel, "clicked", im_setting_list_popup_cancel_cb, cb_data);

    Evas_Object *btn_ok = elm_button_add(popup);
    elm_object_text_set(btn_ok, IM_SETTING_LIST_POPUP_OK);
    elm_object_part_content_set(popup, "button2", btn_ok);
    evas_object_smart_callback_add(btn_ok, "clicked", im_setting_list_popup_ok_cb, cb_data);

    evas_object_show(popup);
}

static void im_setting_list_set_default_keyboard_item_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    elm_genlist_item_selected_set (item, EINA_FALSE);
    im_setting_list_show_ise_selector();
}

static void im_setting_list_keyboard_setting_item_sel_cb(void *data, Evas_Object *obj, void *event_info)
{
    Elm_Object_Item *item = (Elm_Object_Item *)event_info;
    elm_genlist_item_selected_set (item, EINA_FALSE);
    isf_control_show_ise_option_window();
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
    elm_naviframe_prev_btn_auto_pushed_set(naviframe, EINA_FALSE);
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
    elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
    elm_genlist_homogeneous_set (genlist, EINA_TRUE);
    evas_object_show(genlist);
    return genlist;
}

static char *im_setting_list_genlist_group_label_get(void *data, Evas_Object *obj, const char *part)
{
    char *text = (char *)data;
    if (!strcmp(part, "elm.text.main")) {
        return strdup(text);
    }
    return NULL;
}

static Evas_Object *im_setting_list_genlist_group_icon_get(void *data, Evas_Object *obj, const char *part)
{
    Evas_Object *item = NULL;
    if (!strcmp(part, "elm.icon.right")) {
    }
    return item;
}

static char *im_setting_list_genlist_keyboard_list_item_label_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)(data);
    if (!strcmp(part, "elm.text.main.left.top") ||
        !strcmp(part, "elm.text.main.left") ||
        !strcmp(part, "elm.text.main") ||
        !strcmp(part, "elm.text") ||
        !strcmp(part, "elm.text.1")) {
        return strdup(g_ime_info_list[index].appid);
    }
    return NULL;
}

static Evas_Object *im_setting_list_genlist_keyboard_list_item_icon_get(void *data, Evas_Object *obj, const char *part)
{
    int index = (int)(data);
    if (!strcmp(part, "elm.icon.right") || !strcmp(part, "elm.icon")) {
        Evas_Object *ck = elm_check_add(obj);
        elm_object_style_set (ck, "on&off");
        elm_check_state_set(ck, g_ime_info_list[index].is_enabled);
        evas_object_propagate_events_set (ck, EINA_FALSE);
        evas_object_show(ck);
        evas_object_smart_callback_add(ck, "changed", im_setting_list_check_button_change_cb, (void *) (index));
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
    if(NULL == itc_im_list_group)
    {
        itc_im_list_group = elm_genlist_item_class_new();
        if(itc_im_list_group)
        {
            itc_im_list_group->item_style = "groupindex";
            itc_im_list_group->func.text_get = im_setting_list_genlist_group_label_get;
            itc_im_list_group->func.content_get = NULL;
            itc_im_list_group->func.state_get = NULL;
            itc_im_list_group->func.del = NULL;
        }
    }

    if(NULL == itc_im_list_keyboard_list)
    {
        itc_im_list_keyboard_list = elm_genlist_item_class_new();
        if (itc_im_list_keyboard_list)
        {
            itc_im_list_keyboard_list->item_style = "1line";
            itc_im_list_keyboard_list->func.text_get = im_setting_list_genlist_keyboard_list_item_label_get;
            itc_im_list_keyboard_list->func.content_get = im_setting_list_genlist_keyboard_list_item_icon_get;
            itc_im_list_keyboard_list->func.state_get = NULL;
            itc_im_list_keyboard_list->func.del = NULL;
        }
    }

    if(app_type == APP_TYPE_SETTING)
    {
        if(NULL == itc_im_list_item)
        {
            itc_im_list_item = elm_genlist_item_class_new();
            if (itc_im_list_item)
            {
                itc_im_list_item->item_style = "2line.top";
                itc_im_list_item->func.text_get = im_setting_list_genlist_item_label_get;
                itc_im_list_item->func.content_get = NULL;
                itc_im_list_item->func.state_get = NULL;
                itc_im_list_item->func.del = NULL;
            }
        }

        if(NULL == itc_im_list_item_one_line)
        {
            itc_im_list_item_one_line = elm_genlist_item_class_new();
            if (itc_im_list_item_one_line)
            {
                itc_im_list_item_one_line->item_style = "1line";
                itc_im_list_item_one_line->func.text_get = im_setting_list_genlist_item_one_line_label_get;
                itc_im_list_item_one_line->func.content_get = NULL;
                itc_im_list_item_one_line->func.state_get = NULL;
                itc_im_list_item_one_line->func.del = NULL;
            }
        }
    }
}

static void im_setting_list_add_ise(void *data) {
    appdata *ad = (appdata *)data;
    unsigned int i = 0;
    char *active_ise_uuid = NULL;
    im_setting_list_genlist_item_class_create(ad->app_type);

    if(NULL != ad->genlist)
    {
        elm_genlist_clear(ad->genlist);
    }
//    list_item_text item_text;
    memset(&item_text, 0, sizeof(item_text));
    if(ad->app_type == APP_TYPE_SETTING)
    {
        elm_genlist_item_append(ad->genlist,
            itc_im_list_group,
            IM_SETTING_LIST_VIRTUAL_KEYBOARD,
            NULL,
            ELM_GENLIST_ITEM_NONE,
            NULL,
            NULL);

        char *active_ime_appid = NULL;
        isf_control_get_active_ime(&active_ime_appid);
        std::vector<ime_info_s>::iterator iter = g_ime_info_list.begin();
        std::vector<ime_info_s>::iterator end = g_ime_info_list.end();
        for (; iter != end; ++iter)
        {
            if(!strcmp(active_ime_appid, iter->appid))
            {
                break;
            }
        }
        sprintf(item_text[0].main_text, "%s", IM_SETTING_LIST_DEFAULT_KEYBOARD);
        sprintf(item_text[0].sub_text, "%s", iter->appid);
        elm_genlist_item_append(ad->genlist,
            itc_im_list_item,
            (void *)&item_text[0],
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_set_default_keyboard_item_sel_cb,
            NULL);

        sprintf(item_text[1].main_text, "%s", IM_SETTING_LIST_KEYBOARD_SETTING);
        Elm_Object_Item *item = elm_genlist_item_append(ad->genlist,
            itc_im_list_item_one_line,
            (void *)&item_text[1],
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_keyboard_setting_item_sel_cb,
            NULL);

        elm_object_item_disabled_set(item, !(iter->has_option));
        free(active_ime_appid);
    }

    elm_genlist_item_append(ad->genlist,
            itc_im_list_group,
            IM_SETTING_LIST_KEYBOARD_HEADER,
            NULL,
            ELM_GENLIST_ITEM_NONE,
            NULL,
            NULL);

    /* keyboard list */
    for (i = 0; i < g_ime_info_list.size(); i++) {
        Elm_Object_Item *item = elm_genlist_item_append(ad->genlist,
            itc_im_list_keyboard_list,
            (void *)(i),
            NULL,
            ELM_GENLIST_ITEM_NONE,
            im_setting_list_item_sel_cb,
            (void *)(i));

        elm_object_item_disabled_set(item, g_ime_info_list[i].is_preinstalled);
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
    elm_exit();
    return EINA_TRUE;
}

Evas_Object *im_setting_list_list_create(void *data)
{
    appdata *ad = (appdata *)data;
    ad->conform = im_setting_list_conform_create(ad->win);
    ad->naviframe = im_setting_list_naviframe_create(ad->conform);
    ad->genlist = im_setting_list_genlist_create(ad->naviframe);
    im_setting_list_add_ise(ad);

    /* Add genlist to naviframe */
    Evas_Object *back_btn = elm_button_add (ad->naviframe);
    elm_object_style_set (back_btn, "naviframe/back_btn/default");
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
    if(NULL != itc_im_list_keyboard_list)
    {
        elm_genlist_item_class_free(itc_im_list_keyboard_list);
        itc_im_list_keyboard_list = NULL;
    }

    if(NULL != itc_im_list_group)
    {
        elm_genlist_item_class_free(itc_im_list_group);
        itc_im_list_group = NULL;
    }

    if(NULL != itc_im_list_item)
    {
        elm_genlist_item_class_free(itc_im_list_item);
        itc_im_list_item = NULL;
    }
}

void im_setting_list_update_window(void *data)
{
    appdata *ad = (appdata *)data;
    im_setting_list_load_ise_info();
    im_setting_list_add_ise(ad);
}

void
im_setting_list_app_create(void *data)
{
    appdata *ad = (appdata *)data;
    im_setting_list_text_domain_set();
    ad->win = im_setting_list_main_window_create(PACKAGE);
    im_setting_list_bg_create(ad->win);
    im_setting_list_load_ise_info();
    im_setting_list_list_create(ad);
    evas_object_show(ad->win);
}
