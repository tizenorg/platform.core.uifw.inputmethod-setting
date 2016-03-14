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
#include <app.h>
#include <unistd.h>
#include "input_method_setting_list.h"
#include "input_method_setting_list_ui.h"

static bool
app_create(void *data)
{
    LOGD("");
    /* Hook to take necessary actions before main event loop starts
       Initialize UI resources and application's data
       If this function returns true, the main loop of application starts
       If this function returns false, the application is terminated */
    return true;
}

static void
app_control(app_control_h app_control, void *data)
{
    /* Handle the launch request. */
    appdata *ad = (appdata *)data;
    char* type = NULL;
    int res;

    LOGD("");

    if (!ad)
        return;

    if (ad->app_state == APP_STATE_PAUSE || ad->app_state == APP_STATE_RESUME)
    {
        if (ad->win)
        {
            elm_win_activate(ad->win);
            im_setting_list_update_window(ad);
        }
        ad->app_state = APP_STATE_SERVICE;
        return;
    }

    ad->app_type = APP_TYPE_NORMAL;

    res = app_control_get_extra_data(app_control, "caller", &type);
    if (APP_CONTROL_ERROR_NONE == res && NULL != type) {
        if (strcmp(type, "settings") == 0)
        {
            ad->app_type = APP_TYPE_SETTING;
        }
        else if (strcmp(type, "settings_no_rotation") == 0)
        {
            ad->app_type = APP_TYPE_SETTING_NO_ROTATION;
        }
    }

    ad->app_state = APP_STATE_SERVICE;
    im_setting_list_app_create(ad);
    if (NULL != type)
    {
        free(type);
    }
}

static void
app_pause(void *data)
{
    appdata *ad = (appdata *)data;
    LOGD("");
    /* Take necessary actions when application becomes invisible. */

    if (!ad)
        return;
    if(ad->popup)
    {
        evas_object_del(ad->popup);
        ad->popup = NULL;
    }
    ad->app_state = APP_STATE_PAUSE;
}

static void
app_resume(void *data)
{
    appdata *ad = (appdata *)data;
    LOGD("");
    /* Take necessary actions when application becomes visible. */

    if (!ad)
        return;
    if (ad->app_state == APP_STATE_PAUSE)
    {
        if (ad->win)
        {
            elm_win_activate(ad->win);
            im_setting_list_update_window(ad);
        }
        ad->app_state = APP_STATE_RESUME;
        return;
    }
    ad->app_state = APP_STATE_RESUME;
}

static void
app_terminate(void *data)
{
    appdata *ad = (appdata *)data;
    LOGD("");
    /* Release all resources. */

    if (!ad)
        return;
    ad->app_state = APP_STATE_TERMINATE;
    im_setting_list_app_terminate(ad);
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
    LOGD("");
    /*APP_EVENT_LANGUAGE_CHANGED*/
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
    LOGD("");
    /*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
    LOGD("");
    /*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
    LOGD("");
    /*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
    LOGD("");
    /*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
    appdata ad = {0,};
    int ret = 0;

    ui_app_lifecycle_callback_s event_callback = {0,};
    app_event_handler_h handlers[5] = {NULL, };

    event_callback.create = app_create;
    event_callback.terminate = app_terminate;
    event_callback.pause = app_pause;
    event_callback.resume = app_resume;
    event_callback.app_control = app_control;

    ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
    ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

    ret = ui_app_main(argc, argv, &event_callback, &ad);
    if (ret != APP_ERROR_NONE) {
       LOGW("ui_app_main failed, Err=%d\n", ret);
    }

    return ret;
}

