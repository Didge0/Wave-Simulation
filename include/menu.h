#ifndef MENU_H
#define MENU_H

#include <stdbool.h>
#include <stddef.h>

#include <SDL3/SDL.h>

#include "vector.h"

#define MENU_MAX_ITEMS 16u

typedef enum {
      MENU_VALUE_BOOL = 0,
      MENU_VALUE_INT,
      MENU_VALUE_UINT,
      MENU_VALUE_FLOAT,
      MENU_VALUE_ENUM,
      MENU_VALUE_ACTION
} Menu_ValueType;

typedef struct Menu_Item Menu_Item;

typedef const char* (*Menu_EnumLabelFn)(int value, void* user_data);
typedef void (*Menu_OnChangeFn)(Menu_Item* item, void* user_data);
typedef void (*Menu_ActionFn)(void* user_data);

typedef struct {
      int min_value;
      int max_value;
      int step;
} Menu_IntRange;

typedef struct {
      unsigned int min_value;
      unsigned int max_value;
      unsigned int step;
} Menu_UIntRange;

typedef struct {
      float min_value;
      float max_value;
      float step;
} Menu_FloatRange;

typedef struct {
      int min_value;
      int max_value;
      int step;
      Menu_EnumLabelFn label_fn;
} Menu_EnumRange;

typedef struct {
      Menu_ActionFn action;
} Menu_ActionRange;

struct Menu_Item {
      const char* label;
      Menu_ValueType type;
      void* value_ptr;
      void* user_data;
      Menu_OnChangeFn on_change;
      union {
            Menu_IntRange int_range;
            Menu_UIntRange uint_range;
            Menu_FloatRange float_range;
            Menu_EnumRange enum_range;
            Menu_ActionRange action;
      } range;
};

typedef struct {
      Vector items;
      size_t selected_index;
      bool is_open;
} Menu_Params;

void menu_init(Menu_Params* params);
void menu_free(Menu_Params* params);

bool menu_add_bool(Menu_Params* params, const char* label, bool* value_ptr, Menu_OnChangeFn on_change, void* user_data);
bool menu_add_int(Menu_Params* params, const char* label, int* value_ptr, int min_value, int max_value, int step, Menu_OnChangeFn on_change, void* user_data);
bool menu_add_uint(Menu_Params* params, const char* label, unsigned int* value_ptr, unsigned int min_value, unsigned int max_value, unsigned int step, Menu_OnChangeFn on_change, void* user_data);
bool menu_add_float(Menu_Params* params, const char* label, float* value_ptr, float min_value, float max_value, float step, Menu_OnChangeFn on_change, void* user_data);
bool menu_add_enum(Menu_Params* params, const char* label, int* value_ptr, int min_value, int max_value, int step, Menu_EnumLabelFn label_fn, Menu_OnChangeFn on_change, void* user_data);
bool menu_add_action(Menu_Params* params, const char* label, Menu_ActionFn action, void* user_data);

size_t menu_count(const Menu_Params* params);
Menu_Item* menu_get_item(Menu_Params* params, size_t index);
const Menu_Item* menu_get_item_const(const Menu_Params* params, size_t index);
Menu_Item* menu_get_selected(Menu_Params* params);

void menu_select_next(Menu_Params* params);
void menu_select_previous(Menu_Params* params);
void menu_select_column(Menu_Params* params, size_t column);
void menu_select_row(Menu_Params* params, size_t row);
bool menu_change_value(Menu_Params* params, int direction);
bool menu_activate_selected(Menu_Params* params);
bool menu_format_item_value(const Menu_Item* item, char* buffer, size_t buffer_size);
bool menu_bind_default_key_down(Menu_Params* params, SDL_Keycode key, int is_repeat);

bool menu_show(SDL_Window* window, Menu_Params* params);

#endif
