#include<stdio.h>
#include<hash_ring.h>
#include<string.h>
#include<stdlib.h>
#include<assert.h>
int main(){
	hash_ring_t *ring = hash_ring_create(8, HASH_FUNCTION_SHA1);
	char *slot = malloc(10);
	char *keyA = "keyA";

	hash_ring_node_t *node;
	sprintf(slot, "%d", 12313);
	assert(hash_ring_add_node(ring, (uint8_t*)slot, strlen(slot)) == HASH_RING_OK);
	free(slot);
	
	node = hash_ring_find_node(ring, (uint8_t*)keyA, strlen(keyA));
	assert(node != NULL && node->nameLen == strlen(slot) && memcmp(node->name, slot, strlen(slot)) == 0);
}
