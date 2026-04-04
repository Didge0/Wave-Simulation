#ifndef VECTOR_H
#define VECTOR_H

#include <stdlib.h>
#include <stdbool.h>
#define MAX_VECT_SIZE_TO_SHOW 30

typedef struct{
      void* data;
      size_t len;
      size_t capacity;
      size_t elem_size;
}Vector;

typedef void (*Print_fct_ptr)(const void* elem);
typedef bool (*Elem_2_fct)(const void* elem1, const void* elem2);
typedef bool (*Elem_1_fct)(const void* elem1);

/**
 * @param type: Taille (en octet) du type (ex: char-> 1 octet)
 * @param size: Taille du vecteur souhaité
 * @return une struct Vector
 * @note Il est recommandé d'utiliser sizeof(TYPE)
 */
Vector vect_init_size(const size_t type, const size_t size);

/**
 * @param type: Taille (en octet) du type (ex: char-> 1 octet)
 * @return une struct Vector
 * @note Il est recommandé d'utiliser sizeof(TYPE)
 */
Vector vect_init(const size_t type);

/**
 * @param tab : Pointeur vers un tableau d'élément
 * @param size_tab : Nombre d'élément dans le tableau
 * @param type : Taille (en octet) du type (ex: char-> 1 octet)
 * @return une struct Vector
 * @note Il est recommandé d'utiliser sizeof(TYPE)
 */
Vector vect_init_tab(const void* tab, const size_t size_tab, const size_t type);

Vector vect_copy(Vector* src);


/**
 * @param vect: Pointeur vers un objet de type Vector
 * @param value: Pointeur vers la valeur à ajouter dans le vecteur
 * @param value_size: Taille (en octet) de value. Utiliser simplement sizeof()
 * @return -1 si le type de value_size est différent de celui que le vecteur possède vraiment
 */
bool vect_push_back(Vector* vect, const void* value, const size_t value_size);

/**
 * @brief Efface le contenu du vecteur sans désallouer sa mémoire
 */
void vect_clear(Vector* vect);
/**
 * @brief Désalloue la mémoire du vecteur
 */
void vect_free(Vector* vect);
/**
 * @brief Recupère la valeur à l'indice idx
 * @return Pointeur sur la valeur. Si OutOfBound return NULL
 */
void* vect_get(const Vector* vect, const size_t idx);

void vect_pop(Vector* vect);
void vect_erase(Vector*vect, const size_t beg, const size_t end);
void vect_resize(Vector* vect, const size_t size);
void vect_insert(Vector* vect, const size_t idx, const void* value, const size_t value_size);

/**
 * @brief fonction permettant de print un vecteur
 * @param vect : Vecteur à afficher
 * @param print : Poiteur de la fonction qui permet d'afficher un element du tableau.
 * @note Permet de laisser l'utilisateur créé la fonction qui correspond au type d'élément de sont tableau pour ensuite l'utiliser.
 * @warning La fonction affiche seulement les vecteurs dont la taille est inferieur à MAX_VECT_SIZE_TO_SHOW.
 */
void vect_show(const Vector* vect, const Print_fct_ptr print);

/**
 * @brief fonction qui permet de trier le vecteur
 * @param vect : Vecteur à trier
 * @param sort_fct : Pointeur de la fonction qui permet de trier le vecteur
 * @note si le fonction return true alors on ne swap pas les elements
 */
void vect_sort(Vector* vect, const Elem_2_fct sort_fct);

/**
 * @brief Fonction permettant de swap deux element d'un vecteur vect
 */
bool vect_swap(Vector* vect, const size_t idx1, const size_t idx2);

/**
 * @brief Fonction permettant de swap deux element d'un vecteur vect
 * @warning buffer a besoin d'avoir une mémoir suffisament grand pour acceuillir un élément de vect soit vect->elem_size
 */
bool vect_swap_buffer(Vector* vect, const size_t idx1, const size_t idx2, void* buffer);

size_t vect_remove_if(Vector* vect, const Elem_1_fct elem_fct);

/**
 * @brief Retire les doublons succesif du vecteur
 */
size_t vect_unique(Vector* vect);


/**
 * @brief Ajoute un élément et retire le dernier (conservation de la taille et de l'ordre)
 * @warning Cette fonction est en O(n) donc pas optimal il faut que je créé un lib Buffer Circulaire
 */
bool vect_push_back_pop_front(Vector* vect, const void* value, const size_t value_size);

/**
 * @brief Retire l'élément d'indice 0 du vecteur et concervation de la capacité
 * @warning Cette fonction est en O(n) donc pas optimal par ce que les élément sont contigue il faudré une map plus comme ça pas de soucis
 */
bool vect_pop_front(Vector* vect);

/**
 * @brief Retire l'élément d'indice 0 du vecteur et concervation de la capacité
 * @warning Cette fonction est en O(n) donc pas optimal par ce que les élément sont contigue il faudré une map plus comme ça pas de soucis
 */
bool vect_pop_idx(Vector* vect, size_t idx);


#endif

