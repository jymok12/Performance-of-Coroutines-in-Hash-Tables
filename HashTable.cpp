#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HashTable.h"

unsigned long hash_function(int n)
{
  return n % CAPACITY;
}

static Node *allocate_node()
{
  Node *node = (Node *)malloc(sizeof(Node));
  return node;
}

static Node *node_insert(Node *list, Data *data)
{
  if (list == nullptr)
  {
    Node *head = allocate_node();
    head->data = data;
    head->next = nullptr;
    list = head;
    return list;
  }
  else if (list->next == nullptr)
  {
    Node *node = allocate_node();
    node->data = data;
    node->next = nullptr;
    list->next = node;
    return list;
  }

  Node *temp = list;
  while (temp->next->next != nullptr)
  {
    temp = temp->next;
  }

  Node *node = allocate_node();
  node->data = data;
  node->next = nullptr;
  temp->next = node;

  return list;
}

static void free_linkedlist(Node *list)
{
  Node *temp = list;
  while (list != nullptr)
  {
    temp = list;
    list = list->next;
    free(temp->data);
    free(temp);
  }
}

static void free_buckets(HashTable *table)
{
  Node **buckets = table->nodes;
  for (int i = 0; i < table->size; i++)
    free_linkedlist(buckets[i]);
  free(buckets);
}

Data *create_data(int key, int value)
{
  Data *data = (Data *)malloc(sizeof(Data));
  data->key = key;
  data->value = value;
  return data;
}

HashTable *create_table(int size)
{
  HashTable *table = (HashTable *)malloc(sizeof(HashTable));
  table->size = size;
  table->count = 0;
  table->nodes = (Node **)calloc(table->size, sizeof(Node *));
  for (int i = 0; i < table->size; i++)
    table->nodes[i] = nullptr;

  return table;
}

void free_table(HashTable *table)
{
  free_buckets(table);
  free(table);
}

void handle_collision(HashTable *table, unsigned long index, Data *data)
{
  Node *head = table->nodes[index];

  if (head == nullptr)
  {
    head = allocate_node();
    head->data = data;
    table->nodes[index] = head;
    return;
  }
  else
  {
    table->nodes[index] = node_insert(head, data);
    return;
  }
}

void ht_insert(HashTable *table, int key, int value)
{
  Data *data = create_data(key, value);
  unsigned long index = hash_function(key);
  Node *curr = table->nodes[index];

  if (curr == nullptr)
  {
    if (table->count == table->size)
    {
      free(data);
      return;
    }

    table->nodes[index] = node_insert(table->nodes[index], data);
    table->count++;
  }
  else
  {
    if (curr->data->key == key)
    {
      table->nodes[index]->data->value = value;
      return;
    }
    else
    {
      handle_collision(table, index, data);
      return;
    }
  }
}

int ht_search(HashTable *table, int key)
{
  int index = hash_function(key);
  Node *curr = table->nodes[index];

  while (curr != nullptr && curr->data != nullptr)
  {
    if (curr->data->key == key)
      return curr->data->value;
    curr = curr->next;
  }
  return -1;
}

int *HASH_PROBE(int *input, int n, HashTable *table)
{
  int *value = new int[n];
  for (int i = 0; i < n; i++)
  {
    int index = hash_function(input[i]);
    Node *curr = table->nodes[index];
    while (curr != nullptr)
    {
      if (curr->data->key == input[i])
      {
        value[i] = curr->data->value;
        break;
      }
      curr = curr->next;
    }
    if (curr == nullptr)
    {
      value[i] = -1;
    }
  }
  return value;
}

int *HASH_PROBE_GP(int *input, int n, HashTable *table)
{
  int *value = new int[n];
  GP_state *stateArr = new GP_state[n];
  for (int i = 0; i < n; i++)
  {
    stateArr[i].node = table->nodes[hash_function(input[i])];
    __builtin_prefetch(stateArr[i].node);
  }
  int num_finished = 0;
  while (num_finished < n)
  {
    for (int i = 0; i < n; i++)
    {
      if (stateArr[i].node == nullptr)
      {
        value[i] = -1;
        num_finished++;
        continue;
      }
      else if (input[i] == stateArr[i].node->data->key)
      {
        value[i] = stateArr[i].node->data->value;
        num_finished++;
      }
      else
      {
        stateArr[i].node = stateArr[i].node->next;
        __builtin_prefetch(stateArr[i].node);
      }
    };
  };

  return value;
};

int *HASH_PROBE_AMAC(int *input, int n, HashTable *table, uint group_size)
{
  int *value = new int[n];
  AMAC_circular_buffer *buff = new AMAC_circular_buffer(group_size);
  int num_finished = 0;
  int i = num_finished;
  int j = i;
  while (num_finished < n)
  {
    AMAC_state *state = buff->next_state();
    if (state->stage == 0)
    {
      state->key = input[i++];
      state->node = table->nodes[hash_function(state->key)];
      state->stage = 1;
      __builtin_prefetch(state->node);
    }
    else if (state->stage == 1)
    {
      if (state->node == nullptr)
      {
        state->value = -1;
        state->stage = -1;
        value[j++] = state->value;
        num_finished++;
      }
      else if (state->key == state->node->data->key)
      {
        state->value = state->node->data->value;
        state->stage = -1;
        value[j++] = state->value;
        num_finished++;
      }
      else
      {
        state->node = state->node->next;
        __builtin_prefetch(state->node);
      }
    }
  }

  return value;
};

ReturnObject HASH_PROBE_CORO(HashTable *table, int key)
{
  Node *node = table->nodes[hash_function(key)];
  __builtin_prefetch(node);
  co_await std::experimental::suspend_always{};
  while (node)
  {
    if (key == node->data->key)
    {
      co_return node->data->value;
    }
    else
    {
      node = node->next;
      __builtin_prefetch(node);
      co_await std::experimental::suspend_always{};
    }
  }
  co_return -1;
}

Data *ht_get(HashTable *table, int key)
{
  int index = hash_function(key);
  if (table->nodes[index])
  {
    return table->nodes[index]->data;
  }
  return nullptr;
}

void ht_delete(HashTable *table, int key)
{
  int index = hash_function(key);
  Node *head = table->nodes[index];

  Node *curr = head;
  Node *prev = nullptr;

  while (curr && curr->data)
  {
    if (curr->data->key == key)
    {
      if (prev == nullptr)
      {
        free_linkedlist(head);
        table->count--;
        table->nodes[index] = nullptr;
        return;
      }
      else
      {
        prev->next = curr->next;
        curr->next = nullptr;
        free_linkedlist(curr);
        return;
      }
    }
    curr = curr->next;
    prev = curr;
  }
  return;
}

void print_table(HashTable *table)
{
  printf("\n-------------------\n");
  for (int i = 0; i < table->size; i++)
  {
    if (table->nodes[i] != nullptr)
    {
      printf("Index:%d, Key:%d, Value:%d", i, table->nodes[i]->data->key, table->nodes[i]->data->value);
      if (table->nodes[i]->next != nullptr)
      {
        printf(" => Overflow Bucket => ");
        Node *head = table->nodes[i]->next;
        while (head)
        {
          printf("Key:%d, Value:%d ", head->data->key, head->data->value);
          head = head->next;
        }
      }
      printf("\n");
    }
  }
  printf("-------------------\n");
}