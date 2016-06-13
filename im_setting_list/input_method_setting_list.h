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
#ifndef __INPUTMETHOD_SETTING_LIST_H__
#define __INPUTMETHOD_SETTING_LIST_H__

#include <Elementary.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "INPUTMETHOD_SETTING_LIST"

enum {
     APP_STATE_PAUSE = 1,
     APP_STATE_SERVICE,
     APP_STATE_RESUME,
     APP_STATE_TERMINATE,
};

enum {
    APP_TYPE_SETTING = 1,
    APP_TYPE_SETTING_NO_ROTATION,
    APP_TYPE_NORMAL,
};

typedef struct _appdata {
    Evas_Object *win;
    Evas_Object *conform;
    Evas_Object *naviframe;
    Evas_Object *genlist;
    Evas_Object *popup;
    Evas_Object *main_circle_genlist;

    int app_state;
    int app_type;
} appdata;

#endif /* __INPUTMETHOD_SETTING_LIST_H__ */
