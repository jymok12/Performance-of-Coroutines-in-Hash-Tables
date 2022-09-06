#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <experimental/coroutine>

#define CAPACITY 100000

unsigned long hash_function(int str);

typedef struct Data Data;
typedef struct Node Node;
typedef struct HashTable HashTable;

// Define the Hash Table Item here
struct Data
{
  int key;
  int value;
};

// Define the Linkedlist here
struct Node
{
  Data *data;
  Node *next;
};

// Define the Hash Table here
struct HashTable
{
  // Contains an array of pointers
  // to items
  Node **nodes;
  int size;
  int count;
};

static Node *allocate_node();
static Node *node_insert(Node *list, Data *item);

static void free_linkedlist(Node *list);

static Node **create_overflow_buckets(HashTable *table);
static void free_buckets(HashTable *table);

Data *create_item(int key, int value);
HashTable *create_table(int size);

void free_table(HashTable *table);
void handle_collision(HashTable *table, unsigned long index, Data *item);
void ht_insert(HashTable *table, int key, int value);

int ht_search(HashTable *table, int key);

int *HASH_PROBE(int *input, int n, HashTable *table);

struct GP_state
{
  Node *node;
};

int *HASH_PROBE_GP(int input[], int n, HashTable *table);

struct AMAC_state
{
  int key;
  Node *node;
  int value;
  int stage;
};

struct AMAC_circular_buffer
{
  int group_size;
  int next = 0;
  AMAC_state *stateArr;

  AMAC_circular_buffer(int n)
  {
    group_size = n;
    stateArr = new AMAC_state[n];
    for (int i; i < group_size; i++)
    {
      stateArr[i].stage = 0;
    }
  }

  AMAC_state *next_state()
  {
    int curr_next = next;
    next = (next + 1) % group_size;
    return &stateArr[curr_next];
  }
};

int *HASH_PROBE_AMAC(int *input, int n, HashTable *table, uint group_size);

struct ReturnObject {
  struct promise_type {
    unsigned val_;

    ~promise_type() {}
    ReturnObject get_return_object() {
      return {
        .h_ = std::experimental::coroutine_handle<promise_type>::from_promise(*this)
      };
    }
    std::experimental::suspend_never initial_suspend() { return {}; }
    std::experimental::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    void return_value(int val) { val_ = val; }
  };

  std::experimental::coroutine_handle<promise_type> h_;
};

ReturnObject HASH_PROBE_CORO(HashTable *table, int key);

Data *ht_get(HashTable *table, int key);

void ht_delete(HashTable *table, int key);
void print_table(HashTable *table);