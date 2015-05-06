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

#ifndef __INPUT_METHOD_SETTING_EFL_H
#define __INPUT_METHOD_SETTING_EFL_H

#define INPUT_METHOD_SETTING_DEBUG(msg, args...)   LOGD(msg, ##args)
#define INPUT_METHOD_SETTING_ERROR(msg, args...)   LOGE(msg, ##args)

void im_setting_selector_app_create(void *data);
void im_setting_selector_app_pause (void *data);
void action_on_resume (void *priv);
void action_on_destroy (void *priv);

#endif /* __INPUT_METHOD_SETTING_EFL_H */

/*
vi:ts=4:ai:nowrap:expandtab
 */
