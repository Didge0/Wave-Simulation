#include "vector.h"
#include "color.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

Vector vect_init_size(const size_t type, const size_t size){
      Vector vect;
      vect.data = NULL;
      vect.len = 0;
      vect.capacity = 0;
      vect.elem_size = type;
      if(size != 0){
            vect_resize(&vect, size);
      }
      return vect;
}
Vector vect_init(const size_t type){
      return vect_init_size(type,0);
}
Vector vect_init_tab(const void* tab, const size_t size_tab, const size_t type){
      Vector vect = vect_init_size(type, size_tab);
      memcpy(vect.data, tab, size_tab * type);
      return vect;
}
Vector vect_copy(Vector* src){
      Vector dst;
      dst.capacity = src->capacity;
      dst.elem_size = src->elem_size;
      dst.len = src->len;
      dst.data = malloc(dst.capacity * dst.elem_size);
      memcpy(dst.data, src->data, dst.len * dst.elem_size);
      return dst;
}

bool vect_push_back(Vector* vect, const void* value, const size_t value_size){
      if(value_size != vect->elem_size){
            fprintf(stderr, COLOR_RED COLOR_BOLD "Erreur " COLOR_WHITE "vect_push_back " COLOR_RESET ": Taille incompatique [%zd!=%zd]\n",vect->elem_size,value_size);
            return false;
      }
      if(vect->len*vect->elem_size == vect->capacity){
            size_t new_capacity = (vect->capacity == 0)? vect->elem_size : vect->capacity*2;
            void* new_data = realloc(vect->data, new_capacity);
            assert(new_data != NULL);
            vect->data = new_data;
            vect->capacity = new_capacity;
      }
      memcpy((char*)vect->data+vect->len*vect->elem_size,value, vect->elem_size);
      vect->len++;
      return true;
}

void vect_clear(Vector* vect){
      vect->len = 0;
}

void vect_free(Vector* vect){
     free( vect->data);
     vect->data = NULL;
     vect->capacity = 0;
     vect->elem_size = 0;
     vect->len = 0;
}

void* vect_get(const Vector* vect, const size_t idx){
      if(idx>=vect->len){
            fprintf(stderr, COLOR_RED COLOR_BOLD "Erreur " COLOR_WHITE "vect_get[%zd] " COLOR_RESET ": OutOfBound size of vect is %zd\n",idx,vect->len);
            return NULL;  
      }
      return (char*)vect->data + idx*vect->elem_size;
}

void vect_pop(Vector* vect){
      vect->len--;
}

void vect_erase(Vector*vect, const size_t beg, const size_t end){
      if(beg>=vect->len || end>=vect->len){
            fprintf(stderr, COLOR_RED COLOR_BOLD "Erreur " COLOR_WHITE "vect_erase" COLOR_RESET ": OutOfBound size of vect is %zd but your beg is %zd and end is %zd\n",vect->len,beg, end);
            return ;  
      }
      if(beg>end){
            fprintf(stderr, COLOR_RED COLOR_BOLD "Erreur " COLOR_WHITE "vect_erase" COLOR_RESET ": beg is higher than end (%zd < %zd)\n",beg, end);
            return;
      }
      if(end != vect->len - 1){
            memmove(vect_get(vect,beg), vect_get(vect,end+1), (vect->len - end -1) * vect->elem_size);
      }
      

      vect->len -= (end-beg+1);
      if(vect->capacity > 8 && vect->len < vect->capacity/4){
            size_t new_cap = vect->capacity/2;
            while(new_cap >8 && vect->len < new_cap/4){
                  new_cap /= 2;
            }
            
            void* new_data = realloc(vect->data, new_cap * vect->elem_size);
            if(new_data){
                  vect->capacity = new_cap;
                  vect->data = new_data;
            }
      }
}

void vect_resize(Vector* vect, const size_t size){
      if(size<=vect->capacity){
            vect->len = size;
      }else{
            size_t new_capacity = size;
            void* new_data = realloc(vect->data, new_capacity * vect->elem_size);
            assert(new_data != NULL);
            vect->data = new_data;
            vect->capacity = new_capacity;
            
            void* ptr;
            if(vect->len != 0)
                  ptr = (char*)vect_get(vect,vect->len-1) + vect->elem_size;
            else
                  ptr = (char*)vect->data;
            memset(ptr,0,vect->elem_size * (size - vect->len));
            vect->len = size;
      }
}

void vect_insert(Vector* vect, const size_t idx, const void* value, const size_t value_size){
      if(idx>vect->len){
            fprintf(stderr, COLOR_RED COLOR_BOLD "Erreur " COLOR_WHITE "vect_insert" COLOR_RESET ": OutOfBound size of vect is %zd but your idx is %zd\n",vect->len,idx);
            return ;  
      }
      if(value_size != vect->elem_size){
            fprintf(stderr, COLOR_RED COLOR_BOLD "Erreur " COLOR_WHITE "vect_insert " COLOR_RESET ": Taille incompatique [%zd!=%zd]\n",vect->elem_size,value_size);
            return ;
      }
      if(idx == vect->len){
            vect_push_back(vect, value, value_size);
            return;
      }
      if(vect->len == vect->capacity){
            size_t new_capacity = (vect->capacity == 0)? 1 : vect->capacity*2;
            void* new_data = realloc(vect->data, new_capacity*vect->elem_size);
            assert(new_data != NULL);
            vect->data = new_data;
            vect->capacity = new_capacity;
      }
      void* ptrSource = vect_get(vect, idx);
      void* ptrDest = (char*)ptrSource + vect->elem_size;
      memmove(ptrDest, ptrSource, (vect->len - idx) * vect->elem_size);
      memcpy(ptrSource, value, vect->elem_size);
      vect->len++;
}

void vect_show(const Vector* vect, const Print_fct_ptr print){
      if(vect->len > MAX_VECT_SIZE_TO_SHOW)
            return;
      void* val_to_print;
      printf("[");
      for(unsigned int i=0; i<vect->len;i++){
            val_to_print = vect_get(vect, i);
            if(i!=0)
                  printf(",");
            if(val_to_print)
                  print(val_to_print);
      }
      printf("]\n");
}

void vect_sort(Vector* vect, const Elem_2_fct sort_fct){
      size_t j,i;
      void* tmp = malloc(vect->elem_size);
      void* elem1 = NULL;
      void* elem2 = NULL;
      for(j=1; j<vect->len; j++){
            for(i=0; i<vect->len-1; i++){
                  elem1 = vect_get(vect,i);
                  elem2 = vect_get(vect,i+1);
                  if(!sort_fct(elem1, elem2)){
                        memcpy(tmp, elem1, vect->elem_size);
                        memcpy(elem1, elem2, vect->elem_size);
                        memcpy(elem2, tmp, vect->elem_size);
                  }
            }
      }
      free(tmp);
}

bool vect_swap(Vector* vect, const size_t idx1, const size_t idx2){
      if(idx1 >= vect->len || idx2 >= vect->len){
            return 0;
      }
      void* temp_data = malloc(vect->elem_size);
      memcpy(temp_data, vect_get(vect, idx1), vect->elem_size);
      memcpy(vect_get(vect, idx1),vect_get(vect, idx2), vect->elem_size);
      memcpy(vect_get(vect, idx2), temp_data, vect->elem_size);
      free(temp_data);
      return 1;
}

bool vect_swap_buffer(Vector* vect, const size_t idx1, const size_t idx2, void* buffer){
      if(idx1 >= vect->len || idx2 >= vect->len){
            return 0;
      }
      memcpy(buffer, vect_get(vect, idx1), vect->elem_size);
      memcpy(vect_get(vect, idx1),vect_get(vect, idx2), vect->elem_size);
      memcpy(vect_get(vect, idx2), buffer, vect->elem_size);
      return 1;
}

void easter_egg(){
      printf("Bravo tu a trouvé un easter egg !!!\n");
}

size_t vect_remove_if(Vector* vect, const Elem_1_fct pred) {
    size_t write = 0;

    for (size_t read = 0; read < vect->len; ++read) {
        void* elem = vect_get(vect, read);

        if (!pred(elem)) {
            if (write != read) {
                memcpy(vect_get(vect, write), elem, vect->elem_size);
            }
            write++;
        }
    }

    return write;
}

typedef bool (*Elem_2_fct_size)(const void* elem1, const void* elem2, const size_t size);
static void remove_if_for_unique(Vector* vect, const Elem_2_fct_size elem_fct){

}

static bool unique_compar(const void* elem1, const void* elem2, const size_t size){
      return memcmp(elem1, elem2, size) == 0;
}


size_t vect_unique(Vector* vect){
      size_t write = 1;
      void* elem1;
      void* elem2;
      for(unsigned int i=1; i<vect->len; i++){
            elem1 = vect_get(vect, i-1);
            elem2 = vect_get(vect, i);
            if(!unique_compar(elem1,elem2,vect->elem_size)){
                  if(i!=write){
                        memcpy(vect_get(vect,write), elem2, vect->elem_size);
                  }
                  write++;  
            }
      }
      return write;
}

bool vect_push_back_pop_front(Vector* vect, const void* value, const size_t value_size){
      if(value_size != vect->elem_size){
            fprintf(stderr, COLOR_RED COLOR_BOLD "Erreur " COLOR_WHITE "vect_push_back " COLOR_RESET ": Taille incompatique [%zd!=%zd]\n",vect->elem_size,value_size);
            return false;
      }
      memmove(vect->data, ((char*)vect->data + vect->elem_size), (vect->len - 1) * vect->elem_size);
      memcpy((char*)vect->data+((vect->len-1)*vect->elem_size),value, vect->elem_size);
      return true;
}

bool vect_pop_front(Vector* vect){
      if(vect->len == 0){
            return false;
      }

      memmove(vect->data, ((char*)vect->data + vect->elem_size), (vect->len-1) * vect->elem_size);
      vect->len -=1;
      return true;
}

bool vect_pop_idx(Vector* vect, size_t idx){
      if(vect->len == 0 || vect->len <= idx){
            return false;
      }
      if(vect->len-1 == idx){
            vect->len--;
            return true;
      }

      memmove(((char*)vect->data + vect->elem_size * idx), ((char*)vect->data + vect->elem_size * (idx+1)), (vect->len-1-idx) * vect->elem_size);
      vect->len--;
      return true;
}





