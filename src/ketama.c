#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <openssl/md5.h>

#include "ketama.h"

/***** For internal use */

/* Check if line is empty or commentary */
static bool is_commentary(const char *input) {
	size_t pos = 0;
	while (input[pos] != '\n') {
		if (isspace(input[pos])) {
			pos++;
			continue;
		}
		if (input[pos] == '#')
			return true;
		return false;
	}
	return true;
}

/* Skip line */
static const char *skip_line(const char *input) {
	while (*input != '\n' || *input != '\0') input++;
	if (*input == '\0')
		return NULL; /* return NULL in case we found end of string */
	return input + 1; /* return position, otherwise */
}

/**
 * Parse input string from format 'IP:PORT MEMORY\n\0' into
 * 'struct ketama_srv_info'
 * Full verification of input data.
 * Returns -1 if error, while parsing input file
 */
static const char *
read_server_line(const char *input, bool is_file,
		 struct ketama_srv_info *server)
{
	/* Ignoring trailing whitespaces */
	while (isspace(*input))
		input++;

	/* Find where every part is located */
	const char *ip_begin = NULL,     *ip_end = NULL;
	const char *port_begin = NULL,   *port_end = NULL;
	const char *memory_begin = NULL, *memory_end = NULL;
	char *memory_strtoull_end = NULL;

	ip_begin = input;
	/* finding end of ip */
	while (*input != ':') {
		if (*input == '\0')
			goto error; /* premature EOL */
		if (isspace(*input))
			goto error; /* port not found */
		if (!isdigit(*input) && *input != '.')
			goto error; /* bad charachters in ip */
		input++;
	}
	ip_end = input;
	if (ip_end - ip_begin > 15)
		goto error; /* extensive length of ip */

	port_begin = ++input;
	/* finding end of port */
	while (!isspace(*input)) {
		if (*input == '\0')
			goto error; /* premature EOL*/
		if (!isdigit(*input))
			goto error; /* bad charachters in port */
		input++;
	}
	port_end = input;
	if (port_end - port_begin > 5)
		goto error; /* extensive length of port */

	/* skipping whitespaces between port and memory */
	while (isspace(*input)) input++;
	memory_begin = input;
	/* finding memory part */
	while (isdigit(*input)) input++;
	memory_end = input;
	/* skipping whitespaces between memory and '\0' */
	while (isspace(*input)) input++;
	/* if we're reading file, then verify that it's end of line */
	if (is_file && *input != '\0')
		goto error; /* trailing charachters */

	size_t ip_port_len = port_end - ip_begin;
	memcpy(server->addr, ip_begin, ip_port_len);
	server->addr[ip_port_len] = '\0';
	server->memory = strtoull(memory_begin, &memory_strtoull_end, 10);
	if ((server->memory == ULLONG_MAX && errno == ERANGE) ||
	    (memory_strtoull_end != memory_end))
		goto error;

	return input;
error:
	memset(server, 0, sizeof(struct ketama_srv_info));
	return NULL;
}

static int
compare_points(const void *left, const void *right) {
	const struct ketama_point *l = (struct ketama_point *)left;
	const struct ketama_point *r = (struct ketama_point *)right;
	if (l->value < r->value) {
		return -1;
	} else if (l->value > r->value) {
		return 1;
	}
	return 0;
}

static uint64_t
count_memory(struct ketama_srv_info *servers, uint32_t count) {
	uint64_t memory = 0;
	for (int i = 0; i < count; ++i)
		memory += servers[i].memory;
	return memory;
}

/***** Opened for debugging */

int
ketama_hash(const char *input, size_t input_len) {
	unsigned char hash_output[MD5_DIGEST_LENGTH] = {0};
	MD5((unsigned char *)input, input_len, hash_output);
	return (hash_output[3] << 24) | (hash_output[2] << 16) |
	       (hash_output[1] <<  8) | (hash_output[0] <<  0);
}

const struct ketama_point *
ketama_get_point(struct ketama_continuum *cont, char *key) {
	uint32_t hash = ketama_hash(key, strlen(key));
	struct ketama_point *points = cont->points;
	int left = 0, right = cont->points_len, mid;
	uint32_t mid_value = 0, mid_value_prev = 0;

	/* Binary search of nearest max element */
	while (true) {
		mid = (int )((left + right) / 2);
		if (left > right || mid == cont->points_len) {
			return &points[0];
		}

		mid_value  = points[mid].value;
		mid_value_prev = (mid == 0 ? 0 : points[mid - 1].value);

		if (hash < mid_value && hash > mid_value_prev) {
			return &points[mid];
		}

		if (mid_value < hash) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	/* unreachable */
	assert(false);
	return NULL;
}

/***** Public API */

/*** Server list API */
void
ketama_srv_list_print(struct ketama_srv_list *srv_list) {
	if (srv_list->count == 0 || srv_list->servers == NULL) {
		printf("Server List is empty");
	}
	for (uint32_t i = 0; i < srv_list->count; ++i) {
		struct ketama_srv_info *srv = &srv_list->servers[i];
		printf("Server %s, Memory %llu\n", srv->addr, srv->memory);
	}
}

void
ketama_srv_list_free(struct ketama_srv_list *srv_list) {
	if (srv_list) {
		if (srv_list->servers)
			free(srv_list->servers);
		free(srv_list);
	}
}

struct ketama_srv_list *
ketama_srv_list_new(uint32_t count) {
	struct ketama_srv_list *srv_list = NULL;
	srv_list = calloc(1, sizeof(struct ketama_srv_list));
	if (srv_list == NULL)
		goto error;
	if (count > 0) {
		srv_list->servers = calloc(count, sizeof(struct ketama_srv_info));
		if (srv_list->servers == NULL)
			goto error;
		srv_list->allocated = count;
	}
	return srv_list;
error:
	ketama_srv_list_free(srv_list);
	return NULL;
}

int
ketama_srv_list_append(struct ketama_srv_list *srv_list,
		       struct ketama_srv_info *srv) {
	if (srv_list->allocated == srv_list->count) {
		struct ketama_srv_info *tmp = NULL;
		tmp = (struct ketama_srv_info *)calloc(srv_list->allocated + 1,
				sizeof(struct ketama_srv_info));
		if (tmp == NULL)
			goto error; /* Not enough memory, bailing out */
		if (srv_list->allocated != 0) {
			memcpy(tmp, srv_list->servers, srv_list->allocated *
					sizeof(struct ketama_srv_info));
			free(srv_list->servers);
		}
		srv_list->servers = tmp;
		srv_list->allocated += 1;
	}
	memcpy(&(srv_list->servers[srv_list->count]), srv,
	       sizeof(struct ketama_srv_info));
	srv_list->count += 1;
	return 0;
error:
	return -1;
}

ssize_t
ketama_srv_list_find(struct ketama_srv_list *srv_list, const char *ip) {
	for (uint32_t i = 0; i < srv_list->count; ++i) {
		if (strncmp(srv_list->servers[i].addr, ip, 22) == 0) {
			return i;
		}
	}
	return -1;
}

uint64_t
ketama_srv_list_memcount(struct ketama_srv_list *srv_list) {
	uint64_t memory = 0;
	for (int i = 0; i < srv_list->count; ++i)
		memory += srv_list->servers[i].memory;
	return memory;
}

void
ketama_srv_list_delete(struct ketama_srv_list *srv_list,
		       uint32_t position) {
	if (position > srv_list->count)
		return;
	memmove(&(srv_list->servers[position]),
		&(srv_list->servers[position + 1]),
		sizeof(struct ketama_srv_info) * (srv_list->count - position));
	srv_list->count -= 1;
	return;
}

struct ketama_srv_list *
ketama_servers_read_string(const char *input, uint32_t *count)
{
	struct ketama_srv_list *srv_list = NULL;
	srv_list = ketama_srv_list_new(0);
	if (srv_list == NULL)
		goto error; /* error, OOM */
	uint32_t lineno = 0, servers_len = 0;

	while (*input != '\0') {
		lineno++;

		if (is_commentary(input)) {
			if ((input = skip_line(input)) == NULL)
				break; /* we've found end of string */
			continue;
		}
		struct ketama_srv_info server = {0};
		if ((input = read_server_line(input, false, &server)) == NULL)
			goto error; /* error, while parsing file */
		if (ketama_srv_list_append(srv_list, &server) == -1)
			goto error; /* error, OOM */
	}

	*count = servers_len;
	return srv_list;
error:
	*count  = 0;
	ketama_srv_list_free(srv_list);
	return NULL;
}

struct ketama_srv_list *
ketama_servers_read_file(const char *filename, uint32_t *count)
{
	struct ketama_srv_list *srv_list = NULL;
	srv_list = ketama_srv_list_new(0);
	if (srv_list == NULL)
		goto error; /* error, OOM */
	uint32_t lineno = 0, servers_len = 0;
	char input[128] = {0};

	FILE *fh = fopen(filename, "r");
	assert(fh != NULL);
	if (fh == NULL) {
		goto error;
	}
	while (!feof(fh)) {
		if (fgets(input, 127, fh) == NULL)
			continue;
		size_t input_len = strlen(input);

		lineno++;

		if (is_commentary(input))
			continue;
		struct ketama_srv_info server = {0};
		if (read_server_line(input, true, &server) == NULL) {
			goto error; /* error, while parsing file */
		}
		if (ketama_srv_list_append(srv_list, &server) == -1)
			goto error; /* error, OOM */
	}
	fclose(fh);

	*count = servers_len;
	return srv_list;
error:
	*count  = 0;
	if (fh) fclose(fh);
	ketama_srv_list_free(srv_list);
	return NULL;
}

struct ketama_continuum *
ketama_continuum_new(struct ketama_srv_list *srv_list) {
	uint32_t servers_len = srv_list->count;
	struct ketama_continuum *cont = malloc(sizeof(struct ketama_continuum));
	if (cont == NULL)
		goto error;
	memset(cont, 0, sizeof(struct ketama_continuum));
	uint64_t memory = ketama_srv_list_memcount(srv_list);

	/* Initializing server part of cont */
	memcpy(&(cont->srv_list), srv_list, sizeof(struct ketama_srv_list));
	cont->srv_list.servers = calloc(servers_len,
			sizeof(struct ketama_srv_info));
	if (cont->srv_list.servers == NULL)
		goto error;
	memcpy(cont->srv_list.servers, srv_list->servers,
	       servers_len * sizeof(struct ketama_srv_info));

	/* Initializing points part of cont */
	/* 40 hashes * 4 numbers per hash == 160 points per server */
	cont->points_len = servers_len * 40 * 4;
	cont->points = calloc(cont->points_len, sizeof(struct ketama_point));
	if (cont->points == NULL)
		goto error;


	/* position - is current point position */
	uint32_t position = 0;

	/**
	 * We'll make 3 cycles:
	 * 1) iterate between all server.
	 * 2) iterate between all points for this server.
	 *    accordingly to number of available memory.
	 *    count is `floor((mem/overall_mem) * 40 * server_count)`
	 * 3) make 4 iteration for every hash.
	 *    hash is `md5(sprintf('%s-%d', server, point_number))`
	 *
	 * Overall sum is lesser, than server_count * 160, since floor always
	 * will be lesser.
	 */
	for (int srv_no = 0; srv_no < servers_len; ++srv_no) {
		const struct ketama_srv_info *srv = NULL;
		srv = &(cont->srv_list.servers[srv_no]);
		float percentage = (float )srv->memory / (float )memory;
		uint32_t srv_point_count = floorf(percentage * 40.0 *
						  (float )servers_len);

		for (int point = 0; point < srv_point_count; ++point) {
			unsigned char hash_input[30] = {0};
			unsigned char hash_output[MD5_DIGEST_LENGTH] = {0};
			size_t hash_input_len = 0;
			hash_input_len = snprintf((char *)hash_input, 30,
						  "%s-%d", srv->addr, point);
			MD5(hash_input, hash_input_len, hash_output);

			for (int blk = 0; blk < 4; blk++) {
				uint32_t point_value = 0;
				struct ketama_point *current_point = NULL;
				current_point = &cont->points[position++];
				point_value = (hash_output[3 + blk * 4] << 24) |
					      (hash_output[2 + blk * 4] << 16) |
					      (hash_output[1 + blk * 4] << 8 ) |
					      (hash_output[0 + blk * 4] << 0 ) ;
				current_point->value = point_value;
				current_point->srv_id = srv_no;
			}
		}
		assert(position <= cont->points_len);
	}
	cont->points_len = position;

	/* Sort points in ascending order */
	qsort((void *)cont->points, position, sizeof(struct ketama_point),
	      compare_points);

	return cont;
error:
	ketama_continuum_free(cont);
	return NULL;
}

const struct ketama_srv_info *
ketama_get_server(struct ketama_continuum *cont, char *key) {
	const struct ketama_point *point = ketama_get_point(cont, key);
	if (point == NULL) {
		assert(false);
		return NULL;
	}
	return &cont->srv_list.servers[point->srv_id];
}

void
ketama_continuum_free(struct ketama_continuum *cont) {
	if (cont) {
		if (cont->srv_list.servers)
			free(cont->srv_list.servers);
		if (cont->points)
			free(cont->points);
		free(cont);
	}
}

void
ketama_continuum_print(struct ketama_continuum *cont)
{
	if (cont->points == NULL || cont->points_len == 0) {
		printf("Continuum is empty\n");
		return;
	}
	printf("Numpoints in continuum: %llu\n", cont->points_len);
	for (int pos = 0; pos < cont->points_len; ++pos) {
		const struct ketama_point *point = &cont->points[pos];
		printf("%s (%u)\n", cont->srv_list.servers[point->srv_id].addr,
		       point->value);
	}
}
