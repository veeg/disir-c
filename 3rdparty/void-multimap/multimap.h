#ifndef MULTIMAP_H
#define MULTIMAP_H

//! \struct hashmap
//! \brief ADT structure to encapsulate a hashmap instance.
//!
struct multimap;

//! \struct multimap_value_iterator
//! \brief Iterator over different values belonging to the same key.
//!
struct multimap_value_iterator;

//! \brief Create a new, empty hashmap using the given comparison and hashing functions.
//!
//! \param[in] cmpfunc Comparison function used to determine size relation of a pair of keys.
//! \param[in] hashfunc Hashing function used to map key into domain of long integers.
//! \return A new hashmap object, or NULL if allocation was unsuccessful.
//!
struct multimap *
multimap_create (int (*cmpfunc) (const void *, const void *),
        unsigned long (*hashfunc) (const void *));

//! \brief Destroy the given hashmap.
//!
//! \param[in] map Hash map object.
//! \param[in] destroy_key Destroy-function to be called on every key, or NULL.
//! \param[in] destroy_val Destroy-function to be called on every value, or NULL.
//! \return void
//!
void
multimap_destroy (struct multimap *map, 
                  void (*destroy_key) (void *), 
                  void (*destroy_val) (void *));

//! \brief Associate the given value with the given key.
//!
//! The multimap accepts multiple unique values per key.
//! The values are stored in sorted order when inputted to the multimap.
//!
//! \param[in] map Hash map object.
//! \param[in] key Key to be associated with given value.
//! \param[in] val Value that given key will be associated with.
//! \return 0 if mapping was successful, else a non-zero value is returned.
//!
int
multimap_push_value (struct multimap *map, void *key, void *val);

//! \brief Get the first value associated with the given key.
//!
//! Retrieve the fist value pushed to the map with the given key.
//!
//! \param[in] map Hash map object.
//! \param[in] key Key used to identify a value in the hash map.
//! \return First value that key is associated with if mapping exists, else NULL.
//!
void *
multimap_get_first (struct multimap *map, void *key);

//! \brief Get a value iterator for a given key.
//!
//! The value iterator holds all values associated with the  given key
//! in insertion order. Use `multimap_iterator_next()` to retrieve
//! a value.
//!
//! \param[in] map Hash map object.
//! \param[in] key Key used to identify a value in the hash map.
//! \return Iterator object for the given key.
//!
struct multimap_value_iterator *
multimap_fetch (struct multimap *map, void *key);

//! \brief Get the value associated with the given key, and remove it from map.
//!
//! If the given key isn't mapped with any given value, NULL is returned.
//!
//! \param[in] map Hash map object.
//! \param[in] key Key used to identify a value in the hash map.
//! \param[in] value Destroy-function that will be called on the key, or NULL.
//! \return Value that key is associated with if mapping exists, else NULL.
//!
void *
multimap_remove_value (struct multimap *map, void *key, void (*free_key)(void *), void *value);

//! \brief Get number of elements in hash map.
//!
//! \param[in] map Hash map object.
//! \return Number of elements in hash map.
//!
int
multimap_size (struct multimap *map);

//! \brief Check how many values map contains for the given key.
//!
//! \param[in] map Hash map object.
//! \param[in] key Key for which to determine if a mapping exists.
//! \return Number of values stored with this key.
//!
int
multimap_contains_key (struct multimap *map, void *key);

//! \brief Retrieve the next value associated with the iterator
//!
//! The value returned MAY be destroyed between fetching the iterator
//! and retrieving this value. 
//!
//! \param[in] iter Iterator associated with a given key
//! \return Value currently pointed to by the iterator
//!
void *
multimap_iterator_next (struct multimap_value_iterator *iter);

//! \brief Destroy the multimap value iterator.
//!
//! \code
//! res = multimap_iterator_destroy (iter);
//! if (res != 0)
//!   panic ("Destroying iterator failed!");
//! \endcode
//!
//! \param[in] iter Multimap iterator object to destroy.
//! \return 0 if function invocation was successful, -1 if not.
//!
int
multimap_iterator_destroy (struct multimap_value_iterator *iter);

#endif // MULTIMAP_H

