/* HASHING TABLE */

#ifndef __HASH_H
#define __HASH_H

#define TABLE_SIZE 32768    /* size of the hash table (must be power of 2) */


struct table_type{                    /* HASH TABLE */
unsigned char count;
struct object_list **pointer;
};

extern struct table_type table[TABLE_SIZE];


/* hash function */
unsigned int hash(unsigned int id);


/* adds object to hash table */
void add_to_table(struct object_list *pointer);


/* removes object from table */
/* returns pointer to the object */
/* if there's not such an object returns null */
struct object_list * remove_from_table(unsigned int id);



/* tests if object is in table */
/* returns pointer to the object, if no returns 0 */
struct object_list *find_in_table(unsigned int id);


/* initializes hash table */
void hash_table_init(void);


/* removes hash table from memory */
void free_hash_table(void);


#endif
