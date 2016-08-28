#ifndef   _KETAMA_H_
#define   _KETAMA_H_

/**
 * XXX.XXX.XXX.XXX:YYYYY ==
 *   (4 * 3 + 3)[ip] + 1[delimiter] + 5[port] == 21 bytes max
 */

#define MAX_HOST_LEN 22

/* previously 'mcs' */
struct ketama_point {
	uint32_t value;
	uint32_t srv_id;
};

/* previously 'serverinfo' */
struct ketama_srv_info {
	char     addr[MAX_HOST_LEN];
	uint64_t memory;
};

struct ketama_srv_list {
	struct ketama_srv_info *servers;
	uint32_t                count;
	uint32_t                allocated;
};

/* previously 'continuum' */
struct ketama_continuum {
	struct ketama_srv_list  srv_list;
	struct ketama_point    *points;
	uint64_t                points_len;
};

struct ketama_continuum *
ketama_continuum_new(struct ketama_srv_list *srv_list);

/**
 * \brief Frees any allocated memory.
 * \param contptr The continuum that you want to be destroy. */
void ketama_continuum_free(struct ketama_continuum *cont);

/**
 * \brief Maps a key onto a server in the continuum.
 * \param key The key that you want to map to a specific server.
 * \param cont Pointer to the continuum in which we will search.
 * \return The mcs struct that the given key maps to. */
const struct ketama_srv_info *ketama_get_server(struct ketama_continuum *cont, char *key);

/**
 * \brief Print the server list of a continuum to stdout.
 * \param cont The continuum to print. */
void ketama_continuum_print(struct ketama_continuum *cont);

struct ketama_srv_list *
ketama_servers_read_file(const char *filename, uint32_t *count);

struct ketama_srv_list *
ketama_servers_read_string(const char *input, uint32_t *count);

/*vvvvv For debugging purposes */
int
ketama_hash(const char *input, size_t input_len);

const struct ketama_point *
ketama_get_point(struct ketama_continuum *cont, char *key);
/*^^^^^ For debugging purposes */

void
ketama_srv_list_print(struct ketama_srv_list *srv_list);

void
ketama_srv_list_free(struct ketama_srv_list *srv_list);

struct ketama_srv_list *
ketama_srv_list_new(uint32_t count);

int
ketama_srv_list_append(struct ketama_srv_list *srv_list,
		       struct ketama_srv_info *srv);

ssize_t
ketama_srv_list_find(struct ketama_srv_list *srv_list, const char *ip);

uint64_t
ketama_srv_list_memcount(struct ketama_srv_list *srv_list);

void
ketama_srv_list_delete(struct ketama_srv_list *srv_list,
		       uint32_t position);
#endif /* _KETAMA_H_ */
