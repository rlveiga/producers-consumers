#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "buffer.h"

#include "./StateManager/stateManager.h" // Require StateManager for testing

// argument to pthread
typedef struct
{
  int id;
  int nproducts;
} pthread_func_arg;

void *Consumer(void *arg)
{
  pthread_func_arg *targ = (pthread_func_arg *)arg;

  int count = 0;
  while (count < targ->nproducts)
  {
    int data = consome(targ->id);
    count++;
    printf("==C== Consumidor %d leu dado %d\n", targ->id, data);
  }

  printf("Consumidor %d terminou!\n", targ->id);
  return NULL;
}

void *Producer(void *arg)
{
  pthread_func_arg *targ = (pthread_func_arg *)arg;

  int count = 0;
  while (count < targ->nproducts)
  {
    deposita(count);
    count++;
  }

  printf("Produtor %d terminou!\n", targ->id);
  return NULL;
}

int main(int argc, char *argv[])
{
  if (argc != 5)
  {
    puts("Wrong number of arguments: must receive 4 arguments, the size of the buffer, the number of producers, the number of consumers, and the number of itens");
    return EXIT_FAILURE;
  }

  // parse arguments; should use strtol for robustness, but here we prioritize
  // readableness
  int numpos = atoi(argv[1]);
  int numprod = atoi(argv[2]);
  int numcons = atoi(argv[3]);
  int nproducts = atoi(argv[4]);
  if (numpos <= 0 || numprod <= 0 || numcons <= 0 || nproducts < 0)
  {
    puts("All arguments must be positive integers");
    return EXIT_FAILURE;
  }
  // reusing the strings to avoid unnecessary conversions
  printf("Initializing broadcast of %s messages with %s producers and %s consumers; buffer size is %s\n", argv[4], argv[2], argv[3], argv[1]);

  // initializeManager("./tests/7.txt", numprod + numcons); // Initialize state manager with test file
  iniciabuffer(numpos, numprod, numcons);

  pthread_t *cid = (pthread_t *)malloc(sizeof(pthread_t) * numcons);
  pthread_t *pid = (pthread_t *)malloc(sizeof(pthread_t) * numprod);
  pthread_func_arg **cons_args = (pthread_func_arg **)malloc(sizeof(pthread_func_arg *) * numcons);
  pthread_func_arg **prod_args = (pthread_func_arg **)malloc(sizeof(pthread_func_arg *) * numprod);

  int i;
  for (i = 0; i < numcons; i++)
  {
    cons_args[i] = (pthread_func_arg *)malloc(sizeof(pthread_func_arg));
    cons_args[i]->id = i;
    cons_args[i]->nproducts = numprod * nproducts;
    pthread_create(&cid[i], NULL, Consumer, cons_args[i]);
  }

  for (i = 0; i < numprod; i++)
  {
    prod_args[i] = (pthread_func_arg *)malloc(sizeof(pthread_func_arg));
    prod_args[i]->id = i;
    prod_args[i]->nproducts = nproducts;
    pthread_create(&pid[i], NULL, Producer, prod_args[i]);
  }

  for (i = 0; i < numcons; i++)
  {
    pthread_join(cid[i], NULL);
  }

  for (i = 0; i < numprod; i++)
  {
    pthread_join(pid[i], NULL);
  }

  // clean_up
  for (i = 0; i < numprod; i++)
  {
    free(prod_args[i]);
  }
  for (i = 0; i < numcons; i++)
  {
    free(cons_args[i]);
  }

  free(cid);
  free(pid);
  free(cons_args);
  free(prod_args);
  finalizeManager();
  finalizabuffer(numpos, numprod, numcons);

  return EXIT_SUCCESS;
}
