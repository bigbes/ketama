/*
 * Using a known ketama.servers file, and a fixed set of keys
 * print and hash the output of this program using your modified
 * libketama, compare the hash of the output to the known correct
 * hash in the test harness.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ketama.h"

void usage(int argc, char *cmd) {
	if (argc != 2) {
		printf("Usage: %s <ketama.servers file>\n", cmd);
		exit(-1);
	}
}

int main(int argc, char **argv) {
	usage(argc, *argv);
	uint32_t servers_len = 0;
	struct ketama_srv_list *srv_list = NULL;
	struct ketama_continuum *continuum = NULL;
	srv_list = ketama_servers_read_file(argv[1], &servers_len);
	assert(srv_list != NULL);
	continuum = ketama_continuum_new(srv_list);
	assert(continuum != NULL);
	ketama_srv_list_free(srv_list);

	for (int i = 0; i < 1000000; ++i) {
		char key[10];
		sprintf(key, "%d", i);
		unsigned int hash = ketama_hash(key, strlen(key));
		const struct ketama_point *point = NULL;
		point = ketama_get_point(continuum, key);
		const struct ketama_srv_info *server = NULL;
		server = ketama_get_server(continuum, key);

		printf( "%u %u %s\n", hash, point->value, server->addr);
	}

	ketama_continuum_free(continuum);
	return 0;
}
