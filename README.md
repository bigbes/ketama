# Ketama plain C implementation

Originally [libketama](https://github.com/RJ/ketama) was written by RJ (Richard
Jones) that worked in Last.FM. This code is not maintained and have a lot of
features, that's not needed (for example, storing in shared memory) and
can only be loaded from file.

This implementation can:

1. Load database from file. Use `ketama_servers_read_file` function.
2. Load database from string. Use `ketama_servers_read_string` function.
3. Construct it from your code and manipulate it:
   * `ketama_srv_list_new` - create `ketama_srv_list` structure
   * `ketama_srv_list_append` - append `ketama_srv_info` to server list structure
   * `ketama_srv_list_find` - find id of server (in server list) for deletion
   * `ketama_srv_list_delete` - delete server with id from list
   * `ketama_srv_list_free` - free previously allocated `ketama_srv_list` structure

Currently supported only md5 function. Plan to test with murmur3 and fnv hashes.

Example code is located in `t/ketama_test.c` file.
