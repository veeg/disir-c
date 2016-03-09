
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "multimap.h"

#define INIT_NUM_BUCKETS 64 //!< Initial number of buckets
#define INIT_VALUE_LIST_CAPACITY 8 //!< Initial capacity of each mapnodes value_list

//! \brief Data structure describing a list node in hash map.
//!
struct mapnode
{
  void *key;                //!< Key associated with element.
  void **value_list;        //!< Dynamic array of Values for this key.
  int value_size;           //!< Number of elements present in the value_list
  int value_capacity;       //!< Capacity of the allocated value_list.
  struct mapnode *next;     //!< Pointer to next node in chained list.
};

//! \brief Data structure containing the internals of the hash map implementation.
//!
struct multimap 
{
  struct mapnode **buckets; //!< Hash map array.
  int nbuckets;             //!< Number of slots in array.
  int size;                 //!< Number of elements in hash map.
  //! Comparison function.
  int (*cmpfunc) (const void *, const void *);
  //! Hashing function.
  unsigned long (*hashfunc) (const void *);
};

//! \brief Data structure to iterate all values associated with a given key in the map.
//!
struct multimap_value_iterator
{
    void **value_list;      //!< Copy of mapnode value_list at initialization.
    int value_size;         //!< Number of elements in value_list.
    int iterator_index;     //!< Iterators current index into value_list.
};

//! \brief Create a new hash map list node.
//!
//!  \param[in] key Key associated with element.
//!  \param[in] value Value of element.
//!  \return A new list node, or NULL if allocation was unsuccessful.
//!
static struct mapnode *
new_node (void *key, void *value)
{
  struct mapnode *node;
  
  node = malloc (sizeof (struct mapnode));
  if (node == NULL)
    goto cleanup;
  
  node->value_capacity = INIT_VALUE_LIST_CAPACITY;
  node->value_size = 1;
  node->key = key;
  node->next = NULL;

  node->value_list = calloc (node->value_capacity,  sizeof (void *));
  if (node->value_list == NULL)
      goto cleanup;
  node->value_list[0] = value;
  
  return node;

cleanup:
  if (node && node->value_list)
      free (node->value_list);
  if (node)
      free (node);
  return NULL;;
}

//! \brief Delete hash map array of chained lists.
//!
//!  \param[in,out] buckets Array to delete.
//!  \param[in] nbuckets Size of array.
//!  \param[in] free_key Destroy-function called on every key, or NULL.
//!  \param[in] free_val Destroy-function called on every value, or NULL.
//!
static void
delete_buckets (struct mapnode **buckets, int nbuckets,
                void (*free_key) (void *), void (*free_val) (void *))
{
  int i;
  int j;
  struct mapnode *node, *tmp;
  
  for (i = 0; i < nbuckets; i++)
  {
    node = buckets[i];
    while (node)
    {
      tmp = node->next;
     
      for (j = 0; j < node->value_size; j++)
      {
        if (free_val)
          free_val (node->value_list[j]);
      }

      if (free_key)
        free_key (node->key);
     
      free (node->value_list);
      free (node);
      node = tmp;
    }
  }
  
  free (buckets);
}

//! \brief Resize hash map by doubling the number of slots in its array.
//!
//!  \param[in] map Hash map object.
//!
static void
resize_map (struct multimap *map)
{
  int j;
  int idx, old_nbuckets;
  struct mapnode *node, **old_buckets;
  
  // Save old values
  old_buckets = map->buckets;
  old_nbuckets = map->nbuckets;
  
  // Initialize a new, empty hashmap with twice the number of buckets!
  map->nbuckets <<= 1;
  map->buckets = calloc (map->nbuckets, sizeof (struct mapnode *));
  if (!map->buckets)
  {
    // Restore old hashmap
    map->buckets = old_buckets;
    map->nbuckets = old_nbuckets;
    return;
  }
  
  map->size = 0;
  
  // Populate new hashmap with old (key, value) tuples
  for (idx = 0; idx < old_nbuckets; idx++)
  {
    for (node = old_buckets[idx]; node; node = node->next)
    {
      for (j = 0; j < node->value_size; j++)
      {
        multimap_push_value (map, node->key, node->value_list[j]);
      }
    }
  }
  
  // Finally, delete old buckets
  delete_buckets (old_buckets, old_nbuckets, NULL, NULL);
}

struct multimap *
multimap_create (int (*cmpfunc) (const void *, const void *),
            unsigned long (*hashfunc) (const void *))
{
  struct multimap *map;
  
  map = calloc (1, sizeof (struct multimap));
  if (!map)
    goto error_alloc;
  
  map->cmpfunc = cmpfunc;
  map->hashfunc = hashfunc;
  map->nbuckets = INIT_NUM_BUCKETS;
  
  map->buckets = calloc (map->nbuckets, sizeof (struct mapnode *));
  if (!map->buckets)
    goto error_buckets;
  
  return map;
  
error_buckets:
  free (map);
error_alloc:
  return NULL;
}

void
multimap_destroy (struct multimap *map, void (*free_key) (void *), void (*free_val) (void *))
{
  if (map == NULL)
      return;

  delete_buckets (map->buckets, map->nbuckets, free_key, free_val);
  free (map);
}

int
multimap_push_value (struct multimap *map, void *key, void *value)
{
  unsigned long hash, idx;
  struct mapnode **node;
  void *tmp;
  int i;
  
  hash = map->hashfunc (key);
  idx = hash % map->nbuckets;
  node = &map->buckets[idx];
  
  while (*node && map->cmpfunc (key, (*node)->key))
    node = &(*node)->next;
    
  if (*node)
  { 
    // Check if value already exists
    for (i = 0; i < (*node)->value_size; i++)
    {
        if ((*node)->value_list[i] == value)
            return (-1);
    }

    // Key already exists - append to value_lists
    if ((*node)->value_size == (*node)->value_capacity)
    {
      // Resize value_list - list full
      tmp = realloc ((*node)->value_list, ((*node)->value_capacity * 2) * sizeof (void *));
      if (tmp == NULL)
      {
        return -ENOMEM;
      }
      (*node)->value_list = tmp;
      (*node)->value_capacity *= 2;
    }

    (*node)->value_list[(*node)->value_size] = value;
    (*node)->value_size++;
    map->size++; /* Increase size */
    return 0;
  }
  
  // Create a new node 
  *node = new_node (key, value);
  if (!(*node))
    return -ENOMEM;
  
  map->size++; /* Increase size */
  
  // And resize hash map if we have many elements
  if (map->size >= map->nbuckets / 2)
    resize_map (map);
  
  return 0;
}

void *
multimap_get_first (struct multimap *map, void *key)
{
  unsigned long hash, idx;
  struct mapnode **node;

  if (map == NULL)
      return NULL;
  
  hash = map->hashfunc (key);
  idx = hash % map->nbuckets;
  node = &map->buckets[idx];
  
  while (*node && map->cmpfunc (key, (*node)->key))
    node = &(*node)->next;
  
  if (!(*node))
    return NULL;
  
  return (*node)->value_list[0];
}

struct multimap_value_iterator *
multimap_fetch (struct multimap *map, void *key)
{
  unsigned long hash, idx;
  struct mapnode **node;
  struct multimap_value_iterator *iter;

  if (map == NULL)
      return NULL;
  
  hash = map->hashfunc (key);
  idx = hash % map->nbuckets;
  node = &map->buckets[idx];
  
  while (*node && map->cmpfunc (key, (*node)->key))
    node = &(*node)->next;

  if ((*node) == NULL)
      return NULL;

  iter = calloc (1, sizeof (struct multimap_value_iterator));
  if (iter == NULL)
      return NULL;
  iter->value_list = calloc ((*node)->value_size, sizeof (void *));
  if (iter->value_list == NULL)
  {
      free(iter);
      return NULL;
  }

  iter->value_list = memcpy (iter->value_list, (*node)->value_list, (*node)->value_size * sizeof (void *));
  iter->value_size = (*node)->value_size;
  iter->iterator_index = 0;

  return iter;
}

void *
multimap_remove_value(struct multimap *map, void *key, void (*free_key) (void *), void *value)
{
  int i, k;
  void *map_value;
  unsigned long hash, idx;
  struct mapnode *tmp, **node;
  
  hash = map->hashfunc (key);
  idx = hash % map->nbuckets;
  node = &map->buckets[idx];
  
  while (*node && map->cmpfunc (key, (*node)->key))
    node = &(*node)->next;
  
  if ((*node)  == NULL)
    return NULL;

  for (i = 0; i < (*node)->value_size; i++)
  {
    if ((*node)->value_list[i] == value)
    {
      map_value = value; 

      break;
    }
  }

  if (map_value != NULL)
  {
    for ( k = i + 1; k < (*node)->value_size; k++, i++)
    {
      (*node)->value_list[i] = (*node)->value_list[k]; 
    }
    (*node)->value_size -= 1;
  }
  else
  {
    // Found no entries
    return NULL;
  }


  if ((*node)->value_size == 0)
  {
    if (free_key)
      free_key ((*node)->key);

    tmp = (*node)->next;
    free ((*node)->value_list);
    free (*node);
    *node = tmp;
  }

  map->size--;
  
  return map_value;
}

int
multimap_size (struct multimap *map)
{
  return map->size;
}

int
multimap_contains_key (struct multimap *map, void *key)
{
  unsigned long hash, idx;
  struct mapnode **node;
  
  hash = map->hashfunc (key);
  idx = hash % map->nbuckets;
  node = &map->buckets[idx];
  
  while (*node && map->cmpfunc (key, (*node)->key))
    node = &(*node)->next;
  
  return (*node) ? 1 : 0;
}

void *
multimap_iterator_next (struct multimap_value_iterator *iter)
{
  void *value;

  if (iter == NULL)
      return NULL;

  /* Exhausted */
  if (iter->iterator_index == iter->value_size)
      return NULL;

  value = iter->value_list[iter->iterator_index];
  iter->iterator_index++;

  return value;
}

int
multimap_iterator_destroy (struct multimap_value_iterator *iter)
{
    if (iter == NULL)
        return (-1);

    free(iter->value_list);
    free(iter);

    return (0);
}

