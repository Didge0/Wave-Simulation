#include "menu.h"

#include <stdio.h>
#include <string.h>

static bool menu_push_item(Menu_Params* params, const Menu_Item* item){
      if(params == NULL || item == NULL){
            return false;
      }

      if(params->items.len >= MENU_MAX_ITEMS){
            return false;
      }

      return vect_push_back(&params->items, item, sizeof(*item));
}

static void menu_notify_change(Menu_Item* item){
      if(item != NULL && item->on_change != NULL){
            item->on_change(item, item->user_data);
      }
}

static int menu_clamp_int(int value, int min_value, int max_value){
      if(value < min_value){
            return min_value;
      }
      if(value > max_value){
            return max_value;
      }
      return value;
}

static unsigned int menu_clamp_uint(unsigned int value, unsigned int min_value, unsigned int max_value){
      if(value < min_value){
            return min_value;
      }
      if(value > max_value){
            return max_value;
      }
      return value;
}

static float menu_clamp_float(float value, float min_value, float max_value){
      if(value < min_value){
            return min_value;
      }
      if(value > max_value){
            return max_value;
      }
      return value;
}

static int menu_wrap_int(int value, int min_value, int max_value, int step, int direction){
      int step_count;
      int current_index;
      int next_index;

      if(step <= 0){
            step = 1;
      }

      if(max_value < min_value){
            return value;
      }

      value = menu_clamp_int(value, min_value, max_value);
      step_count = ((max_value - min_value) / step) + 1;
      if(step_count <= 0){
            return min_value;
      }

      current_index = (value - min_value) / step;
      next_index = (current_index + direction) % step_count;
      if(next_index < 0){
            next_index += step_count;
      }

      return min_value + (next_index * step);
}

static void menu_copy_fit_text(char* dst, size_t dst_size, const char* src, size_t max_chars){
      size_t src_len = 0u;

      if(dst == NULL || dst_size == 0u){
            return;
      }

      if(src == NULL){
            dst[0] = '\0';
            return;
      }

      src_len = strlen(src);
      if(max_chars + 1u > dst_size){
            max_chars = dst_size - 1u;
      }

      if(src_len <= max_chars){
            memcpy(dst, src, src_len + 1u);
            return;
      }

      if(max_chars <= 3u){
            for(size_t i = 0u; i < max_chars; i++){
                  dst[i] = '.';
            }
            dst[max_chars] = '\0';
            return;
      }

      memcpy(dst, src, max_chars - 3u);
      memcpy(dst + max_chars - 3u, "...", 3u);
      dst[max_chars] = '\0';
}

static size_t menu_grid_columns(void){
      return 2u;
}

static size_t menu_row_count(const Menu_Params* params){
      const size_t columns = menu_grid_columns();

      if(params == NULL || params->items.len == 0u){
            return 0u;
      }

      return (params->items.len + columns - 1u) / columns;
}

static bool menu_has_index(const Menu_Params* params, size_t index){
      return params != NULL && index < params->items.len;
}

static size_t menu_index_from_cell(size_t row, size_t column){
      return (row * menu_grid_columns()) + column;
}

void menu_init(Menu_Params* params){
      if(params == NULL){
            return;
      }

      params->items = vect_init(sizeof(Menu_Item));
      params->selected_index = 0u;
      params->is_open = false;
}

void menu_free(Menu_Params* params){
      if(params == NULL){
            return;
      }

      vect_free(&params->items);
      params->selected_index = 0u;
      params->is_open = false;
}

bool menu_add_bool(Menu_Params* params, const char* label, bool* value_ptr, Menu_OnChangeFn on_change, void* user_data){
      if(params->items.len >= MENU_MAX_ITEMS){
            printf("Impossible d'ajouter l'item '%s': nombre maximum d'items atteint\n", label);
            return false;
      }
      Menu_Item item = {0};

      item.label = label;
      item.type = MENU_VALUE_BOOL;
      item.value_ptr = value_ptr;
      item.on_change = on_change;
      item.user_data = user_data;
      return menu_push_item(params, &item);
}

bool menu_add_int(Menu_Params* params, const char* label, int* value_ptr, int min_value, int max_value, int step, Menu_OnChangeFn on_change, void* user_data){
      if(params->items.len >= MENU_MAX_ITEMS){
            printf("Impossible d'ajouter l'item '%s': nombre maximum d'items atteint\n", label);
            return false;
      }
      Menu_Item item = {0};

      item.label = label;
      item.type = MENU_VALUE_INT;
      item.value_ptr = value_ptr;
      item.range.int_range.min_value = min_value;
      item.range.int_range.max_value = max_value;
      item.range.int_range.step = (step == 0) ? 1 : step;
      item.on_change = on_change;
      item.user_data = user_data;
      return menu_push_item(params, &item);
}

bool menu_add_uint(Menu_Params* params, const char* label, unsigned int* value_ptr, unsigned int min_value, unsigned int max_value, unsigned int step, Menu_OnChangeFn on_change, void* user_data){
      if(params->items.len >= MENU_MAX_ITEMS){
            printf("Impossible d'ajouter l'item '%s': nombre maximum d'items atteint\n", label);
            return false;
      }
      Menu_Item item = {0};

      item.label = label;
      item.type = MENU_VALUE_UINT;
      item.value_ptr = value_ptr;
      item.range.uint_range.min_value = min_value;
      item.range.uint_range.max_value = max_value;
      item.range.uint_range.step = (step == 0u) ? 1u : step;
      item.on_change = on_change;
      item.user_data = user_data;
      return menu_push_item(params, &item);
}

bool menu_add_float(Menu_Params* params, const char* label, float* value_ptr, float min_value, float max_value, float step, Menu_OnChangeFn on_change, void* user_data){
      if(params->items.len >= MENU_MAX_ITEMS){
            printf("Impossible d'ajouter l'item '%s': nombre maximum d'items atteint\n", label);
            return false;
      }
      Menu_Item item = {0};

      item.label = label;
      item.type = MENU_VALUE_FLOAT;
      item.value_ptr = value_ptr;
      item.range.float_range.min_value = min_value;
      item.range.float_range.max_value = max_value;
      item.range.float_range.step = (step == 0.0f) ? 0.1f : step;
      item.on_change = on_change;
      item.user_data = user_data;
      return menu_push_item(params, &item);
}

bool menu_add_enum(Menu_Params* params, const char* label, int* value_ptr, int min_value, int max_value, int step, Menu_EnumLabelFn label_fn, Menu_OnChangeFn on_change, void* user_data){
      if(params->items.len >= MENU_MAX_ITEMS){
            printf("Impossible d'ajouter l'item '%s': nombre maximum d'items atteint\n", label);
            return false;
      }
      Menu_Item item = {0};

      item.label = label;
      item.type = MENU_VALUE_ENUM;
      item.value_ptr = value_ptr;
      item.range.enum_range.min_value = min_value;
      item.range.enum_range.max_value = max_value;
      item.range.enum_range.step = (step == 0) ? 1 : step;
      item.range.enum_range.label_fn = label_fn;
      item.on_change = on_change;
      item.user_data = user_data;
      return menu_push_item(params, &item);
}

bool menu_add_action(Menu_Params* params, const char* label, Menu_ActionFn action, void* user_data){
      if(params->items.len >= MENU_MAX_ITEMS){
            printf("Impossible d'ajouter l'item '%s': nombre maximum d'items atteint\n", label);
            return false;
      }
      Menu_Item item = {0};

      item.label = label;
      item.type = MENU_VALUE_ACTION;
      item.range.action.action = action;
      item.user_data = user_data;
      return menu_push_item(params, &item);
}

size_t menu_count(const Menu_Params* params){
      if(params == NULL){
            return 0u;
      }

      return params->items.len;
}

Menu_Item* menu_get_item(Menu_Params* params, size_t index){
      if(params == NULL){
            return NULL;
      }

      return (Menu_Item*)vect_get(&params->items, index);
}

const Menu_Item* menu_get_item_const(const Menu_Params* params, size_t index){
      if(params == NULL){
            return NULL;
      }

      return (const Menu_Item*)vect_get(&params->items, index);
}

Menu_Item* menu_get_selected(Menu_Params* params){
      if(params == NULL || params->items.len == 0u){
            return NULL;
      }

      if(params->selected_index >= params->items.len){
            params->selected_index = params->items.len - 1u;
      }

      return menu_get_item(params, params->selected_index);
}

void menu_select_next(Menu_Params* params){
      if(params == NULL || params->items.len == 0u){
            return;
      }

      params->selected_index = (params->selected_index + 1u) % params->items.len;
}

void menu_select_previous(Menu_Params* params){
      if(params == NULL || params->items.len == 0u){
            return;
      }

      if(params->selected_index == 0u){
            params->selected_index = params->items.len - 1u;
      }else{
            params->selected_index--;
      }
}

void menu_select_column(Menu_Params* params, size_t column){
      if(params == NULL || params->items.len == 0u){
            return;
      }

      const size_t max_columns = menu_grid_columns();
      const size_t current_row = params->selected_index / max_columns;
      size_t new_index;

      if(column >= max_columns){
            return;
      }

      new_index = menu_index_from_cell(current_row, column);
      if(!menu_has_index(params, new_index)){
            return;
      }

      params->selected_index = new_index;
}

void menu_select_row(Menu_Params* params, size_t row){
      if(params == NULL || params->items.len == 0u){
            return;
      }

      const size_t max_columns = menu_grid_columns();
      const size_t current_column = params->selected_index % max_columns;
      const size_t max_rows = menu_row_count(params);
      size_t new_index;

      if(row >= max_rows){
            return;
      }

      new_index = menu_index_from_cell(row, current_column);
      if(!menu_has_index(params, new_index)){
            while(row > 0u){
                  row--;
                  new_index = menu_index_from_cell(row, current_column);
                  if(menu_has_index(params, new_index)){
                        params->selected_index = new_index;
                        return;
                  }
            }
            return;
      }

      params->selected_index = new_index;
}

bool menu_change_value(Menu_Params* params, int direction){
      Menu_Item* item = menu_get_selected(params);

      if(item == NULL){
            return false;
      }

      switch(item->type){
      case MENU_VALUE_BOOL:
            if(item->value_ptr == NULL){
                  return false;
            }
            *(bool*)item->value_ptr = !(*(bool*)item->value_ptr);
            menu_notify_change(item);
            return true;

      case MENU_VALUE_INT:
            if(item->value_ptr == NULL){
                  return false;
            }
            *(int*)item->value_ptr = menu_clamp_int(
                  *(int*)item->value_ptr + direction * item->range.int_range.step,
                  item->range.int_range.min_value,
                  item->range.int_range.max_value
            );
            menu_notify_change(item);
            return true;

      case MENU_VALUE_UINT:
            if(item->value_ptr == NULL){
                  return false;
            }
            if(direction < 0){
                  unsigned int current = *(unsigned int*)item->value_ptr;
                  unsigned int step = item->range.uint_range.step;
                  *(unsigned int*)item->value_ptr = (current > step) ? (current - step) : 0u;
            }else if(direction > 0){
                  *(unsigned int*)item->value_ptr += item->range.uint_range.step;
            }
            *(unsigned int*)item->value_ptr = menu_clamp_uint(
                  *(unsigned int*)item->value_ptr,
                  item->range.uint_range.min_value,
                  item->range.uint_range.max_value
            );
            menu_notify_change(item);
            return true;

      case MENU_VALUE_FLOAT:
            if(item->value_ptr == NULL){
                  return false;
            }
            *(float*)item->value_ptr = menu_clamp_float(
                  *(float*)item->value_ptr + ((float)direction * item->range.float_range.step),
                  item->range.float_range.min_value,
                  item->range.float_range.max_value
            );
            menu_notify_change(item);
            return true;

      case MENU_VALUE_ENUM:
            if(item->value_ptr == NULL){
                  return false;
            }
            *(int*)item->value_ptr = menu_wrap_int(
                  *(int*)item->value_ptr,
                  item->range.enum_range.min_value,
                  item->range.enum_range.max_value,
                  item->range.enum_range.step,
                  direction
            );
            menu_notify_change(item);
            return true;

      case MENU_VALUE_ACTION:
            if(item->range.action.action == NULL){
                  return false;
            }
            item->range.action.action(item->user_data);
            return true;
      }

      return false;
}

bool menu_activate_selected(Menu_Params* params){
      return menu_change_value(params, 1);
}

bool menu_bind_default_key_down(Menu_Params* params, SDL_Keycode key, int is_repeat){
      const size_t max_columns = menu_grid_columns();
      const size_t current_row = (params != NULL) ? (params->selected_index / max_columns) : 0u;
      const size_t current_column = (params != NULL) ? (params->selected_index % max_columns) : 0u;
      const size_t row_count = menu_row_count(params);

      if(params == NULL || is_repeat){
            return false;
      }

      switch(key){
      case SDLK_UP:
            if(row_count == 0u){
                  return false;
            }
            if(current_row == 0u){
                  size_t target_row = row_count - 1u;
                  size_t target_index = menu_index_from_cell(target_row, current_column);

                  while(target_row > 0u && !menu_has_index(params, target_index)){
                        target_row--;
                        target_index = menu_index_from_cell(target_row, current_column);
                  }
                  if(menu_has_index(params, target_index)){
                        params->selected_index = target_index;
                  }
            }else{
                  menu_select_row(params, current_row - 1u);
            }
            return true;

      case SDLK_DOWN:
            if(row_count == 0u){
                  return false;
            }
            if(current_row + 1u >= row_count){
                  size_t target_index = menu_index_from_cell(0u, current_column);

                  if(menu_has_index(params, target_index)){
                        params->selected_index = target_index;
                  }
            }else{
                  menu_select_row(params, current_row + 1u);
            }
            return true;

      case SDLK_LEFT:
            if(current_column == 0u){
                  menu_select_column(params, max_columns - 1u);
            }else{
                  menu_select_column(params, current_column - 1u);
            }
            return true;

      case SDLK_RIGHT:
            menu_select_column(params, (current_column + 1u) % max_columns);
            return true;

      case SDLK_KP_PLUS:
            return menu_change_value(params, 1);
      case SDLK_KP_MINUS:
            return menu_change_value(params, -1);

      default:
            return false;
      }
}

bool menu_format_item_value(const Menu_Item* item, char* buffer, size_t buffer_size){
      if(item == NULL || buffer == NULL || buffer_size == 0u){
            return false;
      }

      switch(item->type){
      case MENU_VALUE_BOOL:
            if(item->value_ptr == NULL){
                  return false;
            }
            snprintf(buffer, buffer_size, "%s", (*(bool*)item->value_ptr) ? "ON" : "OFF");
            return true;

      case MENU_VALUE_INT:
            if(item->value_ptr == NULL){
                  return false;
            }
            snprintf(buffer, buffer_size, "%d", *(int*)item->value_ptr);
            return true;

      case MENU_VALUE_UINT:
            if(item->value_ptr == NULL){
                  return false;
            }
            snprintf(buffer, buffer_size, "%u", *(unsigned int*)item->value_ptr);
            return true;

      case MENU_VALUE_FLOAT:
            if(item->value_ptr == NULL){
                  return false;
            }
            snprintf(buffer, buffer_size, "%.2f", *(float*)item->value_ptr);
            return true;

      case MENU_VALUE_ENUM:
            if(item->value_ptr == NULL){
                  return false;
            }
            if(item->range.enum_range.label_fn != NULL){
                  const char* label = item->range.enum_range.label_fn(*(int*)item->value_ptr, item->user_data);
                  snprintf(buffer, buffer_size, "%s", (label != NULL) ? label : "?");
            }else{
                  snprintf(buffer, buffer_size, "%d", *(int*)item->value_ptr);
            }
            return true;

      case MENU_VALUE_ACTION:
            snprintf(buffer, buffer_size, "EXEC");
            return true;
      }

      return false;
}

static void menu_show_item(SDL_Renderer* renderer, const Menu_Item* item, const SDL_FRect* item_rect){
      if(renderer == NULL || item == NULL){
            return;
      }

      char value_buffer[64];
      char label_buffer[64];
      char value_display[64];
      if(!menu_format_item_value(item, value_buffer, sizeof(value_buffer))){
            value_buffer[0] = '\0';
      }

      const float text_scale = menu_clamp_float(item_rect->h / 20.0f, 1.0f, 2.6f);
      const float text_left_padding_ratio = 0.04f;
      const float text_top_padding_ratio = 0.18f;
      const float value_column_ratio = 0.58f;
      const float label_area_ratio = 0.50f;
      const float value_area_ratio = 0.34f;
      const float text_x = item_rect->x + (item_rect->w * text_left_padding_ratio);
      const float text_y = item_rect->y + (item_rect->h * text_top_padding_ratio);
      const float value_x = item_rect->x + (item_rect->w * value_column_ratio);
      const size_t max_label_chars = (size_t)menu_clamp_float((item_rect->w * label_area_ratio) / (8.0f * text_scale), 3.0f, 63.0f);
      const size_t max_value_chars = (size_t)menu_clamp_float((item_rect->w * value_area_ratio) / (8.0f * text_scale), 3.0f, 63.0f);

      menu_copy_fit_text(label_buffer, sizeof(label_buffer), (item->label != NULL) ? item->label : "Param", max_label_chars);
      menu_copy_fit_text(value_display, sizeof(value_display), value_buffer, max_value_chars);

      SDL_SetRenderScale(renderer, text_scale, text_scale);
      SDL_SetRenderDrawColor(renderer, 255,255,255,255);
      SDL_RenderDebugTextFormat(
            renderer,
            (int)(text_x / text_scale),
            (int)(text_y / text_scale),
            "%s",
            label_buffer
      );
      SDL_RenderDebugTextFormat(
            renderer,
            (int)(value_x / text_scale),
            (int)(text_y / text_scale),
            "%s",
            value_display
      );
      SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

bool menu_show(SDL_Window* window, Menu_Params* params){
      if(window == NULL || params == NULL){
            return false;
      }

      SDL_Renderer* renderer = SDL_GetRenderer(window);
      if(renderer == NULL){
            return false;
      }
      int width, height;
      const int letter_size = 8;
      const float title_scale = 4.0f;
      const int max_rows = 8;
      const int max_columns = 2;
      const size_t item_count = (params->items.len > MENU_MAX_ITEMS) ? MENU_MAX_ITEMS : params->items.len;
      const float panel_x_ratio = 0.10f;
      const float panel_y_ratio = 0.10f;
      const float panel_w_ratio = 0.80f;
      const float panel_h_ratio = 0.80f;
      const float title_y_ratio = 0.03f;
      const float inner_padding_x_ratio = 0.05f;
      const float inner_padding_top_ratio = 0.14f;
      const float inner_padding_bottom_ratio = 0.08f;
      const float gap_x_ratio = 0.04f;
      const float gap_y_ratio = 0.02f;

      SDL_GetWindowSizeInPixels(window, &width, &height);

      SDL_FRect rect = (SDL_FRect){
            width * panel_x_ratio,
            height * panel_y_ratio,
            width * panel_w_ratio,
            height * panel_h_ratio
      };
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, 61,61,61,100);
      SDL_RenderFillRect(renderer, &rect);
      SDL_SetRenderDrawColor(renderer, 220,220,220,255);
      SDL_RenderRect(renderer, &rect);

      
      const float adaptive_title_scale = menu_clamp_float(rect.h / 120.0f, 1.5f, title_scale);
      const int title_width = (int)((float)width / adaptive_title_scale);
      const int title_length = 4;

      SDL_SetRenderDrawColor(renderer, 255,255,255,255);
      SDL_SetRenderScale(renderer, adaptive_title_scale, adaptive_title_scale);
      SDL_RenderDebugTextFormat(
            renderer,
            (title_width - (title_length * letter_size)) / 2,
            (int)((rect.y + (rect.h * title_y_ratio)) / adaptive_title_scale),
            "MENU"
      );
      SDL_SetRenderScale(renderer, 1.0f, 1.0f);
      

      const float inner_padding_x = rect.w * inner_padding_x_ratio;
      const float inner_padding_y = rect.h * inner_padding_top_ratio;
      const float gap_x = rect.w * gap_x_ratio;
      const float gap_y = rect.h * gap_y_ratio;
      const float cell_width = (rect.w - (2.0f * inner_padding_x) - gap_x) / (float)max_columns;
      const float cell_height = (rect.h - inner_padding_y - (rect.h * inner_padding_bottom_ratio) - (gap_y * (float)(max_rows - 1))) / (float)max_rows;

      for(size_t i = 0u; i < MENU_MAX_ITEMS; i++){
            const int column = (int)(i%max_columns);
            const int row = (int)(i / max_columns);
            const float x = rect.x + inner_padding_x + ((cell_width + gap_x) * (float)column);
            const float y = rect.y + inner_padding_y + ((cell_height + gap_y) * (float)row);
            SDL_FRect item_rect = {x, y, cell_width, cell_height};
            const Menu_Item* item = (i < item_count) ? menu_get_item_const(params, i) : NULL;

            if(i == params->selected_index && item != NULL){
                  SDL_SetRenderDrawColor(renderer, 130,170,220,180);
            }else if(item != NULL){
                  SDL_SetRenderDrawColor(renderer, 20,20,20,180);
            }else{
                  SDL_SetRenderDrawColor(renderer, 35,35,35,90);
            }
            SDL_RenderFillRect(renderer, &item_rect);
            SDL_SetRenderDrawColor(renderer, 200,200,200,255);
            SDL_RenderRect(renderer, &item_rect);

            if(item != NULL){
                  menu_show_item(renderer, item, &item_rect);
            }
      }
      

      return params->items.len > 0u;
}