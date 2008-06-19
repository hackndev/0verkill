/* HASHING TABLE */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "hash.h"
#include "data.h"
#include "error.h"

struct table_type table[TABLE_SIZE];


/* hash function */

#define hash(id) (id&(TABLE_SIZE-1))


/* adds item to hash table */
void add_to_table(struct object_list *pointer)
{
	int c;
	unsigned int a;
	a=hash(pointer->member.id);
	c=table[a].count;
	table[a].pointer=mem_realloc(table[a].pointer,(c+1)*sizeof(struct object *));
	if (!table[a].pointer){ERROR("Not enough memory.\n");EXIT(1);}

	table[a].pointer[c]=pointer;
	table[a].count++;
}


/* removes object from table */
/* returns pointer to the object */
/* if there's not such an object returns null */
struct object_list * remove_from_table(unsigned int id)
{
	unsigned int a;
	int b;
	struct object_list *p;

	a=hash(id);
	if (!table[a].count)return 0;
	for (b=0;b<table[a].count;b++)
		if (table[a].pointer[b]->member.id==id)
		{
			p=table[a].pointer[b];
			memmove(table[a].pointer+b,table[a].pointer+b+1,(table[a].count-b-1)*sizeof(struct object_list *));
			table[a].count--;
			table[a].pointer=mem_realloc(table[a].pointer,table[a].count*sizeof(struct object_list*));
			return p;
		}
	return 0;
}


/* tests if object number id is in table */
/* if true returns pointer, otherwise returns null */
struct object_list* find_in_table(unsigned int id)
{
	unsigned int a;
	int b;

	a=hash(id);
	if (!table[a].count)return 0;
	for (b=0;b<table[a].count;b++)
		if (table[a].pointer[b]->member.id==id) return table[a].pointer[b];
	return 0;
}


/* initializes hash table */
void hash_table_init(void)
{
	int a;

	for (a=0;a<TABLE_SIZE;a++)
	{
		table[a].count=0;
		table[a].pointer=DUMMY;
	}
}


/* removes hash table from memory */
void free_hash_table(void)
{
	int a,b;

	for (a=0;a<TABLE_SIZE;a++)
	{
		for (b=0;b<table[a].count;b++)
			mem_free(table[a].pointer[b]);
		mem_free(table[a].pointer);
	}
}
