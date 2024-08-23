#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "concurrent_list.h"

/* node struct that contains 3 fields
node's value, epointer to the next node, lock of the node */
struct node {
    int value; /* node value */
    struct node* next; /* pointer to the next node */
    pthread_mutex_t lock; /* node's lock */
};

/* list struct that contains 2 fields
pointer to the head of the list, lock of the list */
struct list {
  struct node* head; /* list head pointer */
  pthread_mutex_t lock; /* list lock */
};

/* function that creating a new node by allocating memory to the new node
and inserting the received value to its value field, the function returns a pointer to this new node*/
node* create_node(int value)
{
    node* new_node = (node*)malloc(sizeof(node)); /* allocating memory for the new node */
    if(new_node == NULL) /* allocating memory for the new node has failed*/
    {
        return NULL; 
    }
    new_node->value = value; /* giving the received value */
    new_node->next = NULL; /* no next node right now */
    if(pthread_mutex_init(&(new_node->lock),NULL) != 0) /* initializing the node's lock */
    {
        free(new_node);
        return NULL; 
    }
    return new_node; /* return the pointer to the new node */
}

/* function to print the value of the received node */
void print_node(node* node)
{
  // DO NOT DELETE
  if(node)
  {
    printf("%d ", node->value);
  }
}

/* function that creating a new list by allocating memory to it, and initializing 
its head pointer to NULL and initializing its mutex lock, the function returns a pointer to the new list */
list* create_list()
{
  struct list* new_list = (struct list*)malloc(sizeof(struct list)); /* allocating memory for the new list */
  if (new_list == NULL) /* if allocating memory has failed */
  {
      perror("error");
      exit(1);
  }
  new_list->head = NULL; /* head of the list pointing on NULL */
  if(pthread_mutex_init(&(new_list->lock),NULL) != 0) /* initializing the lock of the list */
  {
      free(new_list); 
      perror("error");
      exit(1);
  }
  return new_list; /* return the pointer to the new list */
}

/* function that deleting the received list by removing all the nodes and removing the list after that */
void delete_list(list* list)
{
    if(list == NULL)
    {
        return;
    }
    pthread_mutex_lock(&(list->lock)); /* locking the list to guarantee the head node */
    
    node* current = list->head; /* current points to the head of the list */
    node* next = NULL; /* initializing next to NULL */
    
    if (current != NULL) /* at least one node exists in the list */
    {
        pthread_mutex_lock(&(current->lock)); /* locking the fisrt node */
    }
    while(current != NULL) /* till the last node */
    {
        next = current->next; /* next points to the next node of the current node */
        if(next != NULL) /* locking next if it's not NULL, so no thread can change it */
        {
            pthread_mutex_lock(&(next->lock));
        }
        pthread_mutex_unlock(&(current->lock)); /* unlock the current node to free it */
        pthread_mutex_destroy(&(current->lock)); /* destroy the lock of the current node to free it after that */
        free(current);
        current = next; /* current now pointing to the next node */
    }
    pthread_mutex_unlock(&(list->lock));
    pthread_mutex_destroy(&(list->lock));
    free(list);
    return;
}

/* function to insert the received value to the list by a node, its receives a list pointer and a value */
void insert_value(list* list, int value)
{        
    // add code here
    if(list != NULL)
    {
        node* new_node = create_node(value); /* creating the new node to insert it */
        if(new_node == NULL)
        {
            delete_list(list);
            perror("error");
            exit(1);
        }
        pthread_mutex_lock(&(list->lock)); /* locking the list */
        if(list->head == NULL)
        {
            list->head = new_node; /* insert to the head of the list */
            pthread_mutex_unlock(&(list->lock)); /* locking the list */
            return;
        }
        //to take into account that head can't be removed when list is locked
        if((list->head->value) > value)
        {
            new_node->next = list->head; /* the new node is the first node so it will point to the head */
            list->head = new_node; /* new node will the head of the list */
            pthread_mutex_unlock(&(list->lock)); /* locking the list */
            return;
        }
        node* current = list->head; /* current points to the head of the list*/
        node* next = current->next;
        pthread_mutex_lock(&(current->lock)); /* locking the current node (head) */
        pthread_mutex_unlock(&(list->lock)); /* unlocking the list because we reached the first node so no thread can move forward through the head */
        while((next != NULL) && ((next->value) < value)) /* moving forward till the next node not NULL and its value equal or greater than value (the value we want to insert) */
        {
            pthread_mutex_lock(&(next->lock)); /* lock next node */
            pthread_mutex_unlock(&current->lock); /* unlock current node */
            current = next;
            next = current->next; /* moving forward the current and the next pointers */
        }
        new_node->next = current->next; /* the field next of the new node points to the field next of the current node (we want to insert the new node between current and next) */
        current->next = new_node; /* the next field of the current node points to the new node */
        pthread_mutex_unlock(&current->lock); /* unlocking the lock of the current node */
    }
}

void remove_value(list* list, int value)
{
    if(list != NULL)
    {
        pthread_mutex_lock(&(list->lock)); /* lock the list */
        node* current;
        node* next;
        
        if(list->head != NULL)
        {
            pthread_mutex_lock(&(list->head->lock)); /* lock the first node in the list */
            pthread_mutex_unlock(&(list->lock)); /* unlock the list */
            current = list->head; /* current starts from the head */
            next = current->next; /* next starts from the second node if exists */
            
            if(current->value == value) /* the head's value is the the wanted value */
            {
                list->head = (list->head)->next; /* head will point to the next node because it will be removed */
                pthread_mutex_unlock(&(current->lock)); /* unlock the node that we want to remove */
                pthread_mutex_destroy(&(current->lock)); /* destroy the lock of the node */
                free(current);
                return;
            }
            while((next != NULL) && (next->value < value) ) /* moving forward till next node not NULL and its value smaller than the value of the node we want to remove */
            {
                pthread_mutex_lock(&(next->lock)); /* lock next to guarantee that no thread can change it */
                pthread_mutex_unlock(&current->lock); /* unlock current */
                current = next;
                next = current->next; 
            }
            if((next != NULL) && (next->value == value)) /* if next's value is the value we want to remove so we will remove */
            {
                current->next=next->next; /* current will point to the next of the next */
                pthread_mutex_destroy(&(next->lock));
                free(next);
            }
            pthread_mutex_unlock(&(current->lock)); /* unlocking current */
        }
        else
        {
            pthread_mutex_unlock(&(list->lock)); /* if the head is NULL we want to unlock the list also */
        }
    }
}

/* function to print the received list by printing the values of all the nodes from smaller to greater */
void print_list(list* list)
{
  // add code here
  struct node* current; 
  if(list != NULL) 
  {
      pthread_mutex_lock(&(list->lock)); /* locking the list to lock the head if exists */
      current = list->head; /* to start printing from the head */
      if(current != NULL)
      {
        pthread_mutex_lock(&(current->lock)); /* locking the first node */
      }
      pthread_mutex_unlock(&(list->lock)); /* unlocking after locking the head node */
      while(current != NULL) /* till the last node */
      {
          print_node(current); /* printing the current node using "print_node" function */
          if (current->next != NULL)
          {
            pthread_mutex_lock(&(current->next->lock)); /* locking next */
          }
          pthread_mutex_unlock(&(current->lock)); /* unlocking current */
          current = current->next; /* moving forward */
      }
  }
  printf("\n"); // DO NOT DELETE
}

/* function to count and print the number of the nodes in the received list that returns non-zero integer by sending 
their values as parameters to the received function*/
void count_list(list* list, int (*predicate)(int))
{
  int count = 0; // DO NOT DELETE

  // add code here
  struct node* current = NULL; /* the node we want to use for current node (initializing it to NULL for preventing it to be garbage value (when head = NULL and it reaches while loop)) */
  if(list != NULL)
  {
      pthread_mutex_lock(&(list->lock)); /* locking the list to lock the head if exists */
      if(list->head != NULL)
      {
        current = list->head; /* starting from the head */
        pthread_mutex_lock(&(current->lock)); /* locking current */
      }
      pthread_mutex_unlock(&(list->lock)); /* unlocking after locking the head node */
      while(current !=NULL)
      {
          if(predicate(current->value)) /* if the returning value of the function is not 0 */
          {
              count++;
          }
          if (current->next != NULL) /* if there is next node so we will lock it then we will unlock the current node */
          {
            pthread_mutex_lock(&(current->next->lock));
          }
          pthread_mutex_unlock(&current->lock); 
          current = current->next; /* moving forward */
      }
  }
  printf("%d items were counted\n", count); // DO NOT DELETE
}

    
