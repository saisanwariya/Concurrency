#include <stdio.h>
#include "linked_list.h"

//Linkedlist Refference: https://www.geeksforgeeks.org/applications-advantages-and-disadvantages-of-linked-list/?ref=lbp

// Creates and returns a new list
list_t* list_create()
{
    //allocate list memory
    list_t* list = (list_t*) malloc(sizeof(list_t));  //malloc returns a void pointer (void*) which can be casted into any type.
    if(list == NULL){                                 //malloc returns NULL if it fails to allocate memory
      perror("malloc");                               //perror prints a descriptive error message to stderr
      return NULL;                                    //return NULL to indicate failure
    }

    //no head in list
    list->head = NULL;                               //set head to NULL
    list->count = 0;                                 //set count to 0

    return list;
}

static void list_clear(list_t * list){
  list_node_t * iter = list->head;                   //set iter to head
  while(iter){
    list_node_t * next = iter->next;                 //set next to iter->next

    free(iter);                                      //free iter

    iter = next;
  }

  list->count = 0;                                   //set count to 0
  list->head = NULL;                                 // set head to NULL
}

// Destroys a list
void list_destroy(list_t* list)                      //free list
{
    list_clear(list);                               //clear list  
    free(list);                                     //free list      
}

// Returns beginning of the list
list_node_t* list_begin(list_t* list)
{
    return list->head;                             //return head
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    return node->next;                             //return node->next
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    return node->data;                             //return node->data
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    return list->count;                            //return list->count
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
  list_node_t * iter = list->head;                  //set iter to head
  while(iter){
    if(iter->data == data){                       
      return iter;
    }
    iter = iter->next;                              //set iter to iter->next
  }
  return NULL;
}

// Inserts a new node in the list with the given data
void list_insert(list_t* list, void* data)
{
    //list nodes must be unique
    if(list_find(list, data)){                      // if data is already in list, return
      return;
    }

    //allocate a new node
    list_node_t * node = (list_node_t *) malloc(sizeof(list_node_t));
    if(node == NULL){                        //malloc returns NULL if it fails to allocate memory   
      return;
    }

    //setup the node
    node->data = data;
    node->prev = node->next = NULL;

    //prepend into list
    if(list->head == NULL){
      list->head = node;
    }else{
      node->next = list->head;
      list->head->prev = node;
      list->head = node;
    }

    list->count++;
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
  if(node->prev == NULL){
    list_node_t* next = node->next;                        //set next to node->next
    list->head = next;
    if(next){
      next->prev = NULL;
    }
  }else{
    list_node_t* prev = node->prev;                         //set prev to node->prev
    list_node_t* next = node->next;                         // set next to node->next

    prev->next = next;
    if(next){
      next->prev = prev;
    }
  }

  list->count--;
}

// Executes a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data))
{
  list_node_t * iter = list->head;
  while(iter){
    func(iter->data);                               //call func on iter->data
    iter = iter->next;                              //set iter to iter->next
  }
}