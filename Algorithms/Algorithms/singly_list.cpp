/*
# 나쁜 코드 예
remove_list_entry(entry)
{
	prev = NULL;
	walk = head;

	// Walk the list
	while (walk != entry) {
		prev = walk;
		walk = walk->next;
	}

	// Remove the entry by updating the
	// head or the previous entry

	if (!prev)
		head = entry->next;
	else
		prev->next = entry->next;
}

# 좋은 코드 예
remove_list_entry(entry)
{
	// The "indirect" pointer points to the
	// *address* of the thing we'll update

	indirect = &head;

	// Walk the list, looking for the thing that
	// points to the entry we want to remove_list_entry
	while ((*indirect) != entry)
		indirect = &(*indirect)->next;

	// .. and just remove it
	*indirect = entry->next;
}
*/
#include "stdafx.h"

#include <stdlib.h>

struct slist_entry
{
	struct slist_entry * next;
};

struct slist_entry * head;

void init_slist_entry(struct slist_entry * entry)
{
	entry->next = NULL;
}

void add_slist_entry(struct slist_entry * entry)
{
	struct slist_entry * walk;

	for (walk = head; walk->next; walk = walk->next);

	walk->next = entry;
	entry->next = NULL;
}

struct slist_entry * remove_slist_entry(struct slist_entry * entry)
{
	struct slist_entry * prev = NULL;
	struct slist_entry * walk = head;

	while (entry != walk)
	{
		prev = walk;
		walk = walk->next;
	}

	if (!prev)
	{
		head = entry->next;
	}
	else
	{
		prev->next = walk->next;
	}
	
	return walk;
}

struct slist_entry * remove_slist_entry2(struct slist_entry * entry)
{
	struct slist_entry ** indirect = &head;

	while ((*indirect) != entry)
	{
		indirect = &(*indirect)->next;
	}

	*indirect = entry->next;

	return entry;
}

int main(int argc, char* argv[])
{
	struct slist_entry * entry0 = (struct slist_entry *)malloc(sizeof(struct slist_entry));
	struct slist_entry * entry1 = (struct slist_entry *)malloc(sizeof(struct slist_entry));
	struct slist_entry * entry2 = (struct slist_entry *)malloc(sizeof(struct slist_entry));
	struct slist_entry * entry3 = (struct slist_entry *)malloc(sizeof(struct slist_entry));

	head = entry0;
	printf("head(0x%p)\nentry0(0x%p) entry1(0x%p) entry2(0x%p) entry3(0x%p)\n\n", head, entry0, entry1, entry2, entry3);

	init_slist_entry(head);

	add_slist_entry(entry1);
	add_slist_entry(entry2);
	add_slist_entry(entry3);

	struct slist_entry * iter;
	for (iter = head; iter->next; iter = iter->next)
	{
		printf("iter(0x%p)\n", iter);
	}
	printf("iter(0x%p)\n\n", iter);

	struct slist_entry * removed = remove_slist_entry2(entry2);
	iter = head;
	do
	{
		printf("iter(0x%p)\n", iter);
		iter = iter->next;
	} while (iter);

	printf("\nhead(0x%p)\n", head);
	printf("removed(0x%p)\n", removed);
	free(removed);

	return 0;
}

