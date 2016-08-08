// ListTest3.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdlib.h>

struct slist_entry
{
	struct slist_entry * next;
};

struct slist_entry * head;

void init_slist_entry(struct slist_entry * head)
{
	head->next = head;
}

void add_slist_entry(struct slist_entry * entry)
{
	struct slist_entry * walk;

	for (walk = head->next; head != walk->next; walk = walk->next);

	walk->next = entry;
	entry->next = head;
}

struct slist_entry * remove_slist_entry(struct slist_entry * entry)
{
	struct slist_entry * target = NULL;
	struct slist_entry * walk = head;

	while (entry != walk)
	{
		target = walk;
		walk = walk->next;
	}

	if (!target)
	{
		head = entry->next;
		walk->next = head;
	}
	else
	{
		walk->next = target->next;
	}
	
	return target;
}

int main(int argc, char* argv[])
{
	struct slist_entry * entry0 = (struct slist_entry *)malloc(sizeof(struct slist_entry));
	struct slist_entry * entry1 = (struct slist_entry *)malloc(sizeof(struct slist_entry));
	struct slist_entry * entry2 = (struct slist_entry *)malloc(sizeof(struct slist_entry));
	struct slist_entry * entry3 = (struct slist_entry *)malloc(sizeof(struct slist_entry));

	head = entry0;
	printf("head(0x%p)\nentry0(0x%p) entry1(0x%p) entry2(0x%p) entry3(0x%p)\n", head, entry0, entry1, entry2, entry3);

	init_slist_entry(head);

	add_slist_entry(entry1);
	add_slist_entry(entry2);
	add_slist_entry(entry3);

	struct slist_entry * iter;
	for (iter = head; iter->next != head; iter = iter->next)
	{
		printf("iter(0x%p)\n", iter);
	}
	printf("iter(0x%p)\n\n\n", iter);

	struct slist_entry * re = remove_slist_entry(entry2);
	iter = head;
	do
	{
		printf("iter(0x%p)\n", iter);
		iter = iter->next;
	} while (iter != head);

	printf("head(0x%p)\n", head);
	free(re);

	return 0;
}

